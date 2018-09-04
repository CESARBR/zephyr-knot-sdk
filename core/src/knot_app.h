/* knot_app.h - KNoT Application Client */

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
 */
void setup(void);
void loop(void);
