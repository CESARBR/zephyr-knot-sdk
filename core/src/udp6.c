/* udp6.c - KNoT Application Client */

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
#include <net/socket.h>

#include "net.h"
#include "udp6.h"
#include "storage.h"

LOG_MODULE_DECLARE(knot, CONFIG_KNOT_LOG_LEVEL);

#define PEER_IPV6_PORT 8886
#define IPV6_LEN	40

static char peer_ipv6[IPV6_LEN];
static net_recv_t recv_cb;
static net_close_t close_cb;
static int socket;

static int start_udp_proto(const struct sockaddr *addr, socklen_t addrlen)
{
	int rc;
	int err;

	socket = zsock_socket(addr->sa_family, SOCK_DGRAM, IPPROTO_UDP);

	if (socket < 0) {
		err = errno;
		LOG_ERR("Failed to create UDP socket: %d", err);
		return -err;
	}

	/* Call connect so we can use send and recv. */
	rc = zsock_connect(socket, addr, addrlen);
	if (rc < 0) {
		err = errno;
		LOG_ERR("Cannot connect to UDP remote: %d", err);
		return -err;
	}

	return rc;
}

int udp6_start(net_recv_t recv, net_close_t close)
{
	int rc;
	struct sockaddr_in6 addr6;

	memset(&addr6, 0, sizeof(addr6));
	addr6.sin6_family = AF_INET6;
	addr6.sin6_port = htons(8886);
	rc = zsock_inet_pton(AF_INET6, peer_ipv6,
			&addr6.sin6_addr);
	if (rc <= 0)
		return -EFAULT;

	rc = start_udp_proto((struct sockaddr *)&addr6, sizeof(addr6));

	/* Close socket if connection fail */
	if (rc) {
		udp6_stop();
		return rc;
	}

	/* Successful start */
	LOG_DBG("UDP connected");

	recv_cb = recv;
	close_cb = close;

	return rc;
}

void udp6_stop(void)
{
	if (socket >= 0) {
		LOG_DBG("Closing socket %d", socket);
		(void)zsock_close(socket);
		socket = -1;
	}

	/* Call connection closed callback */
	if (close_cb != NULL) {
		LOG_WRN("Calling close cb");
		close_cb();
	}
}

int udp6_init(void)
{
	int rc;

	/* Reset callbacks */
	recv_cb = NULL;
	close_cb = NULL;

	LOG_DBG("Initializing UDP handler");

	rc = storage_read(STORAGE_PEER_IPV6, peer_ipv6, sizeof(peer_ipv6));
	if (rc <= 0) {
		LOG_ERR("Failed to read Peer's IPv6");
		return -ENOENT;
	}

	return 0;
}
