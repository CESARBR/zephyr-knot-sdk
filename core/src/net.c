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

#define SYS_LOG_DOMAIN "knot"
#define NET_SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_LOG_ENABLED 1

#include <zephyr.h>
#include <net/net_core.h>
#include <net/net_pkt.h>
#include <net/buf.h>

#include "net.h"
#include "tcp.h"

static struct k_thread rx_thread_data;
static K_THREAD_STACK_DEFINE(rx_stack, 1024);
static struct k_pipe *proto2net;
static struct k_pipe *net2proto;

static bool recv_cb(struct net_buf *netbuf)
{
	unsigned char opdu[32];
	struct net_buf *frag;
	size_t olen;
	u16_t offset = 0;

	memset(opdu, 0, sizeof(opdu));
	/* Data comming from network */
	frag = net_frag_read(netbuf, offset, &offset,
			     netbuf->len, (u8_t *) opdu);
        if (!frag && offset == 0xffff)
		return true;

	/* Sending PROTO to NET thread */
	k_pipe_put(net2proto, opdu, netbuf->len, &olen, netbuf->len, K_NO_WAIT);

	return true;
}

static void net_thread(void)
{
	unsigned char ipdu[32];
	size_t ilen;
	int ret;

	ret = tcp_start(recv_cb);
	if (ret < 0) {
		NET_DBG("NET: TCP start failure");
		return;
	}

	memset(ipdu, 0, sizeof(ipdu));

	while (1) {
		ilen = 0;
		/* Reading data from PROTO thread */
		ret = k_pipe_get(proto2net, ipdu, sizeof(ipdu),
				 &ilen, 0U, K_NO_WAIT);
		if (ret == 0 && ilen)
			ret = tcp_send(ipdu, ilen);

		k_sleep(1000);
	}

	tcp_stop();
}

int net_start(struct k_pipe *p2n, struct k_pipe *n2p)
{
	NET_DBG("NET: Start");

	proto2net = p2n;
	net2proto = n2p;

	k_thread_create(&rx_thread_data, rx_stack,
			K_THREAD_STACK_SIZEOF(rx_stack),
			(k_thread_entry_t) net_thread,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);

	return 0;
}

void net_stop(void)
{
	NET_DBG("NET: Stop");
}
