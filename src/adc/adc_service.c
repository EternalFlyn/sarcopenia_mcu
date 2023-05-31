#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <stdlib.h>

#include "adc_service.h"
#include "foot_pressure_queue.h"

#define DATA_AMOUNT_PER_PACKAGE 5
#define DATA_BYTE_SIZE 6 * 12 / 8 // 6 adc data per sampling, 12 bits per adc data, 8 bits = 1 byte

K_QUEUE_DEFINE(FOOT_PRESSURE_QUEUE);

static struct bt_uuid_128 adc_uuid = BT_UUID_INIT_128(ADC_SERVICE_UUID_VAL);
static struct bt_uuid_128 foot_pressure_uuid = BT_UUID_INIT_128(BT_UUID_128_ENCODE(0xD16F7A3D, 0x1897, 0x40EA, 0x9629, 0xBDF749AC5991));

static bool is_enable_recording = false;
static bool notify_flag = false;

static uint8_t package_num = 0;
static uint8_t sampling_count = 0;

static foot_pressure_data_t raw_array[25];
static uint8_t buffer[247];

static void foot_pressure_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value) {
	notify_flag = (value == BT_GATT_CCC_NOTIFY) ? true : false;
    if (notify_flag) is_enable_recording = true;
}

BT_GATT_SERVICE_DEFINE(adc_service,
    BT_GATT_PRIMARY_SERVICE(&adc_uuid),
    BT_GATT_CHARACTERISTIC(&foot_pressure_uuid.uuid, BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_NONE, NULL, NULL, NULL),
    BT_GATT_CCC(foot_pressure_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

uint32_t prev_us_time = 0;

uint16_t encode(struct k_queue* queue) {
    uint8_t count = sampling_count > 25 ? 25 : sampling_count;
    uint16_t amount = foot_pressure_queue_pop_amount(queue, raw_array, count);
    printk("Amount: %d\n", amount);
    // calculate buffer size 
    // package_number(1 byte) | list_size(1 byte) | data...(x) | check_sum(1 byte)
    uint16_t buffer_len = 3;
    buffer_len += amount * DATA_BYTE_SIZE;

    // add data size to head
    buffer[0] = package_num++;
    buffer[1] = amount;

    // put data into buffer
    uint16_t loc = 2;
    for (int i = 0; i < amount; i++) {
        for (int j = 0; j < 6; j++) {
            uint8_t hi_bit = (raw_array[i].value[j] >> 8) & 0x0F;
            uint8_t lo_bit = raw_array[i].value[j] & 0xFF;
            if (j % 2) {
                buffer[loc] |= hi_bit;
                buffer[loc + 2] = lo_bit;
                loc += 3;
            }
            else {
                buffer[loc] = hi_bit << 4;
                buffer[loc + 1] = lo_bit;
            }
        }
    }

    // put check sum at last
    uint8_t checksum_pos = buffer_len - 1;
    buffer[checksum_pos] = 0;
    
    for (int i = 0; i < checksum_pos; i++) {
        buffer[checksum_pos] += buffer[i];
    }
    return buffer_len;
}

void adc_raw_notify() {
    uint16_t len = encode(&FOOT_PRESSURE_QUEUE);
    printk("notify data length:%d\n", len);
    bt_gatt_notify(NULL, &adc_service.attrs[1], buffer, len);

    // measure notification time interval
    uint32_t time = k_cyc_to_us_near32(k_cycle_get_32());
    printk("notify interval (us): %d\n", time - prev_us_time);
    prev_us_time = time;
}

void enable_recording(bool enable) {
    is_enable_recording = enable;
    if (!enable) foot_pressure_queue_clean(&FOOT_PRESSURE_QUEUE);
}

void adc_data_update(int16_t ha, int16_t lt, int16_t m1, int16_t m5, int16_t arch, int16_t hm) {
    if (!is_enable_recording) return;
    sampling_count++;
    foot_pressure_data_t data = {
        .value = {ha, lt, m1, m5, arch, hm}
    };
    foot_pressure_queue_push(&FOOT_PRESSURE_QUEUE, data);
    
    if (!notify_flag) return;
    if (sampling_count >= DATA_AMOUNT_PER_PACKAGE) {
        adc_raw_notify();
        sampling_count -= DATA_AMOUNT_PER_PACKAGE;
    }
}