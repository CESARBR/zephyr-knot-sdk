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

static int thermo[] = {0, 0, 0};
static int high_temp = 100;
static bool button = false;
static char plate[] = "BRZ0000";
static int plate_upper = 99999;

static void changed_thermo(struct knot_proxy *proxy)
{
	u8_t id;

	id = knot_proxy_get_id(proxy);
	knot_proxy_value_get_basic(proxy, &thermo[id]);

	NET_DBG("Value for thermo %u changed to %d", id, thermo[id]);
}

static void poll_thermo(struct knot_proxy *proxy)
{
	u8_t id;

	id = knot_proxy_get_id(proxy);
	/* Get current temperature from actual object */
	thermo[id]++;
	NET_DBG("Thermo %u read %d", id, thermo[id]);

	/* Pushing temperature to remote */
	knot_proxy_value_set_basic(proxy, &thermo[id]);
}

static void changed_button(struct knot_proxy *proxy)
{
	knot_proxy_value_get_basic(proxy, &button);
	NET_DBG("Value for button changed to %d", button);
}

static void poll_button(struct knot_proxy *proxy)
{
	static int button_count = 0;

	/* Simulate button toogle after 5 readings */
	if (button_count%5 == 0) {
		button = !button;
		if (button)
			NET_DBG("Button pressed");
		else
			NET_DBG("Button released");
	}
	button_count++;

	NET_DBG("Button read %u", button);
	/* Pushing status to remote */
	knot_proxy_value_set_basic(proxy, &button);
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

	id = knot_proxy_get_id(proxy);

	num = (sys_rand32_get() % 7);
	plate[3] = '0' + num;
	plate[4] = '1' + num;
	plate[5] = '2' + num;
	plate[6] = '3' + num;

	knot_proxy_value_set_string(proxy, plate, sizeof(plate));
	NET_DBG("Plate read %s", plate);
}

void setup(void)
{
	bool success;

	/* THERMO_0 - Sent every 5 seconds or at high temperatures */
	if (knot_proxy_register(0, "THERMO_0", KNOT_TYPE_ID_TEMPERATURE,
		      KNOT_VALUE_TYPE_INT, KNOT_UNIT_TEMPERATURE_C,
		      changed_thermo, poll_thermo) == NULL) {
		NET_ERR("THERMO_0 failed to register");
	}
	success = knot_proxy_set_config(0,
					KNOT_EVT_FLAG_TIME |
					KNOT_EVT_FLAG_UPPER_THRESHOLD,
					5, NULL, &high_temp);
	if (!success)
		NET_ERR("THERMO_0 failed to configure");

	/* THERMO_1 - Sent every 5 seconds */
	/* id ONE assigned with wrong KNOT_VALUE TYPE for testing purpose */
	if (knot_proxy_register(1, "THERMO_1", KNOT_TYPE_ID_TEMPERATURE,
		      KNOT_VALUE_TYPE_FLOAT, KNOT_UNIT_TEMPERATURE_C,
		      changed_thermo, poll_thermo) == NULL) {
		NET_ERR("THERMO_1 failed to register");
	}
	success = knot_proxy_set_config(1, KNOT_EVT_FLAG_TIME, 5, NULL, NULL);
	if (!success)
		NET_ERR("THERMO_1 failed to configure");

	/* THERMO_2 - Sent every 30 seconds */
	if (knot_proxy_register(2, "THERMO_2", KNOT_TYPE_ID_TEMPERATURE,
		      KNOT_VALUE_TYPE_INT, KNOT_UNIT_TEMPERATURE_C,
		      changed_thermo, poll_thermo) == NULL) {
		NET_ERR("THERMO_2 failed to register");
	}
	success = knot_proxy_set_config(2, KNOT_EVT_FLAG_TIME, 30, NULL, NULL);
	if (!success)
		NET_ERR("THERMO_2 failed to configure");

	/* BUTTON - Sent after change */
	if (knot_proxy_register(3, "BUTTON", KNOT_TYPE_ID_SWITCH,
		      KNOT_VALUE_TYPE_BOOL, KNOT_UNIT_NOT_APPLICABLE,
		      changed_button, poll_button) == NULL) {
		NET_ERR("BUTTON failed to register");
	}
	success = knot_proxy_set_config(3, KNOT_EVT_FLAG_CHANGE, 0, NULL, NULL);
	if (!success)
		NET_ERR("BUTTON failed to configure");

	/* PLATE - Will fail to configure */
	if (knot_proxy_register(4, "PLATE", KNOT_TYPE_ID_NONE,
		      KNOT_VALUE_TYPE_RAW, KNOT_UNIT_NOT_APPLICABLE,
		      plate_changed, random_plate) == NULL) {
		NET_ERR("PLATE failed to register");
	}
	/* Limit flag added for raw type variable for testing purposes */
	success = knot_proxy_set_config(4,
				   KNOT_EVT_FLAG_UPPER_THRESHOLD |
				   KNOT_EVT_FLAG_TIME,
				   2, NULL, &plate_upper);
	if (!success)
		NET_ERR("PLATE failed to configure");

}

void loop(void)
{
}
