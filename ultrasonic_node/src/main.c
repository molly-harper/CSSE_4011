#include <zephyr/devicetree.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>



#include <zephyr/device.h>

#include <zephyr/sys/printk.h>

#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>

#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>

#include <zephyr/types.h>
#include <stddef.h>


#define DEVICE_NAME "M_Ultrasonic_Node"
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define HIGH    1
#define LOW     0


// Devicetree node identifier 
#define ULTRA_SONIC_NODE DT_PATH(zephyr_user)

// Set GPIO pins for the chainable led
static const struct gpio_dt_spec echoPin = GPIO_DT_SPEC_GET(ULTRA_SONIC_NODE, echo_gpios); 
static const struct gpio_dt_spec triggerPin = GPIO_DT_SPEC_GET(ULTRA_SONIC_NODE, trigger_gpios); 



/*
 * Set iBeacon demo advertisement data. These values are for
 * demonstration only and must be changed for production environments!
 *
 * UUID:  18ee1516-016b-4bec-ad96-bcb96d166e97
 * Major: 0
 * Minor: 0
 * RSSI:  -56 dBm
 */
static struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR),
    BT_DATA_BYTES(BT_DATA_MANUFACTURER_DATA,
        0x4D, 0x00,              // Company ID (Apple)
        0x02, 0x15,             // iBeacon Type
        0x0F, // distance high bit
        0x0F) // distance low bit
};


static const struct bt_data sd[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};


// Callback for updating the measurement
static void update_measurement(uint16_t distance_cm) {
    // ad[1].data[4] and [5] hold the Minor field (distance)

    if (ARRAY_SIZE(ad) < 2 || ad[1].type != BT_DATA_MANUFACTURER_DATA) {
        printk("Invalid ad structure\n");
        return;
    }

    uint8_t *manuf_data = (uint8_t *)ad[1].data;

    // Update Minor (2 bytes) with the distance
    manuf_data[4] = (distance_cm >> 8) & 0xFF;  // High byte
    manuf_data[5] = distance_cm & 0xFF;         // Low byte

    //uint16_t dist_check = ((uint16_t)manuf_data[4] << 8) | manuf_data[5];
    //printk("%d\n", dist_check); 
}

static void bt_ready(int err)
{
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	/* Start advertising */
	err = bt_le_adv_start(BT_LE_ADV_NCONN, ad, ARRAY_SIZE(ad),
			      NULL, 0);
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("iBeacon started\n");
}

void send_measurement(uint16_t distance_cm)
{ 
    int err; 

    bt_le_adv_stop();

}



/**
 * @brief Set the clock and data gpio pins for the LED.
 */
void configure_pins(void) 
{ 
    gpio_pin_configure_dt(&echoPin, GPIO_INPUT); 
    gpio_pin_configure_dt(&triggerPin, GPIO_OUTPUT_ACTIVE);
}

void send_pulse(void)
{
    gpio_pin_set_dt(&triggerPin, HIGH);
    k_busy_wait(10);
    gpio_pin_set_dt(&triggerPin, LOW); 
}



void main(void) { 

    configure_pins(); 


    int err;
    char addr_s[BT_ADDR_LE_STR_LEN];
    bt_addr_le_t addr = {0};
    size_t count = 1;

    // Initialize Bluetooth
    err = bt_enable(NULL);
    if (err) {
        printk("Bluetooth init failed (err %d)\n", err);
        return;
    }

    printk("Bluetooth initialized\n");


    // Start advertising
    err = bt_le_adv_start(BT_LE_ADV_NCONN_IDENTITY, ad, ARRAY_SIZE(ad),
                         sd, ARRAY_SIZE(sd));  // Include scan response data
    if (err) {
        printk("Advertising failed to start (err %d)\n", err);
        return;
    }


    // Get and print the Bluetooth address
    bt_id_get(&addr, &count);
    bt_addr_le_to_str(&addr, addr_s, sizeof(addr_s));
    printk("iBeacon started, advertising as %s\n", addr_s);

    while(1) { 

        send_pulse(); 

        while(gpio_pin_get_dt(&echoPin) == LOW); 
        uint64_t start = k_cycle_get_64(); 

        while(gpio_pin_get_dt(&echoPin) == HIGH); 
        uint64_t end = k_cycle_get_64(); 

        uint64_t pulse_cycles = end - start;
        uint64_t pulse_duration_us = k_cyc_to_us_ceil64(pulse_cycles);

        // Distance in cm (sound speed = 343 m/s, divide by 58 for cm)
        uint16_t distance_cm = pulse_duration_us / 58;

        if (distance_cm < 1390) { 

            update_measurement(distance_cm);

            printk("Distance: %d cm (pulse: %lld us)\n", distance_cm, pulse_duration_us);
            err = bt_le_adv_update_data(ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
            if (err) {
                printk("Failed to update advertising data (err %d)\n", err);
            }

            k_sleep(K_MSEC(200));
        } else { 
            uint16_t bad_measurement = 1390;
            update_measurement(bad_measurement);

            err = bt_le_adv_update_data(ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
            if (err) {
                printk("Failed to update advertising data (err %d)\n", err);
            }

            k_sleep(K_MSEC(200));
        }
    }
}