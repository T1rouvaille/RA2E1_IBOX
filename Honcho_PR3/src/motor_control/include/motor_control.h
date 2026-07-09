/*
*  Copyright (c) 2022, Stanley Black and Decker 
*    All rights reserved
* Author : Kavya Jain 
* Created on : 25 Nov 2022
*/

#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include <zephyr.h>
#include <zephyr/kernel.h>
#include <device.h>
#include <drivers/pwm.h>
#include <drivers/gpio.h>

#include "gpio_def.h"
#include "soc_laserblink.h"

#define VERTMOTORPWM_NODE                               DT_NODELABEL(vert_motor_step)
#define YAWMOTORPWM_NODE                                DT_NODELABEL(yaw_motor_step)
#define MOTORDIR_NODE                                   DT_ALIAS(motordir)
#define VERTEN_NODE                                     DT_ALIAS(motorverten)
#define YAWEN_NODE                                      DT_ALIAS(motoryawen)
#define MOTOR_THREAD_STACK_SIZE                         1024
#define MOTOR_THREAD_PRIORITY                           7
#define BACKLASHCOMP_SPEED                              400

#if !DT_NODE_HAS_STATUS(VERTMOTORPWM_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif

#if !DT_NODE_HAS_STATUS(YAWMOTORPWM_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif

#if !DT_NODE_HAS_STATUS(MOTORDIR_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif

#if !DT_NODE_HAS_STATUS(VERTEN_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif

#if !DT_NODE_HAS_STATUS(YAWEN_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif


static const struct pwm_dt_spec vert_motor_step = PWM_DT_SPEC_GET(VERTMOTORPWM_NODE);
static const struct pwm_dt_spec yaw_motor_step = PWM_DT_SPEC_GET(YAWMOTORPWM_NODE);
static const struct gpio_dt_spec motor_dir = GPIO_DT_SPEC_GET(MOTORDIR_NODE, gpios);
static const struct gpio_dt_spec vertEN = GPIO_DT_SPEC_GET(VERTEN_NODE, gpios);
static const struct gpio_dt_spec yawEN = GPIO_DT_SPEC_GET(YAWEN_NODE, gpios);

extern volatile uint8_t motorstatus_reg;

void vert_timer_complete(struct k_timer *timer_id);
void yaw_timer_complete(struct k_timer *timer_id);
void vertmtrramp_expiry(struct k_timer *timer_id);
void yawmtrramp_expiry(struct k_timer *timer_id);
void mtrsafetytimeout_expiry(struct k_timer *timer_id);

void SWI3_EGU3_IRQHandler(void);
void SWI3_EGU3_IRQHandler(void);

void motor_init(void);
void vert_single(void);
void yaw_single(void);
void vert_ramp(void);
void yaw_ramp(void);
void motor_stop();
void backlash_compenvert(void);
void backlash_compenyaw(void);

static inline void vert_up()
{
    motorstatus_reg |= 1<<3;
    motorstatus_reg &= ~(1<<2);
    gpio_pin_set_dt(&motor_dir , 1);
}

static inline void vert_down()
{
    motorstatus_reg |= 1<<2;
    motorstatus_reg &= ~(1<<3);
    gpio_pin_set_dt(&motor_dir , 0);
}

static inline void yaw_up()
{
    motorstatus_reg |= 1<<1;
    motorstatus_reg &= ~(1<<0);
    gpio_pin_set_dt(&motor_dir , 1);
}

static inline void yaw_down()
{
    motorstatus_reg |= 1<<0;
    motorstatus_reg &= ~(1<<1);
    gpio_pin_set_dt(&motor_dir , 0);
}

#endif  /* MOTOR_CONTROL_H */