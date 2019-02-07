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

int8_t storage_init(void);

int8_t storage_reset(void);

int8_t storage_get(struct storage_app_settings *dest);

int8_t storage_set(const struct storage_app_settings *src);
