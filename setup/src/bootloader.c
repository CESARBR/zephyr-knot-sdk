/* flabootloader.h - Flash driver to control bootload related actions */

/*
 * Copyright (c) 2019, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <logging/log.h>

#include "third_party/nrf_bootloader_app_start.h"

LOG_MODULE_REGISTER(bootloader, LOG_LEVEL_DBG);

int bootloader_start_main(void)
{
	LOG_INF("Starting Main application");
	nrf_bootloader_app_start();
	return -1;	/* Return error as not started main application */
}
