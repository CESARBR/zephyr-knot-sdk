/* peripheral.c - KNoT Application Client */

/*
 * Copyright (c) 2018, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * KNoT peripheral control.
 *
 * This module is responsible for handling the peripheral controlled by KNoT as
 * control buttons and status leds.
 */

#include <board.h>
#include <device.h>
#include <gpio.h>

#include "peripheral.h"

static struct device *gpiob;
static bool rst_flag;

static struct k_timer rst_timer; /* Reset timer */
static struct gpio_callback rst_btn_cb; /* Reset button callback */

static void set_reset(struct k_timer *timer_id)
{
	rst_flag = true;
}

static void rst_btn_edge(struct device *gpiob, struct gpio_callback *cb, u32_t pins)
{
	u32_t port_value;
	bool rise_edge;

	/* Rising or falling edge */
	gpio_port_read(gpiob, &port_value);
	rise_edge = port_value & pins ? true : false;

	/* Stop reset timer on rising edge */
	if (rise_edge) {
		k_timer_stop(&rst_timer);
		return;
	}
	/* Start reset timer on falling edge */
	k_timer_start(&rst_timer, RST_FLASH_TIMEOUT, RST_FLASH_PERIOD);
}

static int reset_init(void)
{
	/* Initializing reset KNoT storage timer */
	k_timer_init(&rst_timer, set_reset, NULL);

	/* Reset Pin: Input, Pull up, Interruption on every edge */
	gpio_pin_configure(gpiob, RST_FLASH_PIN,
		GPIO_DIR_IN | GPIO_PUD_PULL_UP |
		GPIO_INT | GPIO_INT_EDGE | GPIO_INT_DOUBLE_EDGE);

	/* Pin callback init */
	gpio_init_callback(&rst_btn_cb, rst_btn_edge, BIT(RST_FLASH_PIN));
	gpio_add_callback(gpiob, &rst_btn_cb);
	gpio_pin_enable_callback(gpiob, RST_FLASH_PIN);

	rst_flag = false;

	/* TODO: Check for errors */
	return 0;
}

int peripheral_init(void)
{
	/* Initializing GPIO port */
	gpiob = device_get_binding(GPIO_PORT);
	if (unlikely(!gpiob)) {
		return -1;
	}

	return reset_init();
}

bool peripheral_get_reset(void)
{
	return rst_flag;
}
