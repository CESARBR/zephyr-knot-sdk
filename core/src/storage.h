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

/**
 * @brief Prepare environment to use flash memory.
 *
 * @details Initialise flash device and load values.
 *
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int8_t storage_init(void);

/**
 * @brief Clear all stored values.
 *
 * @details Clear info of all credentials.
 *
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int8_t storage_reset(void);

/**
 * @brief Gets value loaded from storage.
 *
 * @details Gets UUID, TOKEN and Device Id loaded from flash memory.
 *
 * @param dest Destination buffer to receive loaded value.
 *
 * @retval 0 for SUCCESS.
 * @retval -ERRNO errno code for Error.
 */

int8_t storage_get(struct storage_app_settings *dest);

/**
 * @brief Store value on NVM.
 *
 * @details Save UUID, TOKEN and Device Id on flash memory.
 *
 * @param src Buffer structure containing values to be stored.
 *
 * @retval 0 for SUCCESS.
 * @retval -ERRNO errno code for Error.
 */
int8_t storage_set(const struct storage_app_settings *src);
