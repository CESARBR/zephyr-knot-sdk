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

#include "tcp.h"

#define PEER_IPV4_PORT 9994
#define PEER_IPV6_PORT 9996

#define BUF_TIMEOUT	K_MSEC(100)
#define WAIT_TIME	K_SECONDS(10)
#define CONNECT_TIME	K_SECONDS(10)

static struct net_app_ctx tcp6;
static struct net_app_ctx tcp4;

static void tcp_received(struct net_app_ctx *ctx,
			 struct net_pkt *ipkt,
			 int status,
			 void *user_data)
{
	u16_t ilen;

	ilen = net_pkt_appdatalen(ipkt);

	NET_DBG("TCP(%p) Received %u bytes", ctx, ilen);

	net_pkt_unref(ipkt);
}

static void tcp_connected(struct net_app_ctx *ctx,
			  int status,
			  void *user_data)
{
	struct net_pkt *opkt;
	char buffer[] = "Hello World";
	int olen;
	int ret;

	opkt = net_app_get_net_pkt(ctx, AF_UNSPEC, BUF_TIMEOUT);
	if (!opkt) {

		NET_DBG("TCP(%p) Can't create TCP packet!", ctx);
		return;
	}

	olen = net_pkt_append(opkt, sizeof(buffer), buffer, K_FOREVER);

	ret = net_app_send_pkt(ctx, opkt, NULL, 0, K_FOREVER, NULL);
	if (ret < 0) {
		NET_DBG("TCP(%p) cam not send %p", ctx, opkt);
		net_pkt_unref(opkt);
	}
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

int tcp_start(void)
{
	int ret = 0;

	NET_DBG("Starting TCP IPv4(%p) IPv6(%p) ...", &tcp4, &tcp6);

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
