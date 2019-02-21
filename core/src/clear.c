/*
 * Copyright (c) 2019, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <settings/settings_ot.h>

#include "storage.h"
#include "clear.h"

int clear_factory(void)
{
	int rc;
	int ret = 0;

	rc = storage_reset();
	if (rc)
		ret = -1;

	rc = settings_ot_reset();
	if (rc)
		ret = -1;

	return ret;
}
