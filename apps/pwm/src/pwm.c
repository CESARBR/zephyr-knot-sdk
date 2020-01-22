/* pwm.c - KNoT Application Client */

/*
 * Copyright (c) 2019, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This example uses a PWM approach to blink a led in a specified time interval
 */

#include <zephyr.h>
#include <net/net_core.h>
#include <logging/log.h>
#include <device.h>
#include <pwm.h>

#include "knot.h"
#include <knot/knot_types.h>
#include <knot/knot_protocol.h>

#define PWM_DRIVER     DT_NORDIC_NRF_PWM_PWM_0_LABEL
#define PWM_CHANNEL    DT_NORDIC_NRF_PWM_4001C000_CH0_PIN
#define PERIOD_NS      26315 /* Period equivalent to a 38kHz frequency */

LOG_MODULE_REGISTER(pwm, LOG_LEVEL_DBG);

struct device *pwm_dev;
u32_t pulse_width;

void setup(void)
{
	/* Peripherals control */
	pwm_dev = device_get_binding(PWM_DRIVER);

	/* KNoT config */
	knot_data_register(0, "PWM", KNOT_TYPE_ID_ANALOG,
				KNOT_VALUE_TYPE_INT, KNOT_UNIT_NOT_APPLICABLE,
				&pulse_width, sizeof(pulse_width), NULL, NULL);
	knot_data_config(0, KNOT_EVT_FLAG_CHANGE, NULL);
}

int64_t last_toggle_time = 0;

void loop(void)
{
	/* Get current time */
	int64_t current_time = k_uptime_get();

	/* Blinks led every 5 seconds */
	if (current_time - last_toggle_time > 5000) {
		if (pulse_width == 0) {
			pulse_width = PERIOD_NS;
			pwm_pin_set_nsec(pwm_dev, PWM_CHANNEL,
					PERIOD_NS, pulse_width);
		}
		else {
			pulse_width = 0;
			pwm_pin_set_nsec(pwm_dev, PWM_CHANNEL,
					PERIOD_NS, pulse_width);
		}
		last_toggle_time = current_time;
	}
}
