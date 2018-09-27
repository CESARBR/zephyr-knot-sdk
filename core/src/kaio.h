/*
 * Copyright (c) 2018, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Internal(Private) functions */

#define KNOT_THING_DATA_MAX    3

const knot_schema *kaio_get_schema(u8_t id);

u8_t kaio_get_last_id(void);

s8_t kaio_read(u8_t id, knot_value_type *value);

s8_t kaio_write(u8_t id, knot_value_type *value);