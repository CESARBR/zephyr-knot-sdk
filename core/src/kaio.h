/*
 * Copyright (c) 2018, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

typedef int (*kaio_callback_t) (u8_t id);

s8_t kaio_register(u8_t id, const char *name,
		   u16_t type_id, u8_t value_type, u8_t unit,
		   kaio_callback_t read_cb, kaio_callback_t write_cb);
