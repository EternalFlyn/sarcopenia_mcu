#ifndef FLYN_ADC_SERVICE_H
#define FLYN_ADC_SERVICE_H

#define ADC_SERVICE_UUID_VAL BT_UUID_128_ENCODE(0xD16F7A3D, 0x1897, 0x40EA, 0x9629, 0xBDF749AC5990)

void enable_recording(bool);
void adc_data_update(int16_t, int16_t, int16_t, int16_t, int16_t, int16_t);

#endif