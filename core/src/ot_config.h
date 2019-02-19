/* ot_config.h - OpenThread Credentials Handler */

/*
 * Copyright (c) 2019, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int ot_config_load(void);

#if defined(CONFIG_NET_L2_OPENTHREAD)
int ot_config_set(void);
#endif
