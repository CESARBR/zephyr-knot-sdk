/*
 * Copyright (c) 2018, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include "knot_protocol.h"
#include "kaio.h"

#define KNOT_THING_DATA_MAX    3

static struct aio {
       /* KNoT identifier */
       u8_t                    id;

       /* Schema values */
       u8_t                    value_type;
       u8_t                    unit;
       uint16_t                type_id;
       const char              *name;

       /* Data values */
       bool                    new_value;
       knot_value_type         last_value;
       u8_t                    *last_value_raw;
       u8_t                    raw_length;

       /* Config values */
       knot_config             config;

       /* Time values */
       u32_t                   last_timeout;
} aio[KNOT_THING_DATA_MAX];
