/*
 * Copyright (c) 2018, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

size_t msg_create_reg(knot_msg *msg, uint64_t id,
		      const char *name, size_t name_len);
size_t msg_create_auth(knot_msg *msg, const char *uuid, const char *token);
size_t msg_create_schema(knot_msg *msg, u8_t id,
		       const knot_schema *schema, bool end);
size_t msg_create_data(knot_msg *msg, u8_t id, knot_value_type *value);
