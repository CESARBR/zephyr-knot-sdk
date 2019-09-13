/* button.c - KNoT Application Client */

/*
 * Copyright (c) 2018, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Use callback to toggle led status when the button is pressed.
 */

#include <zephyr.h>
#include <net/net_core.h>
#include <logging/log.h>

#include "knot.h"
#include <knot/knot_types.h>
#include <knot/knot_protocol.h>

LOG_MODULE_REGISTER(button, LOG_LEVEL_DBG);

/* Tracked value */
bool led = true;

#include <device.h>
#include <gpio.h>
#define GPIO_PORT	SW0_GPIO_CONTROLLER /* General GPIO Controller */
#define BUTTON_PIN	DT_GPIO_KEYS_SW1_GPIO_PIN /* User button */
#define LED_PIN		LED1_GPIO_PIN /* User LED */

struct device *gpiob;		/* GPIO device */
struct gpio_callback button_cb; /* Button pressed callback */

void btn_press(struct device *gpiob,
		      struct gpio_callback *cb, u32_t pins)
{
	led = !led;
	gpio_pin_write(gpiob, LED_PIN, !led); /* Update GPIO */
}

int changed_led(int id)
{
	LOG_INF("Value for led changed to %d", led);
	gpio_pin_write(gpiob, LED_PIN, !led); /* Led is On at LOW */

	return KNOT_CALLBACK_SUCCESS;
}

void setup(void)
{
	bool success;

	/* BUTTON - Sent after change */
	if (knot_data_register(0, "LED", KNOT_TYPE_ID_SWITCH,
			       KNOT_VALUE_TYPE_BOOL, KNOT_UNIT_NOT_APPLICABLE,
			       &led, sizeof(led),
			       changed_led, NULL) < 0) {
		LOG_ERR("LED failed to register");
	}
	success = knot_data_config(0, KNOT_EVT_FLAG_CHANGE, NULL);
	if (!success)
		LOG_ERR("LED failed to configure");

	/* Peripherals control */
	gpiob = device_get_binding(GPIO_PORT);

	/* Button Pin has pull up, interruption on low edge and debounce */
	gpio_pin_configure(gpiob, BUTTON_PIN,
			   GPIO_DIR_IN | GPIO_PUD_PULL_UP | GPIO_INT_DEBOUNCE |
			   GPIO_INT | GPIO_INT_EDGE | GPIO_INT_ACTIVE_LOW);
	gpio_init_callback(&button_cb, btn_press, BIT(BUTTON_PIN));
	gpio_add_callback(gpiob, &button_cb);
	gpio_pin_enable_callback(gpiob, BUTTON_PIN);

	/* Led pin */
	gpio_pin_configure(gpiob, LED_PIN, GPIO_DIR_OUT);
}

void loop(void)
{
}
