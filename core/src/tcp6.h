/* tcp6.h - KNoT Application Client */

/*
 * Copyright (c) 2018, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int tcp6_start(net_recv_t recv, net_close_t close);
void tcp6_stop(void);

int tcp6_send(const u8_t *buf, size_t len);

int tcp6_event_poll(void);
int tcp6_init(void);
