/* counter.c - KNoT Application Client */

/*
 * Copyright (c) 2019, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The knot client application is acting as a client that is run in Zephyr OS,
 * The client sends sensor data encapsulated using KNoT protocol.
 */


#include <zephyr.h>
#include <net/net_core.h>
#include <logging/log.h>
#include <device.h>
#include <gpio.h>

#include "knot.h"
#include <knot/knot_types.h>
#include <knot/knot_protocol.h>

#define LED_PORT	LED1_GPIO_CONTROLLER /* GPIO Controller */
#define LED_PIN		LED1_GPIO_PIN /* User LED */

#define SENSOR_PORT	"GPIO_1"	/* GPIO Controller */
#define SENSOR_PIN	10		/* Digital sensor pin */

LOG_MODULE_REGISTER(counter, LOG_LEVEL_DBG);

bool led; 			/* Tracked value */
int counter;			/* Events counter */

struct device *gpio_led;		/* GPIO device */
struct device *gpio_sensor;		/* GPIO device */

void read_led(struct knot_proxy *proxy)
{
	knot_proxy_value_set_basic(proxy, &led);
}

void write_counter(struct knot_proxy *proxy)
{
	knot_proxy_value_get_basic(proxy, &counter);
	LOG_INF("Value for counter changed to %d", counter);
}

void read_counter(struct knot_proxy *proxy)
{
	knot_proxy_value_set_basic(proxy, &counter);
}

void setup(void)
{
	/* Peripherals control */
	gpio_led = device_get_binding(LED_PORT);
	gpio_sensor = device_get_binding(SENSOR_PORT);

	gpio_pin_configure(gpio_led, LED_PIN, GPIO_DIR_OUT);

	gpio_pin_configure(gpio_sensor, SENSOR_PIN,
			   GPIO_DIR_IN | GPIO_PUD_PULL_DOWN);

	/* Turn off led */
	led = false;
	gpio_pin_write(gpio_led, LED_PIN, !led);

	counter = 0;	/* Reset counter */

	/* KNoT config - LED*/
	knot_data_register(0, "LED", KNOT_TYPE_ID_SWITCH,
			   KNOT_VALUE_TYPE_BOOL, KNOT_UNIT_NOT_APPLICABLE,
			   NULL, read_led);
	knot_data_config(0, KNOT_EVT_FLAG_CHANGE, NULL);

	/* KNoT config - Counter*/
	knot_data_register(1, "Counter", KNOT_TYPE_ID_TEMPERATURE,
		           KNOT_VALUE_TYPE_INT, KNOT_UNIT_TEMPERATURE_C,
		           write_counter, read_counter);
	knot_data_config(1,
			 KNOT_EVT_FLAG_TIME, 30,
			 KNOT_EVT_FLAG_UPPER_THRESHOLD, 10, NULL);
}

int64_t last_toggle_time = 0;

void loop(void)
{
	/* Get current time */
	u32_t sensor;
	int64_t current_time = k_uptime_get();

	/* Turn led on if vibrating */
	gpio_pin_read(gpio_sensor, SENSOR_PIN, &sensor);
	if (sensor) {
		last_toggle_time = current_time;
		LOG_INF("SIGNAL RECEIVED");
		if (led == false) {
			led = true;
			gpio_pin_write(gpio_led, LED_PIN, !led);
			counter++;
		}
	}

	/* Turn led off after 1 second */
	if (led == true &&
	    current_time - last_toggle_time > 1000) {
		led = false;
		gpio_pin_write(gpio_led, LED_PIN, !led);
	}
}
