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
static struct zsock_pollfd fds;
static net_recv_t recv_cb;
static net_close_t close_cb;
static int socket;

static int receive(void)
{
	int rc;
	int err;
	size_t len = 0;
	char buf[128];

	/* Read socket until no data left */
	while (true) {
		rc = zsock_recv(socket, buf + len, sizeof(buf) - len,
				ZSOCK_MSG_DONTWAIT);

		/* Update len and check for more data */
		if (rc > 0) {
			len += rc;
			continue;
		}
		/* Save errno to avoid changes by interruption */
		err = errno;

		if (rc == 0) {
			if (len != 0) {
				/* Read finished */
				LOG_WRN("Nothing left to read");
				break;
			}

			LOG_ERR("No message found (err %d)", err);
			return -EIO;
		}

		/* rc < 0 */
		/* Finish if EAGAIN and EWOULDBLOCK */
		if (err == EAGAIN || err == EWOULDBLOCK)
			break;

		LOG_ERR("Socket read err: %d", rc);

		return -err;
	}

	return recv_cb(buf, len);
}

static void set_fds(void)
{
	fds.fd = socket;
	fds.events = ZSOCK_POLLIN;
}

int udp6_send(const u8_t *buf, size_t len)
{
	ssize_t out_len;

	LOG_DBG("Sending msg");
	while (len) {
		out_len = zsock_send(socket, buf, len, 0);

		if (out_len < 0)
			return -errno;

		buf = buf + out_len;
		len -= out_len;
	}

	return 0;
}

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
	set_fds();
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

int udp6_event_poll(void)
{
	int ret, rc;

	/*
	 * Check if any event occurred on fds poll.
	 */
	ret = zsock_poll(&fds, 1, 0);
	if (ret < 0)
		LOG_ERR("Error in poll: %d", ret);

	if (fds.revents & ZSOCK_POLLIN) {
		LOG_DBG("Msg received");
		rc = receive();
		if (rc) {
			LOG_ERR("Read failure: %d", rc);
			k_sleep(K_FOREVER);
		}
	}

	return ret;
}