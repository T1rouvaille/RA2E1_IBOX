/*
*  Copyright (c) 2022, Stanley Black and Decker 
*    All rights reserved
* Author : Kavya Jain 
* Created on : 12 Sept 2023
*/

#ifndef SYSTEM_MANAGER_H
#define SYSTEM_MANAGER_H

#include <device.h>
#include <init.h>
#include <pm/pm.h>
#include <pm/device.h>
#include <pm/policy.h>
#include <soc.h>
#include <hal/nrf_gpio.h>
#include <hal/nrf_power.h>
#include <nrfx_power.h>

#include "keypad_attiny.h"
#include "motor_control.h"
#include "laser_control.h"
#include "Ble_central.h"
#include "gpio_def.h"
#include "datalog_handler.h"
#include "datalog_comms.h"
#include "utils.h"
#include "custom_adc.h"
#include "watchdog_timer.h"
// #include "button_manager.h"
#include "keypad_attiny.h"
#include "spi.h"
#include "acc_IIM42351.h"

// ###########################################
// # DEFINES 
// ###########################################

#define     DLWIRTE_THREAD_STACK_SIZE       1024   
#define     DLWIRTE_THREAD_PRIORITY         2
#define     SLEEPCALL_THREAD_STACK_SIZE     1024   
#define     SLEEPCALL_THREAD_PRIORITY       1
#define     BATTDETECT_NODE                       DT_ALIAS(battdetect)
#define     PENDULUMINT_NODE                      DT_ALIAS(pendlck)

// ###########################################
// # GLOBAL VAR (PUBLIC)
// ###########################################
extern const k_tid_t DATALOG_WRITE_THREAD;
extern struct k_sem datalogwrite_semaphore;

extern volatile bool system_sleep_flag;
extern bool shutdown;
extern bool  msg_got_poll;
extern bool  msg_disable_ibox_comms;
extern volatile uint8_t pendulum_state;
extern volatile uint8_t sleep_reboot_request;
extern volatile uint8_t memory_corrupt;
// int hardware_init();

// ###########################################
// # FUNCTION PROTOTYPE
// ###########################################
void sys_init(void);
void sys_on(void);
void sys_off(void);
void sys_sleep(void);
void dataLog_write(void);
void sleep_thread_fucn(void);
void v3p3detect_init(void);
void Pendulumswitchint_init(void);
void Battdetect_init(void);
void pendulumlockint_callback_fucntion(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
void battdetect_callback_fucntion(const struct device *dev, struct gpio_callback *cb,uint32_t pins);
void pendulum_locked(void);
void pendulum_unlocked(void);
void Stop_pendlck_timers(void);

#endif /*SYSTEM_MANAGER_H */