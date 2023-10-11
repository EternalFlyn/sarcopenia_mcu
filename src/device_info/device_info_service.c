#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <stdlib.h>

#include "device_info_service.h"

static struct bt_uuid_128 device_service_uuid = BT_UUID_INIT_128(DEVICE_INFO_SERVICE_UUID_VAL);
static struct bt_uuid_128 battery_uuid = BT_UUID_INIT_128(BT_UUID_128_ENCODE(0x58C4FFA1, 0x1548, 0x44D5, 0x9972, 0xF7C25BECB621));

static uint8_t battery_data[4];

static ssize_t read_battery(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf, uint16_t len, uint16_t offset) {
    return bt_gatt_attr_read(conn, attr, buf, len, offset, battery_data, sizeof(battery_data));
}

BT_GATT_SERVICE_DEFINE(device_info_service,
    BT_GATT_PRIMARY_SERVICE(&device_service_uuid),
    BT_GATT_CHARACTERISTIC(&battery_uuid.uuid, BT_GATT_CHRC_READ, BT_GATT_PERM_READ, read_battery, NULL, battery_data)
);

void battery_data_updata(int16_t battery) {
    for (int i = 0; i < 4; i++) {
        battery_data[i] = battery >> (i * 8) & 0xFF;
    }
}