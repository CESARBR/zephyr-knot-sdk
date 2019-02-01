/*
 * Copyright (c) 2018, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include <string.h>

#include "knot_protocol.h"
#include "msg.h"
#include "kaio.h"
#include "knot.h"

#define MIN(a, b)         (((a) < (b)) ? (a) : (b))

static struct aio {
	/* KNoT identifier */
	u8_t			id;

	/* Schema values */
	knot_schema		schema;

	/* Data values */
	bool			new_value;
	knot_value_type		last_value;
	u8_t			*last_value_raw;
	u8_t			raw_length;

	/* Config values */
	knot_config		config;

	/* Time values */
	u32_t			last_timeout;

	knot_callback_t		read_cb;
	knot_callback_t		write_cb;
} aio[KNOT_THING_DATA_MAX];

static u8_t last_id = 0xff;

void knot_start(void)
{
	int index;

	memset(aio, 0, sizeof(aio));

	for (index = 0; (index < sizeof(aio) / sizeof(struct aio)); index++)
		aio[index].id = 0xff;
}

void knot_stop(void)
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
	io->new_value = false;

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
