/*
*  Copyright (c) 2022, Stanley Black and Decker 
*    All rights reserved
* Author : Kavya Jain 
* Created on : 13 July 2023
*/

#include <zephyr.h>
#include <sys/printk.h>

#include "gpio_def.h"
#include "button_manager.h"
#include "system_manager.h"

extern volatile bool lowvoltage_flag;
extern volatile bool pendblink_flag;
extern volatile bool Outoflevel_blink_flag;
extern struct k_timer OneHz_laserpulse;
extern struct k_timer pendulumlocked_blink_timer;
void socled_manager(uint8_t soc_reg);
// void Outoflevel_blink(float x, float y);
void Outoflevel_blink(double z, int count);
void Stop_Acc_timers(void);