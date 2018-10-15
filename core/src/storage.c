/* storage_mock.c - KNoT Thing Storage MOCK handler */

/*
 * Copyright (c) 2018, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This file works as a mock-up for the storage on NVM. It is intended to be
 * used for testing only.
 * To use with the nRF52840 board, use the storage.nrf52840 file.
 */

#include <zephyr.h>
#include <net/net_core.h>
#include <string.h>

#include "storage.h"
#include "knot_protocol.h"

#define SYS_LOG_DOMAIN "knot"
#define NET_SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_LOG_ENABLED 1

#if CONFIG_BOARD_NRF52840_PCA10056
	#include "storage.nrf52840"
#elif CONFIG_BOARD_QEMU_X86
	#include "storage.qemu"
#endif
