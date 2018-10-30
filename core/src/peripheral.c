/* peripheral.c - KNoT Thing Peripheral selector */

/*
 * Copyright (c) 2018, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This file works as a selector for using real peripherals or a mocked up file.
 * To use with the nRF52840 board, use the peripheral.nrf52840 file.
 * To use with qemu, use the peripheral.qemu file.
 */

#include <zephyr.h>
#include <net/net_core.h>

#define SYS_LOG_DOMAIN "knot-peripheral"
#define NET_SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_LOG_ENABLED 1

#if CONFIG_BOARD_NRF52840_PCA10056
	#include "peripheral.nrf52840"
#elif CONFIG_BOARD_QEMU_X86
	#include "peripheral.qemu"
#endif
