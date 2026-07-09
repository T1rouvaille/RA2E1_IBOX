#include <zephyr.h>
#include <zephyr/sys/printk.h>
#include <sys/reboot.h>
#include <math.h>
#include <drivers/clock_control.h>
#include <drivers/clock_control/nrf_clock_control.h>
#include <drivers/counter.h>
#include <nrfx_clock.h>
#include <hal/nrf_gpio.h>

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
#include "system_manager.h"



#define SLEEP_TIME      	1000
#define MAX_REBOOT 			32
#define LOOP_TIME_MS		60000
#define LOOP_TICKS_PER_SEC	25	

#define SOC1_pin	16
#define SOC2_pin	24
#define SOC3_pin	23


// bool shutdown = false;
// bool  msg_got_poll = false;
// bool  msg_disable_ibox_comms = false;


// static int disable_ds_1(const struct device *dev)
// {
// 	ARG_UNUSED(dev);

// 	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_DISK);
// 	return 0;
// }

// SYS_INIT(disable_ds_1, PRE_KERNEL_2, 0);

static int early_init(const struct device *dev)
{
	
	nrf_gpio_cfg_output(SOC1_pin);
	nrf_gpio_cfg_output(SOC2_pin);
	nrf_gpio_cfg_output(SOC3_pin);
	nrf_gpio_pin_set(SOC1_pin);
	nrf_gpio_pin_set(SOC2_pin);
	nrf_gpio_pin_set(SOC3_pin);
	return 0;
}
SYS_INIT(early_init, PRE_KERNEL_1, 0);

// void datalogtimer_expiry(struct k_timer *timer)
// {
// 	DL_Inc_U32(&DL_log.app_time_bucket_0);
	
// 	// DL_Write();
// }

// K_TIMER_DEFINE(datalog_timer, datalogtimer_expiry, NULL);

/**
 * @brief Main loop
 * 
 * @note highest priority loop , implement watchdog feed. 
 * 
 */

void main(void)
{
	printk("Hello World from %s\n", CONFIG_BOARD);
	// // control_init();

	sys_init();

    DL_Inc_U32(&DL_log.main_power_switch_in_cycles);		//Datalog the main switch cycles.


	while(1)
	{
		// k_sem_take(&sleep_reboot, K_FOREVER);
		// if(system_sleep_flag)
		// {
		// 	sys_reboot(0);
		// }
		wdt_feed(wdt, wdt_channel_id);						//Feed the watchdog timer every 5 sec.
		k_sleep(K_SECONDS(5));			
	}
}