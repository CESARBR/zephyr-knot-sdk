/*
 * Copyright (c) 2018, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include <string.h>

#include "knot_protocol.h"
#include "knot_types.h"
#include "msg.h"
#include "proxy.h"
#include "knot.h"

#define MIN(a, b)         (((a) < (b)) ? (a) : (b))

#define check_int_change(proxy, value)	\
	(KNOT_EVT_FLAG_CHANGE & proxy->config.event_flags \
	&& value != proxy->value.val_i.value)

#define check_int_up_limit(proxy, value)	\
	(KNOT_EVT_FLAG_UPPER_THRESHOLD & proxy->config.event_flags \
	&& value > proxy->value.val_i.value)

#define check_int_low_limit(proxy, value)	\
	(KNOT_EVT_FLAG_LOWER_THRESHOLD & proxy->config.event_flags \
	&& value < proxy->value.val_i.value)


static struct knot_proxy {
	/* KNoT identifier */
	u8_t			id;

	/* Schema values */
	knot_schema		schema;

	/* Data values */
	bool			send;
	knot_value_type		value;
	u8_t			len;

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
	proxy->send = true;
	proxy->len = 0;

	strncpy(proxy->schema.name, name,
		MIN(KNOT_PROTOCOL_DATA_NAME_LEN, strlen(name)));

	/* Set default config */
	proxy->config.event_flags = KNOT_EVT_FLAG_TIME;
	proxy->config.time_sec = 30;

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

s8_t proxy_read(u8_t id, knot_value_type *value)
{
	struct knot_proxy *proxy;

	if (proxy_pool[id].id == 0xff)
		return -EINVAL;

	proxy = &proxy_pool[id];

	if (proxy->poll_cb == NULL)
		return 0;

	proxy->len = 0;

	/* FIXME: */
	proxy->poll_cb(proxy, NULL);

	/*
	 * Read callback may set new values. When a
	 * new value is set "len" field is set.
	 */
	return proxy->len;
}

s8_t proxy_write(u8_t id, knot_value_type *value)
{
	struct knot_proxy *proxy;

	if (proxy_pool[id].id == 0xff)
		return -EINVAL;

	proxy = &proxy_pool[id];

	if (proxy->changed_cb == NULL)
		return 0;

	memcpy(&proxy->value, value, sizeof(*value));

	/*
	 * New values sent from cloud are informed to
	 * the user app through write callback.
	 */

	/* FIXME: */
	proxy->changed_cb(proxy, NULL);

	return proxy->len;
}

s8_t proxy_force_send(u8_t id)
{
	struct knot_proxy *proxy;

	if (proxy_pool[id].id == 0xff)
		return -EINVAL;

	proxy = &proxy_pool[id];

	proxy->send = true;

	return 0;
}

static s8_t proxy_set_value(u8_t id, knot_value_type *value)
{
	/* TODO: Split this function */
	struct knot_proxy *proxy;

	if (proxy_pool[id].id == 0xff)
		return -EINVAL;

	proxy = &proxy_pool[id];

	switch (proxy->schema.value_type) {
	case KNOT_VALUE_TYPE_FLOAT:
		/* TODO: Include decimal float part to comparison */
		if (KNOT_EVT_FLAG_CHANGE & proxy->config.event_flags
				&& value->val_f.value_int
				!= proxy->value.val_f.value_int)
			proxy->send = true;

		if (KNOT_EVT_FLAG_UPPER_THRESHOLD & proxy->config.event_flags
				&& value->val_f.value_int
				> proxy->value.val_f.value_int)
			proxy->send = true;

		if (KNOT_EVT_FLAG_LOWER_THRESHOLD & proxy->config.event_flags
				&& value->val_f.value_int
				< proxy->value.val_f.value_int)
			proxy->send = true;
		break;
	case KNOT_VALUE_TYPE_BOOL:
		if (KNOT_EVT_FLAG_CHANGE & proxy->config.event_flags
				&& value->val_b != proxy->value.val_b)
			proxy->send = true;
		break;
	case KNOT_VALUE_TYPE_RAW:
		/* TODO: Deal with raw size */
		if (KNOT_EVT_FLAG_CHANGE & proxy->config.event_flags
				&& !strcmp(value->raw, proxy->value.raw))
			proxy->send = true;
		break;
	default:
		return -1;
	}

	if (proxy->send == true)
		memcpy(value, &proxy->value, sizeof(*value));

	return 0;
}

static s8_t proxy_get_value(u8_t id, knot_value_type *value)
{
	struct knot_proxy *proxy;

	if (proxy_pool[id].id == 0xff)
		return -EINVAL;

	proxy = &proxy_pool[id];

	memcpy(&proxy->value, value, sizeof(*value));

	return sizeof(*value);
}

static bool check_timeout(u8_t id)
{
	u32_t current_time, elapsed_time;
	struct knot_proxy *proxy;

	proxy = &proxy_pool[id];

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

/* TODO: Set bool data */

void knot_set_int(u8_t id, const int value)
{
	struct knot_proxy *proxy;

	proxy = &proxy_pool[id];

	if (proxy->id == 0xff ||
	    proxy->schema.value_type != KNOT_VALUE_TYPE_INT)
		return;

	/* Checking if should send data or not*/
	if ( (proxy->send == true) ||
	     check_timeout(id) ||
	     check_int_change(proxy, value) ||
	     check_int_up_limit(proxy, value) ||
	     check_int_low_limit(proxy, value) )
	{
		proxy->send = true;
		proxy->len = sizeof(int);
		proxy->value.val_i.value = value;
	}
	return;
}

/* TODO: Set float data */

/* TODO: Set raw data */

s8_t knot_get_int(u8_t id, int *value)
{
	struct knot_proxy *proxy;

	proxy = &proxy_pool[id];

	if (proxy->id == 0xff)
		return -EINVAL;

	*value = proxy->value.val_i.value;
	proxy->len = sizeof(int);
	return sizeof(int);
}
