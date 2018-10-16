/* tcp6.h - KNoT Application Client */

/*
 * Copyright (c) 2018, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int tcp6_start(net_recv_t recv_cb, net_close_t close_cb);
void tcp6_stop(void);

int tcp6_send(const u8_t *opdu, size_t olen);
