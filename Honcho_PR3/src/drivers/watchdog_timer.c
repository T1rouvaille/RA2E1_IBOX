/*
*  Copyright (c) 2022, Stanley Black and Decker 
*    All rights reserved
* Author : KXJ0816
* Created on : 12 July 2023
*/

#include "watchdog_timer.h"

//  Private variabls
int wdt_channel_id;
const struct device *wdt ;//= DEVICE_DT_GET(DT_NODELABEL(wdt0));
int err;

struct wdt_timeout_cfg wdt_config = {
		/* Reset SoC when watchdog timer expires. */
		.flags = WDT_FLAG_RESET_SOC,

		/* Expire watchdog after max window */
		.window.min = 0U,
		.window.max = WDT_TIMEOUT,
	};




// Function 
/**
 * @brief 
 * 
 */
void watchdog_init()
{
    wdt = DEVICE_DT_GET(DT_NODELABEL(wdt0));
    if (!wdt)
    {
        printk("Cannot get WDT device\n");
        return;
	}

    wdt_channel_id = wdt_install_timeout(wdt, &wdt_config);
    if (wdt_channel_id < 0) {
		printk("Watchdog install error\n");
		return;
	}

	err = wdt_setup(wdt, WDT_OPT_PAUSE_HALTED_BY_DBG);
	if (err < 0) {
		printk("Watchdog setup error\n");
		return;
	}

}

/**
 * @brief 
 * 
 */
void watchdog_deinit()
{
	// k_thread_abort(watchdog_thread);

	err = wdt_disable(wdt);
	if(err <0){
		printk("Watchdog disable error % d\n", err);
	}
	
}

/**
 * @brief 
 * 
 */
// void watchdog_service()
// {
// 	while (1)
// 	{
// 		int rc =wdt_feed(wdt, wdt_channel_id);	/* code */
// 		printk("Eror%d\n", rc);
// 		k_sleep(K_SECONDS(5));
// 	}
	
// }
// //Thread defination
// K_THREAD_DEFINE(WATCHDOG_THREAD, WATCHDOG_THREAD_STACKSIZE, watchdog_service, NULL, NULL,
// 				NULL, WATCHDOG_THREAD_PRIORITY, 0, 0);