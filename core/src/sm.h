/* sm.c - KNoT Application Client */

/*
 * Copyright (c) 2018, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* KNoT State Machine */

int sm_start(void);
void sm_stop(void);

int sm_run(const unsigned char *ipdu, size_t ilen,
	   unsigned char *opdu, size_t olen);
