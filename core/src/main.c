/* main.c - KNoT Application Client */

/*
 * Copyright (c) 2018, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The knot client application is acting as a client that is run in Zephyr OS,
 * The client sends sensor data encapsulated using KNoT protocol.
 */

#include <zephyr.h>
#include <net/net_core.h>
#include <net/net_app.h>
#include <logging/log.h>

#include "proto.h"
#include "net.h"

LOG_MODULE_REGISTER(knot, LOG_LEVEL_DBG);
K_PIPE_DEFINE(p2n_pipe, 128, 4);
K_PIPE_DEFINE(n2p_pipe, 128, 4);
static struct k_sem quit_lock;

void main(void)
{
	LOG_DBG("*** Welcome to KNoT! %s\n", CONFIG_ARCH);

	k_sem_init(&quit_lock, 0, UINT_MAX);

	/* Guarantees NET and PROTO threads will be created */
	k_sched_lock();

	/*
	 * KNoT state thread: manage device registration, detects
	 * sensor data changes acting like a proxy forwarding data
	 * from sensors to network layer (and oposite). Proto is
	 * consumer of ipdu fifo and producer of opdu.
	 */
	if (proto_start(&p2n_pipe, &n2p_pipe) < 0)
		return;

	/*
	 * Network thread: manage traffic from TCP or non-ip wireless
	 * technologies. Each technology(thread) is responsible for
	 * managing incoming and outgoing data. Net is consumer of
	 * opdu fifo and producer of ipdu.
	 */
	if (net_start(&p2n_pipe, &n2p_pipe) < 0)
		return;

	/* Allows NET and PROTO thread scheduling */
	k_sched_unlock();

	k_sem_take(&quit_lock, K_FOREVER);

	net_stop();
	proto_stop();
}
