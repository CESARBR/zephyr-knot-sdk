/* analog.c - KNoT Application Client */

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

#include <device.h>
#include <gpio.h>
#include <adc.h>
#include <hal/nrf_saadc.h>

LOG_MODULE_REGISTER(hello, LOG_LEVEL_DBG);

/* Tracked value */
bool led;
float adc_norm;

#define GPIO_PORT	LED1_GPIO_CONTROLLER /* General GPIO Controller */
#define LED_PIN		LED1_GPIO_PIN /* User LED */

static struct device *gpiob;		/* GPIO device */
static struct device *adc_dev;

static void read_adc(struct knot_proxy *proxy)
{
	knot_proxy_value_set_basic(proxy, &adc_norm);
}

static const struct adc_channel_cfg channel_cfg = {
	.gain             = ADC_GAIN_1_5,
	.reference        = ADC_REF_INTERNAL,
	.acquisition_time = ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 20),
	.channel_id       = 0,
	.input_positive   = NRF_SAADC_INPUT_AIN7, // Use pin 0.31 as ADC
};

#define LOWER_LIMIT	0.2
#define UPPER_LIMIT	0.8

void setup(void)
{
	/* Configure LED */
	gpiob = device_get_binding(GPIO_PORT);
	gpio_pin_configure(gpiob, LED_PIN, GPIO_DIR_OUT);

	/* Configure ADC */
	adc_dev = device_get_binding(DT_ADC_0_NAME);
	adc_channel_setup(adc_dev, &channel_cfg);

	/* Send readings */
	knot_data_register(0, "Norm", KNOT_TYPE_ID_ANGLE,
			   KNOT_VALUE_TYPE_FLOAT, KNOT_UNIT_ANGLE_DEGREE,
			   NULL, read_adc);
	knot_data_config(0,
			 KNOT_EVT_FLAG_TIME, 20,
			 KNOT_EVT_FLAG_LOWER_THRESHOLD, LOWER_LIMIT,
			 KNOT_EVT_FLAG_UPPER_THRESHOLD, UPPER_LIMIT,
			 NULL);
}

int16_t adc_buffer;

const struct adc_sequence sequence = {
	.channels    = BIT(0),
	.buffer      = &adc_buffer,
	.buffer_size = sizeof(adc_buffer),
	.resolution  = 12,
};

void loop(void)
{
	/* Clear buffer and read value*/
	memset(&adc_buffer, 0, sizeof(adc_buffer));
	adc_read(adc_dev, &sequence);

	/* Convert value */
	adc_norm = (1.0 * adc_buffer)/4095;

	/* Turn led on if out of limits */
	if (adc_norm > UPPER_LIMIT || adc_norm < LOWER_LIMIT) {
		gpio_pin_write(gpiob, LED_PIN, false);
	} else {
		gpio_pin_write(gpiob, LED_PIN, true);
	}
}
