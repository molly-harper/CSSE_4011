/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/printk.h>
#include <zephyr/bluetooth/bluetooth.h>

int observer_start(void);
extern void print_rssi_buffer(void);

int main(void)
{
	int err;

	printk("Starting Observer Demo\n");

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	err = observer_start();
	if (err) {
		printk("Observer start failed (err %d)\n", err);
		return 0;
	}

	// Keep the main thread running
	while (1) {
		k_sleep(K_SECONDS(3));
        print_rssi_buffer();
	}

	return 0;
}