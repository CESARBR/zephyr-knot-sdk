/* net.c - KNoT Application Client */

/*
 * Copyright (c) 2018, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The knot client application is acting as a client that is run in Zephyr OS,
 * The client sends sensor data encapsulated using KNoT netcol.
 */

#define SYS_LOG_DOMAIN "knot-hello"
#define NET_SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_LOG_ENABLED 1

#include <zephyr.h>
#include <net/net_core.h>

#include "knot.h"
#include "knot_types.h"

static int thermo[] = {0, 0, 0};
static bool button = false;

static void poll_thermo(struct knot_proxy *proxy)
{
	u8_t id;

	id = knot_proxy_get_id(proxy);
	NET_DBG("Reading temperature of thermo %u", id);
	/* Get current temperature from actual object */
	thermo[id]++;

	/* Pushing temperature to remote */
	knot_proxy_value_set_basic(proxy, &thermo[id]);
}

static void poll_button(struct knot_proxy *proxy)
{
	u8_t id;
	static int button_count = 0;

	id = knot_proxy_get_id(proxy);
	NET_DBG("Reading status of button with id %u", id);

	/* Simulate button toogle after 5 readings */
	if (button_count%5 == 0) {
		button = !button;
		if (button)
			NET_DBG("Button pressed");
		else
			NET_DBG("Button released");
	}
	button_count++;

	/* Pushing status to remote */
	knot_proxy_value_set_basic(proxy, &button);
}

void setup(void)
{
	if (knot_proxy_register(0, "THERMO_0", KNOT_TYPE_ID_TEMPERATURE,
		      KNOT_VALUE_TYPE_INT, KNOT_UNIT_TEMPERATURE_C,
		      NULL, poll_thermo)  == NULL) {
		NET_ERR("THERMO_0 failed to register");
	}

	/* id ONE assigned with wrong KNOT_VALUE TYPE for testing purpose */
	if (knot_proxy_register(1, "THERMO_1", KNOT_TYPE_ID_TEMPERATURE,
		      KNOT_VALUE_TYPE_FLOAT, KNOT_UNIT_TEMPERATURE_C,
		      NULL, poll_thermo) == NULL) {
		NET_ERR("THERMO_1 failed to register");
	}

	if (knot_proxy_register(2, "THERMO_2", KNOT_TYPE_ID_TEMPERATURE,
		      KNOT_VALUE_TYPE_INT, KNOT_UNIT_TEMPERATURE_C,
		      NULL, poll_thermo) == NULL) {
		NET_DBG("THERMO_2 failed to register");
	}

	if (knot_proxy_register(3, "BUTTON", KNOT_TYPE_ID_SWITCH,
		      KNOT_VALUE_TYPE_BOOL, KNOT_UNIT_NOT_APPLICABLE,
		      NULL, poll_button) == NULL) {
		NET_DBG("BUTTON failed to register");
	}
}

void loop(void)
{
}
