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

#define KNOT_ERR_INVALID			-1
#define KNOT_ERR_PERM				-2
#define KNOT_ERR_SCHEMA_EMPTY			-3
#define KNOT_ERR_CLOUD_FAILURE			-4

// Each KNoT Device or user has a unique ID and token as identification
// mechanism
#define KNOT_PROTOCOL_TOKEN_LEN			40
#define KNOT_PROTOCOL_UUID_LEN			36
// A human readable name for each device
#define KNOT_PROTOCOL_DEVICE_NAME_LEN		64

#define KNOT_PROTOCOL_DATA_ID_MAX		0xFE
#define KNOT_PROTOCOL_DATA_ID_NA		0xFF

// KNoT connect/register messages (from device)
#define KNOT_MSG_REG_REQ			0x10
#define KNOT_MSG_REG_RSP			0x11
#define KNOT_MSG_UNREG_REQ			0x12
#define KNOT_MSG_UNREG_RSP			0x13
#define KNOT_MSG_AUTH_REQ			0x14
#define KNOT_MSG_AUTH_RSP			0x15

/* Direction: endpoint to cloud */
#define KNOT_MSG_SCHM_FRAG_REQ			0x40
#define KNOT_MSG_SCHM_FRAG_RSP			0x41
#define KNOT_MSG_SCHM_END_REQ			0x42
#define KNOT_MSG_SCHM_END_RSP			0x43

/* Direction: both */
#define KNOT_MSG_PUSH_DATA_REQ			0x20
#define KNOT_MSG_PUSH_DATA_RSP			0x21

/* Direction: cloud to endpoint */
#define KNOT_MSG_PUSH_CONFIG_REQ		0x22
#define KNOT_MSG_PUSH_CONFIG_RSP		0x23
#define KNOT_MSG_POLL_DATA_REQ			0x30
#define KNOT_MSG_POLL_DATA_RSP			0x31

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

/* Considering the IEEE 754 single-precision floating-point Standard */
typedef float knot_value_type_float;

typedef int32_t knot_value_type_int;

typedef uint8_t knot_value_type_bool;

typedef union __attribute__ ((packed)) {
	knot_value_type_int	val_i;
	knot_value_type_float	val_f;
	knot_value_type_bool	val_b;
	uint8_t			raw[KNOT_DATA_RAW_SIZE];
} knot_value_type;

/*
 * See KNOT_MSG_PUSH_DATA_REQ: used to send data from endpoint to
 * GW or encapsule request from external clients to the endpoint
 * (set data).
 */
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

#define KNOT_PROTOCOL_DATA_NAME_LEN		23
typedef struct __attribute__ ((packed)) {
	uint8_t			value_type;	// KNOT_VALUE_TYPE_* (int, float, bool, raw)
	uint8_t			unit;		// KNOT_UNIT_*
	uint16_t		type_id;	// KNOT_TYPE_ID_*
	char			name[KNOT_PROTOCOL_DATA_NAME_LEN];
} knot_schema; // 30 octets

typedef struct __attribute__ ((packed)) {
	knot_msg_header		hdr;
	uint8_t			sensor_id;	// App defined sensor id
	knot_schema		values;
} knot_msg_schema;

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
int knot_config_is_valid(uint8_t event_flags, uint8_t value_type,
		uint16_t time_sec, const knot_value_type *lower_limit,
		 const knot_value_type *upper_limit);


#endif //KNOT_PROTOCOL_H
