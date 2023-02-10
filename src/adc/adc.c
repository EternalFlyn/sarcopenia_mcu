
#include <net/socket.h>
#include <stdio.h>
#include <string.h>
#include <drivers/adc.h>
#include <zephyr.h>
#include <hal/nrf_saadc.h>

#include "adc.h"
#include "adc_service.h"

#define ADC_SAMPLE_TIME_US 100*1000 // 10Hz

#define EXTRA_SAMPLING 0
#define CHANNEL_AMOUNT 5
#define BUFFER_SIZE CHANNEL_AMOUNT*(1+EXTRA_SAMPLING)
#define ADC_GAIN ADC_GAIN_1
#define ADC_REFERENCE ADC_REF_VDD_1
#define ADC_ACQUISITION_TIME ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 10)
#define ADC_RESOLUTION 12

//定義channel0~5之AIN腳位
//Sparkfun nrf52840 mini 使用 A0、A1、A7、A6、A5、A4
//nrf52840DK 使用 A1、A2、A4、A5、A6、A7
//nrf5340DK 使用 A0、A1、A2、A3、A4、A5
#define ADC_1ST_CHANNEL_ID 0
#define ADC_1ST_CHANNEL_INPUT NRF_SAADC_INPUT_AIN0
#define ADC_2ND_CHANNEL_ID 1
#define ADC_2ND_CHANNEL_INPUT NRF_SAADC_INPUT_AIN1
#define ADC_3RD_CHANNEL_ID 4
#define ADC_3RD_CHANNEL_INPUT NRF_SAADC_INPUT_AIN7
#define ADC_4TH_CHANNEL_ID 5
#define ADC_4TH_CHANNEL_INPUT NRF_SAADC_INPUT_AIN6
#define ADC_5TH_CHANNEL_ID 2
#define ADC_5TH_CHANNEL_INPUT NRF_SAADC_INPUT_AIN5
#define ADC_6TH_CHANNEL_ID 3
#define ADC_6TH_CHANNEL_INPUT NRF_SAADC_INPUT_AIN4

static int16_t sampling_buffer[BUFFER_SIZE];

static const struct adc_channel_cfg adc1_channel_cfg = {
	.gain = ADC_GAIN,
	.reference = ADC_REFERENCE,
	.acquisition_time = ADC_ACQUISITION_TIME,
	.channel_id = ADC_1ST_CHANNEL_ID,
#if defined(CONFIG_ADC_CONFIGURABLE_INPUTS)
	.input_positive = ADC_1ST_CHANNEL_INPUT,
#endif
};

static const struct adc_channel_cfg adc2_channel_cfg = {
	.gain = ADC_GAIN,
	.reference = ADC_REFERENCE,
	.acquisition_time = ADC_ACQUISITION_TIME,
	.channel_id = ADC_2ND_CHANNEL_ID,
#if defined(CONFIG_ADC_CONFIGURABLE_INPUTS)
	.input_positive = ADC_2ND_CHANNEL_INPUT,
#endif
};

static const struct adc_channel_cfg adc3_channel_cfg = {
	.gain = ADC_GAIN,
	.reference = ADC_REFERENCE,
	.acquisition_time = ADC_ACQUISITION_TIME,
	.channel_id = ADC_3RD_CHANNEL_ID,
#if defined(CONFIG_ADC_CONFIGURABLE_INPUTS)
	.input_positive = ADC_3RD_CHANNEL_INPUT,
#endif
};

static const struct adc_channel_cfg adc4_channel_cfg = {
	.gain = ADC_GAIN,
	.reference = ADC_REFERENCE,
	.acquisition_time = ADC_ACQUISITION_TIME,
	.channel_id = ADC_4TH_CHANNEL_ID,
#if defined(CONFIG_ADC_CONFIGURABLE_INPUTS)
	.input_positive = ADC_4TH_CHANNEL_INPUT,
#endif
};

static const struct adc_channel_cfg adc5_channel_cfg = {
	.gain = ADC_GAIN,
	.reference = ADC_REFERENCE,
	.acquisition_time = ADC_ACQUISITION_TIME,
	.channel_id = ADC_5TH_CHANNEL_ID,
#if defined(CONFIG_ADC_CONFIGURABLE_INPUTS)
	.input_positive = ADC_5TH_CHANNEL_INPUT,
#endif
};

static const struct adc_channel_cfg adc6_channel_cfg = {
	.gain = ADC_GAIN,
	.reference = ADC_REFERENCE,
	.acquisition_time = ADC_ACQUISITION_TIME,
	.channel_id = ADC_6TH_CHANNEL_ID,
#if defined(CONFIG_ADC_CONFIGURABLE_INPUTS)
	.input_positive = ADC_6TH_CHANNEL_INPUT,
#endif
};

static enum adc_action adc_callback(const struct device *dev,
                                    const struct adc_sequence *sequence,
									uint16_t index) {
	printk("adc sample at: %d\n", k_cyc_to_us_near32(k_cycle_get_32()));
	printk("ADC raw value: ");
	for (int i = 0; i < BUFFER_SIZE; i++) {
		printk("%d ", sampling_buffer[i]);
	}
	printk("\n");
	adc_data_update(sampling_buffer[0], sampling_buffer[1], sampling_buffer[2],
	                sampling_buffer[3], sampling_buffer[4], sampling_buffer[5]);

	return ADC_ACTION_REPEAT;
}

const struct adc_sequence_options sequence_opts = {
	.interval_us = ADC_SAMPLE_TIME_US,
	.callback = adc_callback,
	.user_data = NULL,
	.extra_samplings = EXTRA_SAMPLING,
};

int adc_init(const struct device *dev) {
    int ret;

    if (!dev) {
        printk("ADC: Device is not ready.\n");
        return -1;
    }
	ret = adc_channel_setup(dev, &adc1_channel_cfg);
	ret = adc_channel_setup(dev, &adc2_channel_cfg);
	ret = adc_channel_setup(dev, &adc3_channel_cfg);
	ret = adc_channel_setup(dev, &adc4_channel_cfg);
	ret = adc_channel_setup(dev, &adc5_channel_cfg);
	ret = adc_channel_setup(dev, &adc6_channel_cfg);
	if (ret) {
		printk("Error in adc setup: %d\n", ret);
        return ret;
	}

	/* Trigger offset calibration
	 * As this generates a _DONE and _RESULT event
	 * the first result will be incorrect.
	 */
	NRF_SAADC_S->TASKS_CALIBRATEOFFSET = 1;
    printk("adc setup.\n");
    return ret;
}

int adc_sample(const struct device *dev) {
	int ret;

	const struct adc_sequence sequence = {
		.options = &sequence_opts,
		.channels = BIT(ADC_1ST_CHANNEL_ID) | BIT(ADC_2ND_CHANNEL_ID) | BIT(ADC_3RD_CHANNEL_ID)
		          | BIT(ADC_4TH_CHANNEL_ID) | BIT(ADC_5TH_CHANNEL_ID) | BIT(ADC_6TH_CHANNEL_ID),
		.buffer = sampling_buffer,
		.buffer_size = sizeof(sampling_buffer),
		.resolution = ADC_RESOLUTION,
	};

	if (!dev) {
		printk("ADC: Device is not ready.\n");
		return -1;
	}

	ret = adc_read(dev, &sequence);

    if (ret) {
        printk("ADC read err: %d\n", ret);
        return ret;
    }

	return ret;
}