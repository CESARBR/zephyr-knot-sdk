/* gatt_inet6.c - Callbacks and definition for Peer's IPV6 GATT service */

/*
 * Copyright (c) 2019, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <logging/log.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/gatt.h>

#include "gatt_inet6.h"
#include "storage.h"

/* Buffer len */
#define PEER_IPV6_LEN 40

LOG_MODULE_DECLARE(knot_setup, LOG_LEVEL_DBG);

static char peer_ipv6[PEER_IPV6_LEN];	// Peer's Ipv6

/* Custom Service Variables */
static struct bt_uuid_128 config_service_uuid = BT_UUID_INIT_128(
	0x70, 0x14, 0x1c, 0xbe, 0xdd, 0xe6, 0x5a, 0xb3,
	0x8b, 0x49, 0xb4, 0x5d, 0x83, 0x11, 0x60, 0x49);
static const struct bt_uuid_128 peer_ipv6_uuid = BT_UUID_INIT_128(
	0x71, 0x14, 0x1c, 0xbe, 0xdd, 0xe6, 0x5a, 0xb3,
	0x8b, 0x49, 0xb4, 0x5d, 0x83, 0x11, 0x60, 0x49);

/* Read characteristic generic function */
static ssize_t read_ipv6(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, u16_t len, u16_t offset)
{
	char *value = attr->user_data;
	u16_t max_len;
	int rc;

	rc = storage_read(STORAGE_PEER_IPV6, peer_ipv6, sizeof(peer_ipv6));
	if (rc != sizeof(peer_ipv6))
		return BT_GATT_ERR(BT_ATT_ERR_NOT_SUPPORTED);

	max_len = strlen(peer_ipv6);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, value, max_len);
}

/* Write characteristic callback function */
static ssize_t write_ipv6(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 const void *buf, u16_t len, u16_t offset, u8_t flags)
{
	u8_t *value = attr->user_data;
	u16_t max_len = sizeof(peer_ipv6) - 1;	// Last byte preserved for '\0'
	static char build_peer_ipv6[PEER_IPV6_LEN]; // Peer's Ipv6 build buffer
	int rc;

	if (offset + len > max_len)
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);

	if (offset == 0)
		memset(build_peer_ipv6, 0, max_len);

	memcpy(build_peer_ipv6 + offset, buf, len);

	/* Check for prepare write flag */
	if (flags & BT_GATT_WRITE_FLAG_PREPARE)
		return 0;

	/* Store value at "peer_ipv6", pointed as "value" */
	memcpy(value, build_peer_ipv6, PEER_IPV6_LEN);
	rc = storage_write(STORAGE_PEER_IPV6, build_peer_ipv6, PEER_IPV6_LEN);

	if (rc != PEER_IPV6_LEN)
		return BT_GATT_ERR(BT_ATT_ERR_NOT_SUPPORTED);

	return len;
}

/* Config GATT Service Declaration */
static struct bt_gatt_attr config_gatt_attrs[] = {
	/* Vendor Primary Service Declaration */
	BT_GATT_PRIMARY_SERVICE(&config_service_uuid),
	BT_GATT_CHARACTERISTIC(&peer_ipv6_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE |
			       BT_GATT_PERM_PREPARE_WRITE,
			       read_ipv6, write_ipv6, peer_ipv6),
};

static struct bt_gatt_service config_svc = BT_GATT_SERVICE(config_gatt_attrs);

int gatt_inet6_init(void)
{
	int err;

	/* Storage service start */
	err = storage_init();
	if (err) {
		LOG_ERR("Storage service init failed (err %d)", err);
		return err;
	}

	/* GATT service start */
	err = bt_gatt_service_register(&config_svc);
	if (err) {
		LOG_ERR("GATT service init failed (err %d)", err);
		return err;
	}

	return 0;
}
