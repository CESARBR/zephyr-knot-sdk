/* storage.h - KNoT Thing Storage handler */

/*
 * Copyright (c) 2018, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "knot_protocol.h"

#define STORAGE_NET_NAME_LEN 16
#define STORAGE_XPANID_LEN 23
#define STORAGE_MASTERKEY_LEN 47
#define STORAGE_PEER_IPV6_LEN 39

struct storage_app_settings {
	uint64_t device_id;
	char uuid[KNOT_PROTOCOL_UUID_LEN];
	char token[KNOT_PROTOCOL_TOKEN_LEN];
};

struct storage_pan_settings {
	bool need_config;			// PAN info needs settings
	uint16_t pan_id;			// OpenThread PAN Id
	uint16_t channel;			// OpenThread channel
	char net_name[STORAGE_NET_NAME_LEN];	// OpenThread net name
	char xpanid[STORAGE_XPANID_LEN];	// OpenThread expanded PAN Id
	char masterkey[STORAGE_MASTERKEY_LEN];	// OpenThread master key
	char peer_ipv6[STORAGE_PEER_IPV6_LEN];	// OpenThread peer's ipv6
};

enum storage_key {
	STORAGE_APP_SETTINGS_KEY     = 0xFFFF,	/* App related settings */
	STORAGE_PAN_SETTINGS_KEY     = 0xFFFE,	/* PAN related settings */
};

/**
 * @brief Prepare environment to use flash memory.
 *
 * @details Initialise flash device without clearing values.
 *
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int8_t storage_init(void);

/**
 * @brief Delete all stored values.
 *
 * @details Delete info for all known IDs for this file system.
 *
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int8_t storage_reset(void);

/**
 * @brief Gets value from NVM.
 *
 * @details Gets UUID, TOKEN and MAC from flash memory.
 *
 * @param Int key for the value to be retrieved.
 * @param Buffer string to be stored.
 * @param Size buffer len.
 *
 * @retval 0 for SUCCESS.
 * @retval -ERRNO errno code for Error.
 */

int8_t storage_get(enum storage_key key, u8_t *value);

/**
 * @brief Save value on NVM.
 *
 * @details Save UUID, TOKEN and MAC on flash memory.
 *
 * @param Int key for the value to be retrieved.
 * @param Buffer string to be stored.
 * @param Size buffer len.
 *
 * @retval 0 for SUCCESS.
 * @retval -1 for INVALID KEY.
 */

int8_t storage_set(enum storage_key key, const u8_t *value);
