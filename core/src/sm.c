/* sm.c - KNoT Application Client */

/*
 * Copyright (c) 2018, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* KNoT State Machine */

#define SYS_LOG_DOMAIN "knot"
#define NET_SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_LOG_ENABLED 1

#include <zephyr.h>
#include <net/net_core.h>
#include <misc/reboot.h>

#include "knot_protocol.h"
#include "proxy.h"
#include "msg.h"
#include "sm.h"
#include "storage.h"
#include "peripheral.h"

#define TIMEOUT_WIN				3 /* 3 sec */

static struct k_timer to;	/* Re-send timeout */
static bool to_on;		/* Timeout active */
static bool to_exp;		/* Timeout expired */

/*
 * Internally uuid and token must be null terminated. When copying or
 * or sending over the network, null should not be included.
 */
static char uuid[KNOT_PROTOCOL_UUID_LEN + 1];	/* Device uuid */
static char token[KNOT_PROTOCOL_TOKEN_LEN + 1];	/* Device token */
static u64_t device_id;				/* Device id */

enum sm_state {
	STATE_REG,		/* Registers new device */
	STATE_AUTH,		/* Authenticate known device */
	STATE_SCH,		/* Sends schema */
	STATE_ONLINE,		/* Default state: sends & receive */
	STATE_ERROR,		/* Reg, auth, sch error */
};

static enum sm_state state;

static void timer_expired(struct k_timer *to)
{
	to_exp = true;
	to_on = false;
	NET_WARN("TO");
}

static bool cmp_opcode(const u8_t exp_opcode, const u8_t *ipdu, size_t ilen)
{
	const knot_msg *imsg;
	/* No response found */
	if (ilen == 0)
		return false;

	/* No response expected */
	if (exp_opcode == 0xff)
		return false;

	imsg = (knot_msg *) ipdu;

	/* Return true if found expected response */
	return imsg->hdr.type == exp_opcode;
}

/* Check if received OPCODE belongs to white list of actual state */
static bool wl_opcode(const enum sm_state state, const u8_t *ipdu, size_t ilen)
{
	const knot_msg *imsg;
	/* No response found */
	if (ilen == 0)
		return false;

	/* White list is only for ONLINE state */
	if (state != STATE_ONLINE)
		return false;

	imsg = (knot_msg *) ipdu;

	/* Return true if find expected response */
	switch (imsg->hdr.type) {
	/* TODO: Add, after implement,
	 * UNREGISTER_REQ, SET_CONFIG, and GET_CONFIG
	 */
	case KNOT_MSG_SET_DATA:
	case KNOT_MSG_GET_DATA:
		return true;
	default:
		return false;
	}
}

static enum sm_state state_register(u8_t *exp_opcode,
				    const u8_t *ipdu, size_t ilen,
				    u8_t *opdu, size_t olen, size_t *len)
{
	enum sm_state next = STATE_REG;
	const char *devname = CONFIG_KNOT_NAME;
	knot_msg *msg;

	/* First attempt or timeout expired, send register request */
	if (*exp_opcode == 0xff || to_exp) {
		msg = (knot_msg *) opdu;
		*len = msg_create_reg(msg, device_id, devname, strlen(devname));
		*exp_opcode = KNOT_MSG_REGISTER_RESP;
		goto done;
	}

	*len = 0;

	/* Decode received message */
	msg = (knot_msg *) ipdu;

	/* OPCODE verified before entering state. Checking result */
	if (msg->cred.result != KNOT_SUCCESS) {
		next = STATE_ERROR;
		goto done;
	}

	memcpy(uuid, msg->cred.uuid, KNOT_PROTOCOL_UUID_LEN);
	memcpy(token, msg->cred.token, KNOT_PROTOCOL_TOKEN_LEN);

	next = STATE_SCH;
done:
	return next;
}

static enum sm_state state_auth(u8_t *exp_opcode,
				const u8_t *ipdu, size_t ilen,
				u8_t *opdu, size_t olen, size_t *len)
{
	knot_msg *msg;
	enum sm_state next = STATE_AUTH;

	/* First attempt or timeout expired, send auth request */
	if (*exp_opcode == 0xff || to_exp) {
		/* Send authentication request and waiting response */
		msg = (knot_msg *) opdu;
		/* TODO: Read credentials from non-volatile memory */
		*len = msg_create_auth(msg, uuid, token);
		*exp_opcode = KNOT_MSG_AUTH_RESP;
		goto done;
	}

	/* Decode received message */
	msg = (knot_msg *) ipdu;
	*len = 0;

	/* OPCODE verified before entering state. Checking result */
	if (msg->action.result != KNOT_SUCCESS) {
		next = STATE_ERROR;
		goto done;
	}

	/* Credentials are only saved on NVM after all the schemas are sent */
	next =  STATE_ONLINE;

done:
	return next;
}

static enum sm_state state_schema(u8_t *exp_opcode,
				  const u8_t *ipdu, size_t ilen,
				  u8_t *opdu, size_t olen, size_t *len)
{
	knot_msg *imsg = (knot_msg *) ipdu;
	knot_msg *omsg = (knot_msg *) opdu;
	enum sm_state next = STATE_SCH;
	const knot_schema *schema;
	static u8_t id_index = 0;
	u8_t last_id;
	bool end;

	*len = 0;

	/* First attempt or timeout expired, resend schemas */
	if (*exp_opcode == 0xff || to_exp) {
		id_index = 0;
		goto send;
	}

	/* OPCODE verified before entering state. Checking result */
	switch (*exp_opcode) {
	case KNOT_MSG_SCHEMA_RESP:
		/* Resend last fragment if failed */
		if (imsg->action.result == KNOT_SUCCESS)
			id_index++;
		goto send;
	case KNOT_MSG_SCHEMA_END_RESP:
		if (imsg->action.result != KNOT_SUCCESS)
			goto send;
		if (storage_set(STORAGE_KEY_UUID, uuid) < 0) {
			next = STATE_ERROR;
			goto done;
		}
		if (storage_set(STORAGE_KEY_TOKEN, token) < 0) {
			next = STATE_ERROR;
			goto done;
		}
		if (storage_set(STORAGE_KEY_ID,
				(const u8_t *) &device_id) < 0) {
			next = STATE_ERROR;
			goto done;
		}

		NET_DBG("UUID: %s", uuid);
		NET_DBG("token: %s", token);

		next = STATE_ONLINE;
	default:
		goto done;
	}

send:
	/* Send schema */
	last_id = proxy_get_last_id();
	while (id_index <= last_id) {
		schema = proxy_get_schema(id_index);
		if (schema == NULL) {
			/* Ignore invalid aio  */
			id_index++;
			continue;
		}
		end = ((id_index == last_id) ? true : false);
		*exp_opcode = (end ? KNOT_MSG_SCHEMA_END_RESP :
				     KNOT_MSG_SCHEMA_RESP);
		*len = msg_create_schema(omsg, id_index, schema, end);
		break;
	}
done:
	return next;
}

static size_t process_event(u8_t *exp_opcode,
			    const u8_t *ipdu, size_t ilen,
			    u8_t *opdu, size_t olen)
{
	knot_msg *omsg = (knot_msg *) opdu;
	knot_msg *imsg = (knot_msg *) ipdu;
	const knot_value_type *value;
	uint8_t value_len = 0;
	static u8_t id_index = 0;
	u8_t old_id;
	u8_t last_id;
	s8_t len = 0;

	last_id = proxy_get_last_id();

	/* No response expected. Continue polling */
	if (*exp_opcode == 0xff)
		goto polling;

	/*
	 * OPCODE and timeout verified before entering state.
	 * If timeout expired or received an error message simply ignore it
	 * and send data of the next sensor.
	 */
	if (to_exp || imsg->action.result != KNOT_SUCCESS)
		NET_ERR("FAIL SEND FOR ID: %d", id_index);
	else
		proxy_confirm_sent(id_index);

polling:
	/*
	 * The polling is finished when a message to be sent is found or when
	 * finish reading all sensors
	 */
	old_id = id_index; /* Old sensor id */
	do {
		id_index = (id_index < last_id ? id_index + 1 : 0);

		value = proxy_read(id_index, &value_len, true);
		/* Check next sensor if no data is to be sent */
		if (!value) {
			continue;
		}

		/* Send data and wait for response */
		len = msg_create_data(omsg, id_index, value, value_len, false);
		*exp_opcode = KNOT_MSG_DATA_RESP;
		break;
	} while (id_index != old_id);

	if (len <= 0)
		*exp_opcode = 0xff;

	return len;
}

static size_t process_cmd(const u8_t *ipdu, size_t ilen,
			       u8_t *opdu, size_t olen)
{
	knot_msg *imsg = (knot_msg *) ipdu;
	knot_msg *omsg = (knot_msg *) opdu;
	size_t len = 0;

	u8_t id = 0xff;
	const knot_value_type *value;
	uint8_t value_len;

	switch (imsg->hdr.type) {
	case KNOT_MSG_UNREGISTER_REQ:
		/* Clear NVM */
		break;
	case KNOT_MSG_GET_DATA:
		id = imsg->data.sensor_id;

		/* Invalid id */
		if (id > CONFIG_KNOT_THING_DATA_MAX) {
			len = msg_create_error(omsg,
					       KNOT_MSG_DATA,
					       KNOT_INVALID_DATA);
			NET_WARN("Invalid Id!");
			break;
		}

		/* Flag data to be sent and don't wait response */
		proxy_force_send(id);
		value = proxy_read(id, &value_len, false);

		/* Couldn't read value */
		if (unlikely(!value)) {
			len = msg_create_error(omsg,
					       KNOT_MSG_DATA,
					       KNOT_INVALID_DATA);
			NET_WARN("Can't read requested value");
			break;
		}

		len = msg_create_data(omsg, id, value, value_len, false);
		break;
	case KNOT_MSG_SET_DATA:
		id = imsg->data.sensor_id;
		value = &imsg->data.payload;
		/* value_len = payload_len - sensor_id_len */
		value_len = imsg->hdr.payload_len;
		value_len -= sizeof(id);

		if (proxy_write(id, value, value_len) < 0) {
			len = msg_create_error(omsg,
					       KNOT_MSG_DATA_RESP,
					       KNOT_INVALID_DATA);
			break;
		}
		/* TODO: Change protocol to not require id nor value for
		 * set data response messages
		 */
		len = msg_create_data(omsg, id, value, value_len, true);
		break;
	case KNOT_MSG_GET_CONFIG:
		/* TODO */
		break;
	case KNOT_MSG_SET_CONFIG:
		/* TODO */
		break;
	case KNOT_MSG_GET_COMMAND:
		/* TODO */
		break;
	case KNOT_MSG_SET_COMMAND:
		/* TODO */
		break;
	default:
		break;
	}

	return len;
}

static enum sm_state state_online(u8_t *exp_opcode,
				  const u8_t *ipdu, size_t ilen,
				  u8_t *opdu, size_t olen, size_t *len)
{
	enum sm_state next = STATE_ONLINE;
	size_t ret_len = 0;

	/* Incoming commands: higher priority */
	if (ilen != 0)
		/* Received command */
		ret_len = process_cmd(ipdu, ilen, opdu, olen);

	/* Local sensor/actuator */
	if (ret_len == 0)
		/* Local event */
		ret_len = process_event(exp_opcode, ipdu, ilen, opdu, olen);

	if (ret_len > 0)
		*len = ret_len;

	return next;
}

/*
 * Start state machine selecting the first state it should go to.
 * If the thing has credentials stored, send auth request.
 * Otherwise send register request.
 */
int sm_start(void)
{
	int8_t err;

	NET_DBG("SM: Start");

	state = STATE_AUTH; /* Initial state */

	device_id = 0;
	memset(uuid, 0, sizeof(uuid));
	memset(token, 0, sizeof(token));

	/*
	 * 'device_id' stored properly at NVM means that UUID
	 * and token are also available.
	 */
	err = storage_get(STORAGE_KEY_ID, (u8_t *) &device_id);
	if (err < 0 || device_id == 0) {
		device_id = sys_rand32_get();
		device_id *= device_id;
		state = STATE_REG;
		goto done;
	}

	err = storage_get(STORAGE_KEY_UUID, uuid);
	if (err < 0)
		state = STATE_REG;

	err = storage_get(STORAGE_KEY_TOKEN, token);
	if (err < 0)
		state = STATE_REG;

done:
	to_on = false;
	to_exp = false;

	return 0;
}

void sm_stop(void)
{
	NET_DBG("SM: Stop");
	if (to_on)
		k_timer_stop(&to);

	proxy_stop();
}

void sm_init(void)
{
	NET_DBG("SM: Init");

	k_timer_init(&to, timer_expired, NULL);

	/* Initializing proxy slots */
	proxy_init();
}

int sm_run(const u8_t *ipdu, size_t ilen, u8_t *opdu, size_t olen)
{
	enum sm_state next;
	size_t len = 0;
	bool reset;

	/* Expected OPCODE response. Initially not expecting response*/
	static u8_t exp_opcode = 0xff;
	bool got_resp; /* Got right response */

	/* Handle reset flag */
	reset = peripheral_get_reset();
	/* TODO: Consider current state before reseting */
	if (reset) {
		/* TODO: Unregister before reseting */
		NET_INFO("Reseting system...");
		storage_reset();
		#if !CONFIG_BOARD_QEMU_X86
			sys_reboot(SYS_REBOOT_WARM);
		#endif
	}

	/*
	 * When the timer is enabled, if no data is received or the expected
	 * response was not matched, it is not necessary to run the state
	 * machine.
	 * In case of a white listed command, proceed so command can be handled.
	 */
	if (to_on) {
		got_resp = cmp_opcode(exp_opcode, ipdu, ilen);
		if (got_resp) {
			/* Stop timer if response found */
			k_timer_stop(&to);
			to_on = false;
			to_exp = false;

		} else if (wl_opcode(state, ipdu, ilen) == false)
			/* OPCODE doesn't belong to white list. Wait */
			return 0;
	}

	switch (state) {
	case STATE_REG:
		/* Register new device */
		next = state_register(&exp_opcode, ipdu, ilen,
				      opdu, olen, &len);
		break;
	case STATE_AUTH:
		/* Authenticate if registed previously */
		next = state_auth(&exp_opcode, ipdu, ilen, opdu, olen, &len);
		break;
	case STATE_SCH:
		/* Send schemas */
		next = state_schema(&exp_opcode, ipdu, ilen, opdu, olen, &len);
		break;
	case STATE_ONLINE:
		/* Incoming messages and/or changes on sensors */
		next = state_online(&exp_opcode, ipdu, ilen, opdu, olen, &len);
		break;
	default:
		NET_ERR("ERROR");
		next = STATE_ERROR;
		break;
	}

	/* State has changed: Don't wait response */
	if (next != state) {
		exp_opcode = 0xff;
	}

	/* Not waiting response: Stop timer */
	if (exp_opcode == 0xff) {
		if (to_on) {
			k_timer_stop(&to);
			to_on = false;
			to_exp = false;
		}
		goto done;
	}

	/* Waiting response: Run timer */
	if (to_on == false) {
		k_timer_start(&to, K_SECONDS(TIMEOUT_WIN), 0);
		to_on = true;
		to_exp = false;
		goto done;
	}
done:
	state = next;

	return len;
}
