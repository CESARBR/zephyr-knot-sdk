/* gatt_ctrl.c - GATT service to control device */

/*
 * Copyright (c) 2019, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <logging/log.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/gatt.h>

#include "gatt_ctrl.h"

LOG_MODULE_DECLARE(knot_setup, LOG_LEVEL_DBG);

static u8_t cmd;		// Received command

/* Custom Service Variables */
static struct bt_uuid_128 service_uuid = BT_UUID_INIT_128(
	0x70, 0xba, 0x2b, 0xfb, 0x05, 0x74, 0x4a, 0x64,
	0x9a, 0x93, 0xc8, 0x7c, 0xf3, 0x8f, 0x41, 0x81);
static const struct bt_uuid_128 cmd_uuid = BT_UUID_INIT_128(
	0x71, 0xba, 0x2b, 0xfb, 0x05, 0x74, 0x4a, 0x64,
	0x9a, 0x93, 0xc8, 0x7c, 0xf3, 0x8f, 0x41, 0x81);

/* Write characteristic callback function */
static ssize_t write_cmd(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 const void *buf, u16_t len, u16_t offset, u8_t flags)
{
	u8_t *value = attr->user_data;
	u16_t max_len = sizeof(cmd);

	if (offset + len > max_len)
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);

	if (offset == 0)
		memset(value, 0, max_len);

	memcpy(value + offset, buf, len);

	return len;
}

/* Config GATT Service Declaration */
static struct bt_gatt_attr config_gatt_attrs[] = {
	/* Primary Service Declaration */
	BT_GATT_PRIMARY_SERVICE(&service_uuid),
	BT_GATT_CHARACTERISTIC(&cmd_uuid.uuid,
			       BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_WRITE,
			       NULL, write_cmd,
			       &cmd),
};

static struct bt_gatt_service config_svc = BT_GATT_SERVICE(config_gatt_attrs);

int gatt_ctrl_init(void)
{
	int err;

	/* GATT service start */
	err = bt_gatt_service_register(&config_svc);
	if (err) {
		LOG_ERR("GATT service init failed (err %d)", err);
		return err;
	}

	return 0;
}
