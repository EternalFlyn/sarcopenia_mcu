#include <zephyr.h>
#include <sys/printk.h>

#include <settings/settings.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/addr.h>
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

static uint8_t set_bt_addr(const char *addr_str) {
	uint8_t err;
	bt_addr_le_t addr;

	err = bt_addr_le_from_str(addr_str, "random", &addr);
	if (err) {
		printk("Invalid BT address (err %d)\n", err);
	}

	err = bt_id_create(&addr, NULL);
	if (err < 0) {
		printk("Creating new ID failed (err %d)\n", err);
		return 0;
	}
	printk("new addr: %02x:%02x:%02x:%02x:%02x:%02x\n", addr.a.val[5], addr.a.val[4], addr.a.val[3], addr.a.val[2], addr.a.val[1], addr.a.val[0]);
	return err;
}

static int bt_ready() {
	int err;

	struct bt_le_adv_param adv_param = {
		.id = BT_ID_DEFAULT,
		.sid = 0,
		.secondary_max_skip = 0,
		.options = (BT_LE_ADV_OPT_CONNECTABLE |
			    BT_LE_ADV_OPT_USE_NAME |
				BT_LE_ADV_OPT_USE_IDENTITY),
		.interval_min = 0x00a0, /* 100 ms */
		.interval_max = 0x00f0, /* 150 ms */
		.peer = NULL,
	};

	// adv_param.id = set_bt_addr("E0:00:49:00:00:02");

	printk("Bluetooth initialized\n");
	
	err = bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad), sd, 0);
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
	.interval_min = 18,
	.interval_max = 24,
	.timeout = 30
};

static void connected(struct bt_conn *conn, uint8_t error) {
	printk("Connected\n");
	bt_conn_le_param_update(conn, &conn_param);
}

static void disconnected(struct bt_conn *conn, uint8_t reason) {
	printk("Disconnected, reason:%d\n", reason);
	if (reason == 19) enable_recording(false);
}

static struct bt_conn_cb conn_callbacks = { 
	.connected = connected,
	.disconnected = disconnected
};

/* gatt callback */


static void mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx) {
	printk("Updated MTU: TX: %d RX: %d bytes\n", tx, rx);
	if (tx == 236 && rx == 236) {
		mtu_is_setup(true);
	}
	else {
		mtu_is_setup(false);
	}
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