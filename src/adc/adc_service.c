#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <stdlib.h>

#include "adc_service.h"
#include "foot_pressure_queue.h"
#include "adc.h"

#define DATA_AMOUNT_PER_PACKAGE 20
#define DATA_BYTE_SIZE 6 * 12 / 8 // 6 adc data per sampling, 12 bits per adc data, 8 bits = 1 byte
#define NOTIFY_THREAD_AMOUNT 10

K_QUEUE_DEFINE(FOOT_PRESSURE_QUEUE);
K_SEM_DEFINE(notify_sem, 0, NOTIFY_THREAD_AMOUNT);

static struct bt_uuid_128 adc_uuid = BT_UUID_INIT_128(ADC_SERVICE_UUID_VAL);
static struct bt_uuid_128 foot_pressure_uuid = BT_UUID_INIT_128(BT_UUID_128_ENCODE(0xD16F7A3D, 0x1897, 0x40EA, 0x9629, 0xBDF749AC5991));

static bool is_connected = false;
// static bool notify_flag = false;

static uint8_t package_num = 0;
static uint8_t sampling_count = 0;

// for debug
static int package_count = 0;

static foot_pressure_data_t raw_array[25];
static uint8_t buffer[247];

static void foot_pressure_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value) {
	bool notify_flag = (value == BT_GATT_CCC_NOTIFY) ? true : false;
    if (notify_flag) enable_recording(true);
    else enable_recording(false);
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
    sampling_count -= amount;
    // printk("Amount: %d\n", amount);
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
        // printk("ADC raw value: ");
        for (int j = 0; j < 6; j++) {
            // printk("%d ", raw_array[i].value[j]);
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
        // printk("\n");
    }

    // put check sum at last
    uint8_t checksum_pos = buffer_len - 1;
    buffer[checksum_pos] = 0;
    
    for (int i = 0; i < checksum_pos; i++) {
        buffer[checksum_pos] += buffer[i];
    }
    return buffer_len;
}

void notify_complete_callback(struct bt_conn *conn, void *user_data) {
    printk("notify completed\n");
    k_sem_take(&notify_sem, K_NO_WAIT);
}

void adc_raw_notify() {
    k_sem_give(&notify_sem);
    uint16_t len = encode(&FOOT_PRESSURE_QUEUE);
    // printk("notify data length:%d\n", len);
    // bt_gatt_notify(NULL, &adc_service.attrs[1], buffer, len);
    struct bt_gatt_notify_params params = {
        .attr = &adc_service.attrs[1],
        .data = buffer,
        .len = len,
        .func = notify_complete_callback,
    };
    bt_gatt_notify_cb(NULL, &params);
    package_count++;
    printk("package count: %d\n", package_count);

    // measure notification time interval
    uint32_t time = k_cyc_to_us_near32(k_cycle_get_32());
    // printk("notify interval (us): %d\n", time - prev_us_time);
    prev_us_time = time;
}

void enable_recording(bool enable) {
    if (enable) {
        start_adc_sample();
        k_sem_reset(&notify_sem);
    }
    else {
        stop_adc_sample();
        package_count = 0;
        sampling_count = 0;
        foot_pressure_queue_clean(&FOOT_PRESSURE_QUEUE);
    }
}

void device_connected(bool status) {
    is_connected = status;
}

void adc_data_update(int16_t ha, int16_t lt, int16_t m1, int16_t m5, int16_t arch, int16_t hm) {
    sampling_count++;
    foot_pressure_data_t data = {
        .value = {ha, lt, m1, m5, arch, hm}
    };
    foot_pressure_queue_push(&FOOT_PRESSURE_QUEUE, data);
    
    if (!is_connected || k_sem_count_get(&notify_sem) >= NOTIFY_THREAD_AMOUNT) {
        // printk("sem count: %d", k_sem_count_get(&notify_sem));
        return;
    }
    if (sampling_count >= DATA_AMOUNT_PER_PACKAGE) {
        adc_raw_notify();
    }
}