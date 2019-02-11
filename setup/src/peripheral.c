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

/* General GPIO Controller */
#define GPIO_PORT		SW3_GPIO_CONTROLLER
#define BUTTON_PIN		SW3_GPIO_PIN

static struct device *gpiob = NULL;

/*
 * Configure button pin as a pulled up input pin.
 * The button pin is defined by BUTTON_PIN macro.
 *
 * @return	0 if successful init, -1 if error
 */
int peripheral_init(void)
{
	/* Find device */
	gpiob = device_get_binding(GPIO_PORT);
	if (!gpiob)
		return -1;

	return gpio_pin_configure(gpiob, BUTTON_PIN,
				  GPIO_DIR_IN |
				  GPIO_PUD_PULL_UP);
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

	/* Read pin value */
	rc = gpio_pin_read(gpiob, BUTTON_PIN, &val);

	/* Return error if no device found */
	if (rc)
		return PERIPHERAL_BTN_ERROR;

	return val;
}
