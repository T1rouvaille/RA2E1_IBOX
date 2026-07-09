/*
*  Copyright (c) 2022, Stanley Black and Decker 
*    All rights reserved
* Author : Kavya Jain 
* Created on : 13 July 2023
*/

#include "soc_laserblink.h"
static int i =0, pendlaserpulse_cntr = 0;
volatile bool lowvoltage_flag = 0;
volatile bool pendblink_flag =0;
volatile bool Outoflevel_blink_flag = 0;
static bool timer_start_flag = 0;
struct k_timer pendlckpulse_blink_timer;

void soc1blink_timer_expiry(struct k_timer *timer_id)
{
    gpio_pin_toggle_dt(&soc1);
}

void OneHz_laserpulse_expiry(struct k_timer *timer_id)
{
    gpio_pin_toggle_dt(&LaserbuckEN);
}

void OneHz_laserpulse_stop(struct k_timer *timer_id)
{
    gpio_pin_set_dt(&LaserbuckEN, 1);
}


void pendlckpulse_blink_timer_expiry(struct k_timer *timer_id)
{
    pendlaserpulse_cntr++;
    gpio_pin_toggle_dt(&LaserbuckEN);
    if(pendlaserpulse_cntr == 5)
    {
        k_timer_stop(&pendlckpulse_blink_timer);
        pendblink_flag =0;
    }
}

void pendulumlocked_blink_timer_expiry(struct k_timer *timer_id)
{
    pendlaserpulse_cntr =0;
    pendblink_flag = 1;
    gpio_pin_toggle_dt(&LaserbuckEN);
    k_timer_start(&pendlckpulse_blink_timer, K_MSEC(250), K_MSEC(250));

}

K_TIMER_DEFINE(soc1blink_timer, soc1blink_timer_expiry, NULL);
K_TIMER_DEFINE(OneHz_laserpulse, OneHz_laserpulse_expiry, OneHz_laserpulse_stop);
K_TIMER_DEFINE(pendulumlocked_blink_timer, pendulumlocked_blink_timer_expiry, NULL);
K_TIMER_DEFINE(pendlckpulse_blink_timer, pendlckpulse_blink_timer_expiry, NULL);


void socled_manager(uint8_t soc_reg)
{
    switch(soc_reg)
    {
        case 0x01 : 
            DL_Inc_U32(&DL_log.shutdown_battery_undervoltage);
            k_sem_give(&datalogwrite_semaphore);
            k_sleep(K_USEC(500));
            sys_off();
            
            break;
        case 0x02 :
            gpio_pin_set_dt(&soc1, 0);
            gpio_pin_set_dt(&soc2, 0);
            gpio_pin_set_dt(&soc3, 0);
            if(i==0)
            {
                k_timer_start(&soc1blink_timer, K_MSEC(1500), K_MSEC(1500));
                i = 1;
            }

            break;
        case 0x04 :
            gpio_pin_set_dt(&soc1, 1);
            gpio_pin_set_dt(&soc2, 0);
            gpio_pin_set_dt(&soc3, 0);
            
            break;
        case 0x08 :
            gpio_pin_set_dt(&soc1, 1);
            gpio_pin_set_dt(&soc2, 1);
            gpio_pin_set_dt(&soc3, 0);
            
            break;
        case 0x16 :
            gpio_pin_set_dt(&soc1, 1);
            gpio_pin_set_dt(&soc2, 1);
            gpio_pin_set_dt(&soc3, 1);
            
            break;
        // case 0x32:
        //     printk("i m under 9v \n");
        //     // sys_sleep();
        //     break;
        
        
        default:
            break;

    }
}


void Outoflevel_blink(double z, int count)
{
    // printk("entered here \n");
    if((z >= 3.1  || Unit_overtuned_flag) && Outoflevel_blink_flag == 0 && count>=4)
    {
        // printk("timer started ");
        k_timer_start(&OneHz_laserpulse, K_MSEC(500), K_MSEC(500));
        timer_start_flag = 1;
        Outoflevel_blink_flag = 1;
        anglethread_sleep_time = 100;
        count =0;
    }
    else if(z <= 3.0 && !Unit_overtuned_flag && count>=4)
    {
        Stop_Acc_timers();
        timer_start_flag = 0;
        Outoflevel_blink_flag =0;
        anglethread_sleep_time = 150;
    }
    
}

void Stop_Acc_timers()
{
    k_timer_stop(&OneHz_laserpulse);
}