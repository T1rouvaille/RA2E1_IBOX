/*
*  Copyright (c) 2022, Stanley Black and Decker 
*    All rights reserved
* Author : Kavya Jain 
* Created on : 12 Sept 2023
*/

#include "system_manager.h"

// ###########################################
// # GLOBAL VAR
// ###########################################

bool shutdown = false;                                      // Flag to determine the shutdown
bool  msg_got_poll = false;                                 // Flag to determine if Bootloader is connected
bool  msg_disable_ibox_comms = false;                       // Flag to disable ibox commms after 50 msec
bool system_reboot_flag = false;                            // Flag to indicate the state of reboot
volatile bool system_sleep_flag = false;                    // Flag to indiacte the state of sleep mode
volatile uint8_t pendulum_state = 0;                        // Flag to indicate the state of pendulum 
volatile uint8_t sleep_reboot_request = 0;                  // Flag to indicate sleep or reboot request status 0-> default ; 2-> Reboot ; 4-> Sleep
static volatile uint8_t batt_pend_state_flag = 0;           // Flag to indicate if debounce was called from Batt detect or pendulem lock interrupt 0-> default ; 2-> Batt detect ; 4-> pendulum lock
volatile uint8_t memory_corrupt = 0;                        // Flag to detect NVM init failed |resv|resv|resv|resv|resv|datalogsec|flashvarsec|calibsec|

// ###########################################
// # GPIO DECLERATION
// ###########################################

static const struct gpio_dt_spec battdetect_interrupt = GPIO_DT_SPEC_GET_OR(BATTDETECT_NODE, gpios, {0});
static const struct gpio_dt_spec pendulumlock_interrupt = GPIO_DT_SPEC_GET_OR(PENDULUMINT_NODE, gpios, {0});

// ###########################################
// # CALLBACK VARS
// ###########################################


static struct gpio_callback battdetect_cb_data;
static struct gpio_callback pendulumloc_cb_data;

// ###########################################
// # SEMAPHORE DECLERATION
// ###########################################

// K_SEM_DEFINE(sleep_reboot, 0, 10);
// K_SEM_DEFINE(sleep_sem, 0, 1);
K_SEM_DEFINE(datalogwrite_semaphore, 0, 1);             //Semaphore to enable datalog write 

//--------------------------------------------------------------------------------------------------------------------------------------------------//
// ###########################################################
// # KERNEL TIMER EXPIRY FUNCTIONS & CALLBACK HANDLERS
// ##########################################################

/**
 * 
 * @brief Timer expiry function to record the time variable. The count on variables
 *       number of sec that get converted to time  datalogging.
 * @param timer 
 */
void datalogtime_timer_expiry(struct k_timer *timer)
{
    // DL_Inc_U32(&DL_log.app_time_bucket_0);
    DL_Inc_U32(&DL_log.total_runtime);
    if(laser_read_status() & (1 << 2))
       DL_Inc_U32(&DL_log.laser_level_runtime);
    if(laser_read_status() & (1 << 0))
       DL_Inc_U32(&DL_log.laser_plumb1_runtime); 
    if(laser_read_status() & (1 << 1))
       DL_Inc_U32(&DL_log.laser_plumb2_runtime);
    if(((motorstatus_reg&0x83) == 0x81 || (motorstatus_reg&0x83) == 0x82)) 
        DL_Inc_U32(&DL_log.yaw_motor_runtime);
    if(((motorstatus_reg&0x8c) == 0x88 || (motorstatus_reg&0x8c) == 0x84)) 
        DL_Inc_U32(&DL_log.vertical_motor_runtime);
    if(connected_flag_fortime ==1)
        DL_Inc_U32(&DL_log.ble_connected_time);
    
}

/**
 * 
 * @brief software debounce timer
 * 
 * @param timer 
 */
void debounce_timer_expiry(struct k_timer *timer)
{
    
    if(batt_pend_state_flag == 4)
    {
        if(gpio_pin_get(pendulumlock_interrupt.port, pendulumlock_interrupt.pin))
        {
            pendulum_unlocked();
        }
        if (!(gpio_pin_get(pendulumlock_interrupt.port, pendulumlock_interrupt.pin)))
        {
            pendulum_locked();
        }
    }
    batt_pend_state_flag = 0;
}

/**
 * 
 * @brief  timer expiry function to give semaphore to perform datalog write 
 * @param timer 
 */

void datalogwrite_timer_expiry(struct k_timer *timer)
{
    k_sem_give(&datalogwrite_semaphore);
}
K_TIMER_DEFINE(datalogtime_timer, datalogtime_timer_expiry, NULL);
K_TIMER_DEFINE(debounce_timer, debounce_timer_expiry, NULL);
K_TIMER_DEFINE(datalogwrite_timer, datalogwrite_timer_expiry, NULL);


// void battdetect_callback_fucntion(const struct device *dev, struct gpio_callback *cb,uint32_t pins)
// {
//     k_timer_start(&debounce_timer, K_MSEC(1), K_NO_WAIT);
//     batt_pend_state_flag = 1<<1;
// }

void pendulumlockint_callback_fucntion(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    if(power_on)
    {
        k_timer_start(&debounce_timer, K_MSEC(50), K_NO_WAIT);
        batt_pend_state_flag = 1<<2;
    }
}

//----------------------------------------------------------------------------------------------------------------------
// ###########################################
// # FUCTIONS
// ###########################################

/**
 * 
 * @brief Initialize funnction for Datalog initialize
 *       DL_Init_NVM()-> initializes Nordci Flash for use. 
 *       DL_Comms_Init()-> initializes the softare read out and stop all operation for reading on UART for 50ms
 * 
 * #return int 
 */
int hardware_init()
{
	// uint32_t ret;

	// ==== datalog nvm setup ====
	if(DL_Init_NVM()){
		printk("DL NVM setup failed\n"); 
		return 8;
	}
	
	// ==== datalog comms setup ====
	if(DL_Comms_Init()){
		printk("DL Comms setup failed\n"); 
		return 9;
	}

	return 0; // ok
}

// void Battdetect_init()
// {
//     int ret;

// 	if (!device_is_ready(battdetect_interrupt.port)) {
// 		printk("Error: button device %s is not ready\n",
// 		       battdetect_interrupt.port->name);
// 		return;
// 	}

// 	ret = gpio_pin_configure_dt(&battdetect_interrupt, GPIO_INPUT);
// 	if (ret != 0) {
// 		printk("Error %d: failed to configure %s pin %d\n",
// 		       ret, battdetect_interrupt.port->name, battdetect_interrupt.pin);
// 		return;
// 	}

// 	ret = gpio_pin_interrupt_configure_dt(&battdetect_interrupt,
// 					      GPIO_INT_EDGE_BOTH);
//     // ret = gpio_pin_interrupt_configure_dt(&battdetect_interrupt,
// 	// 				      GPIO_INT_LEVEL_HIGH);
// 	if (ret != 0) {
// 		printk("Error %d: failed to configure interrupt on %s pin %d\n",
// 			ret, battdetect_interrupt.port->name, battdetect_interrupt.pin);
// 		return;
// 	}

// 	gpio_init_callback(&battdetect_cb_data, battdetect_callback_fucntion, BIT(battdetect_interrupt.pin));
// 	gpio_add_callback(battdetect_interrupt.port, &battdetect_cb_data);
//     printk("vbatt interrupt ready \n");
    
// }

/**
 * 
 * @brief Initialize function for Pendulum Interrupt 
 * 
 */
void Pendulumswitchint_init()
{
    int ret;

	if (!device_is_ready(pendulumlock_interrupt.port)) {
		printk("Error: button device %s is not ready\n",
		       pendulumlock_interrupt.port->name);
		return;
	}

	ret = gpio_pin_configure_dt(&pendulumlock_interrupt, GPIO_INPUT);
	if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n",
		       ret, pendulumlock_interrupt.port->name, pendulumlock_interrupt.pin);
		return;
	}

	ret = gpio_pin_interrupt_configure_dt(&pendulumlock_interrupt,
					      GPIO_INT_EDGE_BOTH);
	if (ret != 0) {
		printk("Error %d: failed to configure interrupt on %s pin %d\n",
			ret, pendulumlock_interrupt.port->name, pendulumlock_interrupt.pin);
		return;
	}

	gpio_init_callback(&pendulumloc_cb_data, pendulumlockint_callback_fucntion, BIT(pendulumlock_interrupt.pin));
	gpio_add_callback(pendulumlock_interrupt.port, &pendulumloc_cb_data);
    printk("pendulum interrupt ready \n");
}

// void Pendulumswitchint_deinit()
// {
//     ret = gpio_pin_configure_dt(&pendulumlock_interrupt, GPIO_INPUT);
// 	if (ret != 0) {
// 		printk("Error %d: failed to configure %s pin %d\n",
// 		       ret, pendulumlock_interrupt.port->name, pendulumlock_interrupt.pin);
// 		return;
// 	}

// 	ret = gpio_pin_interrupt_configure_dt(&pendulumlock_interrupt,
// 					      GPIO_INT_EDGE_BOTH);
// 	if (ret != 0) {
// 		printk("Error %d: failed to configure interrupt on %s pin %d\n",
// 			ret, pendulumlock_interrupt.port->name, pendulumlock_interrupt.pin);
// 		return;
// 	}
// }

/**
 * 
 * @brief Function to initialize all the periherals
 * 
 */
void sys_init()
{  
    control_init();     
    motor_init();
    laser_init();
    led_init();
    led_off();
    bucks_off();
    enables_off();
    /*****************************************************************************************************************/
    nrfx_power_pofwarn_config_t pof_init;
    pof_init.thr = NRF_POWER_POFTHR_V28;
    nrfx_power_pof_init(&pof_init);                                 //Set interrept to stop datalogging internally
    nrf_power_pofcon_set(NRF_POWER, true, NRF_POWER_POFTHR_V28);    //when teh volatge reaches < 2.8V
    /*****************************************************************************************************************/
    Bluetooth_initialize();
    k_sleep(K_MSEC(1));
    custom_adc_init();
    if(freefall_HWinitconfig() < 0) 
        accinit_failed = 1;
    else    
        accinit_failed = 0;
    printk("accfailed %d\n", accinit_failed);
    if( FLASHVAR_init_NVM() ){
		memory_corrupt |= 0x02;
	}
    if( CALIB_init_NVM() ){
		memory_corrupt |= 0x01;
	}
    k_sched_lock();
    Ibox_init();
	if( hardware_init() ){
		memory_corrupt |= 0x04;
	}
    IBOX();
    k_sched_unlock();
    // k_sleep(K_MSEC(1000));
    if(msg_disable_ibox_comms == true)
    {
        watchdog_init();
    }
    k_thread_start(KEYPAD_ATTINY_THREAD);          //Start Attiny thread, To read button data
    Pendulumswitchint_init();
    // Battdetect_init();
    k_sleep(K_MSEC(1));
    task_delegator(0x0204);                         //Send power button to Turn the unit On. 
}

/**
 * 
 * @brief Function called to start all the threads and start the normal funciton 
 * 
 * @note Api called from button manager thread.
 * 
 */
void sys_on()
{
    k_timer_start(&datalogtime_timer, K_SECONDS(1), K_SECONDS(1));
    k_timer_start(&datalogwrite_timer, K_SECONDS(300), K_SECONDS(300));
    uint8_t default_laser_reg = FLASHVAR_laser_read();
    // watchdog_deinit();
    default_laser_on(&default_laser_reg);
    if(!system_reboot_flag)
    {
        system_reboot_flag = true;
        k_thread_start(PACK_ADC_READ_THREAD);
        k_thread_start(DATALOG_WRITE_THREAD);
        k_thread_start(ACC_ANGLE_READ_THREAD);
        k_thread_start(DROP_READ_THREAD);
        k_thread_start(LASER_CURRENT_READ_THREAD);
        // k_thread_start(PACK_ADC_READ_THREAD);
        // Bluetooth_initialize();
    }
    // printk("I M HERE\n");
    int pendstate = gpio_pin_get(pendulumlock_interrupt.port, pendulumlock_interrupt.pin);      //Get pendulum state for lock led
    if(!pendstate)
        pendulum_locked();
    else 
        pendulum_unlocked();
    BleComms_start();
    // Bluetooth_initialize();
    // ble_init();
    
}

/**
 * @brief Fucntion to turn off unit off. 
 * 
 * @note suspends and end all thread , stop all timer and safely move to shutdown
 * 
 */
void sys_off()
{
    int ret;
    // printk("Reached in idle mode api\n");
    Stop_BleComm();
    k_timer_stop(&datalogtime_timer);
    k_sem_give(&datalogwrite_semaphore);
    k_sleep(K_MSEC(5));
    // DL_Write();
    // adc_thread_time(5);
    // k_thread_suspend(DATALOG_WRITE_THREAD);
    // k_thread_suspend(ACC_ANGLE_READ_THREAD); 
    Stop_Acc_timers();
    // k_thread_suspend(DROP_READ_THREAD);    
    Stop_pendlck_timers();
    led_off();
    bucks_off();

    // printk("i am in idle mode\n");
    while(1)
    {
        
        ret = gpio_pin_set_dt(&SysldoEN, 0);		//Turn the LDO off to cut the power 
        if (ret != 0) 
        {
            printk("ERROR\n");
        }    
        // k_busy_wait(10000000);    
    }
    // int ret = gpio_pin_configure_dt(&SysldoEN, GPIO_OUTPUT_LOW);		//Turn the LDO off to cut the power 
	// if (ret != 0) 
	// {
	// 	return;
	// }
    // // printk("i am in idle mode 3\n");
    // k_busy_wait(10000000);
}

// void sys_sleep()
// {
//     int ret; 
//     led_off();
//     bucks_off();
//     disable_battmeasurement();
//     ret = pm_device_action_run(uart, PM_DEVICE_ACTION_SUSPEND);
//     if(ret >0)
//         printk("error suspending peripheral err code %d \n", ret);
//     ret = pm_device_action_run(dev_adc, PM_DEVICE_ACTION_SUSPEND);
//     if(ret >0)
//         printk("error suspending peripheral err code %d \n", ret);
//     ret = pm_device_action_run(spi_dev, PM_DEVICE_ACTION_SUSPEND);
//     if(ret >0)
//         printk("error suspending peripheral err code %d \n", ret);
//     ret = pm_device_action_run(i2c_dev, PM_DEVICE_ACTION_SUSPEND);
//     if(ret >0)
//         printk("error suspending peripheral err code %d \n", ret);

//     nrf_gpio_input_disconnect((NRF_DT_GPIOS_TO_PSEL(DT_ALIAS(pendlck), gpios)));
//     nrf_gpio_input_disconnect((NRF_DT_GPIOS_TO_PSEL(DT_ALIAS(keypadint), gpios)));
    
//     printk("I AM GOIN TO SLEEEP NOW \n");
    
//     k_sleep(K_MSEC(1200));
//     // int ret ;
//     system_sleep_flag = true;
//     power_on = 0;
//     Stop_BleComm();
//     Stop_Acc_timers();
//     Stop_pendlck_timers();

//     k_thread_abort(KEYPAD_ATTINY_THREAD);
//     k_thread_abort(PACK_ADC_READ_THREAD);
//     k_thread_abort(DATALOG_WRITE_THREAD);
//     k_thread_abort(ACC_ANGLE_READ_THREAD);
//     k_thread_abort(DROP_READ_THREAD);
//     led_off();
//     bucks_off();
//     enables_off();

//     ret = gpio_pin_configure_dt(&SysldoEN, GPIO_OUTPUT_LOW);		//Experimantal #KJ 
// 	if (ret != 0) 
// 	{
// 		return;
// 	}

//     k_sleep(K_SECONDS(2U));

//     ret = pm_state_force(0u, &(struct pm_state_info){PM_STATE_SUSPEND_TO_DISK, 0, 0});
//     // ret = pm_state_force(0u, &(struct pm_state_info){PM_STATE_SOFT_OFF, 0, 0});
//     // if(ret >0)
//     //     printk("error sleeping %d \n", ret);
// 	k_sleep(K_FOREVER);
//     // k_sleep(K_SECONDS(2U));

//     // while (true) {
// 	// 	/* spin to avoid fall-off behavior */
// 	// }
//     // // printk("enter sleep pmode 01 \n");   
    
// }

/**
 * @brief Fucntion to indicate pendulum locked mode api 
 *        Turn the slope LED on and turn off timer
 * 
 */
void pendulum_locked()
{
    printk("in pendulum locked\n");
    pendulum_state = 0;
    gpio_pin_set_dt(&slopeled, 1);
    Outoflevel_blink_flag =0;
    Stop_Acc_timers();
    k_timer_start(&pendulumlocked_blink_timer,K_MSEC(8500), K_MSEC(8500));
}

/**
 * @brief Fucntion to indicate pendulum unlocked mode api 
 *        Turn the slope LED off and turn off locked more blink timer
 * 
 */
void pendulum_unlocked()
{
    printk("in pendulum unlocked\n");
    DL_Inc_U32(&DL_log.Pendulum_unlock_switch_cycles);
    pendulum_state = 1;
    gpio_pin_set_dt(&slopeled, 0);
    k_timer_stop(&pendulumlocked_blink_timer);
}

/**
 * @brief helper function to tunr off timer
 * 
 */
void Stop_pendlck_timers()
{
    k_timer_stop(&pendulumlocked_blink_timer);
}

/**
 * @brief thread function to peridically write datalogging to flash
 * 
 */
void dataLog_write()
{
    while(1)
    {
        k_sem_take(&datalogwrite_semaphore, K_FOREVER);                                 //#KJ
        if(!(memory_corrupt & (1<<2)))
        {
            k_sched_lock();
            // DL_Update_Shutdown_Log();
            DL_Write();
            k_sched_unlock();
            // k_sleep(K_SECONDS(300));              //TO be changed After datalog test      
        }
    }
}
K_THREAD_DEFINE(DATALOG_WRITE_THREAD, DLWIRTE_THREAD_STACK_SIZE, dataLog_write, NULL, NULL,
				NULL, DLWIRTE_THREAD_PRIORITY, 0, -1);


