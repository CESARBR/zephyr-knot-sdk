/* ot_config.c - OpenThread Credentials Handler */

/*
 * Copyright (c) 2019, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <logging/log.h>
#include <settings/settings_ot.h>
#if defined(CONFIG_NET_L2_OPENTHREAD)
#include <net/net_if.h>
#include <net/openthread.h>
#include <openthread/thread.h>
#include <openthread/link.h>
#endif

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

#if defined(CONFIG_NET_L2_OPENTHREAD)
int ot_config_set(void)
{
	struct net_if *iface;
	struct openthread_context *ot_context;
	otExtendedPanId xpanid_ctx;
	otMasterKey masterkey_ctx;
	int rc;

	/* Load interface and context */
	iface = net_if_get_default();
	ot_context = net_if_l2_data(iface);

	/* Convert string to bytes */
	net_bytes_from_str(xpanid_ctx.m8, 8, xpanid);
	net_bytes_from_str(masterkey_ctx.m8, 16, masterkey);

	LOG_DBG("Setting OpenThread credentials");

	/* Disable OpenThread service */
	rc = otThreadSetEnabled(ot_context->instance, false);
	if (rc) {
		LOG_ERR("Failed to stop Thread protocol. (err %d)", rc);
		return rc;
	}
	rc = otIp6SetEnabled(ot_context->instance, false);
	if (rc) {
		LOG_ERR("Failed to disable IPv6 communication. (err %d)", rc);
		return rc;
	}

	/* Set credentials */
	rc = otThreadSetNetworkName(ot_context->instance, net_name);
	if (rc) {
		LOG_ERR("Failed to configure net_name. (err %d)", rc);
		return rc;
	}

	rc = otLinkSetChannel(ot_context->instance, channel);
	if (rc) {
		LOG_ERR("Failed to configure channel. (err %d)", rc);
		return rc;
	}

	rc = otLinkSetPanId(ot_context->instance, panid);
	if (rc) {
		LOG_ERR("Failed to configure panid. (err %d)", rc);
		return rc;
	}

	rc = otThreadSetExtendedPanId(ot_context->instance, &xpanid_ctx);
	if (rc) {
		LOG_ERR("Failed to configure xpanid. (err %d)", rc);
		return rc;
	}

	rc = otThreadSetMasterKey(ot_context->instance, &masterkey_ctx);
	if (rc) {
		LOG_ERR("Failed to configure masterkey. (err %d)", rc);
		return rc;
	}

	/* Enable OpenThread service */
	rc = otIp6SetEnabled(ot_context->instance, true);
	if (rc) {
		LOG_ERR("Failed to enable IPv6 communication. (err %d)", rc);
		return rc;
	}

	rc = otThreadSetEnabled(ot_context->instance, true);
	if (rc) {
		LOG_ERR("Failed to start Thread protocol. (err %d)", rc);
		return rc;
	}

	return OT_ERROR_NONE;
}
#endif
