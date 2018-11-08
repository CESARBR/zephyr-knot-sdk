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
#include "knot_protocol.h"

/* Tracked values */
static int thermo = 0;
static int high_temp = 100000;
static bool led = true;
static char plate[] = "BRZ0000";
static int plate_upper = 99999;

/*
 * Use GPIO only for real boards.
 * Use timer to mock values change if using qemu.
 */
#if CONFIG_BOARD_NRF52840_PCA10056
#include <device.h>
#include <board.h>
#include <gpio.h>
#define GPIO_PORT		SW0_GPIO_NAME /* General GPIO Controller */
#define BUTTON_PIN		SW0_GPIO_PIN /* User button */

static struct device *gpiob;		/* GPIO device */
static struct gpio_callback button_cb; /* Button pressed callback */

static void btn_press(struct device *gpiob,
		       struct gpio_callback *cb, u32_t pins)
{
	led = !led;
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

static void changed_thermo(struct knot_proxy *proxy)
{
	u8_t id;

	id = knot_proxy_get_id(proxy);
	knot_proxy_value_get_basic(proxy, &thermo);

	NET_DBG("Value for thermo with id %u changed to %d", id, thermo);
}

static void poll_thermo(struct knot_proxy *proxy)
{
	u8_t id;
	bool res;

	id = knot_proxy_get_id(proxy);
	/* Get current temperature from actual object */
	thermo++;

	/* Pushing temperature to remote */
	res = knot_proxy_value_set_basic(proxy, &thermo);

	/* Notify if sent */
	if (res)
		NET_DBG("Sending value %d for thermo with id %u", thermo, id);

}

static void changed_led(struct knot_proxy *proxy)
{
	knot_proxy_value_get_basic(proxy, &led);
	NET_DBG("Value for led changed to %d", led);
}

static void poll_led(struct knot_proxy *proxy)
{
	/* Pushing status to remote */
	bool res;
	res = knot_proxy_value_set_basic(proxy, &led);

	/* Notify if sent */
	if (res) {
		if (led)
			NET_DBG("Sending value true for led");
		else
			NET_DBG("Sending value false for led");
	}
}

static void plate_changed(struct knot_proxy *proxy)
{
	int len;

	if (knot_proxy_value_get_string(proxy, plate, sizeof(plate), &len))
		NET_DBG("Plate changed %s", plate);
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
		NET_DBG("Sent plate %s", plate);
}

void setup(void)
{
	bool success;

	/* THERMO - Sent every 5 seconds or at high temperatures */
	if (knot_proxy_register(0, "THERMO", KNOT_TYPE_ID_TEMPERATURE,
		      KNOT_VALUE_TYPE_INT, KNOT_UNIT_TEMPERATURE_C,
		      changed_thermo, poll_thermo) == NULL) {
		NET_ERR("THERMO_0 failed to register");
	}
	success = knot_proxy_set_config(0,
					KNOT_EVT_FLAG_TIME, 5,
					KNOT_EVT_FLAG_UPPER_THRESHOLD,
					high_temp, NULL);
	if (!success)
		NET_ERR("THERMO failed to configure");

	/* BUTTON - Sent after change */
	if (knot_proxy_register(1, "LED", KNOT_TYPE_ID_SWITCH,
		      KNOT_VALUE_TYPE_BOOL, KNOT_UNIT_NOT_APPLICABLE,
		      changed_led, poll_led) == NULL) {
		NET_ERR("LED failed to register");
	}
	success = knot_proxy_set_config(1, KNOT_EVT_FLAG_CHANGE, NULL);
	if (!success)
		NET_ERR("LED failed to configure");

	/* PLATE - Will fail to configure */
	if (knot_proxy_register(2, "PLATE", KNOT_TYPE_ID_NONE,
		      KNOT_VALUE_TYPE_RAW, KNOT_UNIT_NOT_APPLICABLE,
		      plate_changed, random_plate) == NULL) {
		NET_ERR("PLATE failed to register");
	}
	/* Limit flag added for raw type variable for testing purposes */
	success = knot_proxy_set_config(2,
				   KNOT_EVT_FLAG_TIME, 2,
				   KNOT_EVT_FLAG_UPPER_THRESHOLD, plate_upper,
				   NULL);
	if (!success)
		NET_ERR("PLATE failed to configure");

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
#elif CONFIG_BOARD_QEMU_X86
	k_timer_start(&val_update_timer, UPDATE_PERIOD, UPDATE_PERIOD);
#endif

}

void loop(void)
{
}
