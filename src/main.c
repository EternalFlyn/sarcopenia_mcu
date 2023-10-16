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
#include "battery.h"

#define ADC_DEVICE_NAME DT_LABEL(DT_INST(0, nordic_nrf_saadc))

#define THREAD_STACK_SIZE 500
#define THREAD_PRIORITY 5

const struct device *adc_dev;

void main(void) {
	printk("Hello World! %s\n", CONFIG_BOARD);

	// ADC setup
    adc_dev = device_get_binding(ADC_DEVICE_NAME);
	if (adc_init(adc_dev)) return;
	if (battery_init(adc_dev)) return;
	if (bt_init()) return;

	if (start_battery_sample()) return;
	if (start_adc_sample()) return;
}