/* net.c - KNoT Application Client */

/*
 * Copyright (c) 2018, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The knot client application is acting as a client that is run in Zephyr OS,
 * The client sends sensor data encapsulated using KNoT netcol.
 */

#include <zephyr.h>
#include <net/net_core.h>
#include <net/net_pkt.h>
#include <net/buf.h>
#include <logging/log.h>

#include "net.h"
#include "tcp6.h"
#if CONFIG_SETTINGS_OT
	#include "ot_config.h"
#endif

LOG_MODULE_DECLARE(knot, CONFIG_KNOT_LOG_LEVEL);

static struct k_thread rx_thread_data;
static K_THREAD_STACK_DEFINE(rx_stack, 1024);
static struct k_pipe *proto2net;
static struct k_pipe *net2proto;
static bool connected;

static void close_cb(void)
{
	// TODO: Not closing net
}

static bool recv_cb(struct net_buf *netbuf)
{
	// TODO: Not handling received messages
	return true;
}

void ot_disconn(void)
{
	LOG_ERR("NET: OT disconnected");

	close_cb();
}

static void connection_start(void)
{
	int ret;

	LOG_DBG("Waiting for OpenThread to be ready...");
	#if CONFIG_SETTINGS_OT
		while (ot_config_is_ready() == false)
			k_sleep(100);
	#endif

	ret = tcp6_start(recv_cb, close_cb);
	if (ret < 0) {
		LOG_DBG("NET: TCP start failure");
		tcp6_stop();
		return;
	}

	connected = true;
}

static void net_thread(void)
{
	u8_t ipdu[128];
	size_t ilen;
	int ret;

	/* Load and set OpenThread credentials from settings */
	#if CONFIG_SETTINGS_OT
		ret = ot_config_init(&ot_disconn);
		if (ret) {
			LOG_ERR("Failed to init OT handler. \
			Aborting net thread");
			return;
		}

		ret = ot_config_load();
		if (ret) {
			LOG_ERR("Failed to load OT credentials. \
			Aborting net thread");
			return;
		}

		ret = ot_config_stop();
		if (ret) {
			LOG_ERR("Failed to stop OT. Aborting net thread");
			return;
		}

		ret = ot_config_set();
		if (ret) {
			LOG_ERR("Failed to set OT credentials. \
			Aborting net thread");
			return;
		}

		ret = ot_config_start();
		if (ret) {
			LOG_ERR("Failed to start OT. Aborting net thread");
			return;
		}
	#endif

	/* Start TCP layer */
	ret = tcp6_init();
	if (ret) {
		LOG_ERR("Failed to init TCP handler. Aborting net thread");
		return;
	}

	memset(ipdu, 0, sizeof(ipdu));
	connection_start();

	while (1) {
		if (!connected) {
			connection_start();
			goto done;
		}

		ilen = 0;
		/* Reading data from PROTO thread */
		ret = k_pipe_get(proto2net, ipdu, sizeof(ipdu),
				 &ilen, 0U, K_NO_WAIT);

		if (ret == 0 && ilen)
			ret = tcp6_send(ipdu, ilen);
done:
		k_yield();
	}

	tcp6_stop();
}

int net_start(struct k_pipe *p2n, struct k_pipe *n2p)
{
	LOG_DBG("NET: Start");

	proto2net = p2n;
	net2proto = n2p;
	connected = false;

	k_thread_create(&rx_thread_data, rx_stack,
			K_THREAD_STACK_SIZEOF(rx_stack),
			(k_thread_entry_t) net_thread,
			NULL, NULL, NULL, K_PRIO_COOP(10), 0, K_NO_WAIT);

	return 0;
}

void net_stop(void)
{
	LOG_DBG("NET: Stop");
}
