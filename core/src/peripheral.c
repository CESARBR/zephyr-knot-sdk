/* peripheral.c - KNoT Thing Peripheral selector */

/*
 * Copyright (c) 2018, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This file works as a selector for using real peripherals or a mocked up file.
 * To use with qemu, use the peripheral.qemu file.
 */

/*
 * KNoT peripheral control.
 *
 * This module is responsible for handling the peripheral controlled by KNoT as
 * control buttons and status leds.
 */


#include <zephyr.h>
#include <net/net_core.h>
#include <logging/log.h>

LOG_MODULE_DECLARE(knot, CONFIG_KNOT_LOG_LEVEL);

#if CONFIG_BOARD_QEMU_X86
#include "peripheral.qemu"
#else
#include <device.h>
#include <gpio.h>

#include "peripheral.h"

static struct device *rst_gpio;
static struct device *status_gpio;

/* Reset configs */
static bool rst_flag;
static struct k_timer rst_timer; /* Reset timer */
static struct gpio_callback rst_btn_cb; /* Reset button callback */

/* Status led */
static bool status_led;
static s64_t toggle_led_period;
static s64_t last_toggle_time;


static void set_reset(struct k_timer *timer_id)
{
	rst_flag = true;
}

static void rst_btn_edge(struct device *rst_gpio,
			 struct gpio_callback *cb, u32_t pins)
{
	int port_value;

	/* Rising or falling edge */
	port_value = gpio_pin_read(rst_gpio, RST_FLASH_PIN, NULL);

	/* Stop reset timer on rising edge */
	if (port_value) {
		k_timer_stop(&rst_timer);
		return;
	}
	/* Start reset timer on falling edge */
	k_timer_start(&rst_timer, RST_FLASH_TIMEOUT, RST_FLASH_PERIOD);
}

static int reset_init(void)
{
	/* Init GPIO port */
	rst_gpio = device_get_binding(RST_FLASH_PORT);
	if (unlikely(!rst_gpio))
		return -1;

	/* Initializing reset KNoT storage timer */
	k_timer_init(&rst_timer, set_reset, NULL);

	/* Reset Pin: Input, Pull up, Interruption on every edge */
	gpio_pin_configure(rst_gpio, RST_FLASH_PIN,
		GPIO_DIR_IN | GPIO_PUD_PULL_UP |
		GPIO_INT | GPIO_INT_EDGE | GPIO_INT_DOUBLE_EDGE);

	/* Pin callback init */
	gpio_init_callback(&rst_btn_cb, rst_btn_edge, BIT(RST_FLASH_PIN));
	gpio_add_callback(rst_gpio, &rst_btn_cb);
	gpio_pin_enable_callback(rst_gpio, RST_FLASH_PIN);

	rst_flag = false;

	/* TODO: Check for errors */
	return 0;
}

static void toggle_led(void)
{
	status_led = !status_led;
	gpio_pin_write(status_gpio, STATUS_LED_PIN, status_led);
}

static int status_init(void)
{
	/* Init GPIO port */
	status_gpio = device_get_binding(STATUS_LED_PORT);
	if (unlikely(!status_gpio))
		return -1;

	/* Led Pin: Output */
	gpio_pin_configure(status_gpio, STATUS_LED_PIN, GPIO_DIR_OUT);

	/* If led frequency is not set, show as an error */
	peripheral_set_status_period(STATUS_ERROR_PERIOD);

	last_toggle_time = 0; /* Never toggled led */

	/* TODO: Check for errors */
	return 0;
}

int peripheral_init(void)
{
	if (unlikely(reset_init()))
		return -1;

	return status_init();
}

bool peripheral_get_reset(void)
{
	return rst_flag;
}

void peripheral_set_status_period(s64_t status_period)
{
	/* Ignore if up to date */
	if (toggle_led_period == status_period)
		return;

	/* Led on at period update */
	status_led = false;
	gpio_pin_write(status_gpio, STATUS_LED_PIN, status_led);

	toggle_led_period = status_period;
}

bool peripheral_flag_status(void)
{
	if (toggle_led_period < 0)
		return false;

	s64_t actual_time = k_uptime_get();

	if (actual_time - last_toggle_time >= toggle_led_period) {
		last_toggle_time = actual_time;
		toggle_led();
		return true;
	}

	return false;
}

#endif
