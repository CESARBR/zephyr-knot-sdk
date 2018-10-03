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
#include "kaio.h"
#include "knot.h"

#define MIN(a, b)         (((a) < (b)) ? (a) : (b))

#define check_int_change(io, value)	\
	(KNOT_EVT_FLAG_CHANGE & io->config.event_flags \
	&& value != io->value.val_i.value)

#define check_int_up_limit(io, value)	\
	(KNOT_EVT_FLAG_UPPER_THRESHOLD & io->config.event_flags \
	&& value > io->value.val_i.value)

#define check_int_low_limit(io, value)	\
	(KNOT_EVT_FLAG_LOWER_THRESHOLD & io->config.event_flags \
	&& value < io->value.val_i.value)


static struct aio {
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

	knot_callback_t		read_cb;
	knot_callback_t		write_cb;
} aio[KNOT_THING_DATA_MAX];

static u8_t last_id = 0xff;

void kaio_start(void)
{
	int index;

	memset(aio, 0, sizeof(aio));

	for (index = 0; (index < sizeof(aio) / sizeof(struct aio)); index++)
		aio[index].id = 0xff;
}

void kaio_stop(void)
{

}

s8_t knot_register(u8_t id, const char *name,
		   u16_t type_id, u8_t value_type, u8_t unit,
		   knot_callback_t read_cb, knot_callback_t write_cb)
{
	struct aio *io;

	/* Out of index? */
	if (id >= KNOT_THING_DATA_MAX)
		return -EINVAL;

	/* Assigned already? */
	if (aio[id].id != 0xff)
		return -EACCES;

	/* Basic field validation */
	if (knot_schema_is_valid(type_id, value_type, unit) != 0 || !name)
		return -EINVAL;

	io = &aio[id];

	io->id = id;
	io->schema.type_id = type_id;
	io->schema.unit = unit;
	io->schema.value_type = value_type;
	io->send = true;
	io->len = 0;

	strncpy(io->schema.name, name,
		MIN(KNOT_PROTOCOL_DATA_NAME_LEN, strlen(name)));

	/* Set default config */
	io->config.event_flags = KNOT_EVT_FLAG_TIME;
	io->config.time_sec = 30;

	io->read_cb = read_cb;
	io->write_cb = write_cb;

	if (id > last_id || last_id == 0xff)
		last_id = id;

	return 0;
}

const knot_schema *kaio_get_schema(u8_t id)
{
	if (aio[id].id == 0xff)
		return NULL;

	return &(aio[id].schema);
}

u8_t kaio_get_last_id(void)
{
	return last_id;
}

s8_t kaio_read(u8_t id, knot_value_type *value)
{
	struct aio *io;

	if (aio[id].id == 0xff)
		return -EINVAL;

	io = &aio[id];

	if (io->read_cb == NULL)
		return 0;

	io->len = 0;

	io->read_cb(id);

	/*
	 * Read callback may set new values. When a
	 * new value is set "len" field is set.
	 */
	return io->len;
}

s8_t kaio_write(u8_t id, knot_value_type *value)
{
	struct aio *io;

	if (aio[id].id == 0xff)
		return -EINVAL;

	io = &aio[id];

	if (io->write_cb == NULL)
		return 0;

	memcpy(&io->value, value, sizeof(*value));

	/*
	 * New values sent from cloud are informed to
	 * the user app through write callback.
	 */

	io->write_cb(id);

	return io->len;
}

s8_t kaio_force_send(u8_t id)
{
	struct aio *io;

	if (aio[id].id == 0xff)
		return -EINVAL;

	io = &aio[id];

	io->send = true;

	return 0;
}

static s8_t kaio_set_value(u8_t id, knot_value_type *value)
{
	/* TODO: Split this function */
	struct aio *io;

	if (aio[id].id == 0xff)
		return -EINVAL;

	io = &aio[id];

	switch (io->schema.value_type) {
	case KNOT_VALUE_TYPE_FLOAT:
		/* TODO: Include decimal float part to comparison */
		if (KNOT_EVT_FLAG_CHANGE & io->config.event_flags
				&& value->val_f.value_int
				!= io->value.val_f.value_int)
			io->send = true;

		if (KNOT_EVT_FLAG_UPPER_THRESHOLD & io->config.event_flags
				&& value->val_f.value_int
				> io->value.val_f.value_int)
			io->send = true;

		if (KNOT_EVT_FLAG_LOWER_THRESHOLD & io->config.event_flags
				&& value->val_f.value_int
				< io->value.val_f.value_int)
			io->send = true;
		break;
	case KNOT_VALUE_TYPE_BOOL:
		if (KNOT_EVT_FLAG_CHANGE & io->config.event_flags
				&& value->val_b != io->value.val_b)
			io->send = true;
		break;
	case KNOT_VALUE_TYPE_RAW:
		/* TODO: Deal with raw size */
		if (KNOT_EVT_FLAG_CHANGE & io->config.event_flags
				&& !strcmp(value->raw, io->value.raw))
			io->send = true;
		break;
	default:
		return -1;
	}

	if (io->send == true)
		memcpy(value, &io->value, sizeof(*value));

	return 0;
}

static s8_t kaio_get_value(u8_t id, knot_value_type *value)
{
	struct aio *io;

	if (aio[id].id == 0xff)
		return -EINVAL;

	io = &aio[id];

	memcpy(&io->value, value, sizeof(*value));

	return sizeof(*value);
}

static bool check_timeout(u8_t id)
{
	u32_t current_time, elapsed_time;
	struct aio *io;

	io = &aio[id];

	if (!(KNOT_EVT_FLAG_TIME & io->config.event_flags))
		return false;

	current_time = k_uptime_get();
	elapsed_time = current_time - io->last_timeout;
	if (elapsed_time >= (io->config.time_sec * 1000)) {
		io->last_timeout = current_time;
		return true;
	}
	return false;
}

/* TODO: Set bool data */

void knot_set_int(u8_t id, const int value)
{
	struct aio *io;

	io = &aio[id];

	if (io->id == 0xff || io->schema.value_type != KNOT_VALUE_TYPE_INT)
		return;

	/* Checking if should send data or not*/
	if ( (io->send == true) ||
	     check_timeout(id) ||
	     check_int_change(io, value) ||
	     check_int_up_limit(io, value) ||
	     check_int_low_limit(io, value) )
	{
		io->send = true;
		io->len = sizeof(int);
		io->value.val_i.value = value;
	}
	return;
}

/* TODO: Set float data */

/* TODO: Set raw data */

s8_t knot_get_int(u8_t id, int *value)
{
	struct aio *io;

	io = &aio[id];

	if (io->id == 0xff)
		return -EINVAL;

	*value = io->value.val_i.value;
	io->len = sizeof(int);
	return sizeof(int);
}
