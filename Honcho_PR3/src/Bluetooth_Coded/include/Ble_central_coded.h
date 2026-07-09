// /*
// *  Copyright (c) 2022, Stanley Black and Decker 
// *    All rights reserved
// * Author : Kavya Jain 
// * Created on : 13 Jan 2023
// */
/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Nordic UART Bridge Service (NUS) sample
 **/

#ifndef BLE_CENTRAL_H_
#define BLE_CENTRAL_H_

#include <sys/printk.h>
#include <sys/util.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/types.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/hci_vs.h>

#include <bluetooth/hci.h>
#include <bluetooth/services/nus.h>
#include <bluetooth/services/nus_client.h>
#include <bluetooth/gatt_dm.h>
#include <bluetooth/scan.h>

#include <errno.h>
#include <stddef.h>

#include "gpio_def.h"
#include "motor_control.h"

#define MANUFACTURER_DATA_SIZE	22
extern uint8_t asset_tracking_array[MANUFACTURER_DATA_SIZE];


#define LOG_MODULE_NAME	Honcho_LLU
//#define BLE_THREAD_STACKSIZE	2048
//#define BLE_THREAD_PRIORITY 	1
#define UART_BUF_SIZE 9
#define button_data_size 	2
#define SBD_ID1					0xFE				//old version 0x00
#define SBD_ID2					0x00				//old version 0xFE
#define SBD_PTI_SEED			0xFE
#define SBD_PT_Index0			0x09				//0x09 for  DCLE34035 	       20V Robotic Laser Green
#define SBD_PT_Index1			0x09

#define BT_GAP_ADV_VERY_SLOW_5	0X1F40

//#define MANUFACTURER_DATA_SIZE	22
#define ASSET_TRACKING_DEVICE_NAME	"Sleepy LASER"
#define BT_UUID_LASER_VAL 0xFACE
#define NUS_WRITE_TIMEOUT K_MSEC(150)

struct uart_data_t {
	void *fifo_reserved;
	uint8_t  data[UART_BUF_SIZE];
	uint16_t len;
};

extern struct k_fifo button_fifo;


// void start_scan(void);
// void ble_start(void);
//void scanning_start(void);		// Thread entry point

extern struct bt_nus_client nus_client;

void Bluetooth_initialize(void);
void BleComms_start(void);
int asset_tracking_adv_update();
int asset_tracking_adv_update(void);
void Stop_BleComm(void);


#endif /* BLE_CENTRAL_H_ */

