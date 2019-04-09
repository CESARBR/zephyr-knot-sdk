/* udp6.h - KNoT Application Client */

/*
 * Copyright (c) 2018, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int udp6_start(net_recv_t recv, net_close_t close);
void udp6_stop(void);

int udp6_send(const u8_t *buf, size_t len);

int udp6_init(void);
