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

#include "sm.h"
#include "knot_protocol.h"

#define TIMEOUT_WIN				15 /* 15 sec */

static bool is_registered = false;
static bool schemas_done = false;

static struct k_timer to;	/* Re-send timeout */
static bool to_on;		/* Timeout active */
static bool to_exp;		/* Timeout expired */

enum sm_state {
	STATE_REG,		/* Registers new device */
	STATE_AUTH,		/* Authenticate known device */
	STATE_SCH,		/* Sends schema */
	STATE_ONESHOOT,		/* Sends data once */
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
	*len = 0;

	/* Timeout expired, resend message */
	if (resend) {
		/* TODO: Send register request */
		strncpy(opdu, "REG", olen);
		*len = strlen(opdu);
	}
	/* TODO: Check if ipdu received is for register */
	if (strstr(ipdu, "REG") != NULL) {
		/* TODO: Save credentials on storage */
		is_registered = true;
		next = STATE_SCH;
	}
	/* TODO: Check if ipdu received is for error */

	return next;
}

static enum sm_state state_auth(bool resend, const u8_t *ipdu, size_t ilen,
				u8_t *opdu, size_t olen, size_t *len)
{
	knot_msg *msg;
	enum sm_state next = STATE_AUTH;

	/* Default UUID and TOKEN used for testing while NVM is not done */
	const u8_t uuid[] = "365eb258-89d2-43b4-b011-f78a28910000";
	const u8_t token[] = "9bcfa43050f37cd2635adb1d160ceb22c167e461";

	/* Timeout expired (or new device), resend message */
	if (resend) {
		/* Send authentication request and waiting response */
		msg = (knot_msg *) opdu;
		/* TODO: Read credentials from non-volatile memory */
		strncpy(msg->auth.uuid, uuid, KNOT_PROTOCOL_UUID_LEN);
		strncpy(msg->auth.token, token, KNOT_PROTOCOL_TOKEN_LEN);

		msg->hdr.type = KNOT_MSG_AUTH_REQ;
		msg->hdr.payload_len = KNOT_PROTOCOL_UUID_LEN +
					KNOT_PROTOCOL_TOKEN_LEN;
		*len = sizeof(msg->hdr) + msg->hdr.payload_len;
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
	if (msg->hdr.type != KNOT_MSG_AUTH_RESP &&
	    msg->action.result != KNOT_SUCCESS) {
		/* Unexpected PDU opcode */
		next = STATE_ERROR;
		goto done;
	}

	/* TODO: Retrieve from non-volatile memory if all schemas were sent */
	/*
	 * If all schemas were sent move to state online. Otherwise, resend
	 * all the schemas.
	 */
	next = (schemas_done ? STATE_ONLINE : STATE_SCH);

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
	NET_DBG("SM: State Machine start");

	state = STATE_AUTH;
	/* TODO: Read credentials from storage */
	if (is_registered == false)
		state = STATE_REG;

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
		strcpy(opdu, "SCHM");
		len = strlen(opdu);
		next = STATE_ONESHOOT;
		break;
	case STATE_ONESHOOT:
		/* Sends the status of each item. */
		strcpy(opdu, "SHOOT");
		len = strlen(opdu);
		next = STATE_ONLINE;
		break;
	case STATE_ONLINE:
		/* Incoming messages and/or changes on sensors */
		strcpy(opdu, "ONLINE");
		len = strlen(opdu);
		next = STATE_ERROR;
		break;
	default:
		strcpy(opdu, "ERR");
		len = strlen(opdu);
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
