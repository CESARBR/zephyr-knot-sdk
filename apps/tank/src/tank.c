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

/*
 * Floating point services are currently available only for boards based on the
 * ARM Cortex-M4 or the Intel x86 architectures.
 */


#include <zephyr.h>
#include <net/net_core.h>
#include <logging/log.h>

#include "knot.h"
#include <knot/knot_types.h>
#include <knot/knot_protocol.h>

LOG_MODULE_REGISTER(tank, LOG_LEVEL_DBG);

/* Tracked value */
float volume = 10000;

void setup(void)
{
	/* VOLUME - Sent every 5 seconds or at low volumes */
	knot_data_register(0, "VOLUME", KNOT_TYPE_ID_VOLUME,
			   KNOT_VALUE_TYPE_FLOAT, KNOT_UNIT_VOLUME_L,
			   &volume, sizeof(volume), NULL, NULL);

	knot_data_config(0, KNOT_EVT_FLAG_TIME, 5, NULL);
}

void loop(void)
{
	volume += -5 + (sys_rand32_get() % 100001)/10000.0; // Add random value
}
