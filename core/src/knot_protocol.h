/*
 * Copyright (c) 2015, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */
#include <stdint.h>

#ifndef KNOT_PROTOCOL_H
#define KNOT_PROTOCOL_H

#define KNOT_NO_DATA				2
#define KNOT_DONE				1
#define KNOT_SUCCESS				0
#define KNOT_ERROR_UNKNOWN			-1
#define KNOT_INVALID_DEVICE			-2
#define KNOT_INVALID_DATA			-3
#define KNOT_INVALID_DATA_RAW			-4
#define KNOT_DEVICE_NOT_FOUND			-5
#define KNOT_GW_FAILURE				-6
#define KNOT_CLOUD_FAILURE			-7
#define KNOT_CLOUD_OFFLINE			-8
#define KNOT_INVALID_UUID			-9
#define KNOT_INVALID_UUID_CLOUD			-10
#define KNOT_REGISTER_INVALID_DEVICENAME	-11
#define KNOT_INVALID_SCHEMA			-12
#define KNOT_SCHEMA_NOT_FOUND			-13
#define KNOT_SCHEMA_EMPTY			-14
#define KNOT_INVALID_CREDENTIAL			-15
#define KNOT_CREDENTIAL_UNAUTHORIZED		-16

// Each KNoT Device or user has a unique ID and token as identification
// mechanism
#define KNOT_PROTOCOL_TOKEN_LEN			40
#define KNOT_PROTOCOL_UUID_LEN			36
// A human readable name for each device
#define KNOT_PROTOCOL_DEVICE_NAME_LEN		64
// A human readint8_table name for each data source/sink in the device
#define KNOT_PROTOCOL_DATA_NAME_LEN		64

#define KNOT_PROTOCOL_DATA_ID_MAX		0xFE
#define KNOT_PROTOCOL_DATA_ID_NA		0xFF

#define KNOT_MSG_INVALID			0x00
// KNoT connect/register messages (from device)
#define KNOT_MSG_REGISTER_REQ			0x10
#define KNOT_MSG_REGISTER_RESP			0x11
#define KNOT_MSG_UNREGISTER_REQ			0x12
#define KNOT_MSG_UNREGISTER_RESP		0x13
#define KNOT_MSG_AUTH_REQ			0x14
#define KNOT_MSG_AUTH_RESP			0x15
/*
 * KNoT device config messages (from device)
 * END flag indicates end of schema transfer.
 */
#define KNOT_MSG_SCHEMA				0x40
#define KNOT_MSG_SCHEMA_RESP			0x41
#define KNOT_MSG_SCHEMA_END			0x42
#define KNOT_MSG_SCHEMA_END_RESP		0x43
// KNoT data sending config messages (from gateway)
#define KNOT_MSG_GET_CONFIG			0x50
#define KNOT_MSG_SET_CONFIG			0x51
// KNoT request messages (from gateway)
#define KNOT_MSG_GET_DATA			0x30
#define KNOT_MSG_SET_DATA			0x31
#define KNOT_MSG_GET_COMMAND			0x32
#define KNOT_MSG_SET_COMMAND			0x33
// KNoT response messages (from device)
#define KNOT_MSG_DATA				0x20
#define KNOT_MSG_DATA_RESP			0x21
#define KNOT_MSG_COMMAND			0x22
#define KNOT_MSG_CONFIG				0x24
#define KNOT_MSG_CONFIG_RESP			0x25

// KNoT event flags passed by config messages
#define KNOT_EVT_FLAG_NONE			0x00
#define KNOT_EVT_FLAG_TIME			0x01
#define KNOT_EVT_FLAG_LOWER_THRESHOLD		0x02
#define KNOT_EVT_FLAG_UPPER_THRESHOLD		0x04
#define KNOT_EVT_FLAG_CHANGE			0x08
#define KNOT_EVT_FLAG_UNREGISTERED		0x80
#define KNOT_EVENT_FLAG_MAX			(KNOT_EVT_FLAG_TIME |\
						KNOT_EVT_FLAG_LOWER_THRESHOLD |\
						KNOT_EVT_FLAG_UPPER_THRESHOLD |\
						KNOT_EVT_FLAG_CHANGE)

#define KNOT_DATA_RAW_SIZE			16 // 16 bytes for any command
						   // Can be increased if needed

typedef struct __attribute__ ((packed)) {
	uint8_t			type;
	uint8_t			payload_len;
} knot_msg_header;

typedef struct __attribute__ ((packed)) {
	knot_msg_header		hdr;
	int8_t			result;
} knot_msg_result;

typedef struct __attribute__ ((packed)) {
	knot_msg_header		hdr;
	uint8_t			sensor_id;	// App defined sensor id
} knot_msg_item;

typedef struct __attribute__ ((packed)) {
	int32_t			multiplier;
	int32_t			value_int;
	uint32_t		value_dec;
} knot_value_type_float;

typedef struct __attribute__ ((packed)) {
	int32_t			multiplier;
	int32_t			value;
} knot_value_type_int;

typedef uint8_t knot_value_type_bool;

typedef union __attribute__ ((packed)) {
	knot_value_type_int	val_i;
	knot_value_type_float	val_f;
	knot_value_type_bool	val_b;
	uint8_t			raw[KNOT_DATA_RAW_SIZE];
} knot_value_type;

typedef struct __attribute__ ((packed)) {
	knot_msg_header		hdr;
	uint8_t			sensor_id;	// App defined sensor id
	knot_value_type		payload;
} knot_msg_data;

typedef struct __attribute__ ((packed)) {
	uint8_t			event_flags;
	uint16_t		time_sec;
	knot_value_type		lower_limit;
	knot_value_type		upper_limit;
} knot_config;

typedef struct __attribute__ ((packed)) {
	knot_msg_header		hdr;
	uint8_t			sensor_id;	// App defined sensor id
	knot_config		values;
} knot_msg_config; // hdr + 1 + 37 bytes

typedef struct __attribute__ ((packed)) {
	knot_msg_header		hdr;
	int8_t			result;
	char			uuid[KNOT_PROTOCOL_UUID_LEN];
	char			token[KNOT_PROTOCOL_TOKEN_LEN];
} knot_msg_credential; // hdr + 40 + 36 bytes

typedef struct __attribute__ ((packed)) {
	knot_msg_header		hdr;
	uint64_t		id;	/* Device id: mac or user defined */
	char			devName[KNOT_PROTOCOL_DEVICE_NAME_LEN];
} knot_msg_register; // hdr + 64 bytes

/* Requirement: authenticated PHY link */
typedef struct __attribute__ ((packed)) {
	knot_msg_header		hdr;
} knot_msg_unregister;

typedef struct __attribute__ ((packed)) {
	knot_msg_header		hdr;
	char			uuid[KNOT_PROTOCOL_UUID_LEN];
	char			token[KNOT_PROTOCOL_TOKEN_LEN];
} knot_msg_authentication;

typedef struct __attribute__ ((packed)) {
	uint8_t			value_type;	// KNOT_VALUE_TYPE_* (int, float, bool, raw)
	uint8_t			unit;		// KNOT_UNIT_*
	uint16_t		type_id;	// KNOT_TYPE_ID_*
	char			name[KNOT_PROTOCOL_DATA_NAME_LEN];
} knot_schema; // 69 bytes

typedef struct __attribute__ ((packed)) {
	knot_msg_header		hdr;
	uint8_t			sensor_id;	// App defined sensor id
	knot_schema		values;
} knot_msg_schema;

#define KNOT_MSG_SIZE		(sizeof(knot_msg_header) + sizeof(knot_msg_credential))  // must be greater than max structure size defined above

typedef union __attribute__ ((packed)) {
	knot_msg_header		hdr;
	knot_msg_result		action;
	knot_msg_item		item;
	knot_msg_data		data;
	knot_msg_credential	cred;
	knot_msg_register	reg;
	knot_msg_unregister	unreg;
	knot_msg_authentication	auth;
	knot_msg_schema		schema;
	knot_msg_config		config;
	uint8_t			buffer[KNOT_MSG_SIZE];
} knot_msg;

/*
 * Helper function to validate the value type
 */
int knot_value_type_is_valid(uint8_t type);

/*
 * Helper function to verify if type_id is in basic range
 */
int knot_type_id_is_basic(uint16_t type_id);

/*
 * Helper function to verify if type_id is in logic range
 */
int knot_type_id_is_logic(uint16_t type_id);

/*
 * Helper function to validate the schema
 */
int knot_schema_is_valid(uint16_t type_id, uint8_t value_type, uint8_t unit);

/*
 * Helper function to validate the config
 */
int knot_config_is_valid(uint8_t event_flags, uint16_t time_sec,
		knot_value_type *lower_limit, knot_value_type *upper_limit);


#endif //KNOT_PROTOCOL_H
