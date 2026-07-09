/*
*  Copyright (c) 2022, Stanley Black and Decker 
*    All rights reserved
* Author : Kavya Jain 
* Created on : 13 Jan 2023
*/

#include <zephyr/kernel.h>
#include "laser_control.h"

// ###########################################
// # GLOBAL VAR
// ###########################################
static uint32_t period[2] = {PWM_HZ(8500U), PWM_HZ(200000U)};     // Array with 2 PWM frequencies for the laser system 
// static uint32_t period[2] = {PWM_HZ(8475U), PWM_HZ(8475U)}; 
volatile uint32_t duty_cycle = PWM_HZ(200000U)/100U;  
// volatile uint32_t duty_cycle = PWM_HZ(8475U)/40U;  
volatile uint8_t laser_status_reg = 0;                               //Laser Status register. | Resvd | Resvd | Resvd | Resvd | Resvd | Level | Plum1 | Plum2 |
static volatile uint8_t pulsetimer_status = 0;                       //Pulse timer status register
volatile uint8_t variable = 1;

// ###########################################
// # KERNEL TIMER EXPIRY FUNCTIONS
// ###########################################

/**
 * @brief eight msec expiry
 * Timer expiry to get pwm back after detector freq 
 * on all pwm drivers
 * 
 * @param timer_id 
 */
void eight_msec_expiry(struct k_timer *timer_id)
{
    if(laser_status_reg & (1<<0)){
        pwm_set_dt(&plum2_pwm, period[1],duty_cycle);
    }
    if(laser_status_reg & (1<<1)){
        pwm_set_dt(&plum1_pwm, period[1],duty_cycle);
    }
    if(laser_status_reg & (1<<2)){
        pwm_set_dt(&level_pwm, period[1],duty_cycle);
    }
}

K_TIMER_DEFINE(eight_msec_timer,eight_msec_expiry, NULL);

/**
 * @brief pulse timer stop
 * Api called when the pusle timer is stopped explicitly
 * 
 * @param timer_id 
 */
void pulse_timer_stop(struct k_timer *timer_id)
{
    pulsetimer_status = 0x00;
}

/**
 * @brief pulse timer expiry
 * Timer expiry function that starts the detector frequency of 8.5khz@50% duty 
 * for 4ms 
 * 
 * @param timer_id 
 */
void pulse_timer_expiry(struct k_timer *timer_id)
{
    pulsetimer_status = 0x01;
    if(laser_status_reg & (1<<0)){
        pwm_set_dt(&plum2_pwm, period[0], period[0] / 2U);
    }
    if(laser_status_reg & (1<<1)){
        pwm_set_dt(&plum1_pwm, period[0], period[0] / 2U);
    }
    if(laser_status_reg & (1<<2)){
        pwm_set_dt(&level_pwm, period[0], period[0] / 2U);
    }
    
    k_timer_start(&eight_msec_timer,K_MSEC(4),K_NO_WAIT);                   //@KJ Changed it to 8msec to increase detector range
}

K_TIMER_DEFINE(pulse_timer, pulse_timer_expiry, pulse_timer_stop);

/***********************************************************************************************************************/

/**
 * @brief laser init
 * Initializes the PWM driver for 3 lasers 
 * Initializes and configures all the bias pins to low 
 * 
 */
void laser_init(void)
{
    int ret; 

    ret = gpio_pin_configure_dt(&levelbias, GPIO_OUTPUT_LOW);		//here set initial state
	if (ret != 0) {
		return;
	}
    
    ret = gpio_pin_configure_dt(&plum1bias, GPIO_OUTPUT_LOW);		//here set initial state
	if (ret != 0) {
		return;
	}
    
    ret = gpio_pin_configure_dt(&plum2bias, GPIO_OUTPUT_LOW);		//here set initial state
	if (ret != 0) {
		return;
	}

   
    ret = pwm_set_dt(&level_pwm, period[1], period[1] + 1);
    if (ret != 0) {
		return;
	}

    ret = pwm_set_dt(&plum1_pwm, period[1], period[1] + 1);
    if (ret != 0) {
		return;
	}

    ret = pwm_set_dt(&plum2_pwm, period[1], period[1] + 1);
    if (ret != 0) {
		return;
	}

    k_timer_stop(&pulse_timer);                                     //@KJ


}

/**
 * @brief laser plum1 ON
 * helper funtion to turn on Plum1
 * 
 */
void laser_plum1_ON(void)
{
    // gpio_pin_set_dt(&levelbias, 1);
    pwm_set_dt(&plum1_pwm, period[1],duty_cycle);
    laser_status_reg |= 1<<1;
    // laser_plum1_status = 1;
    if(pulsetimer_status == 0)
    {
        //k_timer_start(&pulse_timer,K_MSEC(500U),K_MSEC(500U));
         k_timer_start(&pulse_timer,K_MSEC(44U),K_MSEC(44U));
    }
}

/**
 * @brief laser plum2 ON
 * helper funtion to turn on Plum2
 * 
 */
void laser_plum2_ON(void)
{
    pwm_set_dt(&plum2_pwm, period[1],duty_cycle);
    laser_status_reg |= 1<<0;
    // laser_plum2_status = 1;
    if(pulsetimer_status == 0){
        //k_timer_start(&pulse_timer,K_MSEC(500U),K_MSEC(500U));
         k_timer_start(&pulse_timer,K_MSEC(44U),K_MSEC(44U));
    }
}

/**
 * @brief laser level ON
 * helper funtion to turn on level
 * 
 */
void laser_level_ON(void)
{
    pwm_set_dt(&level_pwm, period[1],duty_cycle);
    laser_status_reg |= 1<<2;
    // laser_level_status = 1;
    if(pulsetimer_status == 0){
       // k_timer_start(&pulse_timer,K_MSEC(500U),K_MSEC(500U));
       k_timer_start(&pulse_timer,K_MSEC(44U),K_MSEC(44U));
    }

}

/**
 * @brief laser plum1 OFF
 * helper funtion to turn off Plum1
 * 
 */
void laser_plum1_OFF(void)
{
    pwm_set_dt(&plum1_pwm, period[1] , period[1] + 1);
    gpio_pin_set_dt(&levelbias, 0);
    laser_status_reg &= ~(1<<1);
    if(laser_read_status() == 0)
        k_timer_stop(&pulse_timer);
   
}

/**
 * @brief laser plum2 ON
 * helper funtion to turn off Plum2
 * 
 */
void laser_plum2_OFF(void)
{
    pwm_set_dt(&plum2_pwm, period[1] , period[1] + 1);
    laser_status_reg &= ~(1<<0);
    if(laser_read_status() == 0)
        k_timer_stop(&pulse_timer);
    
}

/**
 * @brief laser Level OFF
 * helper funtion to turn off Level 
 * 
 */
void laser_level_OFF(void)
{
    pwm_set_dt(&level_pwm,period[1] , period[1] + 1);
    laser_status_reg &= ~(1<<2);
    if(laser_read_status() == 0)
        k_timer_stop(&pulse_timer);

}

/**
 * @brief All laser off
 * Helper function to turn off all laser from one api call
 * 
 */
void all_laser_off(void)
{
    laser_level_OFF();
    laser_plum2_OFF();
    laser_plum1_OFF();
}

/**
 * @brief default Laser on
 * Turn on the last running laser before the laser was turned off 
 * 
 * 
 * @param num read from flash with last run laser
 */
void default_laser_on(uint8_t *num)
{
    gpio_pin_set_dt(&LaserbuckEN, 1);
    duty_cycle =PWM_HZ(200000U)/40U;
    if(*num & 1)
    {
        laser_plum2_ON();
    }
    if(*num & (1<<1))
    {
        laser_plum1_ON();
    }
    if((*num & (1<<2)) || (*num == 0))
    {
        laser_level_ON();
    }
    else 
        return;
}