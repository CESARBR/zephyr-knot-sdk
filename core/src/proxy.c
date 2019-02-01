/*
 * Copyright (c) 2018, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_DOMAIN "knot-hello"
#define NET_SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_LOG_ENABLED 1

#include <zephyr.h>
#include <net/net_core.h>

#include <string.h>

#include "knot_protocol.h"
#include "knot_types.h"
#include "msg.h"
#include "proxy.h"
#include "knot.h"

#define MIN(a, b)         (((a) < (b)) ? (a) : (b))

#define check_bool_change(proxy, bval) \
	(KNOT_EVT_FLAG_CHANGE & proxy->config.event_flags \
	&& bval != proxy->value.val_b)

#define check_int_change(proxy, s32val)	\
	(KNOT_EVT_FLAG_CHANGE & proxy->config.event_flags \
	&& s32val != proxy->value.val_i)

#define check_int_upper_threshold(proxy, s32val)	\
	(KNOT_EVT_FLAG_UPPER_THRESHOLD & proxy->config.event_flags \
	&& s32val > proxy->value.val_i)

#define check_int_lower_threshold(proxy, s32val)	\
	(KNOT_EVT_FLAG_LOWER_THRESHOLD & proxy->config.event_flags \
	&& s32val < proxy->value.val_i)

#define check_float_change(proxy, fval)	\
	(KNOT_EVT_FLAG_CHANGE & proxy->config.event_flags \
	&& fval != proxy->value.val_f)

#define check_float_upper_threshold(proxy, fval)	\
	(KNOT_EVT_FLAG_UPPER_THRESHOLD & proxy->config.event_flags \
	&& fval > proxy->value.val_f)

#define check_float_lower_threshold(proxy, fval)	\
	(KNOT_EVT_FLAG_LOWER_THRESHOLD & proxy->config.event_flags \
	&& fval < proxy->value.val_f)

#define check_raw_change(proxy, rawval, rawlen)	\
	(KNOT_EVT_FLAG_CHANGE & proxy->config.event_flags \
	&& ( proxy->rlen != rawlen \
	     || memcmp(proxy->value.raw, rawval, rawlen) != 0) \
	   )

static struct knot_proxy {
	/* KNoT identifier */
	u8_t			id;

	/* Schema values */
	knot_schema		schema;

	/* Data values */
	knot_value_type		value;

	/* Control variable to send data */
	bool			send; /* 'value' must be sent */
	bool			wait_resp; /* Will send 'value' until resp */
	u8_t			olen; /* Amount to send / Output: temporary */
	u8_t			rlen; /* Length RAW value */

	/* Config values */
	knot_config		config;

	/* Time values */
	u32_t			last_timeout;

	knot_callback_t		poll_cb; /* Poll for local changes */
	knot_callback_t		changed_cb; /* Report new value to user app */
} proxy_pool[KNOT_THING_DATA_MAX];

static u8_t last_id = 0xff;

void proxy_start(void)
{
	int i;

	memset(proxy_pool, 0, sizeof(proxy_pool));

	for (i = 0; (i < sizeof(proxy_pool) / sizeof(struct knot_proxy)); i++)
		proxy_pool[i].id = 0xff;
}

void proxy_stop(void)
{

}

struct knot_proxy *knot_proxy_register(u8_t id, const char *name,
				       u16_t type_id, u8_t value_type,
				       u8_t unit, knot_callback_t changed_cb,
				       knot_callback_t poll_cb)
{
	struct knot_proxy *proxy;

	/* Out of index? */
	if (id >= KNOT_THING_DATA_MAX)
		return NULL;

	/* Assigned already? */
	if (proxy_pool[id].id != 0xff)
		return NULL;

	/* Basic field validation */
	if (knot_schema_is_valid(type_id, value_type, unit) != 0 || !name)
		return NULL;

	proxy = &proxy_pool[id];

	proxy->id = id;
	proxy->schema.type_id = type_id;
	proxy->schema.unit = unit;
	proxy->schema.value_type = value_type;
	proxy->send = false;
	proxy->olen = 0;

	strncpy(proxy->schema.name, name,
		MIN(KNOT_PROTOCOL_DATA_NAME_LEN, strlen(name)));

	/* Set default config */
	proxy->config.event_flags = KNOT_EVT_FLAG_NONE;

	proxy->poll_cb = poll_cb;
	proxy->changed_cb = changed_cb;

	if (id > last_id || last_id == 0xff)
		last_id = id;

	return proxy;
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

/* Return knot_value_type* so it can be flagged as const  */
const knot_value_type *proxy_read(u8_t id, uint8_t *olen, bool wait_resp)
{
	struct knot_proxy *proxy;

	if (proxy_pool[id].id == 0xff)
		return NULL;

	proxy = &proxy_pool[id];

	if (proxy->poll_cb == NULL)
		return NULL;

	proxy->olen = 0;

	/* Wait for response? */
	proxy->wait_resp = wait_resp;

	proxy->poll_cb(proxy);

	/*
	 * Read callback may set new values. When a
	 * new value is set "olen" field is set.
	 */
	if (proxy->olen <= 0)
		return NULL;

	*olen = proxy->olen;
	return &proxy->value;
}

s8_t proxy_write(u8_t id, const knot_value_type *value, u8_t value_len)
{
	struct knot_proxy *proxy;

	if (id > last_id)
		return -EINVAL;

	proxy = &proxy_pool[id];

	if (proxy->id == 0xff)
		return -EINVAL;

	if (proxy->changed_cb == NULL)
		return 0;

	memcpy(&proxy->value, value, sizeof(*value));
	/*
	 * Set string length if raw data. 'value_len' can be ignored for basic
	 * types: knotd is responsible for encoding and setting payload_len.
	 */
	if (proxy->schema.value_type == KNOT_VALUE_TYPE_RAW)
		proxy->rlen = value_len;

	/*
	 * New values sent from cloud are informed to
	 * the user app through write callback.
	 */

	proxy->changed_cb(proxy);

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

void knot_proxy_value_set_basic(struct knot_proxy *proxy, const void *value)
{
	bool change;
	bool upper;
	bool lower;
	bool timeout;

	bool bval;
	s32_t s32val;
	float fval;

	if (unlikely(!proxy))
		return;

	timeout = check_timeout(proxy);
	switch(proxy->schema.value_type) {
	case KNOT_VALUE_TYPE_BOOL:
		bval = *((bool *) value);
		change = check_bool_change(proxy, bval);

		if (proxy->send || timeout || change) {
			proxy->olen = sizeof(bool);
			proxy->value.val_b = bval;
			proxy->send = proxy->wait_resp;
		}
		break;
	case KNOT_VALUE_TYPE_INT:
		s32val = *((s32_t *) value);
		change = check_int_change(proxy, s32val);
		upper = check_int_upper_threshold(proxy, s32val);
		lower = check_int_lower_threshold(proxy, s32val);

		if (proxy->send || timeout || change || upper || lower) {
			proxy->olen = sizeof(int);
			proxy->value.val_i = s32val;
			proxy->send = proxy->wait_resp;
		}
		break;
	case KNOT_VALUE_TYPE_FLOAT:
		fval = *((float *) value);
		change = check_int_change(proxy, fval);
		upper = check_float_upper_threshold(proxy, fval);
		lower = check_float_lower_threshold(proxy, fval);

		if (proxy->send || timeout || change || upper || lower) {
			proxy->olen = sizeof(float);
			proxy->value.val_f = fval;
			proxy->send = true;
		}
		break;
	default:
		return;
	}
}

bool knot_proxy_value_set_string(struct knot_proxy *proxy,
				 const char *value, int len)
{
	bool change;
	bool timeout;

	if (unlikely(!proxy))
		return false;

	if (proxy->schema.value_type != KNOT_VALUE_TYPE_RAW)
		return false;

	timeout = check_timeout(proxy);

	/* Match current value? */
	change = check_raw_change(proxy, value, len);

	if (!proxy->send && !change && !timeout)
		return false;

	/* len may not include null */
	len = MIN(KNOT_DATA_RAW_SIZE, len);
	proxy->olen = len; /* Amount to send */
	proxy->rlen = len; /* RAW type length */
	memcpy(proxy->value.raw, value, len);
	proxy->send = proxy->wait_resp;

	return true;
}

bool knot_proxy_value_get_basic(struct knot_proxy *proxy, void *value)
{
	bool *bval;
	s32_t *s32val;
	float *fval;

	if (unlikely(!proxy))
		return false;

	switch(proxy->schema.value_type) {
	case KNOT_VALUE_TYPE_BOOL:
		bval = (bool *) value;
		*bval = proxy->value.val_b;
		break;
	case KNOT_VALUE_TYPE_INT:
		s32val = (s32_t *) value;
		*s32val = proxy->value.val_i;
		break;
	case KNOT_VALUE_TYPE_FLOAT:
		fval = (float *) value;
		*fval = proxy->value.val_f;
		break;
	default:
		return false;
	}

	return true;
}

bool knot_proxy_value_get_string(struct knot_proxy *proxy,
				 char *value, int len, int *olen)
{
	if (unlikely(!proxy))
		return false;

	if (proxy->schema.value_type != KNOT_VALUE_TYPE_RAW)
		return false;

	*olen = MIN(len, proxy->rlen);
	memcpy(value, proxy->value.raw, *olen);

	return true;
}
