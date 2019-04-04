/* net.h - KNoT Application Client */

/*
 * Copyright (c) 2018, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

typedef int (*net_recv_t) (void *buf, size_t len);
typedef void (*net_close_t) (void);

int net_start(struct k_pipe *p2n, struct k_pipe *n2p);
void net_stop(void);
