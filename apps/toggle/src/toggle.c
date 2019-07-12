/* toggle.c - KNoT Application Client */

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

#define TOGGLE_PORT	LED1_GPIO_CONTROLLER /* General GPIO Controller */
#define TOGGLE_PIN	LED1_GPIO_PIN /* User LED */

LOG_MODULE_REGISTER(toggle, LOG_LEVEL_DBG);

bool toggle = true; 			/* Tracked value */
struct device *gpio_led;		/* GPIO device */

void write_led(struct knot_proxy *proxy)
{
	knot_proxy_value_get_basic(proxy, &toggle);
	LOG_INF("Value for toggle changed to %d", toggle);

	gpio_pin_write(gpio_led, TOGGLE_PIN, !toggle); /* Led is On at LOW */
}

void setup(void)
{
	/* Peripherals control */
	gpio_led = device_get_binding(TOGGLE_PORT);
	gpio_pin_configure(gpio_led, TOGGLE_PIN, GPIO_DIR_OUT);

	/* KNoT config */
	knot_proxy_register(0, "LED", KNOT_TYPE_ID_SWITCH,
			    KNOT_VALUE_TYPE_BOOL, KNOT_UNIT_NOT_APPLICABLE,
			    write_led, NULL);

	knot_proxy_set_config(0, KNOT_EVT_FLAG_CHANGE, NULL);

}

void loop(void)
{
}
