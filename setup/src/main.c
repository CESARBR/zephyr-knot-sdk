/* main.c - KNoT Setup main entry point */
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
#include <logging/log.h>
#include <settings/settings.h>
#include <settings/settings_ot.h>

#include "bt_srv.h"
#include "peripheral.h"
#include "bootloader.h"
#include "storage.h"

LOG_MODULE_REGISTER(knot_setup, LOG_LEVEL_DBG);

void main(void)
{
	int err;
	int btn;

	LOG_DBG("Initializing storage services");

	/* Storage service */
	err = storage_init();
	if (err)
		LOG_ERR("Storage service init failed (err %d)", err);

	/* OT Settings storage */
	err = settings_ot_init();
	if (err)
		LOG_ERR("Settings OT init failed (err %d)", err);

	LOG_DBG("Loading stored values");
	err = settings_load();
	if (err)
		LOG_ERR("Settings load failed (err %d)", err);


	LOG_DBG("Selecting application to run");

	/* Jump to main application if button pressed */
	err = peripheral_init();
	if (err)
		LOG_ERR("Peripheral initialization failed");
	btn = peripheral_btn_status();

	switch (btn) {
	case PERIPHERAL_BTN_NOT_PRESSED:
		LOG_DBG("Setup button not pressed: Loading Main App");
		err = bootloader_start_main();
		if (err == false) /* Successful load */
			return;

		for (;;) {
			LOG_ERR("Failed to load Main App");
			k_sleep(2000);
		}
	case PERIPHERAL_BTN_ERROR:
		LOG_ERR("Button reading error!");
	}

	/* Setup Application */
	LOG_DBG("Initializing Setup App");

	/* Bluetooth services */
	err = bt_srv_init();
	if (err) {
		LOG_ERR("Failed to initialize bluetooth");
		return;
	}
}
