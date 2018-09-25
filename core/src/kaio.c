/*
 * Copyright (c) 2018, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <string.h>

#include "knot_protocol.h"
#include "kaio.h"

#define KNOT_THING_DATA_MAX    3

static struct aio {
       /* KNoT identifier */
       u8_t                    id;

       /* Schema values */
       u8_t                    value_type;
       u8_t                    unit;
       uint16_t                type_id;
       const char              *name; /* TODO */

       /* Data values */
       bool                    new_value;
       knot_value_type         last_value;
       u8_t                    *last_value_raw;
       u8_t                    raw_length;

       /* Config values */
       knot_config             config;

       /* Time values */
       u32_t                   last_timeout;

       kaio_callback_t		read_cb;
       kaio_callback_t		write_cb;
} aio[KNOT_THING_DATA_MAX];


s8_t kaio_register(u8_t id, const char *name,
		   u16_t type_id, u8_t value_type, u8_t unit,
		   kaio_callback_t read_cb, kaio_callback_t write_cb)
{
	struct aio *io;

	/* Assigned already? */
	if (aio[id].id != -1)
		return -EACCES;

	/* Basic field validation */
	if (knot_schema_is_valid(type_id, value_type, unit) != 0 || !name)
		return -EINVAL;

	memset(&aio[id], 0, sizeof(aio[id]));
	io = &aio[id];

	io->id = id;
	io->name = name;
	io->type_id = type_id;
	io->unit = unit;
	io->value_type = value_type;
	io->new_value = false;

	/* Set default config */
	io->config.event_flags = KNOT_EVT_FLAG_TIME;
	io->config.time_sec = 30;

	io->read_cb = read_cb;
	io->write_cb = write_cb;

	return 0;

}
