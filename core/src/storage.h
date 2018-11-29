/* storage.h - KNoT Thing Storage handler */

/*
 * Copyright (c) 2018, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "knot_protocol.h"

struct storage_app_settings {
	uint64_t device_id;
	char uuid[KNOT_PROTOCOL_UUID_LEN];
	char token[KNOT_PROTOCOL_TOKEN_LEN];
};

enum storage_key {
	STORAGE_APP_SETTINGS_KEY = 0xFFFF	/* App related settings */
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
