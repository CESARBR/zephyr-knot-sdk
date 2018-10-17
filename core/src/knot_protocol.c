/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include <stdint.h>
#include <errno.h>

#include "knot_types.h"
#include "knot_protocol.h"

/* Bitmask to identify used unit */
#define UNIT_MASK(unit)		(1 << (unit))

#define UNIT_VOLTAGE_MASK	(UNIT_MASK(KNOT_UNIT_VOLTAGE_V) |		\
					UNIT_MASK(KNOT_UNIT_VOLTAGE_MV) |	\
					UNIT_MASK(KNOT_UNIT_VOLTAGE_KV))

#define UNIT_CURRENT_MASK	(UNIT_MASK(KNOT_UNIT_CURRENT_A) |		\
					UNIT_MASK(KNOT_UNIT_CURRENT_MA))

#define UNIT_RESISTENCE_MASK	(UNIT_MASK(KNOT_UNIT_RESISTENCE_OHM))

#define UNIT_POWER_MASK		(UNIT_MASK(KNOT_UNIT_POWER_W) |			\
					UNIT_MASK( KNOT_UNIT_POWER_KW) |	\
					UNIT_MASK(KNOT_UNIT_POWER_MW))

#define UNIT_TEMP_MASK		(UNIT_MASK(KNOT_UNIT_TEMPERATURE_C) |		\
					UNIT_MASK(KNOT_UNIT_TEMPERATURE_F) |	\
					UNIT_MASK(KNOT_UNIT_TEMPERATURE_K))

#define UNIT_HUM_MASK		(UNIT_MASK(KNOT_UNIT_RELATIVE_HUMIDITY))

#define UNIT_LUM_MASK		(UNIT_MASK(KNOT_UNIT_LUMINOSITY_LM) |		\
					UNIT_MASK(KNOT_UNIT_LUMINOSITY_CD) |	\
					UNIT_MASK(KNOT_UNIT_LUMINOSITY_LX))

#define UNIT_TIME_MASK		(UNIT_MASK(KNOT_UNIT_TIME_S) |			\
					UNIT_MASK(KNOT_UNIT_TIME_MS) |		\
					UNIT_MASK(KNOT_UNIT_TIME_US))

#define UNIT_MASS_MASK		(UNIT_MASK(KNOT_UNIT_MASS_KG) |			\
					UNIT_MASK(KNOT_UNIT_MASS_G) |		\
					UNIT_MASK(KNOT_UNIT_MASS_LB) |		\
					UNIT_MASK(KNOT_UNIT_MASS_OZ))

#define UNIT_PRESSURE_MASK	(UNIT_MASK(KNOT_UNIT_PRESSURE_PA) |		\
					UNIT_MASK(KNOT_UNIT_PRESSURE_PSI) |	\
					UNIT_MASK(KNOT_UNIT_PRESSURE_BAR))

#define UNIT_DISTANCE_MASK	(UNIT_MASK(KNOT_UNIT_DISTANCE_M) |		\
					 UNIT_MASK(KNOT_UNIT_DISTANCE_CM) |	\
					 UNIT_MASK(KNOT_UNIT_DISTANCE_MI) |	\
					 UNIT_MASK(KNOT_UNIT_DISTANCE_IN))

#define UNIT_ANGLE_MASK		(UNIT_MASK(KNOT_UNIT_ANGLE_RAD) |		\
					 UNIT_MASK(KNOT_UNIT_ANGLE_DEGREE))

#define UNIT_VOL_MASK		(UNIT_MASK(KNOT_UNIT_VOLUME_L) |		\
					UNIT_MASK(KNOT_UNIT_VOLUME_ML) |	\
					UNIT_MASK(KNOT_UNIT_VOLUME_FLOZ) |	\
					UNIT_MASK(KNOT_UNIT_VOLUME_GAL))

#define UNIT_AREA_MASK		(UNIT_MASK(KNOT_UNIT_AREA_M2) |			\
					UNIT_MASK(KNOT_UNIT_AREA_HA) |		\
					UNIT_MASK(KNOT_UNIT_AREA_AC))

#define UNIT_RAIN_MASK		(UNIT_MASK(KNOT_UNIT_RAIN_MM))

#define UNIT_DENSITY_MASK	(UNIT_MASK(KNOT_UNIT_DENSITY_KGM3))

#define UNIT_LAT_MASK		(UNIT_MASK(KNOT_UNIT_LATITUDE_DEGREE))

#define UNIT_LONG_MASK		(UNIT_MASK(KNOT_UNIT_LONGITUDE_DEGREE))

#define UNIT_SPEED_MASK		(UNIT_MASK(KNOT_UNIT_SPEED_MS) |		\
					 UNIT_MASK(KNOT_UNIT_SPEED_CMS) |	\
					 UNIT_MASK(KNOT_UNIT_SPEED_KMH) |	\
					 UNIT_MASK(KNOT_UNIT_SPEED_MIH))

#define UNIT_VOLFLOW_MASK	(UNIT_MASK(KNOT_UNIT_VOLUMEFLOW_M3S) |		\
					UNIT_MASK(KNOT_UNIT_VOLUMEFLOW_SCMM) |	\
					UNIT_MASK(KNOT_UNIT_VOLUMEFLOW_LS) |	\
					UNIT_MASK(KNOT_UNIT_VOLUMEFLOW_LM) |	\
					UNIT_MASK(KNOT_UNIT_VOLUMEFLOW_FT3S) |	\
					UNIT_MASK(KNOT_UNIT_VOLUMEFLOW_GALM))

#define UNIT_ENERGY_MASK	(UNIT_MASK(KNOT_UNIT_ENERGY_J) |		\
					UNIT_MASK(KNOT_UNIT_ENERGY_NM) |	\
					UNIT_MASK(KNOT_UNIT_ENERGY_WH) |	\
					UNIT_MASK(KNOT_UNIT_ENERGY_KWH) |	\
					UNIT_MASK(KNOT_UNIT_ENERGY_CAL) |	\
					UNIT_MASK(KNOT_UNIT_ENERGY_KCAL))

#define TYPE_ID_MASK(type)	(type & 0x000F)

static const struct schema {
	uint8_t		value_type;
	uint32_t	unit_mask;
} basic_types[]  = {
	{KNOT_VALUE_TYPE_RAW, 0xFFFF			},
	{KNOT_VALUE_TYPE_INT, UNIT_VOLTAGE_MASK		},
	{KNOT_VALUE_TYPE_INT, UNIT_CURRENT_MASK		},
	{KNOT_VALUE_TYPE_INT, UNIT_RESISTENCE_MASK	},
	{KNOT_VALUE_TYPE_INT, UNIT_POWER_MASK		},
	{KNOT_VALUE_TYPE_INT, UNIT_TEMP_MASK		},
	{KNOT_VALUE_TYPE_INT, UNIT_HUM_MASK		},
	{KNOT_VALUE_TYPE_INT, UNIT_LUM_MASK		},
	{KNOT_VALUE_TYPE_INT, UNIT_TIME_MASK		},
	{KNOT_VALUE_TYPE_INT, UNIT_MASS_MASK		},
	{KNOT_VALUE_TYPE_INT, UNIT_PRESSURE_MASK	},
	{KNOT_VALUE_TYPE_INT, UNIT_DISTANCE_MASK	},
	{KNOT_VALUE_TYPE_FLOAT, UNIT_ANGLE_MASK		},
	{KNOT_VALUE_TYPE_FLOAT, UNIT_VOL_MASK		},
	{KNOT_VALUE_TYPE_FLOAT, UNIT_AREA_MASK		},
	{KNOT_VALUE_TYPE_FLOAT, UNIT_RAIN_MASK		},
	{KNOT_VALUE_TYPE_FLOAT, UNIT_DENSITY_MASK	},
	{KNOT_VALUE_TYPE_FLOAT, UNIT_LAT_MASK		},
	{KNOT_VALUE_TYPE_FLOAT, UNIT_LONG_MASK		},
	{KNOT_VALUE_TYPE_INT, UNIT_SPEED_MASK		},
	{KNOT_VALUE_TYPE_FLOAT, UNIT_VOLFLOW_MASK	},
	{KNOT_VALUE_TYPE_INT, UNIT_ENERGY_MASK		}
};

static const uint8_t logic_types[]  = {
	KNOT_VALUE_TYPE_BOOL,			// KNOT_TYPE_ID_PRESENCE
	KNOT_VALUE_TYPE_BOOL,			// KNOT_TYPE_ID_SWITCH
	KNOT_VALUE_TYPE_RAW			// KNOT_TYPE_ID_COMMAND
};

static const uint8_t generic_types[] = {
	KNOT_VALUE_TYPE_INT			// KNOT_TYPE_ID_ANALOG
};

int knot_value_type_is_valid(uint8_t type)
{
	if (type >= KNOT_VALUE_TYPE_MIN || type < KNOT_VALUE_TYPE_MAX)
		return KNOT_SUCCESS;

	return KNOT_INVALID_DATA;
}

int knot_type_id_is_basic(uint16_t type_id)
{
	if (type_id < KNOT_TYPE_ID_BASIC_MAX)
		return KNOT_SUCCESS;

	return KNOT_INVALID_DATA;
}

int knot_type_id_is_logic(uint16_t type_id)
{
	if (type_id >= KNOT_TYPE_ID_LOGIC_MIN &&
	    type_id < KNOT_TYPE_ID_LOGIC_MAX)
		return KNOT_SUCCESS;

	return KNOT_INVALID_DATA;
}

int knot_type_id_is_generic(uint16_t type_id)
{
	if (type_id >= KNOT_TYPE_ID_GENERIC_MIN &&
	    type_id < KNOT_TYPE_ID_GENERIC_MAX)
		return KNOT_SUCCESS;

	return KNOT_INVALID_DATA;
}

int knot_schema_is_valid(uint16_t type_id, uint8_t value_type, uint8_t unit)
{
	/* int/float/bool/raw ? */
	if (knot_value_type_is_valid(value_type) == KNOT_SUCCESS) {

		/* Verify basic type IDs */
		if (knot_type_id_is_basic(type_id) == KNOT_SUCCESS) {
			if (basic_types[type_id].value_type == value_type &&
			    basic_types[type_id].unit_mask & UNIT_MASK(unit))
				return KNOT_SUCCESS;

		/* Verify logic type IDs */
		} else if (knot_type_id_is_logic(type_id) == KNOT_SUCCESS) {
			if (logic_types[TYPE_ID_MASK(type_id)] == value_type)
				return KNOT_SUCCESS;
		} else if (knot_type_id_is_generic(type_id) == KNOT_SUCCESS) {
			if (generic_types[TYPE_ID_MASK(type_id)] == value_type)
				return KNOT_SUCCESS;
		}
	}

	return KNOT_INVALID_SCHEMA;
}

int knot_config_is_valid(uint8_t event_flags, uint16_t time_sec,
		knot_value_type *lower_limit, knot_value_type *upper_limit)
{

	int diff;

	/* Check if event_flags are valid */
	if ((event_flags | KNOT_EVT_FLAG_NONE) &&
		!(event_flags & KNOT_EVENT_FLAG_MAX))
		/*
		 * TODO: DEFINE KNOT_CONFIG ERRORS IN PROTOCOL
		 * KNOT_INVALID_CONFIG in new protocol
		 */
		return KNOT_ERROR_UNKNOWN;

	/* Check consistency of time_sec */
	if (event_flags & KNOT_EVT_FLAG_TIME) {
		if (time_sec == 0)
			/*
			 * TODO: DEFINE KNOT_CONFIG ERRORS IN PROTOCOL
			 * KNOT_INVALID_CONFIG in new protocol
			 */
			return KNOT_ERROR_UNKNOWN;
	} else {
		if (time_sec > 0)
			/*
			 * TODO: DEFINE KNOT_CONFIG ERRORS IN PROTOCOL
			 * KNOT_INVALID_CONFIG in new protocol
			 */
			return KNOT_ERROR_UNKNOWN;
	}


	/* Check consistency of limits */
	if (event_flags & (KNOT_EVT_FLAG_LOWER_THRESHOLD |
			KNOT_EVT_FLAG_UPPER_THRESHOLD)) {

		diff = upper_limit->val_f -
			lower_limit->val_f;

		if (diff < 0)
			/*
			 * TODO: DEFINE KNOT_CONFIG ERRORS IN PROTOCOL
			 * KNOT_INVALID_CONFIG in new protocol
			 */
			return KNOT_ERROR_UNKNOWN;

	}

	return KNOT_SUCCESS;

}
