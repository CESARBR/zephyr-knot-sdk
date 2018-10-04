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

static void sensor_write(struct knot_proxy *proxy,
			 struct knot_proxy_value *pvalue)
{
	return;
}

static void sensor_read(struct knot_proxy *proxy,
			struct knot_proxy_value *pvalue)
{
	return;
}

void setup(void)
{
	knot_proxy_register(0, "K0", KNOT_TYPE_ID_VOLTAGE,
		      KNOT_VALUE_TYPE_INT, KNOT_UNIT_VOLTAGE_V,
		      sensor_read, sensor_write);

	/* id ONE left unassigned for testing purpose */

	knot_proxy_register(2, "K2", KNOT_TYPE_ID_VOLTAGE,
		      KNOT_VALUE_TYPE_INT, KNOT_UNIT_VOLTAGE_V,
		      sensor_read, sensor_write);
}

void loop(void)
{
}
