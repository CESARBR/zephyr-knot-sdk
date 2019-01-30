/* sm.c - KNoT Application Client */

/*
 * Copyright (c) 2018, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* KNoT State Machine */

#include <zephyr.h>
#include <net/net_core.h>
#include <logging/log.h>
#include <misc/reboot.h>

#include "knot_protocol.h"
#include "proxy.h"
#include "msg.h"
#include "sm.h"
#include "storage.h"
#include "peripheral.h"

LOG_MODULE_DECLARE(knot, LOG_LEVEL_DBG);

#define TIMEOUT_WIN				3 /* 3 sec */

static struct k_timer to;	/* Re-send timeout */
static bool to_on;		/* Timeout active */
static bool to_xpr;		/* Timeout expired */

/*
 * Internally uuid and token must be null terminated. When copying or
 * or sending over the network, null should not be included.
 */
static char uuid[KNOT_PROTOCOL_UUID_LEN + 1];	/* Device uuid */
static char token[KNOT_PROTOCOL_TOKEN_LEN + 1];	/* Device token */
static u64_t device_id;				/* Device id */
static struct storage_app_settings app_settings;/* Storage helper struct */

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
	to_xpr = true;
	to_on = false;
	LOG_WRN("Timeout expired!");
}

static bool cmp_opcode(const u8_t xpt_opcode, const u8_t *ipdu, size_t ilen)
{
	const knot_msg *imsg;
	/* No response found */
	if (ilen == 0)
		return false;

	/* No response expected */
	if (xpt_opcode == 0xff)
		return false;

	imsg = (knot_msg *) ipdu;

	/* Return true if found expected response */
	return imsg->hdr.type == xpt_opcode;
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
	 * UNREG_REQ, SET_CONFIG, and GET_CONFIG
	 */
	case KNOT_MSG_PUSH_DATA_REQ:
	case KNOT_MSG_POLL_DATA_REQ:
		return true;
	default:
		return false;
	}
}

static enum sm_state state_register(u8_t *xpt_opcode,
				    const u8_t *ipdu, size_t ilen,
				    u8_t *opdu, size_t olen, size_t *len)
{
	enum sm_state next = STATE_REG;
	const char *devname = CONFIG_KNOT_NAME;
	knot_msg *msg;

	/* First attempt or timeout expired, send register request */
	if (*xpt_opcode == 0xff || to_xpr) {
		msg = (knot_msg *) opdu;
		*len = msg_create_reg(msg, device_id, devname, strlen(devname));
		*xpt_opcode = KNOT_MSG_REG_RSP;
		goto done;
	}

	*len = 0;

	/* Decode received message */
	msg = (knot_msg *) ipdu;

	/* OPCODE verified before entering state. Checking result */
	if (msg->cred.result != 0) {
		next = STATE_ERROR;
		goto done;
	}

	memcpy(uuid, msg->cred.uuid, KNOT_PROTOCOL_UUID_LEN);
	memcpy(token, msg->cred.token, KNOT_PROTOCOL_TOKEN_LEN);

	next = STATE_SCH;
done:
	return next;
}

static enum sm_state state_auth(u8_t *xpt_opcode,
				const u8_t *ipdu, size_t ilen,
				u8_t *opdu, size_t olen, size_t *len)
{
	knot_msg *msg;
	enum sm_state next = STATE_AUTH;

	/* First attempt or timeout expired, send auth request */
	if (*xpt_opcode == 0xff || to_xpr) {
		/* Send authentication request and waiting response */
		msg = (knot_msg *) opdu;
		/* TODO: Read credentials from non-volatile memory */
		*len = msg_create_auth(msg, uuid, token);
		*xpt_opcode = KNOT_MSG_AUTH_RSP;
		goto done;
	}

	/* Decode received message */
	msg = (knot_msg *) ipdu;
	*len = 0;

	/* OPCODE verified before entering state. Checking result */
	if (msg->action.result != 0) {
		next = STATE_ERROR;
		goto done;
	}

	/* Credentials are only saved on NVM after all the schemas are sent */
	LOG_INF("Successfully authenticated!");
	next =  STATE_ONLINE;

done:
	return next;
}

static enum sm_state state_schema(u8_t *xpt_opcode,
				  const u8_t *ipdu, size_t ilen,
				  u8_t *opdu, size_t olen, size_t *len)
{
	const knot_msg *imsg = (knot_msg *) ipdu;
	knot_msg *omsg = (knot_msg *) opdu;
	enum sm_state next = STATE_SCH;
	const knot_schema *schema;
	static u8_t id_index = 0;
	u8_t last_id;
	size_t res;
	bool end;

	*len = 0;

	/* First attempt or timeout expired, resend schemas */
	if (*xpt_opcode == 0xff || to_xpr) {
		id_index = 0;
		goto send;
	}

	/* OPCODE verified before entering state. Checking result */
	switch (*xpt_opcode) {
	case KNOT_MSG_SCHM_FRAG_RSP:
		/* Resend last fragment if failed */
		if (imsg->action.result == 0)
			id_index++;
		goto send;
	case KNOT_MSG_SCHM_END_RSP:
		if (imsg->action.result != 0)
			goto send;

		LOG_DBG("Setting credentials!");
		app_settings.device_id 	= device_id;
		memcpy(app_settings.uuid, uuid, KNOT_PROTOCOL_UUID_LEN);
		memcpy(app_settings.token, token, KNOT_PROTOCOL_TOKEN_LEN);

		res = storage_set(&app_settings);
		if (res) {
			LOG_ERR("Failed to set KNoT credentials");
			next = STATE_ERROR;
			goto done;
		}

		LOG_INF("Successfully registered!");
		LOG_INF("UUID: %s", uuid);
		LOG_INF("token: %s", token);

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
		*xpt_opcode = (end ? KNOT_MSG_SCHM_END_RSP :
				     KNOT_MSG_SCHM_FRAG_RSP);
		LOG_DBG("Creating schema message");
		*len = msg_create_schema(omsg, id_index, schema, end);
		break;
	}
done:
	return next;
}

static size_t process_event(u8_t *xpt_opcode,
			    const u8_t *ipdu, size_t ilen,
			    u8_t *opdu, size_t olen)
{
	knot_msg *omsg = (knot_msg *) opdu;
	const knot_msg *imsg = (knot_msg *) ipdu;
	const knot_value_type *value;
	u8_t value_len = 0;
	static u8_t id_index = 0;
	u8_t old_id;
	u8_t last_id;
	s8_t len = 0;

	last_id = proxy_get_last_id();

	/* No response expected. Continue polling */
	if (*xpt_opcode == 0xff)
		goto polling;

	/*
	 * OPCODE and timeout verified before entering state.
	 * If timeout expired or received an error message simply ignore it
	 * and send data of the next sensor.
	 */
	if (to_xpr || imsg->action.result != 0)
		LOG_ERR("FAIL SEND FOR ID: %d", id_index);
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
		*xpt_opcode = KNOT_MSG_PUSH_DATA_RSP;
		break;
	} while (id_index != old_id);

	if (len <= 0)
		*xpt_opcode = 0xff;

	return len;
}

static size_t process_cmd(const u8_t *ipdu, size_t ilen,
			  u8_t *opdu, size_t olen)
{
	const knot_msg *imsg = (knot_msg *) ipdu;
	knot_msg *omsg = (knot_msg *) opdu;
	size_t len = 0;

	u8_t id = 0xff;
	const knot_value_type *value;
	u8_t value_len;

	switch (imsg->hdr.type) {
	case KNOT_MSG_UNREG_REQ:
		/* Clear NVM */
		break;
	case KNOT_MSG_POLL_DATA_REQ:
		id = imsg->data.sensor_id;

		/* Invalid id */
		if (id > CONFIG_KNOT_THING_DATA_MAX) {
			len = msg_create_error(omsg,
					       KNOT_MSG_PUSH_DATA_REQ,
					       KNOT_ERR_INVALID);
			LOG_WRN("Invalid Id!");
			break;
		}

		/* Flag data to be sent and don't wait response */
		proxy_force_send(id);
		value = proxy_read(id, &value_len, false);

		/* FIXME: */
		/* Couldn't read value */
		if (unlikely(!value)) {
			len = msg_create_error(omsg,
					       KNOT_MSG_PUSH_DATA_REQ,
					       KNOT_ERR_INVALID);
			LOG_WRN("Can't read requested value");
			break;
		}

		len = msg_create_data(omsg, id, value, value_len, false);
		break;
	case KNOT_MSG_PUSH_DATA_REQ:
		id = imsg->data.sensor_id;
		value = &imsg->data.payload;
		/* value_len = payload_len - sensor_id_len */
		value_len = imsg->hdr.payload_len;
		value_len -= sizeof(id);

		if (proxy_write(id, value, value_len) < 0) {
			len = msg_create_error(omsg,
					       KNOT_MSG_PUSH_DATA_RSP,
					       KNOT_ERR_INVALID);
			break;
		}
		/* TODO: Change protocol to not require id nor value for
		 * set data response messages
		 */
		len = msg_create_data(omsg, id, value, value_len, true);
		break;
	case KNOT_MSG_PUSH_CONFIG_REQ:
		/* TODO */
		break;
	default:
		break;
	}

	return len;
}

static enum sm_state state_online(u8_t *xpt_opcode,
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
		ret_len = process_event(xpt_opcode, ipdu, ilen, opdu, olen);

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

	LOG_DBG("SM: Start");

	state = STATE_AUTH; /* Initial state */

	device_id = 0;
	memset(uuid, 0, sizeof(uuid));
	memset(token, 0, sizeof(token));

	/*
	 * 'app-settings' stored properly at NVM means that UUID, Token and id
	 * are available.
	 */
	err = storage_get(&app_settings);
	if (err < 0) {
		LOG_INF("KNoT credentials not found");
		device_id = sys_rand32_get();
		device_id *= device_id;
		state = STATE_REG;
		LOG_DBG("STATE: REG");
		goto done;
	}

	LOG_INF("KNoT credentials found");
	device_id = app_settings.device_id;
	memcpy(uuid, app_settings.uuid, KNOT_PROTOCOL_UUID_LEN);
	memcpy(token, app_settings.token, KNOT_PROTOCOL_TOKEN_LEN);
	LOG_DBG("STATE: AUTH");

done:
	/* Initially disconnected */
	peripheral_set_status_period(STATUS_DISCONN_PERIOD);

	to_on = false;
	to_xpr = false;

	return 0;
}

void sm_stop(void)
{
	LOG_DBG("SM: Stop");
	if (to_on)
		k_timer_stop(&to);

	proxy_stop();
}

void sm_init(void)
{
	LOG_DBG("SM: Init");

	k_timer_init(&to, timer_expired, NULL);

	/* Initializing proxy slots */
	proxy_init();
}

int sm_run(const u8_t *ipdu, size_t ilen, u8_t *opdu, size_t olen)
{
	enum sm_state next;
	s64_t status_blink_period;
	size_t len = 0;
	bool reset;

	/* Expected OPCODE response. Initially not expecting response*/
	static u8_t xpt_opcode = 0xff;
	bool got_resp; /* Got right response */

	/* Handle reset flag */
	reset = peripheral_get_reset();
	/* TODO: Consider current state before reseting */
	if (reset) {
		/* TODO: Unregister before reseting */
		LOG_INF("Reseting system...");
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
		got_resp = cmp_opcode(xpt_opcode, ipdu, ilen);
		if (got_resp) {
			/* Stop timer if response found */
			k_timer_stop(&to);
			to_on = false;
			to_xpr = false;
			LOG_DBG("Got expected resp");


		} else if (wl_opcode(state, ipdu, ilen) == false)
			/* OPCODE doesn't belong to white list. Wait */
			return 0;
	}

	switch (state) {
	case STATE_REG:
		/* Register new device */
		next = state_register(&xpt_opcode, ipdu, ilen,
				      opdu, olen, &len);
		break;
	case STATE_AUTH:
		/* Authenticate if registered previously */
		next = state_auth(&xpt_opcode, ipdu, ilen, opdu, olen, &len);
		break;
	case STATE_SCH:
		/* Send schemas */
		next = state_schema(&xpt_opcode, ipdu, ilen, opdu, olen, &len);
		break;
	case STATE_ONLINE:
		/* Incoming messages and/or changes on sensors */
		next = state_online(&xpt_opcode, ipdu, ilen, opdu, olen, &len);
		break;
	default:
		LOG_ERR("ERROR");
		next = STATE_ERROR;
		break;
	}

	/* State has changed: Don't wait response */
	if (next != state) {
		xpt_opcode = 0xff;

		status_blink_period = STATUS_DISCONN_PERIOD;

		switch (next) {
		case STATE_REG:
			LOG_DBG("STATE: REG");
			break;
		case STATE_AUTH:
			LOG_DBG("STATE: AUTH");
			break;
		case STATE_SCH:
			LOG_DBG("STATE: SCH");
			break;
		case STATE_ONLINE:
			LOG_DBG("STATE: ONLINE");
			status_blink_period = STATUS_CONN_PERIOD;
			break;
		default:
			LOG_DBG("STATE: ERROR");
			status_blink_period = STATUS_ERROR_PERIOD;
			break;
		}

		peripheral_set_status_period(status_blink_period);

	}

	/* Not waiting response: Stop timer */
	if (xpt_opcode == 0xff) {
		if (to_on) {
			k_timer_stop(&to);
			to_on = false;
			to_xpr = false;
			LOG_DBG("Timer off");
		}
		goto done;
	}

	/* Waiting response: Run timer */
	if (to_on == false) {
		k_timer_start(&to, K_SECONDS(TIMEOUT_WIN), 0);
		to_on = true;
		to_xpr = false;
		LOG_DBG("Timer on");
		goto done;
	}
done:
	state = next;
	peripheral_flag_status();

	return len;
}
