/*
 * Copyright (c) 2018, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <string.h>

#include "knot_protocol.h"

void msg_create_schema(knot_msg_schema *msg, u8_t id,
		       const knot_schema *schema, bool end)
{
	msg->hdr.type = (end ? KNOT_MSG_SCHEMA_END : KNOT_MSG_SCHEMA);
	msg->sensor_id = id;

	msg->values.value_type = schema->value_type;
	msg->values.unit = schema->unit;
	/* TODO: missing endianess */
	msg->values.type_id = schema->type_id;
	strncpy(msg->values.name, schema->name, KNOT_PROTOCOL_DATA_NAME_LEN);

	msg->hdr.payload_len = sizeof(msg->values) + sizeof(msg->sensor_id);
}
