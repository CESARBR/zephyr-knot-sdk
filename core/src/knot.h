/*
 * Copyright (c) 2018, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Callback functions that can be called so the data item value can be updated
 * before/after being read/written.
 */
#define KNOT_CALLBACK_SUCCESS	 0  // Able to read/write value
#define KNOT_CALLBACK_FAIL	-1  // Failed to read/write value

typedef int (*knot_callback_t) (int id);

/*
 * Similar to Arduino:
 * setup() is called once and loop() is always called at idle state.
 * Sensors and actuators should be registered at setup() function
 * definition and loop() must NOT be blocking.
 *
 * setup() and loop() must be defined at user app context.
 */
void setup(void);
void loop(void);

/* Set knot to track and update data items */
int knot_data_register(u8_t id, const char *name,
		       u16_t type_id, u8_t value_type, u8_t unit,
		       void *target, size_t target_len,
		       knot_callback_t write_cb, knot_callback_t read_cb);

/*
 * This fuction configures which events should send proxy value to cloud
 *
 * @param id Sensor ID.
 * @param ... Optional list of event flags.
 *
 * This function must end with NULL
 */
bool knot_data_config(u8_t id, ...);
