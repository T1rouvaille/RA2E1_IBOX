/*
*  Copyright (c) 2022, Stanley Black and Decker 
*    All rights reserved
* Author : Kavya Jain 
* Created on : 21 Spet 2023
*/

#ifndef ACC_IIM42351_H
#define ACC_IIM42351_H

#include <stdio.h>
#include <zephyr.h>
#include <device.h>
#include <math.h>
#include <drivers/i2c.h>

#include "soc_laserblink.h"
#include "system_manager.h"
#include "calibration.h"
#include <hal/nrf_gpio.h>

static const struct device *i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));



#define ACC_ADR 0x69 
#define ACC_REG_WAI 0x75


#define AC_CFG_REG1  0x2A
#define AC_CFG_REG2  0x2B

#define ACC_REG_BANK_SEL    0x76
#define ACC_DEVICE_CONFIG    0x11
#define ACC_CONFIG0         0x50
#define AC_APEX_CONFIG0     0x56
#define AC_APEX_CONFIG1     0x40
#define AC_APEX_CONFIG2     0x41
#define AC_APEX_CONFIG3     0x42
#define AC_APEX_CONFIG4     0x43
#define AC_APEX_CONFIG5     0x44
#define AC_APEX_CONFIG6     0x45    
#define AC_APEX_CONFIG7     0x46
#define AC_APEX_CONFIG8     0x47
#define AC_APEX_CONFIG9     0x48
#define AC_APEX_CONFIG10    0x49

#define ACC_ANGLE_THREAD_PRIORITY   6
#define ACC_ANGLE_STACK_SIZE        1024
#define DROP_READ_PRIORITY                  7
#define DROP_READ_STACK_SIZE                1024

#define PI  3.14159265358979323846

extern const k_tid_t ACC_ANGLE_READ_THREAD;
extern const k_tid_t DROP_READ_THREAD;
extern uint16_t anglethread_sleep_time;
extern volatile uint8_t Unit_overtuned_flag;
extern volatile int accinit_failed;

uint32_t moving_average_calc(uint32_t *arr,uint16_t input, uint8_t index);
int freefall_HWinitconfig(void);
double x_angle_calc(void);
double y_angle_calc(void);
double z_angle_calc(void);
void acc_reg_init(void);
void acc_angle_read(void);
void Stop_Acc_timers(void);
void ACC_CALIBRATION(void);
// void Readoffset_flash(void); @KJ , static void at the moment, will see if it works and change it and call it in main()
#endif /*ACC_IIM42351_H */