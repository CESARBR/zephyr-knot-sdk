/* i2c-accelerometer-gyroscope.c - KNoT Application Client */

/*
 * Copyright (c) 2020, CESAR. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This example uses I2C communication protocol to get acceleration and angular
 * velocity from a gy-251 sensor.
 */

#include <zephyr.h>
#include <net/net_core.h>
#include <logging/log.h>
#include <device.h>
#include <i2c.h>

#include "knot.h"
#include <knot/knot_types.h>
#include <knot/knot_protocol.h>

/* I2C slave address */
#define MPU_ADDRESS				0X68

/* MPU6050 internal registers */
#define MPU6050_REG_PWR_MGMT1			0x6B
#define MPU6050_SLEEP_EN			BIT(6)
#define MPU6050_REG_GYRO_CFG			0x1B
#define MPU6050_REG_ACCEL_CFG			0x1C
#define MPU6050_ACCEL_REG_DATA_START		0x3B
#define MPU6050_GYRO_REG_DATA_START		0X43

/* Full Scale Range to +- 2g */
#define FS_SEL					0x00
/* Full Scal Range to 250deg/seg */
#define AFS_SEL					0x00

/* Time interval (in mseg) to get new measures */
#define TIME_INTERVAL				200

#define REGISTERS_NO				6
#define AXIS_NO					3
#define SENSORS_NO				6

#define GYRO_NAME_OFFSET			3

/*
 * Note that measured data has 16 bits, what makes necessary to put together
 * the MSB's with their LSB's.
 * Example:
 * buf[0] will have MSB's for x axis acceleration and buf[1] will have LSB's
 * for x axis acceleration, that is, the raw data of x axis acceleration will
 * be buf16[0] = (buf[0] << 8) | buf(1)
 */
#define MERGE_MSB_LSB(a,b)			( a << 8 | b )

/* Buffer vector to store data from sensor 8 bit registers */
u8_t accel_reg_buffer[REGISTERS_NO], gyro_reg_buffer[REGISTERS_NO];
/* Buffer vector to store measured data from sensor */
s16_t accel_measurements[AXIS_NO], gyro_measurements[AXIS_NO];

/*
 * Each vector position corresponds to an axis,
 * e.g.:
 * accel[0] -> acceleration in x axis
 * accel[1] -> acceleration in y axis
 * accel[2] -> acceleration in z axis
 * the same logic stands for gyroscope measurements
 */
s32_t accel[AXIS_NO], gyro[AXIS_NO];

/*
 * Offset values for sensor calibration
 * To obtain these values, put the sensor in your chosen origin and realise a
 * group of measurements, then take the mean value for each variable.
 * Obs: Note that if you're using a high precision sensibility, the measures
 * will vary slightly everytime.
 */
s32_t accel_offset[AXIS_NO] = { 236,
				-752,
				20996
				};
s32_t gyro_offset[AXIS_NO] = {  75,
				-147,
				23
				};

LOG_MODULE_REGISTER(accelgyro, LOG_LEVEL_DBG);

struct device *i2c_dev;

void setup(void)
{
	int i = 0;
	/* Name list vector for KNoT sensors */
	char *name_list[SENSORS_NO] = { "ACCELX", "ACCELY", "ACCELZ",
				"GYROX", "GYROY", "GYROZ" };

	/* Peripherals control */
	i2c_dev = device_get_binding(DT_NORDIC_NRF_I2C_0_LABEL);
	/* Checks if peripheral was properly configured */
	if (!i2c_dev) {
		printk("I2C: Device driver not found.\n");

		return;
	}
	/* Take sensor out of sleep mode */
	i2c_reg_update_byte(i2c_dev, MPU_ADDRESS,
				MPU6050_REG_PWR_MGMT1, MPU6050_SLEEP_EN, 0);
	/* Set the full scale range for gyroscope */
	i2c_reg_write_byte(i2c_dev, MPU_ADDRESS, MPU6050_REG_GYRO_CFG, FS_SEL);
	/* Set the full scale range for accelerometer */
	i2c_reg_write_byte(i2c_dev, MPU_ADDRESS,
				MPU6050_REG_ACCEL_CFG, AFS_SEL);
	/* KNoT config */
	for (i = 0; i < AXIS_NO; i++) {
		knot_data_register(i, name_list[i], KNOT_TYPE_ID_ACCELERATION,
					KNOT_VALUE_TYPE_INT,
					KNOT_UNIT_ACCELERATION_MS2,
					&accel[i], sizeof(accel[i]),
					NULL, NULL);
		knot_data_config(i, KNOT_EVT_FLAG_CHANGE, NULL);
		knot_data_register(i + GYRO_NAME_OFFSET,
					name_list[i + GYRO_NAME_OFFSET],
					KNOT_TYPE_ID_ANGULAR_VELOCITY,
					KNOT_VALUE_TYPE_FLOAT,
					KNOT_UNIT_ANGULAR_VELOCITY_DEGSEG,
					&gyro[i], sizeof(gyro[i]), NULL, NULL);
		knot_data_config(i + GYRO_NAME_OFFSET,
					KNOT_EVT_FLAG_CHANGE, NULL);
	}
}

void loop(void)
{
	static u64_t last_toggle_time = 0;
	static int i = 0; /* Buffer vector index */
	int k;

	if (i > AXIS_NO) {
		i = 0;
	}

	/* Merge MSB with LSB */
	accel_measurements[i] = MERGE_MSB_LSB(accel_reg_buffer[2 * i],
						accel_reg_buffer[2 * i + 1]);
	gyro_measurements[i] = MERGE_MSB_LSB(gyro_reg_buffer[2 * i],
						gyro_reg_buffer[2 * i + 1]);

	/* Get current time */
	int64_t current_time = k_uptime_get();
	/* Get new measures in a specified time interval */
	if (current_time - last_toggle_time > TIME_INTERVAL) {
		i2c_burst_read(i2c_dev, MPU_ADDRESS,
					 MPU6050_ACCEL_REG_DATA_START,
					 accel_reg_buffer, REGISTERS_NO);
		i2c_burst_read(i2c_dev, MPU_ADDRESS,
					 MPU6050_GYRO_REG_DATA_START,
					 gyro_reg_buffer, REGISTERS_NO);
		for (k = 0; k < AXIS_NO; k++) {
			accel[k] = accel_measurements[k] - accel_offset[k];
			gyro[k] = gyro_measurements[k] - gyro_offset[k];
		}

		last_toggle_time = current_time;
	}

	i++;
}
