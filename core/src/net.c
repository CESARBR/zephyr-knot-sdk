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
#if CONFIG_NET_UDP
#include "udp6.h"
#elif CONFIG_NET_TCP
#include "tcp6.h"
#endif
#if CONFIG_SETTINGS_OT
	#include "ot_config.h"
#endif

LOG_MODULE_DECLARE(knot, CONFIG_KNOT_LOG_LEVEL);

static struct k_thread rx_thread_data;
static K_THREAD_STACK_DEFINE(rx_stack, 1024);
static struct k_pipe *proto2net;
static struct k_pipe *net2proto;
static bool connected;

K_SEM_DEFINE(conn_sem, 0, 1);

#define CONN_RETRY_TIME K_SECONDS(5)

static void close_cb(void)
{
	/* Flag as not connected */
	k_sem_take(&conn_sem, K_NO_WAIT);
	connected = false;
}

static int recv_cb(void *buf, size_t len)
{
	int rc;
	u8_t opdu[128];

	if (len > sizeof(opdu)) {
		LOG_ERR("NET: SMALL PDU");
		return -EMSGSIZE;
	}

	memcpy(opdu, buf, len);
	/* Sending recv data to PROTO thread */
	rc = k_pipe_put(net2proto, opdu, len, &len, len, K_NO_WAIT);
	if (rc)
		LOG_ERR("Pipe write failed. Err: %d", rc);

	return rc;
}

void ot_disconn(void)
{
	LOG_ERR("NET: OT disconnected");

	close_cb();
}

static int connection_start(void)
{
	int ret;

	LOG_DBG("Waiting for OpenThread to be ready...");
	#if CONFIG_SETTINGS_OT
		while (ot_config_is_ready() == false)
			k_sleep(100);
	#endif

	#if CONFIG_NET_UDP
		ret = udp6_start(recv_cb, close_cb);
		if (ret < 0) {
			LOG_DBG("NET: UDP start failure");
			goto done;
		}
		LOG_DBG("NET: UDP started");
	#elif CONFIG_NET_TCP
		ret = tcp6_start(recv_cb, close_cb);
		if (ret < 0) {
			LOG_DBG("NET: TCP start failure");
			goto done;
		}

		LOG_DBG("NET: TCP started");
	#endif

	connected = true;
	k_sem_give(&conn_sem);

done:
	return ret;
}

static void net_thread(void)
{
	u8_t ipdu[128];
	size_t ilen;
	int ret;

	#if CONFIG_NET_UDP
		/* Start UDP layer */
		ret = udp6_init();
		if (ret) {
			LOG_ERR("Failed to init UDP handler. Aborting net thread");
			return;
		}
	#elif CONFIG_NET_TCP
		/* Start TCP layer */
		ret = tcp6_init();
		if (ret) {
			LOG_ERR("Failed to init TCP handler. Aborting net thread");
			return;
		}
	#endif

	memset(ipdu, 0, sizeof(ipdu));
	connection_start();

	while (1) {
		if (!connected) {
			ret = connection_start();
			if (ret) {
				/* Wait before retrying connecting */
				LOG_ERR("Waiting to retry to connecting...");
				k_sleep(CONN_RETRY_TIME);
				goto done;
			}
		}
		/* Look for incoming messages */
		#if CONFIG_NET_UDP
			udp6_event_poll();
		#elif CONFIG_NET_TCP
			tcp6_event_poll();
		#endif

		ilen = 0;
		/* Reading data from PROTO thread */
		ret = k_pipe_get(proto2net, ipdu, sizeof(ipdu),
				 &ilen, 0U, K_NO_WAIT);
		if (ret != 0) {
			LOG_ERR("Pipe read failed");
			goto done;
		}

		/* No message to send */
		if (ilen == 0)
			goto done;

		/* Send message */
		#if CONFIG_NET_UDP
			ret = udp6_send(ipdu, ilen);
		#elif CONFIG_NET_TCP
			ret = tcp6_send(ipdu, ilen);
		#endif

		if (ret <= 0)
			LOG_ERR("Msg send fail (%d)", ret);
		else
			LOG_DBG("Sent %d bytes", ret);

done:
		k_yield();
	}

	#if CONFIG_NET_UDP
		udp6_stop();
	#elif CONFIG_NET_TCP
		tcp6_stop();
	#endif
}

int net_start(struct k_pipe *p2n, struct k_pipe *n2p)
{
	int ret;
	LOG_DBG("NET: Start");

	proto2net = p2n;
	net2proto = n2p;
	connected = false;


	/* Load and set OpenThread credentials from settings */
	#if CONFIG_SETTINGS_OT
		ret = ot_config_init(&ot_disconn);
		if (ret) {
			LOG_ERR("Failed to init OT handler. "
				"Aborting net thread");
			return ret;
		}

		ret = ot_config_load();
		if (ret) {
			LOG_ERR("Failed to load OT credentials. "
				"Aborting net thread");
			return ret;
		}

		ret = ot_config_stop();
		if (ret) {
			LOG_ERR("Failed to stop OT. "
				"Aborting net thread");
			return ret;
		}

		ret = ot_config_set();
		if (ret) {
			LOG_ERR("Failed to set OT credentials. "
				"Aborting net thread");
			return ret;
		}

		ret = ot_config_start();
		if (ret) {
			LOG_ERR("Failed to start OT. "
				"Aborting net thread");
			return ret;
		}
	#endif

	k_thread_create(&rx_thread_data, rx_stack,
			K_THREAD_STACK_SIZEOF(rx_stack),
			(k_thread_entry_t) net_thread,
			NULL, NULL, NULL, K_PRIO_PREEMPT(15), 0, K_NO_WAIT);

	return 0;
}

void net_stop(void)
{
	LOG_DBG("NET: Stop");
}
