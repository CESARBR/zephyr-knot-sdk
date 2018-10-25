/* peripheral.c - KNoT Application Client */

/*
 * Copyright (c) 2018, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* General GPIO Controller */
#define GPIO_PORT		SW0_GPIO_NAME

/* Reset flash controller */
#define RST_FLASH_PIN		SW3_GPIO_PIN
#define RST_FLASH_TIMEOUT	2000 /* Millis */
#define RST_FLASH_PERIOD	200 /* Millis */

int peripheral_init(void);

bool peripheral_get_reset(void);
