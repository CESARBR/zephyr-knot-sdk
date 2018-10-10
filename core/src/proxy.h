/*
 * Copyright (c) 2018, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Internal(Private) functions */

#define KNOT_THING_DATA_MAX    5

const knot_schema *proxy_get_schema(u8_t id);

void proxy_start(void);

void proxy_stop(void);

u8_t proxy_get_last_id(void);

s8_t proxy_read(u8_t id, knot_value_type *value);

s8_t proxy_write(u8_t id, knot_value_type *value, u8_t value_len);

s8_t proxy_force_send(u8_t id);
