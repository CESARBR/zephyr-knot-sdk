/* sm.c - KNoT Application Client */

/*
 * Copyright (c) 2018, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* KNoT State Machine */

#define SYS_LOG_DOMAIN "knot"
#define NET_SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_LOG_ENABLED 1

#include <zephyr.h>
#include <net/net_core.h>

#include "sm.h"

int sm_start(void)
{
	NET_DBG("SM: State Machine start");

	return 0;
}

void sm_stop(void)
{
	NET_DBG("SM: Stop");
}

int sm_run(const unsigned char *ipdu, size_t ilen,
	   unsigned char *opdu, size_t olen)
{
	return 0;
}
