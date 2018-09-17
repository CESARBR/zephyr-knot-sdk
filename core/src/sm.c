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

#define TIMEOUT_DISABLED			0xFFFF
#define TIMEOUT_WIN				15000

static bool is_registered = false;

enum sm_state {
	STATE_REG,		/* Registers new device */
	STATE_AUTH,		/* Authenticate known device */
	STATE_SCH,		/* Sends schema */
	STATE_ONESHOOT,		/* Sends data once */
	STATE_ONLINE,		/* Default state: sends & receive */
	STATE_ERROR,		/* Reg, auth, sch error */
};

static enum sm_state state;

static enum sm_state state_register(bool restart,
				    const unsigned char *ipdu, size_t ilen,
				    unsigned char *opdu, size_t olen,
				    size_t *len)
{
	enum sm_state next = STATE_REG;
	*len = 0;

	/* Timeout expired, resend message */
	if (restart) {
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

	return 0;
}

void sm_stop(void)
{
	NET_DBG("SM: Stop");
}

int sm_run(const unsigned char *ipdu, size_t ilen,
	   unsigned char *opdu, size_t olen)
{
	static int elapsed_time = TIMEOUT_DISABLED;
	bool restart = false;
	enum sm_state next;
	size_t len = 0;

	/* TODO: Check if timeout expired */

	/*
	 * In the first states (reg, auth and sch) timeout is enabled, if no
	 *  data is received, it is not necessary to run the state machine.
	 */

	if (elapsed_time != TIMEOUT_DISABLED && ilen == 0)
		return 0; /* Waiting RSP */

	restart = (elapsed_time > TIMEOUT_WIN ? true : false);

	switch (state) {
	case STATE_REG:
		/* Register new device */
		next = state_register(restart, ipdu, ilen, opdu, olen, &len);
		break;
	case STATE_AUTH:
		/* Authenticate if registed previously */
		strcpy(opdu, "AUTH");
		next = STATE_SCH;
		break;
	case STATE_SCH:
		/* Send schemas */
		strcpy(opdu, "SCHM");
		next = STATE_ONESHOOT;
		break;
	case STATE_ONESHOOT:
		/* Sends the status of each item. */
		strcpy(opdu, "SHOOT");
		next = STATE_ONLINE;
		break;
	case STATE_ONLINE:
		/* Incoming messages and/or changes on sensors */
		strcpy(opdu, "ONLINE");
		next = STATE_ONLINE;
		break;
	default:
		strcpy(opdu, "ERR");
		next = STATE_ERROR;
		break;
	}

	state = next;

	return len;
}
