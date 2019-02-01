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

#include "knot_protocol.h"
#include "kaio.h"
#include "msg.h"
#include "sm.h"
#include "storage.h"

#define TIMEOUT_WIN				15 /* 15 sec */

#define THING_NAME				"ZephyrThing0"

static bool schemas_done = false;

static struct k_timer to;	/* Re-send timeout */
static bool to_on;		/* Timeout active */
static bool to_exp;		/* Timeout expired */

static char uuid[KNOT_PROTOCOL_UUID_LEN];	/* Device uuid */
static char token[KNOT_PROTOCOL_TOKEN_LEN];	/* Device token */
static uint64_t device_id = 0;			/* Device id */

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
}

static enum sm_state state_register(bool resend, const u8_t *ipdu, size_t ilen,
				    u8_t *opdu, size_t olen, size_t *len)
{
	enum sm_state next = STATE_REG;
	const char *devname = THING_NAME;
	knot_msg *msg;

	/* Timeout expired, resend message */
	if (resend) {
		msg = (knot_msg *) opdu;
		*len = msg_create_reg(msg, device_id, devname, strlen(devname));
		goto done;
	}

	*len = 0;
	/* Finish if no message was received */
	if (ilen == 0)
		goto done;

	/* Decode received message */
	msg = (knot_msg *) ipdu;

	if (msg->cred.hdr.type != KNOT_MSG_REGISTER_RESP)
		goto done;

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

static enum sm_state state_auth(bool resend, const u8_t *ipdu, size_t ilen,
				u8_t *opdu, size_t olen, size_t *len)
{
	knot_msg *msg;
	enum sm_state next = STATE_AUTH;

	/* Timeout expired (or new device), resend message */
	if (resend) {
		/* Send authentication request and waiting response */
		msg = (knot_msg *) opdu;
		/* TODO: Read credentials from non-volatile memory */
		*len = msg_create_auth(msg, uuid, token);
		goto done;
	}

	/*
	 * If the ipdu is not error nor auth, it is some async message
	 * and it is ignored
	 */
	if (ilen == 0)
		goto done;

	msg = (knot_msg *) ipdu;
	*len = 0;

	/* Unexpected PDU opcode */
	if (msg->hdr.type != KNOT_MSG_AUTH_RESP)
		goto done;

	if (msg->action.result != KNOT_SUCCESS) {
		next = STATE_ERROR;
		goto done;
	}

	/* Credentials are only saved on NVM after all the schemas are sent */
	next = (strlen(uuid) && strlen(token) ? STATE_ONLINE : STATE_SCH);

done:
	return next;
}

static enum sm_state state_schema(bool resend, const u8_t *ipdu, size_t ilen,
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

	/* Timeout expired (or new device), resend message */
	if (resend) {
		id_index = 0;
		goto send;
	}

	/* Waiting response ... */
	if (ilen == 0)
		goto done;

	switch (imsg->hdr.type) {
	case KNOT_MSG_SCHEMA_RESP:
		/* Resend last fragment if failed */
		if (imsg->action.result == KNOT_SUCCESS)
			id_index++;
		goto send;
	case KNOT_MSG_SCHEMA_END_RESP:
		if (imsg->action.result != KNOT_SUCCESS)
			goto send;
		schemas_done = true;
		/* TODO: Save credentials to NVS only after schemas done */
		next = STATE_ONLINE;
	default:
		goto done;
	}

send:
	/* Send schema */
	last_id = kaio_get_last_id();
	while (id_index <= last_id) {
		schema = kaio_get_schema(id_index);
		if (schema == NULL) {
			/* Ignore invalid aio  */
			id_index++;
			continue;
		}
		end = ((id_index == last_id) ? true : false);
		*len = msg_create_schema(omsg, id_index, schema, end);
		break;
	}
done:
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

	NET_DBG("SM: State Machine start");

	memset(uuid, 0, sizeof(uuid));
	memset(token, 0, sizeof(token));

	/* TODO: Check for id from storage */
	if (device_id == 0) {
		device_id = sys_rand32_get();
		device_id *= device_id;
		/* TODO: Save id to storage */
	}

	err = storage_get(STORAGE_KEY_UUID, uuid) ||
					storage_get(STORAGE_KEY_TOKEN, token);
	if (err)
		state = STATE_REG;
	else
		state = STATE_AUTH;

	k_timer_init(&to, timer_expired, NULL);
	to_on = false;
	to_exp = false;

	return 0;
}

void sm_stop(void)
{
	NET_DBG("SM: Stop");
	if (to_on)
		k_timer_stop(&to);

}

int sm_run(const u8_t *ipdu, size_t ilen, u8_t *opdu, size_t olen)
{
	enum sm_state next;
	size_t len = 0;
	bool resend;

	/* TODO: Check if timeout expired */

	/*
	 * In the first states (reg, auth and sch) timeout is enabled, if no
	 *  data is received, it is not necessary to run the state machine.
	 */

	if (to_on && ilen == 0)
		return 0; /* Waiting RSP */

	switch (state) {
	case STATE_REG:
		/* Register new device */
		resend = ((to_exp || to_on == false ) ? true : false);
		next = state_register(resend, ipdu, ilen, opdu, olen, &len);
		break;
	case STATE_AUTH:
		/* Authenticate if registed previously */
		resend = ((to_exp || to_on == false ) ? true : false);
		next = state_auth(resend, ipdu, ilen, opdu, olen, &len);
		break;
	case STATE_SCH:
		/* Send schemas */
		resend = ((to_exp || to_on == false ) ? true : false);
		next = state_schema(resend, ipdu, ilen, opdu, olen, &len);
		break;
	case STATE_ONLINE:
		/* Incoming messages and/or changes on sensors */
		strcpy(opdu, "ONLINE");
		next = STATE_ERROR;
		break;
	default:
		strcpy(opdu, "ERR");
		next = STATE_ERROR;
		break;
	}

	/* State has changed: Stop timer */
	if (next != state) {
		if (to_on) {
			k_timer_stop(&to);
			to_on = false;
			to_exp = false;
		}
		goto done;
	}

	/* At same state: Waiting RSP or Resending (timeout expired) */
	if (len && to_on == false) {
		k_timer_start(&to, K_SECONDS(TIMEOUT_WIN), 0);
		to_on = true;
		to_exp = false;
		goto done;
	}
done:
	state = next;

	return len;
}
