/* net.c - KNoT Application Client */

/*
 * Copyright (c) 2019, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The knot client application is acting as a client that is run in Zephyr OS,
 * The client sends sensor data encapsulated using KNoT netcol.
 */


#include <zephyr.h>
#include <net/net_core.h>
#include <logging/log.h>

#include "knot.h"
#include <knot/knot_types.h>
#include <knot/knot_protocol.h>

LOG_MODULE_REGISTER(thermo, LOG_LEVEL_DBG);

/* Tracked value */
static int thermo = 0;
static int high_temp = 100000;

static void changed_thermo(struct knot_proxy *proxy)
{
	u8_t id;

	id = knot_proxy_get_id(proxy);
	knot_proxy_value_get_basic(proxy, &thermo);

	LOG_INF("Value for thermo with id %u changed to %d", id, thermo);
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
		LOG_INF("Sending value %d for thermo with id %u", thermo, id);

}

void setup(void)
{
	bool success;

	/* THERMO - Sent every 5 seconds or at high temperatures */
	if (knot_proxy_register(0, "THERMO", KNOT_TYPE_ID_TEMPERATURE,
		      KNOT_VALUE_TYPE_INT, KNOT_UNIT_TEMPERATURE_C,
		      changed_thermo, poll_thermo) == NULL) {
		LOG_ERR("THERMO_0 failed to register");
	}
	success = knot_proxy_set_config(0, KNOT_EVT_FLAG_TIME, 5,
					KNOT_EVT_FLAG_UPPER_THRESHOLD,
					high_temp, NULL);
	if (!success)
		LOG_ERR("THERMO failed to configure");

}

void loop(void)
{
}
