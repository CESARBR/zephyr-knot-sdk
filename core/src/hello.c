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
#include "knot_protocol.h"

static int thermo = 0;
static int high_temp = 100000;
static bool button = false;
static char plate[] = "BRZ0000";
static int plate_upper = 99999;

static void changed_thermo(struct knot_proxy *proxy)
{
	u8_t id;

	id = knot_proxy_get_id(proxy);
	knot_proxy_value_get_basic(proxy, &thermo);

	NET_DBG("Value for thermo with id %u changed to %d", id, thermo);
}

static void poll_thermo(struct knot_proxy *proxy)
{
	u8_t id;
	bool res;

	id = knot_proxy_get_id(proxy);
	/* Get current temperature from actual object */
	thermo++;

	/* Pushing temperature to remote */
	res = knot_proxy_value_set_basic(proxy, &thermo);

	/* Notify if sent */
	if (res)
		NET_DBG("Sending value %d for thermo with id %u", thermo, id);

}

static void changed_button(struct knot_proxy *proxy)
{
	knot_proxy_value_get_basic(proxy, &button);
	NET_DBG("Value for button changed to %d", button);
}

static void poll_button(struct knot_proxy *proxy)
{
	static int button_count = 0;
	bool res;

	/* Simulate button toogle after 5000 readings */
	if (button_count%5000 == 0)
		button = !button;
	button_count++;

	/* Pushing status to remote */
	res = knot_proxy_value_set_basic(proxy, &button);

	/* Notify if sent */
	if (res) {
		if (button)
			NET_DBG("Sending value true for button");
		else
			NET_DBG("Sending value false for button");
	}
}

static void plate_changed(struct knot_proxy *proxy)
{
	int len;

	if (knot_proxy_value_get_string(proxy, plate, sizeof(plate), &len))
		NET_DBG("Plate changed %s", plate);
}

static void random_plate(struct knot_proxy *proxy)
{
	u8_t id;
	int num;
	bool res;
	id = knot_proxy_get_id(proxy);

	num = (sys_rand32_get() % 7);
	plate[3] = '0' + num;
	plate[4] = '1' + num;
	plate[5] = '2' + num;
	plate[6] = '3' + num;

	res = knot_proxy_value_set_string(proxy, plate, sizeof(plate));

	/* Notify if sent */
	if (res)
		NET_DBG("Sent plate %s", plate);
}

void setup(void)
{
	bool success;

	/* THERMO - Sent every 5 seconds or at high temperatures */
	if (knot_proxy_register(0, "THERMO", KNOT_TYPE_ID_TEMPERATURE,
		      KNOT_VALUE_TYPE_INT, KNOT_UNIT_TEMPERATURE_C,
		      changed_thermo, poll_thermo) == NULL) {
		NET_ERR("THERMO_0 failed to register");
	}
	success = knot_proxy_set_config(0,
					KNOT_EVT_FLAG_TIME, 5,
					KNOT_EVT_FLAG_UPPER_THRESHOLD,
					high_temp, NULL);
	if (!success)
		NET_ERR("THERMO failed to configure");

	/* BUTTON - Sent after change */
	if (knot_proxy_register(1, "BUTTON", KNOT_TYPE_ID_SWITCH,
		      KNOT_VALUE_TYPE_BOOL, KNOT_UNIT_NOT_APPLICABLE,
		      changed_button, poll_button) == NULL) {
		NET_ERR("BUTTON failed to register");
	}
	success = knot_proxy_set_config(1, KNOT_EVT_FLAG_CHANGE, NULL);
	if (!success)
		NET_ERR("BUTTON failed to configure");

	/* PLATE - Will fail to configure */
	if (knot_proxy_register(2, "PLATE", KNOT_TYPE_ID_NONE,
		      KNOT_VALUE_TYPE_RAW, KNOT_UNIT_NOT_APPLICABLE,
		      plate_changed, random_plate) == NULL) {
		NET_ERR("PLATE failed to register");
	}
	/* Limit flag added for raw type variable for testing purposes */
	success = knot_proxy_set_config(2,
				   KNOT_EVT_FLAG_TIME, 2,
				   KNOT_EVT_FLAG_UPPER_THRESHOLD, plate_upper,
				   NULL);
	if (!success)
		NET_ERR("PLATE failed to configure");

}

void loop(void)
{
}
