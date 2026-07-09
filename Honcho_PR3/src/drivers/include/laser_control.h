/*
*  Copyright (c) 2022, Stanley Black and Decker 
*    All rights reserved
* Author : Kavya Jain 
* Created on :  13 Jan 2023
*/

#ifndef LASER_CONTROL_H
#define LASER_CONTROL_H

#include <zephyr.h>
#include <zephyr/kernel.h>
#include <device.h>
#include <drivers/pwm.h>
#include <drivers/gpio.h>

#include "gpio_def.h"


#define plum1pwm_NODE        DT_NODELABEL(plum1_pwm)
#define plum2pwm_NODE        DT_NODELABEL(plum2_pwm)
#define levelpwm_NODE        DT_NODELABEL(level_pwm)
#define lvlbias_NODE         DT_ALIAS(lvlbias)
#define plum1bias_NODE       DT_ALIAS(plm1bias)
#define plum2bias_NODE       DT_ALIAS(plm2bias)

static const struct pwm_dt_spec plum1_pwm = PWM_DT_SPEC_GET(plum1pwm_NODE);
static const struct pwm_dt_spec plum2_pwm = PWM_DT_SPEC_GET(plum2pwm_NODE);
static const struct pwm_dt_spec level_pwm = PWM_DT_SPEC_GET(levelpwm_NODE);
static const struct gpio_dt_spec levelbias = GPIO_DT_SPEC_GET(lvlbias_NODE, gpios);
static const struct gpio_dt_spec plum1bias = GPIO_DT_SPEC_GET_OR(plum1bias_NODE, gpios, 
                                        {0});
static const struct gpio_dt_spec plum2bias = GPIO_DT_SPEC_GET_OR(plum2bias_NODE, gpios, 
                                        {0});


extern volatile uint32_t duty_cycle; 
extern struct k_timer pulse_timer;                                  //Duty Cycle variable for the laser PWM
// static uint32_t duty_cycle_array[4] = {PWM_HZ(100000U)/100U, PWM_HZ(100000U)/3U,PWM_HZ(100000U)/ 2U};               //Duty cycle brightness change cyclic array. val from array get fed to dutycycle and used by all lasers.
static uint32_t duty_cycle_array[4] = {1, 2500, 3250};               //100%, 50%, 35%. 10^9/freq * 100-duty_req(in %) i.e to 35%duty. (10^9/200000 * (100-35)/100) -> 5000 * 0.65 ->3250
// static uint32_t duty_cycle_array[4] = {1, 58997, 76470};   
extern volatile uint8_t laser_status_reg; 
extern volatile uint8_t variable;       


void laser_init(void);
void laser_plum1_ON(void);
void laser_plum2_ON(void);
void laser_level_ON(void);
void laser_plum1_OFF(void);
void laser_plum2_OFF(void);
void laser_level_OFF(void);
void all_laser_off(void);
void default_laser_on(uint8_t *num);

static inline void change_brightness(uint8_t i)
{
    k_timer_stop(&pulse_timer);
    duty_cycle = duty_cycle_array[i];// start pwm again
    printk("Brightness is %u", duty_cycle);
    k_timer_start(&pulse_timer,K_MSEC(500U),K_MSEC(500U));
}

volatile inline uint8_t laser_read_status(void)
{
    return laser_status_reg;
}

#endif /* LASER_CONTROL_H */