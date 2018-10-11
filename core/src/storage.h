/* storage.h - KNoT Thing Storage handler */

/*
 * Copyright (c) 2018, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

enum storage_key {
	STORAGE_KEY_ID		= 0xFFFF,
	STORAGE_KEY_UUID	= 0xFFFE,
	STORAGE_KEY_TOKEN	= 0xFFFD,
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
 * @details Gets UUID, TOKEN or MAC from flash memory.
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
 * @details Save UUID, TOKEN or MAC on flash memory.
 *
 * @param Int key for the value to be retrieved.
 * @param Buffer string to be stored.
 * @param Size buffer len.
 *
 * @retval 0 for SUCCESS.
 * @retval -1 for INVALID KEY.
 */

int8_t storage_set(enum storage_key key, const u8_t *value);
