/*
 * Copyright (c) 2018, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <net/net_core.h>
#include <logging/log.h>

#include <string.h>
#include <limits.h>

#include <knot/knot_protocol.h>
#include <knot/knot_types.h>
#include "msg.h"
#include "proxy.h"
#include "knot.h"

LOG_MODULE_DECLARE(knot, CONFIG_KNOT_LOG_LEVEL);

#define MIN(a, b)         (((a) < (b)) ? (a) : (b))

#define check_bool_change(proxy, bval) \
	(KNOT_EVT_FLAG_CHANGE & proxy->config.event_flags \
	&& bval != proxy->value.val_b)

#define check_int_change(proxy, s32val)	\
	(KNOT_EVT_FLAG_CHANGE & proxy->config.event_flags \
	&& s32val != proxy->value.val_i)

#define check_int_upper_threshold(proxy, s32val)	\
	(KNOT_EVT_FLAG_UPPER_THRESHOLD & proxy->config.event_flags \
	&& s32val > proxy->config.upper_limit.val_i)

#define check_int_lower_threshold(proxy, s32val)	\
	(KNOT_EVT_FLAG_LOWER_THRESHOLD & proxy->config.event_flags \
	&& s32val < proxy->config.lower_limit.val_i)

#define check_float_change(proxy, fval)	\
	(KNOT_EVT_FLAG_CHANGE & proxy->config.event_flags \
	&& fval != proxy->value.val_f)

#define check_float_upper_threshold(proxy, fval)	\
	(KNOT_EVT_FLAG_UPPER_THRESHOLD & proxy->config.event_flags \
	&& fval > proxy->config.upper_limit.val_f)

#define check_float_lower_threshold(proxy, fval)	\
	(KNOT_EVT_FLAG_LOWER_THRESHOLD & proxy->config.event_flags \
	&& fval < proxy->config.lower_limit.val_f)

#define check_raw_change(proxy, rawval, rawlen)	\
	(KNOT_EVT_FLAG_CHANGE & proxy->config.event_flags \
	&& memcmp(proxy->value.raw, rawval, rawlen) != 0)

static struct knot_proxy {
	/* KNoT identifier */
	u8_t			id;

	/* Schema values */
	knot_schema		schema;

	/* Data values */
	knot_value_type		value;

	/* Watched/Controlled variable */
	void			*target;
	size_t			 target_len;

	/* Control variable to send data */
	bool			send; /* 'value' must be sent */
	bool			wait_resp; /* Will send 'value' until resp */
	bool 			upper_flag; /* Upper limit crossed */
	bool 			lower_flag; /* Lower limit crossed */
	u8_t			olen; /* Amount to send / Output: temporary */

	/* Config values */
	knot_config		config;

	/* Time values */
	u32_t			last_timeout;

	knot_callback_t		read_cb; /* Poll for local changes */
	knot_callback_t		write_cb; /* Report new value to user app */
} proxy_pool[CONFIG_KNOT_THING_DATA_MAX];

static u8_t last_id = 0xff;

void proxy_init(void)
{
	int i;

	memset(proxy_pool, 0, sizeof(proxy_pool));

	for (i = 0; (i < sizeof(proxy_pool) / sizeof(struct knot_proxy)); i++)
		proxy_pool[i].id = 0xff;
}

void proxy_stop(void)
{

}

int knot_data_register(u8_t id, const char *name,
		       u16_t type_id, u8_t value_type, u8_t unit,
		       void *target, size_t target_len,
		       knot_callback_t write_cb, knot_callback_t read_cb)
{
	struct knot_proxy *proxy;

	/* Out of index? */
	if (id >= CONFIG_KNOT_THING_DATA_MAX) {
		LOG_ERR("Register for ID %d failed: "
			"id >= CONFIG_KNOT_THING_DATA_MAX (%d)",
			id, CONFIG_KNOT_THING_DATA_MAX);
		return -1;
	}

	/* Assigned already? */
	if (proxy_pool[id].id != 0xff) {
		LOG_ERR("Register for ID %d failed: "
			"Id already registered", id);
		return -1;
	}

	/* Has value? */
	if (unlikely(!target)) {
		LOG_ERR("Register for ID %d failed: "
			"Null value pointer", id);
		return -1;
	}

	/* Compatible buffer length? */
	switch(value_type) {
	case KNOT_VALUE_TYPE_BOOL:
		if (target_len == sizeof(bool))
			break;
		LOG_ERR("Register for ID %d failed: "
			"Incompatible target_len %d for type "
			"KNOT_VALUE_TYPE_BOOL", id, target_len);
		return -1;
	case KNOT_VALUE_TYPE_INT:
		if (target_len == sizeof(int))
			break;
		LOG_ERR("Register for ID %d failed: "
			"Incompatible target_len %d for type "
			"KNOT_VALUE_TYPE_INT", id, target_len);
		return -1;
	case KNOT_VALUE_TYPE_FLOAT:
		if (target_len == sizeof(float))
			break;
		LOG_ERR("Register for ID %d failed: "
			"Incompatible target_len %d for type "
			"KNOT_VALUE_TYPE_FLOAT", id, target_len);
		return -1;
	case KNOT_VALUE_TYPE_RAW:
		if (target_len > 0 && target_len <= KNOT_DATA_RAW_SIZE)
			break;
		LOG_ERR("Register for ID %d failed: "
			"Incompatible target_len %d for type "
			"KNOT_VALUE_TYPE_RAW", id, target_len);
		return -1;
	default:
		LOG_ERR("Register for ID %d failed: "
			"Invalid value type", id);
		return -1;
	}

	/* Basic field validation */
	if (knot_schema_is_valid(type_id, value_type, unit) != 0 || !name) {
		LOG_ERR("Register for ID %d failed: "
			"Invalid schema", id);
		return -1;
	}

	proxy = &proxy_pool[id];

	proxy->id = id;
	proxy->schema.type_id = type_id;
	proxy->schema.unit = unit;
	proxy->schema.value_type = value_type;
	proxy->target = target;
	proxy->target_len = target_len;
	proxy->send = false;
	proxy->upper_flag = false;
	proxy->lower_flag = false;
	proxy->olen = 0;

	strncpy(proxy->schema.name, name,
		MIN(KNOT_PROTOCOL_DATA_NAME_LEN, strlen(name)));

	/* Set default config */
	proxy->config.event_flags = KNOT_EVT_FLAG_NONE;

	proxy->read_cb = read_cb;
	proxy->write_cb = write_cb;

	if (id > last_id || last_id == 0xff)
		last_id = id;

	return id;
}

bool knot_data_config(u8_t id, ...)
{
	va_list event_args;

	struct knot_proxy *proxy;
	u8_t event;
	u8_t event_flags = KNOT_EVT_FLAG_NONE;
	u16_t timeout_sec = 0;
	knot_value_type lower_limit;
	knot_value_type upper_limit;

	lower_limit.val_i = 0;
	upper_limit.val_i = 0;

	if (id >= CONFIG_KNOT_THING_DATA_MAX) {
		LOG_ERR("Config for ID %d failed: "
			"id >= CONFIG_KNOT_THING_DATA_MAX (%d)",
			id, CONFIG_KNOT_THING_DATA_MAX);
		return false;
	}

	proxy = &proxy_pool[id];

	if (proxy->id != id) {
		LOG_ERR("Config for ID %d failed: "
			"Proxy not found!", id);
		return false;
	}

	/* Read arguments and set event_flags */
	va_start(event_args, id);
	do {
		event = (u8_t) va_arg(event_args, int);
		switch(event) {
		case KNOT_EVT_FLAG_NONE:
			break;
		case KNOT_EVT_FLAG_CHANGE:
			event_flags |= KNOT_EVT_FLAG_CHANGE;
			break;
		case KNOT_EVT_FLAG_TIME:
			timeout_sec = (u16_t) va_arg(event_args, int);
			event_flags |= KNOT_EVT_FLAG_TIME;
			break;
		case KNOT_EVT_FLAG_UPPER_THRESHOLD:
			if(proxy->schema.value_type == KNOT_VALUE_TYPE_INT)
				upper_limit.val_i = (s32_t) va_arg(event_args,
							 int);
			if(proxy->schema.value_type == KNOT_VALUE_TYPE_FLOAT)
				upper_limit.val_f = (float) va_arg(event_args,
							 double);
			event_flags |= KNOT_EVT_FLAG_UPPER_THRESHOLD;
			break;
		case KNOT_EVT_FLAG_LOWER_THRESHOLD:
			if(proxy->schema.value_type == KNOT_VALUE_TYPE_INT)
				lower_limit.val_i = (s32_t) va_arg(event_args,
							 int);
			if(proxy->schema.value_type == KNOT_VALUE_TYPE_FLOAT)
				lower_limit.val_f = (float) va_arg(event_args,
							 double);
			event_flags |= KNOT_EVT_FLAG_LOWER_THRESHOLD;
			break;
		default:
			va_end(event_args);
			LOG_ERR("Config for ID %d failed: "
				"Invalid config flags", id);
			return false;
		}

	} while(event);
	va_end(event_args);

	if (knot_config_is_valid(event_flags, proxy->schema.value_type,
				 timeout_sec, &lower_limit, &upper_limit) != 0) {
		LOG_ERR("Config for ID %d failed: "
			"Invalid config values", id);
		return false;
	}

	/* Set upper and lower limits */
	if (event_flags & KNOT_EVT_FLAG_UPPER_THRESHOLD)
		memcpy(&proxy->config.upper_limit,
		       &upper_limit, sizeof(upper_limit));

	if (event_flags & KNOT_EVT_FLAG_LOWER_THRESHOLD)
		memcpy(&proxy->config.lower_limit,
		       &lower_limit, sizeof(lower_limit));

	/* Set event flags and timeout */
	proxy->config.event_flags = event_flags;
	proxy->config.time_sec = timeout_sec;

	return true;
}

/* Proxy properties */
u8_t knot_proxy_get_id(struct knot_proxy *proxy)
{
	if (unlikely(!proxy))
		return 0xff;

	return proxy->id;
}

const knot_schema *proxy_get_schema(u8_t id)
{
	if (proxy_pool[id].id == 0xff)
		return NULL;

	return &(proxy_pool[id].schema);
}

u8_t proxy_get_last_id(void)
{
	return last_id;
}

static bool check_timeout(struct knot_proxy *proxy)
{
	u32_t current_time, elapsed_time;

	if (!(KNOT_EVT_FLAG_TIME & proxy->config.event_flags))
		return false;

	current_time = k_uptime_get();
	elapsed_time = current_time - proxy->last_timeout;
	if (elapsed_time >= (proxy->config.time_sec * 1000)) {
		proxy->last_timeout = current_time;
		return true;
	}
	return false;
}

static bool set_proxy_value(struct knot_proxy *proxy,
			    const knot_value_type value, size_t len)
{
	bool change;
	bool upper;
	bool lower;
	bool timeout;
	bool ret;

	bool bval;
	s32_t s32val;
	float fval;

	ret = false; /* Default not sending */

	if (unlikely(!proxy))
		goto done;

	timeout = check_timeout(proxy);
	switch(proxy->schema.value_type) {
	case KNOT_VALUE_TYPE_BOOL:
		bval = value.val_b;
		change = check_bool_change(proxy, bval);

		if (proxy->send || timeout || change) {
			proxy->olen = proxy->target_len;
			proxy->value.val_b = bval;
			proxy->send = proxy->wait_resp;
			ret = true;
		}
		break;
	case KNOT_VALUE_TYPE_INT:
		s32val = value.val_i;
		change = check_int_change(proxy, s32val);
		upper = check_int_upper_threshold(proxy, s32val);
		lower = check_int_lower_threshold(proxy, s32val);

		if ( proxy->send || timeout || change ||
		    (upper && proxy->upper_flag == false) ||
		    (lower && proxy->lower_flag == false)) {
			proxy->olen = proxy->target_len;
			proxy->value.val_i = s32val;
			proxy->send = proxy->wait_resp;
			ret = true;
		}
		proxy->upper_flag = upper; /* Send only at crossing */
		proxy->lower_flag = lower; /* Send only at crossing */
		break;
	case KNOT_VALUE_TYPE_FLOAT:
		fval = value.val_f;
		change = check_int_change(proxy, fval);
		upper = check_float_upper_threshold(proxy, fval);
		lower = check_float_lower_threshold(proxy, fval);

		if ( proxy->send || timeout || change ||
		    (upper && proxy->upper_flag == false) ||
		    (lower && proxy->lower_flag == false)) {
			proxy->olen = proxy->target_len;
			proxy->value.val_f = fval;
			proxy->send = proxy->wait_resp;
			ret = true;
		}
		proxy->upper_flag = upper; /* Send only at crossing */
		proxy->lower_flag = lower; /* Send only at crossing */
		break;
	case KNOT_VALUE_TYPE_RAW:
		change = check_raw_change(proxy, value.raw, len);
		if (proxy->send || change || timeout) {
			proxy->olen = len; /* Amount to send */
			memcpy(proxy->value.raw, value.raw, len);
			proxy->send = proxy->wait_resp;
			ret = true;
		}
	}
done:
	return ret;
}

/* Return knot_value_type* so it can be flagged as const  */
const knot_value_type *proxy_read(u8_t id, u8_t *olen, bool wait_resp)
{
	struct knot_proxy *proxy;
	knot_value_type read_val;
	bool send_msg;

	if (proxy_pool[id].id == 0xff)
		return NULL;

	proxy = &proxy_pool[id];

	proxy->olen = 0;

	/* Wait for response? */
	proxy->wait_resp = wait_resp;

	/* Execute read callback if set */
	if (proxy->read_cb != NULL &&
	    proxy->read_cb(id) < 0) {
		LOG_INF("Read callback failed to ID %d", id);
		return NULL;
	}

	/* Typecast value and read it */
	switch(proxy->schema.value_type) {
	case KNOT_VALUE_TYPE_BOOL:
		read_val.val_b = *((bool*) proxy->target);
		break;
	case KNOT_VALUE_TYPE_INT:
		read_val.val_i = *((int*) proxy->target);
		break;
	case KNOT_VALUE_TYPE_FLOAT:
		read_val.val_f = *((float*) proxy->target);
		break;
	case KNOT_VALUE_TYPE_RAW:
		memcpy(read_val.raw, proxy->target, proxy->target_len);
		break;
	default:
		return NULL;
	}

	/* Send message if proxy value is updated */
	send_msg = set_proxy_value(proxy, read_val, proxy->target_len);
	if (send_msg == false)
		return NULL;

	*olen = proxy->olen;
	return &proxy->value;
}

s8_t proxy_write(u8_t id, const knot_value_type *value, u8_t value_len)
{
	struct knot_proxy *proxy;

	/* Backup values */
	knot_value_type old_value;

	if (id > last_id)
		return -EINVAL;

	proxy = &proxy_pool[id];

	if (proxy->id == 0xff)
		return -EINVAL;

	memcpy(&proxy->value, value, sizeof(*value));

	/*
	 * New values sent from cloud are informed to
	 * the user app through write callback.
	 */
	switch(proxy->schema.value_type) {
	case KNOT_VALUE_TYPE_BOOL:
		/* Copy without backup if no write callback set */
		if (proxy->write_cb == NULL) {
			*((bool*) proxy->target) = value->val_b;
			break;
		}

		/* Store old value before trying to update */
		old_value.val_b = *((bool*) proxy->target);
		*((bool*) proxy->target) = value->val_b;

		/* Get back to old value if write callback failed */
		if (proxy->write_cb(id) < 0) {
			LOG_INF("Write callback failed to ID %d", id);
			*((bool*) proxy->target) = old_value.val_b;
			return -EAGAIN;
		}
		break;
	case KNOT_VALUE_TYPE_INT:
		/* Copy without backup if no write callback set */
		if (proxy->write_cb == NULL) {
			*((int*) proxy->target) = value->val_i;
			break;
		}

		/* Store old value before trying to update */
		old_value.val_i = *((int*) proxy->target);
		*((int*) proxy->target) = value->val_i;

		/* Get back to old value if write callback failed */
		if (proxy->write_cb(id) < 0) {
			LOG_INF("Write callback failed to ID %d", id);
			*((int*) proxy->target) = old_value.val_i;
			return -EAGAIN;
		}
		break;
	case KNOT_VALUE_TYPE_FLOAT:
		/* Copy without backup if no write callback set */
		if (proxy->write_cb == NULL) {
			*((float*) proxy->target) = value->val_f;
			break;
		}

		/* Store old value before trying to update */
		old_value.val_f = *((float*) proxy->target);
		*((float*) proxy->target) = value->val_f;

		/* Get back to old value if write callback failed */
		if (proxy->write_cb(id) < 0) {
			LOG_INF("Write callback failed to ID %d", id);
			*((float*) proxy->target) = old_value.val_f;
			return -EAGAIN;
		}
		break;
	case KNOT_VALUE_TYPE_RAW:
		/* Abort if buffer overflow */
		if (value_len > proxy->target_len) {
			LOG_WRN("Write failed for ID %d: "
				"Msg too big for buffer (%d > %d)",
				id, value_len, proxy->target_len);
			return -EFBIG;
		}

		/* Copy without backup if no write callback set */
		if (proxy->write_cb == NULL) {
			memset(proxy->target, 0, proxy->target_len);
			memcpy(proxy->target, value->raw, value_len);
			break;
		}

		/* Store old values */
		memcpy(old_value.raw, proxy->target, proxy->target_len);

		/* Update value */
		memset(proxy->target, 0, proxy->target_len);
		memcpy(proxy->target, value->raw, value_len);

		/* Get back to old value if write callback failed */
		if (proxy->write_cb(id) < 0) {
			LOG_INF("Write callback failed to ID %d", id);
			memcpy(proxy->target, old_value.raw,
			       proxy->target_len);
			return -EAGAIN;
		}
		break;
	default:
		return -EINVAL;
	}

	return proxy->olen;
}

s8_t proxy_force_send(u8_t id)
{
	struct knot_proxy *proxy;

	if (proxy_pool[id].id == 0xff)
		return -EINVAL;

	proxy = &proxy_pool[id];

	/* Flag 'value' to be sent, but don't wait response */
	proxy->send = true;

	return 0;
}

s8_t proxy_confirm_sent(u8_t id)
{
	struct knot_proxy *proxy;

	proxy = &proxy_pool[id];

	if (proxy->id == 0xff)
		return -EINVAL;

	/* No need to resend */
	proxy->send = false;

	return 0;
}
