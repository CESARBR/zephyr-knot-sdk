/*
 * Copyright (c) 2019, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if !CONFIG_BOARD_QEMU_X86
#include <zephyr.h>
#include <settings/settings_ot.h>
#include <logging/log.h>
#include <flash.h>

#include "storage.h"
#include "clear.h"

LOG_MODULE_REGISTER(knot_clear, CONFIG_KNOT_LOG_LEVEL);

int clear_ot_nvs(void)
{
	struct device *flash_dev;
	int rc;

	flash_dev = device_get_binding(DT_FLASH_DEV_NAME);
	if (!flash_dev) {
		LOG_ERR("Flash driver was not found!");
		return -1;
	}

	/* Erase OpenThread flash partition */
	flash_write_protection_set(flash_dev, false);
	rc = flash_erase(flash_dev,
			 FLASH_AREA_OT_STORAGE_OFFSET,
			 FLASH_AREA_OT_STORAGE_SIZE);
	if (rc)
		LOG_ERR("Failed to clear OpenThread's NVS");

	flash_write_protection_set(flash_dev, true);

	return rc;
}

static int clear_settings(void)
{
	struct device *flash_dev;
	int rc;

	flash_dev = device_get_binding(DT_FLASH_DEV_NAME);
	if (!flash_dev) {
		LOG_ERR("Flash driver was not found!");
		return -1;
	}

	/* Erase Storage flash partition */
	flash_write_protection_set(flash_dev, false);
	rc = flash_erase(flash_dev,
			 FLASH_AREA_STORAGE_OFFSET,
			 FLASH_AREA_STORAGE_SIZE);
	if (rc)
		LOG_ERR("Failed to clear Storage flash partition");

	flash_write_protection_set(flash_dev, true);

	return rc;
}

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

	rc = clear_ot_nvs();
	if (rc)
		ret = -1;

	rc = clear_settings();
	if (rc)
		ret = -1;

	return ret;
}
#endif
