/*
*  Copyright (c) 2022, Stanley Black and Decker 
*    All rights reserved
* Author : Kavya Jain 
* Created on : 25 Nov 2022
*/

#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include <zephyr/kernel.h>

#include "keypad_attiny.h"
#include "motor_control.h"
#include "laser_control.h"
#include "gpio_def.h"
#include "custom_adc.h"
#include "datalog_handler.h"
#include "system_manager.h"
#include "calibration.h"



#define     BUTTONMANAGER_THREAD_STACK_SIZE       2048
#define     BUTTONMANAGER_THREAD_PRIORITY         2
#define     BUTTONTYPE_MASK                       0x3F0F

extern struct k_fifo button_fifo;
extern const k_tid_t button_data_manager; 
// extern log_t log;
extern struct k_sem vertmotorrampsem;
extern struct k_sem yawmotorrampsem;
extern volatile uint8_t power_on;

void button_manager(void);
void task_delegator(uint16_t data);
int lsearch(uint16_t data, uint16_t *arr, int size);

#endif /* BUTTTON_MANAGER_H */