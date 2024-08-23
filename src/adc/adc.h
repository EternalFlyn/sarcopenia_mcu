#ifndef FLYN_ADC_H
#define FLYN_ADC_H

#include <device.h>

#define ADC_SAMPLE_TIME_MS 10 // 100Hz
#define CHANNEL_AMOUNT 6
#define ADC_GAIN ADC_GAIN_1_4
#define ADC_REFERENCE ADC_REF_VDD_1_4
#define ADC_ACQUISITION_TIME ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 40)
#define ADC_RESOLUTION 12

int adc_init(const struct device *);
int start_adc_sample();
void stop_adc_sample();

#endif