/* storage_mock.c - KNoT Thing Storage MOCK handler */

/*
 * Copyright (c) 2018, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This file works as a mock-up for the storage on NVM. It is intended to be
 * used for testing only.
 * To use with the nRF52840 board, use the storage.nrf52840 file.
 */

#include <zephyr.h>
#include <net/net_core.h>
#include <string.h>

#include "storage.h"
#include "knot_protocol.h"

#define SYS_LOG_DOMAIN "knot"
#define NET_SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_LOG_ENABLED 1

#define MAC_ADDR_LEN 12 // in bytes

/* Lens */

static u8_t mac_buf[MAC_ADDR_LEN];
static u8_t uuid_buf[KNOT_PROTOCOL_UUID_LEN];
static u8_t token_buf[KNOT_PROTOCOL_TOKEN_LEN];

int8_t storage_init(void)
{
	NET_DBG("Initializing mock storage");

	memset(mac_buf, 0, sizeof(mac_buf));
	memset(uuid_buf, 0, sizeof(uuid_buf));
	memset(token_buf, 0, sizeof(token_buf));

	return 0;
}

int8_t storage_reset(void)
{
	NET_DBG("Reseting mock storage");

	memset(mac_buf, 0, sizeof(mac_buf));
	memset(uuid_buf, 0, sizeof(uuid_buf));
	memset(token_buf, 0, sizeof(token_buf));

	return 0;
}

int8_t storage_get(enum storage_key key, u8_t *value)
{
	switch (key) {
	case STORAGE_KEY_UUID:
		if (!strlen(uuid_buf))
			goto err;
		memcpy(value, uuid_buf, KNOT_PROTOCOL_UUID_LEN);
		break;
	case STORAGE_KEY_TOKEN:
		if (!strlen(token_buf))
			goto err;
		memcpy(value, token_buf, KNOT_PROTOCOL_TOKEN_LEN);
		break;
	case STORAGE_KEY_MAC:
		if (!strlen(mac_buf))
			goto err;
		memcpy(value, mac_buf, MAC_ADDR_LEN);
		break;
	}

	return 0;

err:
	return -ENOENT;
}

int8_t storage_set(enum storage_key key, const u8_t *value)
{

	switch (key) {
	case STORAGE_KEY_UUID:
		memcpy(uuid_buf, value, KNOT_PROTOCOL_UUID_LEN);
		break;
	case STORAGE_KEY_TOKEN:
		memcpy(token_buf, value, KNOT_PROTOCOL_TOKEN_LEN);
		break;
	case STORAGE_KEY_MAC:
		memcpy(mac_buf, value, MAC_ADDR_LEN);
		break;
	}

	return 0;
}
