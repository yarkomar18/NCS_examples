/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <logging/log.h>
#include <dk_buttons_and_leds.h>
#include <remote.h>

#define LOG_MODULE_NAME app
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define RUN_STATUS_LED DK_LED1
#define CONN_STATUS_LED DK_LED2
#define RUN_LED_BLINK_INTERVAL 1000

static struct bt_conn *current_conn;

void on_connected(struct bt_conn *conn, uint8_t err);
void on_disconnected(struct bt_conn *conn, uint8_t err); 
void on_notif_changed(enum bt_button_notifications_enabled status);
void on_data_received(struct bt_conn *conn, const uint8_t *const data, uint16_t len);

struct bt_conn_cb bluetooth_callbacks = {
	.connected 	= on_connected,
	.disconnected 	= on_disconnected,
};

struct bt_remote_service_cb remote_callbacks = {
	.notif_changed = on_notif_changed,
	.data_received = on_data_received,
};

void on_connected(struct bt_conn *conn, uint8_t err) 
{
	if(err) {
		LOG_ERR("Connection failed (err: %d)", err);
		return;
	}
	LOG_INF("Connected.");
	current_conn = bt_conn_ref(conn);
	dk_set_led_on(CONN_STATUS_LED);
}

void on_disconnected(struct bt_conn *conn, uint8_t err)
{
	LOG_INF("Disconnected (reason: %u)", err);
	
	if (current_conn) {
		bt_conn_unref(current_conn);
		current_conn = NULL;
		dk_set_led_off(CONN_STATUS_LED);
	}
}

void on_notif_changed(enum bt_button_notifications_enabled status)
{
	if (status == BT_BUTTON_NOTIFICATIONS_ENABLED) {
        LOG_INF("Notifications enabled");
    } else {
        LOG_INF("Notifications disabled");
    }
}

void on_data_received(struct bt_conn *conn, const uint8_t *const data, uint16_t len)
{
    uint8_t temp_str[len+1];
    memcpy(temp_str, data, len);
    temp_str[len] = 0x00;

    LOG_INF("Received data on conn %p. Len: %d", (void *)conn, len);
    LOG_INF("Data: %s", log_strdup(temp_str));
}

void button_handler(uint32_t button_state, uint32_t has_changed) 
{
	int button = 0;
	int err;
	if (button_state && has_changed) {
		switch (has_changed) {
			case DK_BTN1_MSK:
				button = 1;
			break;
			case DK_BTN2_MSK:
				button = 2;
			break;
			case DK_BTN3_MSK:
				button = 3;
			break;
			case DK_BTN4_MSK:
				button = 4;
			break;
		}
		LOG_INF("Button %d pressed", button);
		set_button_value(button);
		err = send_button_notification(current_conn, button);
		if (err) {
			LOG_ERR("Couldn't send notification (err %d)", err);
		}
	}
}

static void configure_dk_buttons_leds(void)
{
	int err;
	err = dk_leds_init();
	if (err) {
        LOG_ERR("Couldn't init LEDS (err %d)", err);
    }
	err = dk_buttons_init(button_handler);
	if (err) {
        LOG_ERR("Couldn't init buttons (err %d)", err);
    }
}

void main(void)
{
	int err;
	int led_state = 0;
	LOG_INF("Hello World! %s\n", CONFIG_BOARD);
	configure_dk_buttons_leds();

	err = bluetooth_init(&bluetooth_callbacks, &remote_callbacks);
	if (err) {
		LOG_INF("Couldn't initialize Bluetooth. err: %d", err);
	}

	for(;;) {
		dk_set_led(RUN_STATUS_LED, led_state);
		led_state = !led_state;
		k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
	}
}
