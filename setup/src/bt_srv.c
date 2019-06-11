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

#include <settings/settings.h>
#include <settings/settings_ot.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/services/setup_ot.h>
#include <mgmt/smp_bt.h>
#include <mgmt/buf.h>

#ifdef CONFIG_MCUMGR_CMD_IMG_MGMT
#include "img_mgmt/img_mgmt.h"
#endif

#include "bt_srv.h"
#include "gatt_inet6.h"
#include "clear.h"

LOG_MODULE_DECLARE(knot_setup, LOG_LEVEL_DBG);

#define ADVERTISING_TOGGLE_MS 500

bool active_conn;

/* Advertise Peer's IPV6 GATT service's UUID  */
static const struct bt_data ad_inet6[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_SOME,
		      0x70, 0x14, 0x1c, 0xbe, 0xdd, 0xe6, 0x5a, 0xb3,
		      0x8b, 0x49, 0xb4, 0x5d, 0x83, 0x11, 0x60, 0x49),
};

/* Advertise Peer's MCUMGR service's UUID  */
static const struct bt_data ad_mcumgr[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_SOME,
		      0x84, 0xaa, 0x60, 0x74, 0x52, 0x8a, 0x8b, 0x86,
		      0xd3, 0x4c, 0xb7, 0x1d, 0x1d, 0xdc, 0x53, 0x8d),
};

/* Scan respond OpenThread Settings GATT service's UUID */
static const struct bt_data scan_resp_ot[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_SOME,
		      0x30, 0x0d, 0x90, 0xb4, 0x7b, 0x81, 0xec, 0x9b,
	              0x41, 0xd4, 0x9a, 0xaa, 0x9c, 0xe4, 0xa9, 0xa8),
};

static void advertise(const struct bt_data *adv)
{
	int err;

	bt_le_adv_stop();

	/* Using ad_inet6 as default size for ad_inet6 and ad_mcumgr */
	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME,
			      adv, ARRAY_SIZE(ad_inet6),
			      scan_resp_ot, ARRAY_SIZE(scan_resp_ot));
	if (err) {
		LOG_ERR("Advertising failed to start (err %d)", err);
	}
}

static void connected(struct bt_conn *conn, u8_t err)
{
	active_conn = true;

	if (err) {
		LOG_ERR("Connection failed (err %u)", err);
	} else {
		LOG_DBG("Connected");
	}
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
	active_conn = false;
	LOG_DBG("Disconnected (reason %u)", reason);
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

static void pairing_complete_cb(struct bt_conn *conn, bool bonded)
{
	LOG_DBG("Pairing complete: %d", bonded);
}

static void pairing_failed_cb(struct bt_conn *conn)
{
	LOG_DBG("Pairing failed!");
	bt_conn_disconnect(conn, BT_HCI_ERR_AUTHENTICATION_FAIL);
}

static struct bt_conn_auth_cb auth_cb = {
	.pairing_complete = pairing_complete_cb,
	.pairing_failed = pairing_failed_cb,
};

static void ot_updated(void)
{
	static bool ot_nvs_wiped = false;

	LOG_DBG("OpenThread value updated");

	if (ot_nvs_wiped == false) {
		clear_ot_nvs();
		ot_nvs_wiped = true;
	}
}

int bt_srv_init(void)
{
	int err;

	active_conn = false;

	/* Peer's IPV6 GATT service */
	err = gatt_inet6_init();
	if (err) {
		LOG_ERR("Peer's IPV6 Config GATT service init failed (err %d)", err);
		return err;
	}

	/* Set OpenThread value updated callback */
	setup_ot_updated_cb_register(&ot_updated);

#ifdef CONFIG_MCUMGR_CMD_IMG_MGMT
	img_mgmt_register_group();
#endif

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth enable failed (err %d)", err);
		return err;
	}
	LOG_DBG("Bluetooth initialized");

	bt_conn_cb_register(&conn_callbacks);
	bt_conn_auth_cb_register(&auth_cb);

	/* Initialize the Bluetooth mcumgr transport. */
	smp_bt_register();

	settings_load();

	LOG_INF("Bluetooth ready for advertising...");

	return 0;
}

/* Alternate advertising if not connected */
void bt_srv_toggle_advertising(void)
{
	const struct bt_data *adv_tmp;
	static s64_t last_toggle_time = 0;
	s64_t current_time;
	static bool adv_bool = false;

	/* Ignore if connected */
	if (active_conn)
		return;

	/* Wait for toggle time */
	current_time = k_uptime_get();
	if (current_time - last_toggle_time < ADVERTISING_TOGGLE_MS)
		return;

	/* Toggle advertising */
	adv_tmp = (adv_bool) ? ad_inet6 : ad_mcumgr;
	advertise(adv_tmp);
	adv_bool = !adv_bool;

	/* Update time */
	last_toggle_time = current_time;
}
