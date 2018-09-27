/*
 * Copyright (c) 2018, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Similar to Arduino:
 * setup() is called once and loop() is always called at idle state.
 * Sensors and actuactors should be registered at setup() function
 * definition and loop() must NOT be blooking.
 *
 * setup() and loop() must be defined at user app context.
 */
void setup(void);
void loop(void);

/* Public functions: Used by KNoT apps */
void knot_start(void);
void knot_stop(void);

typedef int (*knot_callback_t) (u8_t id);

s8_t knot_register(u8_t id, const char *name,
		   u16_t type_id, u8_t value_type, u8_t unit,
		   knot_callback_t read_cb, knot_callback_t write_cb);
