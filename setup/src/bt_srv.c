/* bt_srv.c - Bluetooth services handler */
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

#include <settings/settings_ot.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>

#include "bt_srv.h"

LOG_MODULE_DECLARE(knot_setup, LOG_LEVEL_DBG);

/* Advertise OpenThread Settings Service */
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL,
		      0x30, 0x0d, 0x90, 0xb4, 0x7b, 0x81, 0xec, 0x9b,
	              0x41, 0xd4, 0x9a, 0xaa, 0x9c, 0xe4, 0xa9, 0xa8),
};

static void connected(struct bt_conn *conn, u8_t err)
{
	if (err) {
		LOG_ERR("Connection failed (err %u)", err);
	} else {
		LOG_DBG("Connected");
	}
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
	LOG_DBG("Disconnected (reason %u)", reason);
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

int bt_srv_init(void)
{
	int err;

	/* OT Settings storage system */
	err = settings_ot_init();
	if (err) {
		LOG_ERR("Settings OT init failed (err %d)", err);
		return err;
	}

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth enable failed (err %d)", err);
		return err;
	}
	LOG_DBG("Bluetooth initialized");

	bt_conn_cb_register(&conn_callbacks);

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		LOG_ERR("Advertising failed to start (err %d)", err);
		return err;
	}
	LOG_INF("Advertising successfully started");

	return 0;
}
