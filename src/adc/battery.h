#ifndef FLYN_BATTERY_H
#define FLYN_BATTERY_H

#include <device.h>

int battery_init(const struct device *);
int start_battery_sample();
void stop_battery_sample();

#endif