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
#include "ot_config.h"

LOG_MODULE_REGISTER(knot_setup, LOG_LEVEL_DBG);

void main(void)
{
	int err;
	int btn;
	bool ipv6_set;
	bool ot_set;

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

	/* Check for stored values */
	ipv6_set = storage_is_set(STORAGE_PEER_IPV6);
	if (ipv6_set)
		LOG_DBG("Peer's IPv6 set");
	else
		LOG_DBG("Peer's IPv6 not set");

	err = ot_config_load();
	ot_set = (err == 0); /* OT config loaded. Has OT settings */

	/* Jump to main application if button pressed */
	err = peripheral_init();
	if (err)
		LOG_ERR("Peripheral initialization failed");
	btn = peripheral_btn_status();

	switch (btn) {
	case PERIPHERAL_BTN_PRESSED:
		LOG_DBG("Setup button pressed");
		break;
	case PERIPHERAL_BTN_NOT_PRESSED:
		LOG_DBG("Setup button not pressed");
		break;
	case PERIPHERAL_BTN_ERROR:
	default:
		LOG_ERR("Button reading error!");
	}

	/*
	 * Run Setup App if button pressed or if PAN Settings
	 * not set.
	 */
	if (btn == PERIPHERAL_BTN_PRESSED ||
	    ipv6_set == false || ot_set == false)
		goto setup;

	/* Load Main App */
	err = bootloader_start_main();
	if (err == false) /* Successful load */
		return;

	/* Flag error if Main App not loaded */
	for (;;) {
		LOG_ERR("Failed to load Main App");
		k_sleep(2000);
	}

setup:
	/* Setup Application */
	LOG_DBG("Initializing Setup App");

	/* Bluetooth services */
	err = bt_srv_init();
	if (err) {
		LOG_ERR("Failed to initialize bluetooth");
		return;
	}

	while (1) {
		bt_srv_toggle_advertising();
	}
}
