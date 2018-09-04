/* tcp.h - KNoT Application Client */

/*
 * Copyright (c) 2018, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int tcp_start(net_recv_t recv_cb);
void tcp_stop(void);

int tcp_send(const u8_t *opdu, size_t olen);
