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

#define SYS_LOG_DOMAIN "knot"
#define NET_SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_LOG_ENABLED 1

#include <zephyr.h>

#include <net/net_core.h>
#include "tcp.h"

void main(void)
{
	NET_DBG("*** Welcome to KNoT! %s\n", CONFIG_ARCH);
	if (IS_ENABLED(CONFIG_NET_TCP))
		tcp_start();

	for (;;)
		k_sleep(K_FOREVER);

	if (IS_ENABLED(CONFIG_NET_TCP))
		tcp_stop();
}
