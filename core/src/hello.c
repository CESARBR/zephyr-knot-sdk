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

static unsigned int write_counter = 0;

static void sensor_write(u8_t id)
{
	return;
}

static void sensor_read(u8_t id)
{
	int read_value;
	knot_get_int(id, &read_value);
	NET_DBG("Got value for id %u: %u", id, read_value);
	return;
}

void setup(void)
{
	knot_register(0, "K0", KNOT_TYPE_ID_VOLTAGE,
		      KNOT_VALUE_TYPE_INT, KNOT_UNIT_VOLTAGE_V,
		      sensor_read, sensor_write);

	/* id ONE left unassigned for testing purpose */

	knot_register(2, "K2", KNOT_TYPE_ID_VOLTAGE,
		      KNOT_VALUE_TYPE_INT, KNOT_UNIT_VOLTAGE_V,
		      sensor_read, sensor_write);
}

void loop(void)
{
	NET_DBG("Setting value for id %u: %u", 0, write_counter);
	knot_set_int(0, write_counter);

	/* Setting sensor 1 for error testing */
	NET_DBG("Setting value for id %u: %u", 1, 123456);
	knot_set_int(1, 123456);

	NET_DBG("Setting value for id %u: %u", 2, ~write_counter);
	knot_set_int(2, ~write_counter);
	write_counter++;
}
