/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/printk.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>

#define NAME_LEN 30

#define NUM_BEACONS 8
#define BUFFER_SIZE (NUM_BEACONS + 1)  // 8 RSSI values + 1 for ultrasonic distance

int16_t rssi_buffer[BUFFER_SIZE];


// Function Declarations
static bool extract_rssi_values(struct net_buf_simple *ad, int8_t *rssi_values);
static bool is_our_ibeacon(struct net_buf_simple *ad);
static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type, struct net_buf_simple *ad);
int observer_start(void);
extern void print_rssi_buffer(void);


void print_rssi_buffer(void) {
    printk("RSSI Buffer values:\n");
    for (int i = 0; i < NUM_BEACONS; i++) {
        printk("Beacon %d: %d\n", i + 1, rssi_buffer[i]);
    }
    printk("Ultrasonic distance: %d\n", rssi_buffer[NUM_BEACONS]);
}




static bool is_our_ibeacon(struct net_buf_simple *ad)
{
    uint8_t len, type;
    while (ad->len > 1) {
        len = net_buf_simple_pull_u8(ad);
        if (len == 0) {
            break;
        }
        type = net_buf_simple_pull_u8(ad);
        
        if (type == BT_DATA_MANUFACTURER_DATA && len >= 4) {
            if (ad->data[0] == 0x4D && ad->data[1] == 0x00) { // 0x4D is our unqiue identifier
                if (ad->data[2] == 0x02 && ad->data[3] == 0x15) {
                    return true;
                }
            }
        }
        net_buf_simple_pull(ad, len - 1);
    }
    return false;
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
                         struct net_buf_simple *ad)
{
    char addr_str[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

    struct net_buf_simple_state state;
    net_buf_simple_save(ad, &state);

    if (is_our_ibeacon(ad)) {
        struct net_buf_simple_state state;
        net_buf_simple_save(ad, &state);

        //printk("Ultrasonic Node found: %s (RSSI %d)\n", addr_str, rssi);

        const uint8_t *data = ad->data;
        uint16_t dist = ((uint16_t)data[4] << 8) | data[5];
        //printk("Ultrasonic distance: %d\n", dist);
        rssi_buffer[NUM_BEACONS] = dist;
        net_buf_simple_restore(ad, &state);
        return;
    }

    net_buf_simple_restore(ad, &state);

    if (strncmp(addr_str, "D2:9B:44:CE:1A:EC", 17) == 0) {
        struct net_buf_simple_state state;
        int8_t beacon_rssi[8];

        net_buf_simple_save(ad, &state);

        if (extract_rssi_values(ad, beacon_rssi)) {
            // printk("\nMobile Node Update (MAC: %s)\n", addr_str);
            // printk("Direct RSSI: %d\n", rssi);
            // printk("Beacon RSSI values:\n");
            for (int i = 0; i < 8; i++) {
                rssi_buffer[i] = beacon_rssi[i];
              //  printk("Beacon %d: %d dBm\n", i + 1, beacon_rssi[i]);
            }
        }

        net_buf_simple_restore(ad, &state);
    }
}



// Function to extract RSSI values from manufacturer data
static bool extract_rssi_values(struct net_buf_simple *ad, int8_t *rssi_values) {
    while (ad->len > 1) {
        uint8_t len = net_buf_simple_pull_u8(ad);
        uint8_t type;

        if (len == 0) {
            break;
        }

        type = net_buf_simple_pull_u8(ad);

        if (type == BT_DATA_MANUFACTURER_DATA && len >= 4) {  
            const uint8_t *rssi_data = &ad->data[4];
            
            for (int i = 0; i < 8; i++) {
                rssi_values[i] = (int8_t)rssi_data[i];
            }
            return true;
        
        }
        
        net_buf_simple_pull(ad, len - 1);
    }
    return false;
}






#if defined(CONFIG_BT_EXT_ADV)
static bool data_cb(struct bt_data *data, void *user_data)
{
	char *name = user_data;
	uint8_t len;

	switch (data->type) {
	case BT_DATA_NAME_SHORTENED:
	case BT_DATA_NAME_COMPLETE:
		len = MIN(data->data_len, NAME_LEN - 1);
		(void)memcpy(name, data->data, len);
		name[len] = '\0';
		return false;
	default:
		return true;
	}
}

static const char *phy2str(uint8_t phy)
{
	switch (phy) {
	case BT_GAP_LE_PHY_NONE: return "No packets";
	case BT_GAP_LE_PHY_1M: return "LE 1M";
	case BT_GAP_LE_PHY_2M: return "LE 2M";
	case BT_GAP_LE_PHY_CODED: return "LE Coded";
	default: return "Unknown";
	}
}

static void scan_recv(const struct bt_le_scan_recv_info *info,
		      struct net_buf_simple *buf)
{
	char le_addr[BT_ADDR_LE_STR_LEN];
	char name[NAME_LEN];
	uint8_t data_status;
	uint16_t data_len;

	(void)memset(name, 0, sizeof(name));

	data_len = buf->len;
	bt_data_parse(buf, data_cb, name);

	data_status = BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS(info->adv_props);

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));
	printk("[DEVICE]: %s, AD evt type %u, Tx Pwr: %i, RSSI %i "
	       "Data status: %u, AD data len: %u Name: %s "
	       "C:%u S:%u D:%u SR:%u E:%u Pri PHY: %s, Sec PHY: %s, "
	       "Interval: 0x%04x (%u ms), SID: %u\n",
	       le_addr, info->adv_type, info->tx_power, info->rssi,
	       data_status, data_len, name,
	       (info->adv_props & BT_GAP_ADV_PROP_CONNECTABLE) != 0,
	       (info->adv_props & BT_GAP_ADV_PROP_SCANNABLE) != 0,
	       (info->adv_props & BT_GAP_ADV_PROP_DIRECTED) != 0,
	       (info->adv_props & BT_GAP_ADV_PROP_SCAN_RESPONSE) != 0,
	       (info->adv_props & BT_GAP_ADV_PROP_EXT_ADV) != 0,
	       phy2str(info->primary_phy), phy2str(info->secondary_phy),
	       info->interval, info->interval * 5 / 4, info->sid);
}

static struct bt_le_scan_cb scan_callbacks = {
	.recv = scan_recv,
};
#endif /* CONFIG_BT_EXT_ADV */

int observer_start(void)
{
	struct bt_le_scan_param scan_param = {
		.type       = BT_LE_SCAN_TYPE_PASSIVE,
		.options    = BT_LE_SCAN_OPT_FILTER_DUPLICATE,
		.interval   = BT_GAP_SCAN_FAST_INTERVAL,
		.window     = BT_GAP_SCAN_FAST_WINDOW,
	};
	int err;

#if defined(CONFIG_BT_EXT_ADV)
	bt_le_scan_cb_register(&scan_callbacks);
	printk("Registered scan callbacks\n");
#endif /* CONFIG_BT_EXT_ADV */

	err = bt_le_scan_start(&scan_param, device_found);
	if (err) {
		printk("Start scanning failed (err %d)\n", err);
		return err;
	}
	printk("Started scanning...\n");

	return 0;
}