/* storage_mock.c - KNoT Thing Storage MOCK handler */

/*
 * Copyright (c) 2018, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This file works as a selector for using real flash or a mocked up file.
 * To use with qemu, use the storage.qemu file.
 */

#include <zephyr.h>
#include <net/net_core.h>
#include <logging/log.h>
#include <string.h>

#include "storage.h"

LOG_MODULE_REGISTER(knot_storage, CONFIG_KNOT_LOG_LEVEL);

#if CONFIG_BOARD_QEMU_X86
#include "storage.qemu"
#else
#include <settings/settings.h>

/* Storage path identifiers */
#define NAMESPACE		"knot"

#define UUID_KEY		"uuid"
#define TOKEN_KEY		"token"
#define DEVID_KEY		"devid"
#define IPV6_KEY		"ipv6"

#define SAVE_UUID_KEY		NAMESPACE "/" UUID_KEY
#define SAVE_TOKEN_KEY		NAMESPACE "/" TOKEN_KEY
#define SAVE_DEVID_KEY		NAMESPACE "/" DEVID_KEY
#define SAVE_IPV6_KEY		NAMESPACE "/" IPV6_KEY

/* Buffer sizes */
#define UUID_LEN	36
#define TOKEN_LEN	40
#define IPV6_LEN	40

/* Buffers */
static char uuid[UUID_LEN];		/* Device UUID */
static char token[TOKEN_LEN];		/* Device Token */
static char peer_ipv6[TOKEN_LEN];	/* Peer's IPV6 */
static uint64_t devid;			/* Device ID */

struct key_fmt {
	const char *save_key;	/* Settings name or key */
	void *buffer;		/* Pointer to buffers */
	size_t bsize;		/* Buffer size */
	bool loaded;		/* Value loaded from storage */
};

/* Map with info of used buffers */
static struct key_fmt buf_info[] = {
	{ SAVE_UUID_KEY,	uuid,		sizeof(uuid),		false },
	{ SAVE_TOKEN_KEY,	token,		sizeof(token),		false },
	{ SAVE_DEVID_KEY,	&devid,		sizeof(devid),		false },
	{ SAVE_IPV6_KEY,	peer_ipv6,	sizeof(peer_ipv6),	false },
};

static int set(int argc, char **argv, void *value_ctx)
{
	struct key_fmt *fmt;
	int rc;

	if (argc != 1)
		return -ENOENT;

	/* Get keys */
	if (!strcmp(argv[0], UUID_KEY))
		fmt = &buf_info[STORAGE_CRED_UUID];
	else if (!strcmp(argv[0], TOKEN_KEY))
		fmt = &buf_info[STORAGE_CRED_TOKEN];
	else if (!strcmp(argv[0], DEVID_KEY))
		fmt = &buf_info[STORAGE_CRED_DEVID];
	else if (!strcmp(argv[0], IPV6_KEY))
		fmt = &buf_info[STORAGE_PEER_IPV6];
	else /* Ignore invalid key */
		return -ENOENT;

	/* Get values from storage */
	rc = settings_val_read_cb(value_ctx, fmt->buffer, fmt->bsize);

	/* Sign if value was loaded or not */
	fmt->loaded = (rc < 0) ? false : true ;

	return rc;
}

static int commit(void)
{
	if (buf_info[STORAGE_CRED_UUID].loaded &&
	    buf_info[STORAGE_CRED_TOKEN].loaded &&
	    buf_info[STORAGE_CRED_DEVID].loaded )
		LOG_DBG("KNoT credentials found");
	else
		LOG_DBG("KNoT credentials not found");

	if (buf_info[STORAGE_PEER_IPV6].loaded)
		LOG_DBG("Peer's IPV6 found");
	else
		LOG_DBG("Peer's IPV6 not found");

	return 0;
}

static struct settings_handler handler = {
	.name = NAMESPACE,
	.h_set = set,
	.h_commit = commit,
};

int storage_init(void)
{
	int err;

	LOG_DBG("Initializing Storage Settings");
	err = settings_subsys_init();
	if (err) {
		LOG_ERR("Settings subsys init failed (err %d)", err);
		return err;
	}

	LOG_DBG("Register settings handler");
	err = settings_register(&handler);
	if (err) {
		LOG_ERR("Settings register failed (err %d)", err);
		return err;
	}

	return 0;
}

static int clear_value(enum storage_keys key)
{
	struct key_fmt *fmt;
	int rc;

	fmt = &buf_info[key];

	rc = settings_delete(fmt->save_key);
	if (rc)
		LOG_ERR("Deleting key \"%s\" failed (err %d)", fmt->save_key,
							       rc);
	else
		fmt->loaded = false;

	return rc;
}

int storage_reset(void)
{
	int rc;

	/* Clear app credentials */
	rc = clear_value(STORAGE_CRED_UUID);
	if (rc)
		return rc;

	rc = clear_value(STORAGE_CRED_TOKEN);
	if (rc)
		return rc;

	rc = clear_value(STORAGE_CRED_DEVID);
	if (rc)
		return rc;

	return clear_value(STORAGE_PEER_IPV6);
}

bool storage_is_set(enum storage_keys key)
{
	struct key_fmt *fmt;

	fmt = &buf_info[key];
	return fmt->loaded;
}

int storage_read(const enum storage_keys key, void *dest, int len)
{
	const struct key_fmt *fmt;
	int olen;

	if (len <= 0)
		return -EINVAL;

	if (storage_is_set(key) == false)
		return -ENOENT;

	fmt = &buf_info[key];

	/* Return buffer value */
	olen = (len < fmt->bsize) ? len : fmt->bsize;
	memcpy(dest, fmt->buffer, olen);

	return olen;
}

int storage_write(enum storage_keys key, const void *src, const int len)
{
	struct key_fmt *fmt;
	int err;
	int olen;

	if (key < 0 || key >= sizeof(buf_info))
		return -EINVAL;

	if (len <= 0)
		return -EINVAL;

	/* Store value */
	/* Cast to (void *) due to lack of const identifier on value argument */
	fmt = &buf_info[key];
	olen = (len < fmt->bsize) ? len : fmt->bsize;
	err = settings_save_one(fmt->save_key, (void *) src, olen);
	if (err) {
		LOG_ERR("Failed to save value for key \"%s\"", fmt->save_key);
		return err;
	}

	/* Save value to buffer */
	memcpy(fmt->buffer, src, olen);

	/* Set value as available */
	fmt->loaded = true;

	return olen;
}


#endif
