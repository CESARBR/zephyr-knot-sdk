/* peripheral.h - Peripherals handler */

/*
 * Copyright (c) 2019, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Helper defines and function to ease peripherals handling.
 * Built based on nrf52840_pca10056 board.
 */

int peripheral_init(void);
int peripheral_btn_status(void);
void peripheral_toggle_led(void);

enum {
	PERIPHERAL_BTN_PRESSED = 0,
	PERIPHERAL_BTN_NOT_PRESSED = 1,
	PERIPHERAL_BTN_ERROR = -1
};
