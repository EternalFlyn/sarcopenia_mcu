#include <zephyr.h>
#include <drivers/adc.h>
#include <hal/nrf_saadc.h>

#include "battery.h"
#include "device_info_service.h"

#define BATTERY_SAMPLE_TIME_S 1
#define ADC_GAIN ADC_GAIN_1_4
#define ADC_REFERENCE ADC_REF_VDD_1_4
#define ADC_ACQUISITION_TIME ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 40)
#define ADC_RESOLUTION 12

#define BATTERY_CHANNEL_ID 6
#define BATTERY_CHANNEL_INPUT NRF_SAADC_INPUT_AIN6

const struct device *battery_device;
static int16_t battery_buffer[1];

static const struct adc_channel_cfg battery_channel_cfg = {
	.gain = ADC_GAIN,
	.reference = ADC_REFERENCE,
	.acquisition_time = ADC_ACQUISITION_TIME,
	.channel_id = BATTERY_CHANNEL_ID,
#if defined(CONFIG_ADC_CONFIGURABLE_INPUTS)
	.input_positive = BATTERY_CHANNEL_INPUT,
#endif
};

static enum adc_action battery_callback(const struct device *dev,
                                    const struct adc_sequence *sequence,
									uint16_t index) {
	battery_data_updata(battery_buffer[0]);
	return ADC_ACTION_CONTINUE;
}

const struct adc_sequence_options battery_sequence_opts = {
	.callback = battery_callback,
	.user_data = NULL,
};

const struct adc_sequence battery_seq = {
	.options = &battery_sequence_opts,
	.channels = BIT(BATTERY_CHANNEL_ID),
	.buffer = battery_buffer,
	.buffer_size = sizeof(battery_buffer),
	.resolution = ADC_RESOLUTION,
};

void battery_work_handler(struct k_work *work) {
    if (!battery_device) {
        printk("Battery: Device is not ready.\n");
        return;
    }

    int ret = adc_read(battery_device, &battery_seq);

    if (ret) {
        printk("Battery read err: %d\n", ret);
        return;
    }
}

K_WORK_DEFINE(battery_work, battery_work_handler);

void battery_timer_handler(struct k_timer *timer) {
    k_work_submit(&battery_work);
}

K_TIMER_DEFINE(battery_timer, battery_timer_handler, NULL);

int battery_init(const struct device *dev) {
    battery_device = dev;
    if (!battery_device) {
        printk("Battery: Device is not ready.\n");
        return -1;
    }

    int ret = adc_channel_setup(dev, &battery_channel_cfg);

    if (ret) {
		printk("Error in battery channel setup: %d\n", ret);
        return ret;
	}

    NRF_SAADC->TASKS_CALIBRATEOFFSET = 1;
    printk("battery setup completed.\n");
    return ret;
}

int start_battery_sample() {
    if (!battery_device) {
        printk("Battery: Device is not ready.\n");
        return -1;
    }

    k_timer_start(
        &battery_timer,
        K_SECONDS(BATTERY_SAMPLE_TIME_S),
        K_SECONDS(BATTERY_SAMPLE_TIME_S)
    );

    return 0;
}

void stop_battery_sample() {
    k_timer_stop(&battery_timer);
}