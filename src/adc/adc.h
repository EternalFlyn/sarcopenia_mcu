#ifndef FLYN_ADC_H
#define FLYN_ADC_H

#include <device.h>

int adc_init(const struct device *);
int start_adc_sample();
void stop_adc_sample();

#endif