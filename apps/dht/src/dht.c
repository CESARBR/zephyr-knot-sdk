/* dht.c - KNoT Application Client */

/*
 * Copyright (c) 2019, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This example uses a DHT series sensor (DHT11/DHT22) to monitor ambient
 * temperature and relative humidity. This example works for both DHT11 and
 * DHT22 sensors, but it is configured by default for DHT11. If you want to
 * use it with DHT22, change DHT_NAME and DHT_CHIP flags on prj.conf file.
 */

#include <zephyr.h>
#include <net/net_core.h>
#include <logging/log.h>
#include <device.h>
#include <sensor.h>

#include "knot.h"
#include <knot/knot_types.h>
#include <knot/knot_protocol.h>

LOG_MODULE_REGISTER(dht, LOG_LEVEL_DBG);

struct device *dev;
int temperature, relative_humidity;

/* Get temperature values from DHT sensor*/
int on_get_temperature(int id)
{
	struct sensor_value temp;

	sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
	temperature = temp.val1;
	LOG_INF("Temperature value: %d \n", temperature);

	return KNOT_CALLBACK_SUCCESS;
}

/* Get relative humidity values from DHT sensor*/
int on_get_humidity(int id)
{
	struct sensor_value humidity;

	sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &humidity);
	relative_humidity = humidity.val1;
	LOG_INF("Relative humidity value: %d \n", relative_humidity);

	return KNOT_CALLBACK_SUCCESS;
}

void setup(void)
{
	/* Peripherals control */
	dev = device_get_binding(CONFIG_DHT_NAME);
	if (dev == NULL) {
		LOG_ERR("Could not get DHT device\n");
		}
	/* KNoT config */
	knot_data_register(0, "TEMPERATURE", KNOT_TYPE_ID_TEMPERATURE,
				KNOT_VALUE_TYPE_INT, KNOT_UNIT_TEMPERATURE_C,
				&temperature, sizeof(temperature),
				 NULL, on_get_temperature);
	knot_data_config(0, KNOT_EVT_FLAG_TIME, 10, NULL);
	knot_data_register(1, "RELATIVE HUMIDITY",
				KNOT_TYPE_ID_RELATIVE_HUMIDITY,
				KNOT_VALUE_TYPE_INT,
				KNOT_UNIT_RELATIVE_HUMIDITY,
				&relative_humidity,
				sizeof(relative_humidity),
				NULL, on_get_humidity);
	knot_data_config(1, KNOT_EVT_FLAG_TIME, 10, NULL);
}

void loop(void)
{
}
