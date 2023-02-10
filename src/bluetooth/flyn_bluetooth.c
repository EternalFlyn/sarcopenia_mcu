#include <zephyr.h>
#include <sys/printk.h>

#include <settings/settings.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>

#include "flyn_bluetooth.h"
#include "adc_service.h"

/* bt ready */

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR))
};

static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID16_ALL),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, ADC_SERVICE_UUID_VAL)
};

static int bt_ready() {
	int err;

	printk("Bluetooth initialized\n");

	if (IS_ENABLED(CONFIG_SETTINGS)) settings_load();
	
	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), sd, 0);
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return err;
	}

	printk("Advertising successfully started\n");
    return err;
}

/* connect callback */

static struct bt_le_conn_param conn_param = {
	// 15 ms to 15 ms
	.interval_min = 6,
	.interval_max = 6,
	.timeout = 30
};

static void connected(struct bt_conn *conn, uint8_t error) {
	printk("Connected\n");
	bt_conn_le_param_update(conn, &conn_param);
}

static void disconnected(struct bt_conn *conn, uint8_t reason) {
	printk("Disconnected, reason:%d\n", reason);
}

static struct bt_conn_cb conn_callbacks = { 
	.connected = connected,
	.disconnected = disconnected
};

/* gatt callback */


static void mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx) {
	printk("Updated MTU: TX: %d RX: %d bytes\n", tx, rx);
}

static struct bt_gatt_cb gatt_callbacks = {
	.att_mtu_updated = mtu_updated
};

/* -------------------- */

int bt_init() {
    int err;

    err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return err;
	}

    err = bt_ready();
    if (err) return err;

	bt_conn_cb_register(&conn_callbacks);
	bt_gatt_cb_register(&gatt_callbacks);
	
    return err;
}