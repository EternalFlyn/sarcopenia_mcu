#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <stdlib.h>

#include "adc_service.h"
#include "adc_queue.h"

#define DATA_AMOUNT_PER_PAGKAGE 150

K_QUEUE_DEFINE(EMG_LEFT);
K_QUEUE_DEFINE(EMG_RIGHT);

static struct bt_uuid_128 adc_uuid = BT_UUID_INIT_128(ADC_SERVICE_UUID_VAL);
static struct bt_uuid_128 emg_left_uuid = BT_UUID_INIT_128(BT_UUID_128_ENCODE(0xD16F7A3D, 0x1897, 0x40EA, 0x9629, 0xBDF749AC5991));
static struct bt_uuid_128 emg_right_uuid = BT_UUID_INIT_128(BT_UUID_128_ENCODE(0xD16F7A3D, 0x1897, 0x40EA, 0x9629, 0xBDF749AC5992));

static uint8_t package_num = 0;
static uint8_t sampling_count = 0;

static int16_t raw_array[DATA_AMOUNT_PER_PAGKAGE];
static uint8_t buffer[247];

static uint8_t notify_flag, left_flag, right_flag;

static void notify_flag_control() {
    notify_flag = left_flag && right_flag;
    if (!notify_flag) {
        adc_queue_clean(&EMG_LEFT);
        adc_queue_clean(&EMG_RIGHT);
        package_num = 0;
        sampling_count = 0;
    }
}

static void left_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value) {
	left_flag = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
    notify_flag_control();
}

static void right_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value) {
	right_flag = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
    notify_flag_control();
}

BT_GATT_SERVICE_DEFINE(adc_service,
    BT_GATT_PRIMARY_SERVICE(&adc_uuid),
    BT_GATT_CHARACTERISTIC(&emg_left_uuid.uuid, BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_NONE, NULL, NULL, NULL),
    BT_GATT_CCC(left_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    BT_GATT_CHARACTERISTIC(&emg_right_uuid.uuid, BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_NONE, NULL, NULL, NULL),
    BT_GATT_CCC(right_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

uint16_t adc_encode(struct k_queue* queue) {
    uint16_t amount = adc_queue_pop_amount(queue, raw_array, DATA_AMOUNT_PER_PAGKAGE);
    printk("Amount: %d\n", amount);
    // calculate buffer size 
    // package_number(1 bytes) | list_size(1 bytes) | data...(x) | check_sum(1 byte)
    uint16_t buffer_len = 3;
    buffer_len += amount * 12 / 8;
    if (amount % 2) buffer_len++;

    // malloc buffer & add data size to head
    buffer[0] = package_num;
    buffer[1] = amount;

    // put data into buffer
    uint16_t loc = 2;
    for (int i = 0; i < amount; i++) {
        uint8_t hi_bit = (raw_array[i] >> 8) & 0x0F;
        uint8_t lo_bit = raw_array[i] & 0xFF;
        if (i % 2) {
            buffer[loc] |= hi_bit;
            buffer[loc + 2] = lo_bit;
            loc += 3;
        }
        else {
            buffer[loc] = hi_bit << 4;
            buffer[loc + 1] = lo_bit;
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

uint32_t prev_us_time = 0;

void adc_raw_notify() {
    if (!notify_flag) return;
    
    printk("package number: %d\n", package_num);
    uint16_t len = adc_encode(&EMG_LEFT);
    printk("buffer length: %d\n", len);
    bt_gatt_notify(NULL, &adc_service.attrs[1], buffer, len);

    len = adc_encode(&EMG_RIGHT);
    printk("buffer length: %d\n", len);
    bt_gatt_notify(NULL, &adc_service.attrs[4], buffer, len);
    package_num++;

    // measure notification time interval
    uint32_t time = k_cyc_to_us_near32(k_cycle_get_32());
    printk("notify interval (us): %d\n", time - prev_us_time);
    prev_us_time = time;
}

void adc_data_update(int16_t left, int16_t right) {
    if (!notify_flag) return;
    sampling_count++;

    adc_queue_push(&EMG_LEFT, left);
    adc_queue_push(&EMG_RIGHT, right);

    if (sampling_count == DATA_AMOUNT_PER_PAGKAGE) {
        adc_raw_notify();
        sampling_count = 0;
    } 
}