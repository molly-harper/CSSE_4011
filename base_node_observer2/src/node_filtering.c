#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>

#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>

#include <string.h>



#include <zephyr/sys/slist.h>

LOG_MODULE_REGISTER(Node_filtering, LOG_LEVEL_DBG);

static sys_slist_t ibeacon_list; // list to hold the ibeacons

// ibeacon_node structure
struct ibeacon_node { 

    sys_snode_t node;

    char ble_name[32];
    char mac_addr[17];
    int major;
    int minor;
    float x;
    float y;
    char left_neighbor[32];
    char right_neighbor[32];
}; 

// list of known nodes
static const struct ibeacon_node knownBeacons[] = { 
    { 
        .ble_name = "4011-A", 
        .mac_addr = "F5:75:FE:85:34:67", 
        .major = 2753, 
        .minor = 32998, 
        .x = 0, 
        .y = 0 , 
        // need left nd right neighbors
    }, 
    { 
        .ble_name = "4011-B", 
        .mac_addr = "E5:73:87:06:1E:86", 
        .major = 32975, 
        .minor = 20959, 
        .x = 1.5, 
        .y = 0, 
        // need left nd right neighbors
    }, 
    { 
        .ble_name = "4011-C", 
        .mac_addr = "CA:99:9E:FD:98:B1", 
        .major = 26679, 
        .minor = 40363, 
        .x = 3, 
        .y = 0, 
        // need left nd right neighbors
    }, 
    { 
        .ble_name = "4011-D", 
        .mac_addr = "CB:1B:89:82:FF:FE", 
        .major = 41747, 
        .minor = 38800, 
        .x = 3, 
        .y = 2, 
        // need left nd right neighbors
    }, 
    { 
        .ble_name = "4011-E", 
        .mac_addr = "D4:D2:A0:A4:5C:AC", 
        .major = 30679, 
        .minor = 51963, 
        .x = 3, 
        .y = 4, 
        // need left nd right neighbors
    }, 
    { 
        .ble_name = "4011-F", 
        .mac_addr = "C1:13:27:E9:B7:7C", 
        .major = 6195, 
        .minor = 18394, 
        .x = 1.5, 
        .y = 4, 
        // need left nd right neighbors
    }, 
    { 
        .ble_name = "4011-G", 
        .mac_addr = "F1:04:48:06:39:A0", 
        .major = 30525, 
        .minor = 30544, 
        .x = 0, 
        .y = 4, 
        // need left nd right neighbors
    }, 
    { 
        .ble_name = "4011-H", 
        .mac_addr = "CA:0C:E0:DB:CE:60", 
        .major = 57395, 
        .minor = 28931, 
        .x = 0, 
        .y = 2, 
        // need left nd right neighbors
    }, 
    { 
        .ble_name = "4011-I", 
        .mac_addr = "D4:7F:D4:7C:20:13", 
        .major = 60345, 
        .minor = 49995, 
        .x = 4.5, 
        .y = 0, 
        // need left nd right neighbors
    }, 
    { 
        .ble_name = "4011-J", 
        .mac_addr = "F7:0B:21:F1:C8:E1", 
        .major = 12249, 
        .minor = 30916, 
        .x = 6, 
        .y = 0, 
        // need left nd right neighbors
    }, 
    { 
        .ble_name = "4011-K", 
        .mac_addr = "FD:E0:8D:FA:3E:4A", 
        .major = 36748, 
        .minor = 11457, 
        .x = 6, 
        .y = 2, 
        // need left nd right neighbors
    }, 
    { 
        .ble_name = "4011-L", 
        .mac_addr = "EE:32:F7:28:FA:AC", 
        .major = 27564, 
        .minor = 27589 , 
        .x = 6, 
        .y = 4, 
        // need left nd right neighbors
    }, 
    { 
        .ble_name = "4011-M", 
        .mac_addr = "F7:3B:46:A8:D7:2C", 
        .major = 49247, 
        .minor = 52925, 
        .x = 4.5, 
        .y = 4, 
        // need left nd right neighbors
    },  
}; 

static int cmd_add_beacon(const struct shell *shell, size_t argc, char **argv) 
{ 

    char* nodeName = argv[1]; // get entered name

    bool beaconFound = false; // Flag to check whether the entered beacon is in the list of known beacons

    for (int beaconNumber = 0; beaconNumber < ARRAY_SIZE(knownBeacons); beaconNumber++) { 
        if (strcmp(knownBeacons[beaconNumber].ble_name, nodeName) == 0) { 
            struct ibeacon_node *new_ibeacon = k_malloc(sizeof(struct ibeacon_node)); 

            if (!new_ibeacon) { 
                LOG_ERR("Unable to add new iBeacon"); 
                return -ENOMEM; 
            }

            *new_ibeacon = knownBeacons[beaconNumber];  // Set the new beacon info to the info held in the list

            sys_slist_append(&ibeacon_list, &new_ibeacon->node);    // append the new beacon to the list of beacons

            beaconFound = true; // set flag to indicate the beacon was in the known beacon list
        }
    }

    // If the beacon was not within the list, log error
    if (!beaconFound) { 
        LOG_ERR("Not valid iBeacon"); 
        return 1; 
    }

    return 0; 

}

static int cmd_remove_beacon(const struct shell *shell, size_t argc, char **argv) 
{ 
    char* nodeName = argv[1]; 
    
    struct ibeacon_node *node; 
    struct ibeacon_node *prev = NET_BUF_EXTERNAL_DATA; 


    SYS_SLIST_FOR_EACH_CONTAINER(&ibeacon_list, node, node) {

        if (strcmp(node->ble_name, nodeName) == 0) {
            // Remove from list
            sys_slist_remove(&ibeacon_list,
                             prev ? &prev->node : NULL,
                             &node->node);

            k_free(node);
            LOG_INF("%s removed from current ibeacons", nodeName); 
            return 0;
        }
        prev = node;
    }

    return 0; 
}

static int cmd_show_active(const struct shell *shell, size_t argc, char **argv)
{
    struct ibeacon_node *node;

    SYS_SLIST_FOR_EACH_CONTAINER(&ibeacon_list, node, node) {

        printk("%s [%s] Major: %d Minor: %d Pos: (%.1f, %.1f)\n",
            node->ble_name, node->mac_addr, node->major, node->minor, node->x, node->y);
    }

    if (sys_slist_is_empty(&ibeacon_list)) { 
        LOG_INF("No iBeacons currently selected"); 
    }

    return 0;
}

// Register shell commands
SHELL_STATIC_SUBCMD_SET_CREATE(
    sub_beacon,
    SHELL_CMD(add, NULL, "Add known beacon by name", cmd_add_beacon),
    SHELL_CMD(show, NULL, "Show active beacons", cmd_show_active),
    SHELL_CMD(remove, NULL, "Remove beacon by name", cmd_remove_beacon),
    SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(beacon, &sub_beacon, "Beacon list operations", NULL);

// Init list on boot
static int beacon_list_init(const struct device *dev)
{
    ARG_UNUSED(dev);
    sys_slist_init(&ibeacon_list);
    return 0;
}

SYS_INIT(beacon_list_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY); 

