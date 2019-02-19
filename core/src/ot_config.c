/* ot_config.c - OpenThread Credentials Handler */

/*
 * Copyright (c) 2019, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <logging/log.h>
#include <settings/settings_ot.h>

#include "ot_config.h"

#define NET_NAME_LEN 17
#define XPANID_LEN 24
#define MASTERKEY_LEN 48

LOG_MODULE_REGISTER(ot_config, LOG_LEVEL_DBG);

static char net_name[NET_NAME_LEN];
static char xpanid[XPANID_LEN];
static char masterkey[MASTERKEY_LEN];
static u16_t panid;
static u8_t channel;

int ot_config_load(void)
{
	int rc;

	LOG_DBG("Reading OpenThread settings");

	rc = settings_ot_read(SETTINGS_OT_PANID, &panid);
	if (rc != sizeof(panid)) {
		LOG_ERR("Failed to read PANID from settings. (err %d)", rc);
		return rc;
	}

	rc = settings_ot_read(SETTINGS_OT_CHANNEL, &channel);
	if (rc != sizeof(channel)) {
		LOG_ERR("Failed to read CHANNEL from settings. (err %d)", rc);
		return rc;
	}

	rc = settings_ot_read(SETTINGS_OT_NET_NAME, net_name);
	if (rc != sizeof(net_name)) {
		LOG_ERR("Failed to read NET_NAME from settings. (err %d)", rc);
		return rc;
	}

	rc = settings_ot_read(SETTINGS_OT_XPANID, xpanid);
	if (rc != sizeof(xpanid)) {
		LOG_ERR("Failed to read XPANID from settings. (err %d)", rc);
		return rc;
	}

	rc = settings_ot_read(SETTINGS_OT_MASTERKEY, masterkey);
	if (rc != sizeof(masterkey)) {
		LOG_ERR("Failed to read MASTERKEY from settings. (err %d)", rc);
		return rc;
	}

	LOG_DBG("Read OpenThread settings:");
	LOG_DBG("  panid: %d", panid);
	LOG_DBG("  channel: %d", channel);
	LOG_DBG("  net_name: %s", net_name);
	LOG_DBG("  xpanid: %s", xpanid);
	LOG_DBG("  masterkey: %s", masterkey);

	return 0;
}
