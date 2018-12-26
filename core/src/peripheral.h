/* peripheral.c - KNoT Application Client */

/*
 * Copyright (c) 2018, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* General GPIO Controller */
#define GPIO_PORT		SW0_GPIO_CONTROLLER

/* Reset flash controller */
#define RST_FLASH_PIN		SW3_GPIO_PIN
#define RST_FLASH_TIMEOUT	2000 /* Millis */
#define RST_FLASH_PERIOD	200 /* Millis */

/* Status led toggle controller. Toggle led periods in millis */
#define STATUS_LED_PIN		LED3_GPIO_PIN
#define STATUS_DISCONN_PERIOD	1000 /* Disconnected. Toggle every 500 ms */
#define STATUS_CONN_PERIOD	-1   /* Connected. Don't toggle */
#define STATUS_ERROR_PERIOD	100  /* Error. Toggle every 100 ms */

int peripheral_init(void);

bool peripheral_get_reset(void);

void peripheral_set_status_period(s64_t status);

bool peripheral_flag_status(void);
