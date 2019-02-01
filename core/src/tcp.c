/* tcp.c - KNoT Application Client */

/*
 * Copyright (c) 2018, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_DOMAIN "knot"
#define NET_SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_LOG_ENABLED 1

#include <zephyr.h>
#include <errno.h>
#include <stdio.h>

#include <net/net_pkt.h>
#include <net/net_core.h>
#include <net/net_context.h>

#include <net/net_app.h>

#include "net.h"
#include "tcp.h"

#define PEER_IPV4_PORT 9994
#define PEER_IPV6_PORT 9996

#define BUF_TIMEOUT	K_MSEC(100)
#define WAIT_TIME	K_SECONDS(10)
#define CONNECT_TIME	K_SECONDS(10)

static struct net_app_ctx tcp6;
static struct net_app_ctx tcp4;
static net_recv_t recv;

static void tcp_received(struct net_app_ctx *ctx,
			 struct net_pkt *ipkt,
			 int status,
			 void *user_data)
{
	struct net_buf *netbuf;
	size_t ilen;
	size_t dlen;
	size_t hlen;

	ilen = net_pkt_get_len(ipkt);
	dlen = net_pkt_appdatalen(ipkt);

	if (ilen != dlen) {
		netbuf = ipkt->frags;
		hlen = net_pkt_appdata(ipkt) - netbuf->data;
		net_buf_pull(netbuf, hlen);
		recv(netbuf);
	}

	net_pkt_unref(ipkt);
}

static void tcp_connected(struct net_app_ctx *ctx,
			  int status,
			  void *user_data)
{
	NET_DBG("TCP(%p) connected", ctx);
}

static int connect_tcp(struct net_app_ctx *ctx, const char *peer, int port)
{
	int ret;

	NET_DBG("TCP(%p) Connecting to %s %d ...", ctx, peer, port);

	ret = net_app_init_tcp_client(ctx, NULL, NULL, peer, port,
				      WAIT_TIME, NULL);
	if (ret < 0) {
		NET_ERR("Cannot init %s TCP client (%d)", peer, ret);
		goto fail;
	}

	ret = net_app_set_cb(ctx, tcp_connected, tcp_received, NULL, NULL);
	if (ret < 0) {
		NET_ERR("Cannot set callbacks (%d)", ret);
		goto fail;
	}

	ret = net_app_connect(ctx, CONNECT_TIME);
	if (ret < 0) {
		NET_ERR("Cannot connect TCP (%d)", ret);
		goto fail;
	}

fail:
	return ret;
}

int tcp_start(net_recv_t recv_cb)
{
	int ret = 0;

	NET_DBG("Starting TCP IPv4(%p) IPv6(%p) ...", &tcp4, &tcp6);

	recv = recv_cb;

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		ret = connect_tcp(&tcp6, CONFIG_NET_APP_PEER_IPV6_ADDR,
				  PEER_IPV6_PORT);
		if (ret < 0)
			NET_ERR("Cannot init IPv6 TCP client (%d)", ret);
	}

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		ret = connect_tcp(&tcp4, CONFIG_NET_APP_PEER_IPV4_ADDR,
				  PEER_IPV4_PORT);
		if (ret < 0)
			NET_ERR("Cannot init IPv4 TCP client (%d)", ret);
	}

	return ret;
}

void tcp_stop(void)
{
	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		net_app_close(&tcp6);
		net_app_release(&tcp6);
	}

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		net_app_close(&tcp4);
		net_app_release(&tcp4);
	}
}

int tcp_send(const u8_t *opdu, size_t olen)
{
	struct net_pkt *opkt;
	int ret;

	opkt = net_app_get_net_pkt(&tcp4, AF_UNSPEC, BUF_TIMEOUT);
	if (!opkt)
		return -EIO;

	olen = net_pkt_append(opkt, olen, opdu, K_FOREVER);

	ret = net_app_send_pkt(&tcp4, opkt, NULL, 0, K_FOREVER, NULL);
	if (ret < 0)
		net_pkt_unref(opkt);

	return 0;
}
