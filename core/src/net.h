/* net.h - KNoT Application Client */

/*
 * Copyright (c) 2018, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

typedef bool (*net_recv_t) (struct net_buf *netbuf);

int net_start(struct k_pipe *p2n, struct k_pipe *n2p);
void net_stop(void);
