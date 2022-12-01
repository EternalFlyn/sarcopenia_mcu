/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <sys/printk.h>
#include <devicetree.h>
#include <drivers/gpio.h>

#include "flyn_bluetooth.h"
#include "adc.h"
#include "adc_service.h"
#include "mpu6050.h"
#include "mpu6050_service.h"

#define IMU_SAMPLE_TIME 100

#define ADC_DEVICE_NAME DT_LABEL(DT_INST(0, nordic_nrf_saadc))

#define THREAD_STACK_SIZE 500
#define THREAD_PRIORITY 5

const struct device *adc_dev;
const struct device *i2c_dev;
const struct device *gpio0_dev;
static struct mpu6050_data imu_data;

void imu_sample_event() {
	int err = 0;
	int cnt = 0;
	while (true) {
		err = mpu6050_sample(i2c_dev, &imu_data);
		if (err) {
			printk("Error in i2c sampling: %d\n", err);
			return;
		}
		imu_data_update(imu_data.ax, imu_data.ay, imu_data.az, imu_data.gx, imu_data.gy, imu_data.gz);
		
		accelerometer_notify();
		gyroscope_notify();

		// printk("imu sample at: %d\n", k_cyc_to_us_near32(k_cycle_get_32()));

		// printk("ax: %d, ay: %d, az: %d\n", imu_data.ax, imu_data.ay, imu_data.az);
		// printk("gx: %d, gy: %d, gz: %d\n", imu_data.gx, imu_data.gy, imu_data.gz);
		k_msleep(IMU_SAMPLE_TIME);
	}
}

void main(void) {
	printk("Hello World! %s\n", CONFIG_BOARD);

	// ADC setup
    adc_dev = device_get_binding(ADC_DEVICE_NAME);
	if (adc_init(adc_dev)) return;

	// I2C setup
	i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c1));
	if (mpu6050_init(i2c_dev)) return;

	if (bt_init()) return;

	gpio0_dev =  device_get_binding("GPIO_0");
	gpio_pin_configure(gpio0_dev, 30, GPIO_OUTPUT);
	gpio_pin_set(gpio0_dev, 30, 1);

	adc_sample(adc_dev);
}

K_THREAD_DEFINE(imu_sample_thread, THREAD_STACK_SIZE, imu_sample_event, NULL, NULL, NULL, THREAD_PRIORITY, 0, 0);