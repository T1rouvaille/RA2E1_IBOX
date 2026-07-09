/*
*  Copyright (c) 2022, Stanley Black and Decker 
*    All rights reserved
* Author : Kavya Jain  
* Created on : 14 Sept 2023
*/

#ifndef  CALIBRATION_H
#define  CALIBRATION_H

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
#define CALIB_ID 			1
#define FLASHVAR_ID         2
#define VARIABLE_FLASH_NODE_LABEL laserstatus_area
#define CALIB_FLASH_NODE_LABEL  calib_area

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// @ DATATYPES
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

typedef struct{
    uint8_t mac_addr[6];
    // int32_t acc_x_zero_offset;
    // int32_t acc_y_zero_offset;
    // int32_t acc_z_zero_offset;
    double acc_roll_angle_zero_offset;
    double acc_pitch_angle_zero_offset;
    // float   acc_yaw_angle_zero_offset;
}calib_t;

typedef struct{
    uint32_t laser_status_mem;
    uint32_t Drop_detect_mem;
}flashvar_t;

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// @ VARIABLE DEFINES 
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

extern calib_t calib;
extern flashvar_t flash_mem;
extern volatile bool system_calibrated_flag;
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// @ FUNCTION PROTOTYPES
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

void CALIB_init_var(void);
int CALIB_init_NVM(void);
void CALIB_write(void);
calib_t CALIB_read(void);
void FLASHVAR_init_var(void);
int FLASHVAR_init_NVM(void);
void FLASHVAR_write(void);
void FLASHVAR_update(uint32_t *item, uint32_t num);
uint8_t FLASHVAR_laser_read(void);
// void CALIB_Update_bucket(/*pass the variable to save*/);

#endif /* CALIBRATION_H */