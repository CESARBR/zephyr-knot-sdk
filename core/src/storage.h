/* storage.h - KNoT Thing Storage handler */

/*
 * Copyright (c) 2018, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Enum start values as 0 and count up */
enum storage_keys {
	STORAGE_CRED_UUID = 0,
	STORAGE_CRED_TOKEN,
	STORAGE_CRED_DEVID,
	STORAGE_PEER_IPV6,
};

int storage_init(void);
int storage_reset(void);

int storage_read(enum storage_keys key, void *dest, int len);
int storage_write(enum storage_keys key, const void *src, int len);
