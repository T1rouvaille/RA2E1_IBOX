/*
*  Copyright (c) 2022, Stanley Black and Decker 
*    All rights reserved
* Author : Kavya Jain 
* Created on : 25 Nov 2022
*/

#include "motor_control.h"

//private variables
static volatile uint32_t vertmotorfreq, yawmotorfreq;
volatile uint8_t motorstatus_reg = 0;                           //register to indicate the dir of the motor run        | IsMotorMoving | Resvd | Resvd | Resvd | VertUP | VertDown | YawUP | YawDown |

//Timer defination
K_TIMER_DEFINE(vert_pulse_timer, vert_timer_complete, NULL);
K_TIMER_DEFINE(yaw_pulse_timer, yaw_timer_complete, NULL);
K_TIMER_DEFINE(vertmtrramp_timer,vertmtrramp_expiry, NULL);
K_TIMER_DEFINE(yawmtrramp_timer, yawmtrramp_expiry, NULL);
K_TIMER_DEFINE(mtrsafetytimeout, mtrsafetytimeout_expiry, NULL);

void vert_timer_complete(struct k_timer *timer_id)
{
    pwm_set_dt(&vert_motor_step,0,0);
    gpio_pin_set_dt(&vertEN, 1);
}

void yaw_timer_complete(struct k_timer *timer_id)
{
    pwm_set_dt(&yaw_motor_step,0,0);
    gpio_pin_set_dt(&yawEN, 1);
}

void vertmtrramp_expiry(struct k_timer *timer_id)
{
    vertmotorfreq += 100;
    // printk("value of freq %u", vertmotorfreq);
    pwm_set_dt(&vert_motor_step, PWM_HZ(vertmotorfreq), PWM_HZ(vertmotorfreq)/ 2U);
    if(vertmotorfreq == 400)
    {
        k_timer_stop(&vertmtrramp_timer);
    }
}

void yawmtrramp_expiry(struct k_timer *timer_id)
{
    yawmotorfreq += 100;
    // printk("value of freq %u", vertmotorfreq);                                                                                                                
    pwm_set_dt(&yaw_motor_step, PWM_HZ(yawmotorfreq), PWM_HZ(yawmotorfreq)/ 2U);
    if(yawmotorfreq == 400)
    {
        k_timer_stop(&yawmtrramp_timer);
    }
}

void mtrsafetytimeout_expiry(struct k_timer *timer_id)
{                                                                                          
    motor_stop();
}

/*******************************************************************************************************************/


void vert_single()
{
    //gpio_pin_set_dt(&vert_dir, 1);
    gpio_pin_set_dt(&vertEN, 0);
    pwm_set_dt(&vert_motor_step,PWM_HZ(80U), PWM_HZ(80U) /2U);//200
    k_timer_start(&vert_pulse_timer, K_MSEC(500), K_NO_WAIT);                            // 1/PWM_FREQ * (No. of steps) set to 100steps 
    //k_timer_start(&vert_pulse_timer, K_MSEC(100), K_NO_WAIT);  
}

void yaw_single()
{
    //gpio_pin_set_dt(&vert_dir, 0);
    gpio_pin_set_dt(&yawEN, 0);
    pwm_set_dt(&yaw_motor_step,PWM_HZ(100), PWM_HZ(100U) /2U);//100
    k_timer_start(&yaw_pulse_timer, K_MSEC(10), K_NO_WAIT);                              // 1/PWM_FREQ * (No. of steps)
}

void vert_ramp()
{
    motorstatus_reg |= 0x80;
    vertmotorfreq = 100;
    pwm_set_dt(&vert_motor_step, PWM_HZ(262U), PWM_HZ(262U)/ 2U);
    k_timer_start(&mtrsafetytimeout, K_SECONDS(300), K_NO_WAIT);

    // pwm_set_dt(&vert_motor_step, PWM_HZ(vertmotorfreq), PWM_HZ(vertmotorfreq)/ 2U);
    // k_timer_start(&vertmtrramp_timer, K_MSEC(2000), K_MSEC(2000));
}

void yaw_ramp()
{
    motorstatus_reg |= 0x80;
  //  pwm_set_dt(&yaw_motor_step,PWM_HZ(100U), PWM_HZ(100U) /2U);
    // k_timer_start(&yaw_pulse_timer, K_MSEC(20), K_NO_WAIT);   
 //   k_busy_wait(30000); 
 //   pwm_set_dt(&yaw_motor_step,0,0);  
    yawmotorfreq = 100; //100
    pwm_set_dt(&yaw_motor_step, PWM_HZ(262U), PWM_HZ(262U)/ 2U);
   // pwm_set_dt(&yaw_motor_step, PWM_HZ(yawmotorfreq), PWM_HZ(yawmotorfreq)/ 2U);
 //   k_timer_start(&yawmtrramp_timer, K_MSEC(2000), K_MSEC(2000));
    k_timer_start(&mtrsafetytimeout, K_SECONDS(300), K_NO_WAIT);
/*
motorstatus_reg |= 0x80;
yawmotorfreq = 100; //100
pwm_set_dt(&yaw_motor_step,PWM_HZ(262U), PWM_HZ(262U) /2U);
k_timer_start(&mtrsafetytimeout, K_SECONDS(300), K_NO_WAIT);*/
}

void motor_stop()
{
    int err;
    printk("Motor Stopped\n");
    motorstatus_reg &= ~(0x80);
    err = gpio_pin_set_dt(&MotorbuckEN, 0);   
    if(err)
        printk("Error occured");
    
    err = gpio_pin_set_dt(&yawEN, 1);
    if(err)
        printk("Error occured");
    
    err = gpio_pin_set_dt(&vertEN, 1);
    if(err)
        printk("Error occured");

    k_timer_stop(&vertmtrramp_timer);
    k_timer_stop(&yawmtrramp_timer);

    err = pwm_set_dt(&yaw_motor_step, 0, 0);
    if(err)
        printk("Error occured");

    err = pwm_set_dt(&vert_motor_step, 0, 0);
    if(err)
        printk("Error occured");
    
    k_timer_stop(&mtrsafetytimeout);
}

void motor_init()
{
    int ret;
    ret = gpio_pin_configure_dt(&motor_dir,GPIO_OUTPUT_LOW);
  
    ret = gpio_pin_configure_dt(&vertEN,GPIO_OUTPUT_HIGH);
    if (ret < 0) {
		return;
	}
    ret = gpio_pin_configure_dt(&yawEN,GPIO_OUTPUT_HIGH);
    if (ret < 0) {
		return;
    }

    pwm_set_dt(&vert_motor_step,0,0);
    pwm_set_dt(&yaw_motor_step, 0,0);
 
}

void backlash_compenvert()
{
    pwm_set_dt(&vert_motor_step, PWM_HZ(400), PWM_HZ(400)/ 2U);
    //pwm_set_dt(&vert_motor_step, PWM_HZ(300), PWM_HZ(300)/ 2U);
   // k_sleep(K_MSEC(625));
    k_sleep(K_MSEC(60));
    pwm_set_dt(&vert_motor_step, 0, 0);
}

void backlash_compenyaw()
{
    gpio_pin_set_dt(&yawEN, 0);
    pwm_set_dt(&yaw_motor_step, PWM_HZ(400), PWM_HZ(400)/ 2U);
    //k_sleep(K_MSEC(375));
    k_sleep(K_MSEC(850));
    pwm_set_dt(&yaw_motor_step, 0, 0);
}
// /********************************************************************************************************/

// void SWI1_EGU1_IRQHandler(void)
// {   
//     // Clear SWI Event. 
//     NRF_EGU1->EVENTS_TRIGGERED[0] = 0;

//     motor_stop();

// }

// void init_egu()
// {  
//     // Enable TRIGGERED0 interrupt
//     NRF_EGU1->INTENSET = EGU_INTENSET_TRIGGERED0_Enabled << EGU_INTENSET_TRIGGERED0_Pos;
    
//     // Set EGU3 Interrupt pririty to 6.
//     NVIC_SetPriority(SWI1_EGU1_IRQn,6);
    
//     //Enable SWI3_EGU3 interrupt. 
//     NVIC_EnableIRQ(SWI1_EGU1_IRQn);
// }

// /***********************************************************************************************************/

