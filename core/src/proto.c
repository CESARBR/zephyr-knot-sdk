//* proto.c - KNoT Application Client */

/*
 * Copyright (c) 2018, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The knot client application is acting as a client that is run in Zephyr OS,
 * The client sends sensor data encapsulated using KNoT protocol.
 */

#define SYS_LOG_DOMAIN "knot"
#define NET_SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_LOG_ENABLED 1

#include <zephyr.h>
#include <net/net_core.h>
#include <net/buf.h>

#include "knot_app.h"
#include "proto.h"

static struct k_thread rx_thread_data;
static K_THREAD_STACK_DEFINE(rx_stack, 1024);
static struct k_pipe *proto2net;
static struct k_pipe *net2proto;

static void proto_thread(void)
{
	/* Considering KNOT Max MTU 32 */
	unsigned char ipdu[32];
	unsigned char opdu[32];
	size_t olen;
	size_t ilen;
	int ret;

	/* Calling KNoT app: setup() */
	setup();

	while (1) {
		/* Calling KNoT app: loop() */
		loop();

		ilen = 0;
		memset(&ipdu, 0, sizeof(ipdu));
		/* Reading data from NET thread */
		ret = k_pipe_get(net2proto, ipdu, sizeof(ipdu),
				 &ilen, 0U, K_NO_WAIT);

		olen = 0;

		/* Sending data to NET thread */
		k_pipe_put(proto2net, opdu, olen, &olen, olen, K_NO_WAIT);

		k_sleep(1000);
	}
}

int proto_start(struct k_pipe *p2n, struct k_pipe *n2p)
{
	NET_DBG("PROTO: Start");

	proto2net = p2n;
	net2proto = n2p;
	k_thread_create(&rx_thread_data, rx_stack,
			K_THREAD_STACK_SIZEOF(rx_stack),
			(k_thread_entry_t) proto_thread,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);

	return 0;
}

void proto_stop(void)
{
	NET_DBG("PROTO: Stop");
}

