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

LOG_MODULE_DECLARE(knot, LOG_LEVEL_DBG);

/* Tracked value */
static float volume = 10000;

static void changed_volume(struct knot_proxy *proxy)
{
	u8_t id;

	id = knot_proxy_get_id(proxy);
	knot_proxy_value_get_basic(proxy, &volume);

	LOG_INF("Value for volume with id %u changed", id);
}

static void poll_volume(struct knot_proxy *proxy)
{
	u8_t id;
	bool res;
	int int_part;
	int dec_part;

	id = knot_proxy_get_id(proxy);
	/* Get current volume from actual object */
	int_part = (sys_rand32_get() % 10000);
	dec_part = (sys_rand32_get() % 1000);
	volume = int_part + dec_part/1000.0;

	/* Pushing volume to remote */
	res = knot_proxy_value_set_basic(proxy, &volume);

	/* Notify if sent */
	if (res)
		LOG_INF("Sending value %d.%d for tank volume with id %u",
		 int_part, dec_part, id);

}

void setup(void)
{
	bool success;

	/* VOLUME - Sent every 5 seconds or at low volumes */
	if (knot_proxy_register(0, "VOLUME", KNOT_TYPE_ID_VOLUME,
		      KNOT_VALUE_TYPE_FLOAT, KNOT_UNIT_VOLUME_L,
		      changed_volume, poll_volume) == NULL) {
		LOG_ERR("VOLUME_0 failed to register");
	}
	success = knot_proxy_set_config(0, KNOT_EVT_FLAG_TIME, 5, NULL);
	if (!success)
		LOG_ERR("VOLUME failed to configure");

}

void loop(void)
{
}
