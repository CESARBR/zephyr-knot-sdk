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

LOG_MODULE_REGISTER(plate, LOG_LEVEL_DBG);

/* Tracked value */
static char plate[] = "KNT0000";

static void plate_changed(struct knot_proxy *proxy)
{
	int len;

	if (knot_proxy_value_get_string(proxy, plate, sizeof(plate), &len))
		LOG_INF("Plate changed %s", plate);
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
		LOG_INF("Sent plate %s", plate);
}

void setup(void)
{
	bool success;

	/* PLATE - Sent every 10 seconds */
	if (knot_proxy_register(0, "PLATE", KNOT_TYPE_ID_NONE,
		      KNOT_VALUE_TYPE_RAW, KNOT_UNIT_NOT_APPLICABLE,
		      plate_changed, random_plate) == NULL) {
		LOG_ERR("PLATE failed to register");
	}
	success = knot_proxy_set_config(0, KNOT_EVT_FLAG_TIME, 10, NULL);
	if (!success)
		LOG_ERR("PLATE failed to configure");
}

void loop(void)
{
}
