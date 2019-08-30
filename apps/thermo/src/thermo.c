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
int thermo = 0;
int high_temp = 100000;

void setup(void)
{
	bool success;

	/* THERMO - Sent every 5 seconds or at high temperatures */
	if (knot_data_register(0, "THERMO", KNOT_TYPE_ID_TEMPERATURE,
			       KNOT_VALUE_TYPE_INT, KNOT_UNIT_TEMPERATURE_C,
			       &thermo, sizeof(thermo),
			       NULL, NULL) < 0) {
		LOG_ERR("THERMO_0 failed to register");
	}
	success = knot_data_config(0, KNOT_EVT_FLAG_TIME, 5,
					KNOT_EVT_FLAG_UPPER_THRESHOLD,
					high_temp, NULL);
	if (!success)
		LOG_ERR("THERMO failed to configure");

}

void loop(void)
{
	thermo++;
}
