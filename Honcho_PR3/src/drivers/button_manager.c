/*
 *  Copyright (c) 2022, Stanley Black and Decker
 *    All rights reserved
 * Author : Kavya Jain
 * Created on : 25 Nov 2022
 */

#include <stdlib.h>
#include "button_manager.h"


// ###########################################
// # KERNEL FIFO DECLERATION 
// ###########################################
K_FIFO_DEFINE(button_fifo);

// ###########################################
// # GLOBAL VAR
// ###########################################
volatile uint8_t power_on = 0;
static uint8_t BRGT_CYC = 0; // Brightness change variable. Increment eveytime brightness change button is pushed.
uint16_t single_presskey[5] = {0x2002, 0x1001, 0x0808, 0x0804, 0x0204};
static struct button_data_t
{
    uint16_t data;
    uint16_t len;
} *buf;

// ###########################################
// # THREAD DEFINATION
// ###########################################
K_THREAD_DEFINE(button_data_manager, BUTTONMANAGER_THREAD_STACK_SIZE, button_manager, NULL, NULL,
                NULL, BUTTONMANAGER_THREAD_PRIORITY, 0, 0);

/**
 * @brief button_manager
 * Thread function for button manager. It is a blocked thread fucntion activated only when the data is pushed 
 * into FIFO from spi or bluetooth
 */
void button_manager(void)
{
    uint16_t data;
    while (1)
    {
        buf = k_fifo_get(&button_fifo, K_FOREVER);
        data = buf->data;
        // printk("data %x \n", data);
        if (lsearch((data & BUTTONTYPE_MASK), single_presskey, (int)(sizeof(single_presskey) / sizeof(*single_presskey))))
        {
            data = data & BUTTONTYPE_MASK;
            // printk("data %x \n", data);
        }
        // gpio_pin_toggle_dt(&dropled);
        printk("Button data is : 0x%x\n", data);
        if (!lowvoltage_flag )
            task_delegator(data);
        k_free(buf);
        // k_sleep(K_MSEC(1));
    }
}

/**
 * @brief task_delegator
 * Helper api function to calll required function based on watev button is pressed. 
 * 
 * @param data 2 bytes of button data code
 */
void task_delegator(uint16_t data)
{
    volatile int laser_status = 0;
    if ((power_on == 0 && data == 0x0204) || power_on == 1) //enter only if the power is off and power button is pressed.
    {

        switch (data)
        {
        case 0x2002:                                            //Brightness case                                        
            // Laser brightness reduction function
            if (laser_read_status())
            {
                if(remote_data)
                {
                    DL_Inc_U32(&DL_log.global_brightness_remote_switch_cycles);
                    remote_data =0;
                }
                else
                    DL_Inc_U32(&DL_log.global_brightness_switch_cycles);
                ++BRGT_CYC;
                if (BRGT_CYC == 255)
                    BRGT_CYC = 0;
                change_brightness(BRGT_CYC % 3);
            }
            break;

        case 0x1001:                                            //Level laser case
            // Check for battery voltage
            // Check for pendulum lock
            // if on then turn off
            if (laser_read_status() & (1 << 2))
            {
                laser_level_OFF();
                if (laser_read_status() == 0)
                {
                    gpio_pin_set_dt(&LaserbuckEN, 0);
                }
            }
            // Laser level ON.
            else
            {
                if (laser_read_status() == 0)
                {
                    gpio_pin_set_dt(&LaserbuckEN, 1);
                }
                // gpio_pin_set_dt(&levelbias, 1);
                laser_level_ON();
                if(remote_data)
                {
                    DL_Inc_U32(&DL_log.laser_1_remote_switch_cycles);
                    remote_data =0;
                }
                else
                    DL_Inc_U32(&DL_log.laser_1_switch_cycles);
            }
            laser_status = laser_read_status();
            FLASHVAR_update(&flash_mem.laser_status_mem, (uint32_t)laser_status);
            if(!(memory_corrupt &(1<<1)))
                FLASHVAR_write();

            break;

        case 0x0808:                                                //Plumb1 laser case
            // Check for battery voltage
            // Check for pendulum lock
            // if on then turn off
            if (laser_read_status() & (1 << 0))
            {
                laser_plum2_OFF();
                if (laser_read_status() == 0)
                {
                    gpio_pin_set_dt(&LaserbuckEN, 0);
                }
            }
            // Laser level ON. thread.
            else
            {
                if (laser_read_status() == 0)
                {
                    gpio_pin_set_dt(&LaserbuckEN, 1);
                }
                laser_plum2_ON();
                if(remote_data)
                {
                    DL_Inc_U32(&DL_log.laser_2_remote_switch_cycles);
                    remote_data =0;
                }
                else
                    DL_Inc_U32(&DL_log.laser_2_switch_cycles);
            }
            laser_status = laser_read_status();
            FLASHVAR_update(&flash_mem.laser_status_mem, (uint32_t)laser_status);
            if(!(memory_corrupt &(1<<1)))
                FLASHVAR_write();

            
            break;

        case 0x0804:                                                    //Plumb2 laser 
            // Check for battery voltage
            // Check for pendulum lock
            // if on then turn off
            if (laser_read_status() & (1 << 1))
            {
                laser_plum1_OFF();
                if (laser_read_status() == 0)
                {
                    gpio_pin_set_dt(&LaserbuckEN, 0);
                }
            }
            // Laser level ON. thread.
            else
            {
                if (laser_read_status() == 0)
                {
                    gpio_pin_set_dt(&LaserbuckEN, 1);
                }
                laser_plum1_ON();
                if(remote_data)
                {
                    DL_Inc_U32(&DL_log.laser_3_remote_switch_cycles);
                    remote_data =0;
                }
                else
                    DL_Inc_U32(&DL_log.laser_3_switch_cycles);
            }
            laser_status = laser_read_status();
            FLASHVAR_update(&flash_mem.laser_status_mem, (uint32_t)laser_status);
            if(!(memory_corrupt &(1<<1)))
                FLASHVAR_write();
            
            break;

        case 0x0842:                                                       //single press up motor.
            // enable vert motor driver
            if(remote_data)
            {
                DL_Inc_U32(&DL_log.z_translate_up_remote_switch);
                remote_data =0;
            }
            else
                DL_Inc_U32(&DL_log.z_translate_up_switch);
            gpio_pin_set_dt(&MotorbuckEN, 1);
            // if dir change, run backlash compensation
            //  if(motorstatus_reg & (1<<2))
            //  {
            //      backlash_compenvert();                                   @Add direction for compensation
            //  }
            vert_up();
            vert_single();
            break;

        case 0x0812:                                                        //Long press Up arrow
            // enable vert motor driver
            if(remote_data)
            {
                DL_Inc_U32(&DL_log.z_translate_up_remote_switch);
                remote_data =0;
            }
            else
                DL_Inc_U32(&DL_log.z_translate_up_switch);
            gpio_pin_set_dt(&MotorbuckEN, 1);
            // if dir change, run backlash compensation
            //  if(motorstatus_reg & (1<<2))
            //  {
            //      backlash_compenvert();
            //  }
            vert_up();
            gpio_pin_set_dt(&vertEN, 0);
            vert_ramp();
            break;

        case 0x0841:                                                    //Single press Down arrow
            // enable vert motor driver
            if(remote_data)
            {
                DL_Inc_U32(&DL_log.z_translate_down_remote_switch);
                remote_data =0;
            }
            else
                DL_Inc_U32(&DL_log.z_translate_down_switch);
            gpio_pin_set_dt(&MotorbuckEN, 1);
            // if dir change, run backlash compensation
            // if dir change, run backlash compensation
            //  if(motorstatus_reg & (1<<3))
            //  {
            //      backlash_compenvert();
            //  }
            vert_down();
            vert_single();
            break;

        case 0x0811:                                                     //Long Press down arrow
            // enable vert motor driver
            if(remote_data)
            {
                DL_Inc_U32(&DL_log.z_translate_down_remote_switch);
                remote_data =0;
            }
            else
                DL_Inc_U32(&DL_log.z_translate_down_switch);
            gpio_pin_set_dt(&MotorbuckEN, 1);
            // if dir change, run backlash compensation
            //  if(motorstatus_reg & (1<<3))
            //  {
            //      backlash_compenvert();
            //  }
            vert_down();
            gpio_pin_set_dt(&vertEN, 0);
            vert_ramp();
            break;

        case 0x0448:                                                      //Single press counterclockwise 
            // enable vert motor driver
            if(remote_data)
            {
                DL_Inc_U32(&DL_log.z_rotate_yaw_counterclockwise_remote_switch);
                remote_data =0;
            }
            else
                DL_Inc_U32(&DL_log.z_rotate_yaw_counterclockwise_switch);
            gpio_pin_set_dt(&MotorbuckEN, 1);
            // if dir change, run backlash compensation
            if (motorstatus_reg & (1 << 1))
            {
                yaw_down();
                backlash_compenyaw();
            }
            yaw_down();
            yaw_single();
            break;

        case 0x0418:                                                     //Long press counterclockwise
            // enable vert motor driver
            if(remote_data)
            {
                DL_Inc_U32(&DL_log.z_rotate_yaw_counterclockwise_remote_switch);
                remote_data =0;
            }
            else
                DL_Inc_U32(&DL_log.z_rotate_yaw_counterclockwise_switch);
            gpio_pin_set_dt(&MotorbuckEN, 1);
            // if dir change, run backlash compensation
            if (motorstatus_reg & (1 << 1))
            {
                yaw_down();
                backlash_compenyaw();
            }
            yaw_down();
            gpio_pin_set_dt(&yawEN, 0);
            // yaw_single();              //Single b4 ramp, change later @KJ
            // gpio_pin_set_dt(&yawEN, 0);
            yaw_ramp();
            break;

        case 0x0444:                                                            //Single Press clockwise 
            // enable vert motor driver
            if(remote_data)
            {
                DL_Inc_U32(&DL_log.z_rotate_yaw_clockwise_remote_switch);
                remote_data =0;
            }
            else
                DL_Inc_U32(&DL_log.z_rotate_yaw_clockwise_switch);
            gpio_pin_set_dt(&MotorbuckEN, 1);
            // if dir change, run backlash compensation
            if (motorstatus_reg & (1 << 0))
            {
                yaw_up();
                backlash_compenyaw();
            }
            yaw_up();
            yaw_single();
            break;

        case 0x0414:                                                                //Long press clockwise
            // enable vert motor driver
            if(remote_data)
            {
                DL_Inc_U32(&DL_log.z_rotate_yaw_clockwise_remote_switch);
                remote_data =0;
            }
            else
                DL_Inc_U32(&DL_log.z_rotate_yaw_clockwise_switch);
            gpio_pin_set_dt(&MotorbuckEN, 1);
            // if dir change, run backlash compensation
            if (motorstatus_reg & (1 << 0))
            {
                yaw_up();
                backlash_compenyaw();
            }
            yaw_up();
            gpio_pin_set_dt(&yawEN, 0);
            // yaw_single();              //Single b4 ramp, change later @KJ
            // gpio_pin_set_dt(&yawEN, 0);
            yaw_ramp();
            break;
        case 0x0204:                                                             //Power Button 
            // Power OFF routine.
            if (power_on == 1)
            {
                // shut off everything and sleep;
                power_on = 0;
                sys_off();
                BRGT_CYC =0;
                // sys_sleep();
                // printk("power OFF sequence \n");
            }
            else if (power_on == 0)
            {
                power_on = 1;
                sys_on();
                // printk("power On sequence \n");
            }

            break;
        default:
            break;
        }
    }
}

/**
 * @brief lsearch
 * @note Helper function for linear search 
 * 
 * @param data data to be searched
 * @param arr araay where the data is to be searched
 * @param size size of array
 * @return int o is unsuccessful 
 *              1 is successful
 */
int lsearch(uint16_t data, uint16_t arr[], int size)
{
    for (int i = 0; i < size; i++)
    {
        // printk("arr is %x and data is %x \n", arr[i], data);
        if (arr[i] == data)
        {
            return 1;
        }
    }
    return 0;
}