/* tcp6.c - KNoT Application Client */

/*
 * Copyright (c) 2018, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <logging/log.h>
#include <errno.h>
#include <stdio.h>

#include <net/net_pkt.h>
#include <net/net_core.h>
#include <net/net_context.h>

#include "net.h"
#include "tcp6.h"
#include "storage.h"

LOG_MODULE_DECLARE(knot, CONFIG_KNOT_LOG_LEVEL);

#define PEER_IPV6_PORT 8886
#define IPV6_LEN	40

#define BUF_TIMEOUT	K_MSEC(100)
#define WAIT_TIME	K_SECONDS(10)
#define CONNECT_TIME	K_SECONDS(10)

static char peer_ipv6[IPV6_LEN];
static net_recv_t recv;

static int connect_tcp6(const char *peer, int port, void *user_data)
{
	LOG_DBG("TCP Connecting to %s %d ...", peer, port);

	// TODO: Not connecting TCP

	return 0;
}

int tcp6_start(net_recv_t recv_cb, net_close_t close_cb)
{
	int ret = -EPERM;

	LOG_DBG("Starting TCP IPv6 ...");

	recv = recv_cb;

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		ret = connect_tcp6(peer_ipv6, PEER_IPV6_PORT, close_cb);
		if (ret < 0)
			LOG_ERR("Cannot init IPv6 TCP client (%d)", ret);
	}

	return ret;
}

void tcp6_stop(void)
{
	// TODO: Not stopping connection
}

int tcp6_send(const u8_t *opdu, size_t olen)
{
	// TODO: Not sending messages
	return 0;
}

int tcp6_init(void)
{
	int rc;

	LOG_DBG("Initializing TCP handler");

	rc = storage_read(STORAGE_PEER_IPV6, peer_ipv6, sizeof(peer_ipv6));
	if (rc <= 0) {
		LOG_ERR("Failed to read Peer's IPv6");
		return -ENOENT;
	}

	return 0;
}
