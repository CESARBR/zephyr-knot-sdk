/*
 * Copyright (c) 2018, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <string.h>

#include "knot_protocol.h"

#include "msg.h"

size_t msg_create_error(knot_msg *msg, uint8_t id, int8_t result)
{
	msg->action.hdr.type = id;
	msg->action.result = result;

	return (sizeof(knot_msg_result) - sizeof(msg->action.hdr));
}

size_t msg_create_reg(knot_msg *msg, uint64_t id,
		      const char *name, size_t name_len)
{
	msg->reg.hdr.type = KNOT_MSG_REGISTER_REQ;
	memcpy(msg->reg.devName, name, name_len);
	msg->reg.hdr.payload_len = name_len + sizeof(msg->reg.id);
	/* TODO: missing endianness */
	msg->reg.id = id;

	return (sizeof(msg->reg.hdr) + msg->reg.hdr.payload_len);
}

size_t msg_create_auth(knot_msg *msg, const char *uuid, const char *token)
{
	strncpy(msg->auth.uuid, uuid, KNOT_PROTOCOL_UUID_LEN);
	strncpy(msg->auth.token, token, KNOT_PROTOCOL_TOKEN_LEN);

	msg->hdr.type = KNOT_MSG_AUTH_REQ;
	msg->hdr.payload_len = KNOT_PROTOCOL_UUID_LEN +
		KNOT_PROTOCOL_TOKEN_LEN;
	return (sizeof(msg->hdr) + msg->hdr.payload_len);
}

size_t msg_create_schema(knot_msg *msg, u8_t id,
		       const knot_schema *schema, bool end)
{
	msg->hdr.type = (end ? KNOT_MSG_SCHEMA_END : KNOT_MSG_SCHEMA);
	msg->schema.sensor_id = id;

	msg->schema.values.value_type = schema->value_type;
	msg->schema.values.unit = schema->unit;
	/* TODO: missing endianess */
	msg->schema.values.type_id = schema->type_id;
	strncpy(msg->schema.values.name, schema->name,
		KNOT_PROTOCOL_DATA_NAME_LEN);

	msg->hdr.payload_len = sizeof(msg->schema.values) +
				sizeof(msg->schema.sensor_id);

	return (sizeof(msg->hdr) + msg->hdr.payload_len);
}

size_t msg_create_data(knot_msg *msg, u8_t id,
		       const knot_value_type *value, uint8_t value_len,
		       bool resp)
{
	msg->hdr.type = resp ? KNOT_MSG_DATA_RESP : KNOT_MSG_DATA;
	msg->data.sensor_id = id;

	msg->hdr.payload_len = sizeof(id) + value_len;
	memcpy(&msg->data.payload, value, value_len);

	return (sizeof(msg->hdr) + msg->hdr.payload_len);
}
