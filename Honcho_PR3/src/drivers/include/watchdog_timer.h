
/*
*  Copyright (c) 2022, Stanley Black and Decker 
*    All rights reserved
* Author : KXJ0816
* Created on : 12 July 2023
*/

#ifndef WATCHDOG_TIMER_H
#define WATCHDOG_TIMER_H

#include <zephyr.h>
#include <device.h>
#include <drivers/watchdog.h>
#include <sys/printk.h>
#include <stdbool.h>

#define WDT_TIMEOUT                     10000U
#define WATCHDOG_THREAD_STACKSIZE       256
#define WATCHDOG_THREAD_PRIORITY        5

extern int wdt_channel_id;
extern const struct device *wdt;
extern const k_tid_t WATCHDOG_THREAD;

void watchdog_init(void);
void watchdog_deinit(void);
void watchdog_service(void);

#endif /* WATCHDOG_TIMER_H */
