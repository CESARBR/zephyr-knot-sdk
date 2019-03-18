/* ot_config.c - OpenThread Credentials Handler */

/*
 * Copyright (c) 2019, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if CONFIG_SETTINGS_OT
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

#define CHILD_TIMEOUT 5

#define NET_NAME_LEN 17
#define XPANID_LEN 24
#define MASTERKEY_LEN 48

LOG_MODULE_REGISTER(ot_config, CONFIG_KNOT_LOG_LEVEL);

#if defined(CONFIG_NET_L2_OPENTHREAD)
static struct openthread_context *ot_context;
static otDeviceRole role = OT_DEVICE_ROLE_DISABLED;
ot_config_disconn_cb disconn_cb;
#endif

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
static void update_role(void)
{
	role = otThreadGetDeviceRole(ot_context->instance);

	if (role == OT_DEVICE_ROLE_CHILD) {
		LOG_DBG("OT role: %u (child)", role);
		return;
	}

	LOG_WRN("OT role: %u (not child)", role);
	/* Call disconnection callback if set */
	if (disconn_cb != NULL)
		disconn_cb();

}

static void change_cb(otChangedFlags aFlags, void *aContext)
{
	/* Ignore if no role change */
	if ((aFlags & OT_CHANGED_THREAD_ROLE) == false)
		return;

	LOG_DBG("OT Role changed");
	update_role();
}

int ot_config_init(ot_config_disconn_cb ot_disconn_cb)
{
	struct net_if *iface;
	int rc;

	disconn_cb = ot_disconn_cb;

	/* Load interface and context */
	LOG_DBG("Initializing OpenThread handler");
	iface = net_if_get_default();
	if (iface == NULL) {
		LOG_ERR("Failed to get net interface");
		return -1;
	}

	ot_context = net_if_l2_data(iface);
	if (ot_context == NULL) {
		LOG_ERR("Failed to get OT context");
		return -1;
	}

	rc = otSetStateChangedCallback(ot_context->instance,
				       &change_cb,
				       ot_context);
	if (rc)
		LOG_ERR("Failed to set change cb (err %d)", rc);

	/* Time for Thing to become detached if disconnected */
	otThreadSetChildTimeout(ot_context->instance, CHILD_TIMEOUT);

	return rc;
}

int ot_config_set(void)
{
	otExtendedPanId xpanid_ctx;
	otMasterKey masterkey_ctx;
	int rc;

	/* Convert string to bytes */
	net_bytes_from_str(xpanid_ctx.m8, 8, xpanid);
	net_bytes_from_str(masterkey_ctx.m8, 16, masterkey);

	LOG_DBG("Setting OpenThread credentials");

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

	return OT_ERROR_NONE;
}

int ot_config_start(void)
{
	int rc;

	/* Enable OpenThread service */
	LOG_DBG("Starting OpenThread service");
	rc = otIp6SetEnabled(ot_context->instance, true);
	if (rc) {
		LOG_ERR("Failed to enable IPv6 communication. (err %d)", rc);
		return rc;
	}

	rc = otThreadSetEnabled(ot_context->instance, true);
	if (rc)
		LOG_ERR("Failed to start Thread protocol. (err %d)", rc);

	return rc;
}

int ot_config_stop(void)
{
	int rc;

	/* Disable OpenThread service */
	LOG_DBG("Stopping OpenThread service");
	rc = otThreadSetEnabled(ot_context->instance, false);
	if (rc) {
		LOG_ERR("Failed to stop Thread protocol. (err %d)", rc);
		return rc;
	}

	rc = otIp6SetEnabled(ot_context->instance, false);
	if (rc)
		LOG_ERR("Failed to disable IPv6 communication. (err %d)", rc);

	return rc;
}

bool ot_config_is_ready(void)
{
	/* Device is ready when its role is set to Child */
	return (role == OT_DEVICE_ROLE_CHILD);
}

#endif // endif CONFIG_NET_L2_OPENTHREAD
#endif // endif CONFIG_SETTINGS_OT