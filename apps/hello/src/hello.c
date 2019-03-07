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


#include <zephyr.h>
#include <net/net_core.h>
#include <logging/log.h>

#include "knot.h"
#include <knot/knot_types.h>
#include <knot/knot_protocol.h>

LOG_MODULE_DECLARE(knot, LOG_LEVEL_DBG);

/* Tracked value */
static bool led = true;

/*
 * Use GPIO only for real boards.
 * Use timer to mock values change if using qemu.
 */
#if CONFIG_BOARD_NRF52840_PCA10056
#include <device.h>
#include <gpio.h>
#define GPIO_PORT		SW0_GPIO_CONTROLLER /* General GPIO Controller */
#define BUTTON_PIN		DT_GPIO_KEYS_SW0_GPIO_PIN /* User button */
#define LED_PIN			13 /* User button */

static struct device *gpiob;		/* GPIO device */
static struct gpio_callback button_cb; /* Button pressed callback */

static void btn_press(struct device *gpiob,
		       struct gpio_callback *cb, u32_t pins)
{
	led = !led;
	gpio_pin_write(gpiob, LED_PIN, !led); /* Update GPIO */

}
#elif CONFIG_BOARD_QEMU_X86

#define UPDATE_PERIOD K_SECONDS(3) /* Update values each 3 seconds */
static void val_update(struct k_timer *timer_id)
{
	led = !led;
	k_timer_start(timer_id, UPDATE_PERIOD, UPDATE_PERIOD);
}
K_TIMER_DEFINE(val_update_timer, val_update, NULL);
#endif

static void changed_led(struct knot_proxy *proxy)
{
	knot_proxy_value_get_basic(proxy, &led);
	LOG_INF("Value for led changed to %d", led);

#if CONFIG_BOARD_NRF52840_PCA10056
	gpio_pin_write(gpiob, LED_PIN, !led); /* Led is On at LOW */
#endif
}

static void poll_led(struct knot_proxy *proxy)
{
	/* Pushing status to remote */
	bool res;
	res = knot_proxy_value_set_basic(proxy, &led);

	/* Notify if sent */
	if (res) {
		if (led)
			LOG_INF("Sending value true for led");
		else
			LOG_INF("Sending value false for led");
	}
}

void setup(void)
{
	bool success;

	/* BUTTON - Sent after change */
	if (knot_proxy_register(0, "LED", KNOT_TYPE_ID_SWITCH,
		      KNOT_VALUE_TYPE_BOOL, KNOT_UNIT_NOT_APPLICABLE,
		      changed_led, poll_led) == NULL) {
		LOG_ERR("LED failed to register");
	}
	success = knot_proxy_set_config(0, KNOT_EVT_FLAG_CHANGE, NULL);
	if (!success)
		LOG_ERR("LED failed to configure");

	/* Peripherals control */
#if CONFIG_BOARD_NRF52840_PCA10056
	/* Read button */
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

#elif CONFIG_BOARD_QEMU_X86
	k_timer_start(&val_update_timer, UPDATE_PERIOD, UPDATE_PERIOD);
#endif

}

void loop(void)
{
}
