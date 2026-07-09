/*!
 * @file Datalog_Handler.h
 *
 *  Created on: 11 Nov 2022
 *      Author: KXL1003A
 *
 */

#ifndef datalog_handler_H_
#define datalog_handler_H_

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// @ Includes
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
#include <Stdint.h>
#include <fs/nvs.h>
#include <drivers/flash.h>
#include <storage/flash_map.h>
#include <string.h>

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// @ DEFINES
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
#define DL_LOG_SIZE 444 
#define DL_HEADER_SIZE 4
#define LOG_TICKS_PER_MIN (60)
#define DL_FAULT_HISTORY_SIZE 80
#define DL_CHECK_WORD  0xABADCAFE
#define LOG_ID 			1
#define STORAGE_NODE_LABEL storage

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// @ DATATYPES
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

typedef struct{
	// @@@@ Configurable area SIZE = 444 bytes log @@@@
	
	// 36 x u32 Buckets 0..143
	// 0..39
	uint32_t app_time_bucket_0; // this doesn't actually record time, more # different use time lenghts
	uint32_t app_time_bucket_1;
	uint32_t app_time_bucket_2;
	uint32_t app_time_bucket_3;
	uint32_t total_time_above_temperature_threshold; // rest are times
	uint32_t total_time_below_temperature_threshold;
	uint32_t rotation_speed_none;
	uint32_t rotation_speed_1_laser_off;
	uint32_t rotation_speed_2_laser_off;
	uint32_t rotation_speed_3_laser_off;
	// 40..71
	uint32_t rotation_speed_none_laser_on;
	uint32_t rotation_speed_1_laser_on;
	uint32_t rotation_speed_2_laser_on;
	uint32_t rotation_speed_3_laser_on;
	uint32_t battery_pack_SOC_hi;
	uint32_t battery_pack_SOC_med;
	uint32_t battery_pack_SOC_lo;
	uint32_t self_leveling;

	uint32_t _spare_buckets[18]; 
	
	// 36 x u32 counters 144..287
	uint32_t main_power_switch_in_cycles;
	uint32_t scan_toggle_cycles;
	uint32_t RPM_toggle_cycles;
	uint32_t find_my_remote;
	uint32_t Pendulum_unlock_switch_cycles;
	uint32_t global_brightness_switch_cycles;
	uint32_t laser_1_switch_cycles;
	uint32_t laser_2_switch_cycles;
	uint32_t laser_3_switch_cycles;
	uint32_t z_translate_up_switch;
	// 184..223
	uint32_t z_translate_down_switch;
	uint32_t z_rotate_yaw_clockwise_switch;
	uint32_t z_rotate_yaw_counterclockwise_switch;
	uint32_t limit_switch_1;
	uint32_t limit_switch_2;
	uint32_t limit_switch_3;
	uint32_t limit_switch_4;
	uint32_t BLE_switch;
	uint32_t bluetooth_pairing_operation_1;
	uint32_t bluetooth_pairing_operation_2;
	// 224..243
	uint32_t bluetooth_pairing_operation_3;
	uint32_t unique_devices_paired_with_unit;
	uint32_t TX_packets;
	uint32_t RX_packets;
	uint32_t dropped_connections_with_a_paired_device;

	uint32_t _spare_cnt[11];

	// 20 x u16 faults 288..327
	uint16_t tilt_sensor_fault;
	uint16_t hibernate_sleep_mode;
	uint16_t total_count_drops_topples;
	uint16_t shutdown_ntc_hot_pack;
	uint16_t shutdown_ntc_cold_pack;
	uint16_t shutdown_battery_undervoltage;
	uint16_t shutdown_maximum_current_Limit;
	uint16_t shutdown_8V_buck_upper_limit;
	uint16_t shutdown_8V_buck_lower_limit;
	// 306..
	uint16_t shutdown_MCU;
	uint16_t _spare_faults[10];
	
	// @@@@ Non Configurable area @@@@
	// 80 x u8 Fault history  ... compatcted to 8 bit 328..407
	uint8_t fault_history[DL_FAULT_HISTORY_SIZE];
	
	uint32_t calibration; // 408..411
	uint32_t Log_Cntr;  // 412..415
	uint32_t product_serial[6]; // 416..439 TODO: - actual size? how to enter? currently it's a spare but this may be product serial num
	uint32_t check_word; //440..443
}log_t;

typedef enum{
	flt_none, flt_eoc, flt_laser_flt, flt_overtemp, flt_last
}fault_t;

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// @ FUNCTION PROTOTYPES
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

void DL_Log_App_Time(uint32_t run_time);
void DL_Update_Buckets(/* pass data to funct containing time bucket info*/);
void DL_Increment_History( fault_t fault );
void DL_Update_Shutdown_Log(/*pass vars holding shutdown fault flags*/ );
void DL_Init_Log(void);
int DL_Init_NVM(void);
//void DL_Debug_Write(void);
void DL_Write(void);

void DL_Inc_U32(uint32_t *item);
void DL_Add_U32(uint32_t *item, uint32_t num);
void DL_Inc_U16(uint16_t *item);
void DL_Add_U16(uint16_t *item, uint16_t num);




// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// @ GLOBAL VARS (PUBLIC)
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@


// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// @ GLOBAL CONSTS (PUBLIC)
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

#endif /* Datalog_Handler_H_ */
