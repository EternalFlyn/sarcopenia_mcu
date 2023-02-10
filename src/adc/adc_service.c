#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <stdlib.h>

#include "adc_service.h"

static struct bt_uuid_128 adc_uuid = BT_UUID_INIT_128(ADC_SERVICE_UUID_VAL);
static struct bt_uuid_128 foot_pressure_uuid = BT_UUID_INIT_128(BT_UUID_128_ENCODE(0xD16F7A3D, 0x1897, 0x40EA, 0x9629, 0xBDF749AC5991));

static int16_t raw_array[6];
static uint8_t notify_flag;

static void foot_pressure_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value) {
	notify_flag = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}

BT_GATT_SERVICE_DEFINE(adc_service,
    BT_GATT_PRIMARY_SERVICE(&adc_uuid),
    BT_GATT_CHARACTERISTIC(&foot_pressure_uuid.uuid, BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_NONE, NULL, NULL, NULL),
    BT_GATT_CCC(foot_pressure_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

uint32_t prev_us_time = 0;

void adc_raw_notify() {
    if (!notify_flag) return;

    bt_gatt_notify(NULL, &adc_service.attrs[1], raw_array, 6);

    // measure notification time interval
    uint32_t time = k_cyc_to_us_near32(k_cycle_get_32());
    printk("notify interval (us): %d\n", time - prev_us_time);
    prev_us_time = time;
}

void adc_data_update(int16_t ha, int16_t lt, int16_t m1, int16_t m5, int16_t arch, int16_t hm) {
    if (!notify_flag) return;
    
    raw_array[0] = ha;
    raw_array[1] = lt;
    raw_array[2] = m1;
    raw_array[3] = m5;
    raw_array[4] = arch;
    raw_array[5] = hm;
    adc_raw_notify();
}