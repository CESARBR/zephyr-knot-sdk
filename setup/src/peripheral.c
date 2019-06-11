/* peripheral.c - Peripherals handler */

/*
 * Copyright (c) 2019, CESAR. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <zephyr.h>
#include <device.h>
#include <gpio.h>
#include "peripheral.h"

/* Button GPIO Controller */
#define BUTTON_GPIO		SW0_GPIO_CONTROLLER
#define BUTTON_PIN		SW0_GPIO_PIN

/* LEDs GPIO Controllers */
#define LED0_GPIO		LED0_GPIO_CONTROLLER
#define LED0_PIN		LED0_GPIO_PIN
#define LED1_GPIO		LED1_GPIO_CONTROLLER
#define LED1_PIN		LED1_GPIO_PIN

#define LED_TOGGLE_PERIOD	500 /* Led toggle period in ms*/

static struct device *gpio_button = NULL;
static struct device *gpio_led_dev[2];
static char *gpio_led_name[2] = {LED0_GPIO, LED1_GPIO};
static u32_t gpio_led_pin[2] = {LED0_PIN, LED1_PIN};

/*
 * Configure button pin as a pulled up input pin.
 * The button pin is defined by BUTTON_PIN macro.
 */
static int configure_button(void)
{
	/* Find device */
	gpio_button = device_get_binding(BUTTON_GPIO);
	if (!gpio_button)
		return -1;

	/* Configure button */
	return gpio_pin_configure(gpio_button, BUTTON_PIN,
				  GPIO_DIR_IN |
				  GPIO_PUD_PULL_UP);
}

/*
 * Configure LED as output pin.
 */
static int configure_led(const char *name, const u32_t pin, struct device **dev)
{
	/* Configure LEDs */
	*dev = device_get_binding(name);

	if (!*dev)
		return -1;

	return gpio_pin_configure(*dev, pin, GPIO_DIR_OUT);
}

/*
 * Configure peripherals.
 * Return 0 in case of success
 */
int peripheral_init(void)
{
	int rc;

	rc = configure_button();
	if (rc)
		return rc;

	/* Configure LEDs */
	rc = configure_led(gpio_led_name[0], gpio_led_pin[0], &gpio_led_dev[0]);
	if (rc)
		return rc;

	rc = configure_led(gpio_led_name[1], gpio_led_pin[1], &gpio_led_dev[1]);
	return rc;
}

/*
 * Read actual status of button.
 *
 * @return	 0 button pressed
 *		 1 button not pressed
 *		-1 GPIO error
 */

int peripheral_btn_status(void)
{
	int val;
	int rc;

	/* Fail if port not defined */
	if (!gpio_button)
		return PERIPHERAL_BTN_ERROR;

	/* Read pin value */
	rc = gpio_pin_read(gpio_button, BUTTON_PIN, &val);

	/* Return error if no device found */
	if (rc)
		return PERIPHERAL_BTN_ERROR;

	return val;
}
