/*
*  Copyright (c) 2022, Stanley Black and Decker 
*    All rights reserved
* Author : Kavya Jain 
* Created on : 21 Spet 2023
*/

#include "acc_IIM42351.h"

// ###########################################
// # GLOBAL VAR
// ###########################################

uint8_t who_am_i = 0;                                                           //Temp variable to save the value of i2c
uint8_t read_reg = 0, drop_flag =0;       
volatile uint8_t Unit_overtuned_flag =0;                                        //Flag to notify the state of Z axis, / overturned Unit
double xgval = 0,ygval =0, zgval = 0;
double xprev =0, yprev = 0, zprev=0;
volatile double x_zero_offset = 0, y_zero_offset = 0, z_zero_offset = 0;
uint16_t anglethread_sleep_time = 150, tapdropthread_sleep_time = 500;          //Variable for sleep time 
volatile int accinit_failed = 0;                                                //variable to determine the state of acc initialize
volatile int start_i2crecover =0;                                               //variable to determine the state of i2c bus

// ###########################################
// # CONSTANTS
// ###########################################
const double g_sensitivity = (double)4000/65536;

//------------------------------------------------------------------------------------------------------------------------
// ###########################################
// # KERNEL TIMER EXPIRY FUNCTIONS
// ###########################################

/**
 * @brief timer fucntion to handle the drop led blinking
 * 
 * @note  this timer is no linger use.  
 * 
 * @param timer_id 
 */
// void dropblink_timer_expiry(struct k_timer *timer_id)
// {
//     gpio_pin_toggle_dt(&dropled);
// }

// K_TIMER_DEFINE(dropblink_timer, dropblink_timer_expiry, NULL);

// ###########################################
// # FUCTIONS
// ###########################################

/**
 * @brief helper fucntio to implement the low pas filter 
 * 
 * @param arr   input array on which low pass filter is appllied
 * @param input new sample comming from accelerometer
 * @param index var to track the position in arr
 * @param sum summ of the arr to take avg
 * @return uint32_t  output the filtered value.
 */

static uint32_t low_pass_filter(uint32_t *arr,uint16_t input, uint8_t index, uint32_t *sum)
{
    uint32_t output =0;
    *sum = *sum - arr[index%16] + input;
    arr[index%16] = input;
    output = (*sum>>4);
    return output;

}


//Calculation for X,Y,Z values for IIM42531
/*********************************************************************************************************************************************************************/

/**
 * @brief helper function to to read the x raw value and convert to gval 
 * 
 */
static void read_x_angle()
{
    int ret;
    uint16_t temp;
    double x_sum=0;
    
    ret = i2c_reg_read_byte(i2c_dev, ACC_ADR,
                                        0x1F, &who_am_i);
    temp = who_am_i;
    ret = i2c_reg_read_byte(i2c_dev, ACC_ADR,
                                    0x20, &who_am_i);
    temp = ((temp <<8) | (who_am_i));
    // printk("%u \t",temp);
    xgval = (double)(temp*g_sensitivity);
    xgval = xgval < 2000 ? xgval : xgval - 4000;  

    
    // printk("%u\t",x_sum);
    // xgval = (double)(x_sum*g_sensitivity);
    // xgval = xgval < 2000 ? xgval : xgval - 4000;

    // // printk("%f, %u\n", exponential_avg_filter(xval, MA_xsum1), filtered_xval); //, who_am_i);//, gval, roll);
    // printk("%f\t",xgval);
}


/**
 * @brief helper function to to read the y raw value and convert to gval 
 * 
 */

static void read_y_angle()
{
     int ret;
    uint16_t temp;
    double y_sum = 0;

    ret = i2c_reg_read_byte(i2c_dev, ACC_ADR,
                                        0x21, &who_am_i);
    temp = who_am_i;
    ret = i2c_reg_read_byte(i2c_dev, ACC_ADR,
                                    0x22, &who_am_i);
    temp = ((temp <<8) | (who_am_i)); 
    ygval =(double) (temp*g_sensitivity);
    ygval = ygval < 2000 ? ygval : ygval - 4000;

    
    // ygval = (double)(y_sum*g_sensitivity);
    // ygval = ygval < 2000 ? ygval : ygval - 4000;
    // printk("%f\t", ygval);// who_am_i);

}

/**
 * @brief helper function to to read the z raw value and convert to gval 
 * 
 */
static uint16_t read_z_angle()
{
    int ret;
    uint16_t temp;
    double z_sum = 0;
    
    ret = i2c_reg_read_byte(i2c_dev, ACC_ADR,
                                        0x23, &who_am_i);
    temp = who_am_i;
    ret = i2c_reg_read_byte(i2c_dev, ACC_ADR,
                                    0x24, &who_am_i);
    temp = ((temp <<8) | (who_am_i));// z_zero_offset;

    // Unit_overtuned_flag = temp < 32768 ? 0 : 1;
    zgval =(double) (temp*g_sensitivity);
    zgval = zgval < 2000 ? zgval : zgval - 4000;
    return temp;
    // printk("z value is %u \t", temp);
    // Unit_overtuned_flag = zgval > 0 ? 0 : 1;
    // Unit_overtuned_flag = z_sum < 32768 ? 0 : 1;
    // zgval =(double) (z_sum*g_sensitivity);
    // zgval = zgval < 2000 ? zgval : zgval - 4000;
    // printk("%f \n",zgval);
    // return z_sum;
    // printk("z value is %u \t", temp);

}

/**
 * @brief API fucntion read the raw value of X
 * 
 * @param temp Pointer to variable where the value to be saved.
 * @return int Pointer with the x raw value
 */
static int read_xraw_val(uint16_t *temp)
{
    uint16_t rand =0;
    int ret;
    ret = i2c_reg_read_byte(i2c_dev, ACC_ADR,0x1F, &who_am_i);
    if (ret != 0)
    {
        // sys_reboot(SYS_REBOOT_WARM);
        printk("i2c error \n");
        return -1;
    }
    rand = who_am_i;
    ret = i2c_reg_read_byte(i2c_dev, ACC_ADR,0x20, &who_am_i);
    if (ret != 0)
    {
        // sys_reboot(SYS_REBOOT_WARM);
        printk("i2c error \n");
        return -1;
    }
    rand = ((rand <<8) | (who_am_i));
    *temp = rand;
    return 0;
}


/**
 * @brief API fucntion read the raw value of Y
 * 
 * @param temp Pointer to variable where the value to be saved.
 * @return int Pointer with the x raw value
 */
static int read_yraw_val(uint16_t *temp)
{
    uint16_t rand =0;
    int ret;
    ret = i2c_reg_read_byte(i2c_dev, ACC_ADR,0x21, &who_am_i);
    if (ret != 0)
    {
        // sys_reboot(SYS_REBOOT_WARM);
        printk("i2c error \n");
        return -1;
    }
    rand = who_am_i;
    ret = i2c_reg_read_byte(i2c_dev, ACC_ADR,0x22, &who_am_i);
    if (ret != 0)
    {
        // sys_reboot(SYS_REBOOT_WARM);
        printk("i2c error \n");
        return -1;
    }
    rand = ((rand <<8) | (who_am_i));
    *temp = rand;
    return 0;
}

/**
 * @brief API fucntion read the raw value of Z
 * 
 * @param temp Pointer to variable where the value to be saved.
 * @return int Pointer with the x raw value
 */
static int read_zraw_val(uint16_t *temp)
{
    uint16_t rand =0;
    int ret;
    ret = i2c_reg_read_byte(i2c_dev, ACC_ADR,0x23, &who_am_i);
    if (ret != 0)
    {
        // sys_reboot(SYS_REBOOT_WARM);
        printk("i2c error \n");
        return -1;
    }
    rand = who_am_i;
    ret = i2c_reg_read_byte(i2c_dev, ACC_ADR,0x24, &who_am_i);
    if (ret != 0)
    {
        // sys_reboot(SYS_REBOOT_WARM);
        printk("i2c error \n");
        return -1;
    }
    rand = ((rand <<8) | (who_am_i));
    *temp = rand;
    return 0;
}

// static uint16_t read_yraw_val()
// {
//     uint16_t return_val =0;
//     int ret;
//     ret = i2c_reg_read_byte(i2c_dev, ACC_ADR,0x21, &who_am_i);
//     while (ret != 0)
//     {
//         // sys_reboot(SYS_REBOOT_WARM);
//         printk("i2c error \n");
//     }
//     return_val = who_am_i;
//     ret = i2c_reg_read_byte(i2c_dev, ACC_ADR,0x22, &who_am_i);
//     while (ret != 0)
//     {
//         // sys_reboot(SYS_REBOOT_WARM);
//         printk("i2c error \n");
//     }
//     return_val = ((return_val <<8) | (who_am_i));
//     return return_val;
// }
// static uint16_t read_zraw_val()
// {
//     uint16_t return_val =0;
//     int ret;
//     ret = i2c_reg_read_byte(i2c_dev, ACC_ADR,0x23, &who_am_i);
//     while (ret != 0)
//     {
//         // sys_reboot(SYS_REBOOT_WARM);
//         printk("i2c error \n");
//     }
//     return_val = who_am_i;
//     ret = i2c_reg_read_byte(i2c_dev, ACC_ADR,0x24, &who_am_i);
//     while (ret != 0)
//     {
//         // sys_reboot(SYS_REBOOT_WARM);
//         printk("i2c error \n");
//     }
//     return_val = ((return_val <<8) | (who_am_i));
//     return return_val;
// }
/*******************************************************************************************************/
// MIn and MAx elimination filter implementation 

// static void read_raw_val()
// {
//     int ret;
//     uint16_t temp_x =0, temp_y =0, temp_z =0;
//     double z_sum =0, x_sum=0, y_sum=0; 
//     double xmax =0,xmin =6000;
//     double ymax =0,ymin =6000;
//     double zmax =0,zmin =6000;

//     for(int i =0; i< 34; i++)
//     {
//         // ret = i2c_reg_read_byte(i2c_dev, ACC_ADR,0x1F, &who_am_i);
//         // temp_x = who_am_i;
//         // ret = i2c_reg_read_byte(i2c_dev, ACC_ADR,0x20, &who_am_i);
//         temp_x = read_xraw_val();
//         xgval = (double)(temp_x*g_sensitivity);
//         xgval = xgval < 2000 ? xgval : xgval - 4000;
//         if(xmax < xgval)
//             xmax = xgval;
//         if(xmin > xgval)
//             xmin = xgval;
//         // ret = i2c_reg_read_byte(i2c_dev, ACC_ADR,0x21, &who_am_i);
//         // temp_y = who_am_i;
//         // ret = i2c_reg_read_byte(i2c_dev, ACC_ADR,0x22, &who_am_i);
//         // temp_y = ((temp_y <<8) | (who_am_i));
//         temp_y = read_yraw_val();
//         ygval = (double)(temp_y*g_sensitivity);
//         ygval = ygval < 2000 ? ygval : ygval - 4000;
//         if(ymax < ygval)
//             ymax = ygval;
//         if(ymin > ygval)
//             ymin = ygval;
//         // ret = i2c_reg_read_byte(i2c_dev, ACC_ADR,0x23, &who_am_i);
//         // temp_z = who_am_i;
//         // ret = i2c_reg_read_byte(i2c_dev, ACC_ADR,0x24, &who_am_i);
//         // temp_z = ((temp_z <<8) | (who_am_i));
//         temp_z = read_zraw_val();
//         zgval =(double) (temp_z*g_sensitivity);
//         zgval = zgval < 2000 ? zgval : zgval - 4000;
//         if(zmax < zgval)
//             zmax = zgval;
//         if(zmin > zgval)
//             zmin = zgval;
        
//         x_sum = x_sum + xgval;
//         y_sum = y_sum +ygval;
//         z_sum += zgval;
//         // firstrun_startup =1;
//         k_busy_wait(5);
//         // printk("x %f,  y %f  \n", xgval, ygval);
//     }
//     x_sum = x_sum - xmax - xmin;
//     y_sum = y_sum - ymax - ymin;
//     z_sum = z_sum - zmax - zmin;
    
//     xgval = x_sum /32;
//     ygval = y_sum /32;
//     zgval = z_sum /32;

//     // printk("x %f,  y %f  \n", xgval, ygval);
//     // printk("roll %0.5f pitch %0.5f,yaw%0.5f\n", xgval, ygval ,zgval);
// }
/*******************************************************************************************************************/

// static int firstrun_prev_val_init()
// {
//     int ret;
//     uint16_t temp_x =0, temp_y =0, temp_z =0;
//     printk("entered here\n");
//     for(int i =0; i< 32; i++)
//     {

//         // temp_x = read_xraw_val();
//         ret = read_xraw_val(&temp_x);
//         if(ret < 0)
//             return -1;
        
//         // temp_x = 32700;
//         xgval = (double)(temp_x*g_sensitivity);
//         xgval = xgval < 2000 ? xgval : xgval - 4000;
//         xprev = xprev + xgval;

//         // temp_y = read_yraw_val();
//         ret = read_yraw_val(&temp_y);
//         if(ret < 0)
//             return -1;
        
//         // temp_y = 32700;
//         ygval = (double)(temp_y*g_sensitivity);
//         ygval = ygval < 2000 ? ygval : ygval - 4000;
//         yprev = yprev +ygval;

//         // temp_z = read_zraw_val();
//         ret = read_zraw_val(&temp_z);
//         if(ret < 0)
//             return -1;    

//         // temp_z = 32700;
//         zgval =(double) (temp_z*g_sensitivity);
//         zgval = zgval < 2000 ? zgval : zgval - 4000;
//         zprev += zgval;
//         k_busy_wait(5);
//         // printk("x %f,  y %f  \n", xgval, ygval);
//     }
//     xprev = xprev /32;
//     yprev = yprev /32;
//     zprev = zprev /32;

//     xgval = 0; ygval = 0; zgval = 0;
//     printk("entered here\n");
//     return 0;
// }

/**
 * @brief Read the raw value of x, y, and z axis and apply filtering to 
 * removes any values > 100mg on difference of current value vs previous value. 
 * Tajes an avg of valid samples over a window of 32 samples.
 * 
 * @return 2 is successful 
 * @return -1 is i2c encountered an error
 */
static int read_raw_val()
{
    int ret;
    uint16_t temp_x =0, temp_y =0, temp_z =0;
    double z_sum =0, x_sum=0, y_sum=0; 
    int zeroxcount = 0, zeroycount = 0, zerozcount =0 ;
    for(int i =0; i< 32; i++)
    {
        ret = read_xraw_val(&temp_x);
        if(ret < 0)
            return -1;
        // temp_x = 32700;
        xgval = (double)(temp_x*g_sensitivity);
        xgval = xgval < 2000 ? xgval : xgval - 4000;
        if(fabs(xprev - xgval) >100)
        {
            // printk("diff x is %f \t", fabs(xprev - xgval));
            xprev = xgval;
            xgval = 0;
            zeroxcount++;
        }
        // temp_y = read_yraw_val();
        ret = read_yraw_val(&temp_y);
        if(ret < 0)
            return -1;
        // temp_y = 32700;
        ygval = (double)(temp_y*g_sensitivity);
        ygval = ygval < 2000 ? ygval : ygval - 4000;
        if(fabs(yprev - ygval) >100)
        {
            // printk("diff y is %f \t", fabs(yprev - ygval));
            yprev = ygval;
            ygval = 0;
            zeroycount++;
        }

        ret = read_zraw_val(&temp_z);
        if(ret < 0)
            return -1;    

        // temp_z = read_zraw_val();
        // temp_z = 32700;
        zgval =(double) (temp_z*g_sensitivity);
        zgval = zgval < 2000 ? zgval : zgval - 4000;
        if(fabs(zprev - zgval) >100)
        {
            // printk("diff z is %f \n", fabs(zprev - zgval));
            zprev = zgval;
            zgval = 0;
            zerozcount++;
        }

        x_sum += xgval;
        y_sum += ygval;
        z_sum += zgval;
        
        k_sleep(K_USEC(5));
        // printk("x %f,  y %f  \n", xgval, ygval);
    }

    
    xgval = x_sum /(32-zeroxcount);
    ygval = y_sum /(32-zeroycount);
    zgval = z_sum /(32-zerozcount);
    Unit_overtuned_flag = zgval > 0 ? 0 : 1;

   
    // printk("x %f,  y %f  \n", xgval, ygval);
    // printk("roll %0.5f pitch %0.5f,yaw%0.5f\n", xgval, ygval ,zgval);
     return 2;
}

/**
 * @brief Caluculates the roll angle basedon y ans z g values 
 *        using the maths from coordinate geometry 
 * 
 * @return double x angle value 
 */
double x_angle_calc()
{
    
    long y2 = (ygval * ygval);
    long z2 = (zgval * zgval);
    
    double x_angle = sqrt(y2 + z2);
    x_angle = xgval / x_angle;
    // printk("x angle is %ld \t", x_angle);
    x_angle = atan(x_angle);
    x_angle = (x_angle*180.00) / PI;
    x_angle = floorf(x_angle*100)/100;              //To round the decimal to 2 decimal places.
    // printk("x angle is %.2f \t", x_angle);
    return x_angle;
}

/**
 * @brief Caluculates the pitch angle basedon x ans z g values 
 *        using the maths from coordinate geometry 
 * 
 * @return double y angle value 
 */
double y_angle_calc()
{
    long x2 = (xgval * xgval);
    long z2 = (zgval * zgval);
    
    double y_angle = sqrt(x2 + z2);
    y_angle = ygval / y_angle;
    // printk("x angle is %ld \t", x_angle);
    y_angle = atan(y_angle);
    y_angle = (y_angle*180.00) / PI;
    y_angle = floorf(y_angle*100)/100;                              //To round the decimal to 2 decimal places.
    // printk("y angle is %.2f \n", y_angle);
    return y_angle;
}

/**
 * @brief Calculates the yaw angle basedon x ans y g values 
 *        using the maths from coordinate geometry 
 * 
 * @return double z angle value 
 */
double z_angle_calc()
{
    long x2 = (xgval * xgval);
    long y2 = (ygval * ygval);
    
    double z_angle = sqrt(x2 + y2);
    z_angle = z_angle / zgval;
    // printk("x angle is %ld \t", x_angle);
    z_angle = atan(z_angle);
    z_angle = (z_angle*180.00) / PI;
    z_angle = floorf(z_angle*100)/100;                              //To round the decimal to 2 decimal places.
    // printk("z angle is %.2f \n", z_angle);
    return z_angle;
}

/**
 * @brief helper function to average the angles
 *        Takes an average of 16 angles. 
 * 
 * @param arr arra over which average will be taken 
 * @return double  return the abf of angles.  
 */
static double angle_average(double *arr)
{
    double sum =0;
    for(int i =0 ;i<16 ; i++)
    {
        sum += arr[i]; 
    }
    sum= sum/16;
    // printk ("angle is %.2f\n", sum);
    return sum;

}

/**
 * @brief Function to initialize accelerometer and set to give out drop detect data
 * and all 3 axis g value . 
 * 
 * @return 0 if successful 
 *        -1 if unsuccessful or i2c error. 
 */
int freefall_HWinitconfig(void)
{
 
    uint8_t i2c_read=0;
    int ret = i2c_reg_write_byte(i2c_dev, ACC_ADR,
                                    0x76, 0x00);                // Bank 0
    // printk("ret No 1 is %d\n ", ret);
    if(ret < 0)
    {
        return -1;
    }
    ret = i2c_reg_write_byte(i2c_dev, ACC_ADR,
                                    0x11, 0x01);                //soft reset 
    // printk("ret No 2 is %d \n", ret);
    if(ret < 0)
    {
        return -1;
    }
    k_busy_wait(2000);

    unsigned char temp_count =0;
    do{
        ret = i2c_reg_read_byte(i2c_dev, ACC_ADR,
                                        0x2D, &i2c_read);        //wait for soft reset 
        temp_count++;
        // printk("soft reset reg %x", i2c_read);
    } 
    while((i2c_read != 0x10 ) && (temp_count<10));
    
    printk("soft reset on IIM42351 complete \n");
    printk("soft reset on IIM42351 complete \n");
    ret = i2c_reg_write_byte(i2c_dev, ACC_ADR,
                                    0x4C, 0x32);                //Disable SPI
    // printk("ret No 3 is %d\n ", ret);
     if(ret < 0)
    {
        return -1 ;
    }
    ret = i2c_reg_write_byte(i2c_dev, ACC_ADR,
                                    0x13, 0x28);                //set i2c slew rate to <2ns

    if(ret < 0)
    {
        return -1 ;
    }
    ret = i2c_reg_write_byte(i2c_dev, ACC_ADR,
                                    0x76, 0x00);                // Bank 0
    if(ret < 0)
    {
        return -1 ;
    }
    ret = i2c_reg_write_byte(i2c_dev, ACC_ADR,
                                    0x14, 0x03);                // INT Config0 reg , pulsed mode and push pull on INT1 pin
    if(ret < 0)
    {
        return -1 ;
    }
    ret = i2c_reg_write_byte(i2c_dev, ACC_ADR,
                                    0x56, 0x01);                //dmp_ODR 500hz
    if(ret < 0)
    {
        return -1 ;
    }
    ret = i2c_reg_write_byte(i2c_dev, ACC_ADR,
                                    0x4D, 0x98);                //acc sing LP mode uses RC clock
    if(ret < 0)
    {
        return -1 ;
    }
    ret = i2c_reg_write_byte(i2c_dev, ACC_ADR,

                                    0x50, 0x6F);                //2g selected , 500khZ ACC_ODR
    if(ret < 0)
    {
        return -1 ;
    }
    k_busy_wait(1000);
    
    ret = i2c_reg_read_byte(i2c_dev, ACC_ADR,
                                        0x50, &i2c_read);
    
    if(ret < 0)
    {
        return -1 ;
    }

    if(i2c_read == 0x6F)
        printk("acc configured properly \n");
    else 
    {   
        printk("acc not configured \n");
        return -1;
    }

    ret = i2c_reg_write_byte(i2c_dev, ACC_ADR,
                                    0x53, 0x0D);                //UI Filter deafult setting
    if(ret < 0)
    {
        return -1 ;
    }

    ret = i2c_reg_write_byte(i2c_dev, ACC_ADR,
                                    0x52, 0x11);                //Filtering to 1
    if(ret < 0)
    {
        return -1 ;
    }
    ret = i2c_reg_write_byte(i2c_dev, ACC_ADR,
                                    0x4e, 0x02);                //ACC is LOW POWER
    if(ret < 0)
    {
        return -1 ;
    }
    k_busy_wait(20000);

    ret = i2c_reg_read_byte(i2c_dev, ACC_ADR,
                                        0x4e, &i2c_read);
    // if(i2c_read == 0x02)
    //     printk("acc LP config done \n");
    // else 
    //     printk("acc LP not configured \n");

    if(ret < 0)
    {
        return -1 ;
    }
    ret = i2c_reg_write_byte(i2c_dev, ACC_ADR,
                                    0x76, 0x04);                //Bank 4
    // ret = i2c_reg_write_byte(i2c_dev, ACC_ADR,
    //                                 AC_APEX_CONFIG5, 0x88);     //lowG 536mg, 1 sample 
    if(ret < 0)
    {
        return -1 ;
    }
    ret = i2c_reg_write_byte(i2c_dev, ACC_ADR,
                                    AC_APEX_CONFIG5, 0x38);     //lowG 250mg, 1 sample 
    // ret = i2c_reg_write_byte(i2c_dev, ACC_ADR,
    //                                 AC_APEX_CONFIG6, 0x58);      //0x20); // highg 3000mg, 1 sample   
    if(ret < 0)
    {
        return -1 ;
    }
    ret = i2c_reg_write_byte(i2c_dev, ACC_ADR,
                                    AC_APEX_CONFIG6, 0x8C);      //0x20); // highg 4500mg, 5 sample  
    if(ret < 0)
    {
        return -1 ;
    }
    ret = i2c_reg_write_byte(i2c_dev, ACC_ADR,
                                    AC_APEX_CONFIG4, 0xA4);  //0x24);  //lowg_peak_th 156mg, highg_peak_th 156mg
    if(ret < 0)
    {
        return -1 ;
    }
    ret = i2c_reg_write_byte(i2c_dev, ACC_ADR,
                                    AC_APEX_CONFIG10, 0x9E);   // min 50cm, ff max duration 531cm , debounce 2s  
                                                                //0x1C);   // min 13cm, ff max duration 201cm , debounce 2s
    
    if(ret < 0)
    {
        return -1 ;
    }
    ret = i2c_reg_write_byte(i2c_dev, ACC_ADR,
                                    0x76, 0x00);        //Bank 0
    if(ret < 0)
    {
        return -1 ;
    }
    ret = i2c_reg_write_byte(i2c_dev, ACC_ADR,
                                    0x4B, 0x20);        //DMP reset
    if(ret < 0)
    {
        return -1 ;
    }
    ret = i2c_reg_read_byte(i2c_dev, ACC_ADR,
                                        0x4B, &i2c_read);        //wait for DMP reset 
    if(ret < 0)
    {
        return -1 ;
    }
    if(i2c_read == 0x00)
        printk("DMP reset complete \n");
    else 
    {
        printk("DMP did not reset \n");
        return -1;
    }
    
    ret = i2c_reg_write_byte(i2c_dev, ACC_ADR,
                                    0x4B, 0x40);        //DMP enable
    if(ret < 0)
    {
        return -1 ;
    }
    ret = i2c_reg_read_byte(i2c_dev, ACC_ADR,
                                        0x34, &i2c_read);        //Check apex_data 3 reister for DMP idle
    if(ret < 0)
    {
        return -1 ;
    }
    if(i2c_read == 0x04)
        printk("DMP ENABLED, and init complete\n");
    else 
    {
        printk("DMP not enabled properly \n");
        return -1 ;
    }
    
    ret = i2c_reg_write_byte(i2c_dev, ACC_ADR,
                                    0x56, 0x45);                //Enable FF, TAP, DMP_ODR at 500hZ
    if(ret < 0)
    {
        return -1 ;
    }
    k_busy_wait(60000);

    ret = i2c_reg_read_byte(i2c_dev, ACC_ADR,
                                        0x56, &i2c_read);
    if(ret < 0)
    {
        return -1 ;
    }
    if(i2c_read == 0x45)
        printk("freefall set, and init complete\n");
    else 
    {
        printk("freefall not enabled properly \n");
        return -1 ;
    }
    k_busy_wait(1000);

    return 1;
                               
}
/**
 * @brief calibration api to start the calibration process. 
 *         This function is called from datalog_comms.c 
 * 
 */
void ACC_CALIBRATION()
{
    int ret;
    uint16_t temp_x =0, temp_y =0, temp_z =0;
    // x_zero_offset = 0;
    // y_zero_offset = 0;
    // z_zero_offset = 0;
    double roll =0, pitch =0;
    // float angle_sum =0;
    // printk("entered here ");


   BEGIN :  
        for(int i = 0; i <32; i++)
        {
            ret = read_raw_val();
            if(ret < 0)
                goto BEGIN ;
            roll += x_angle_calc();
            pitch += y_angle_calc();
            k_busy_wait(500);

        }
    roll = roll /32;
    pitch = pitch /32;
    // printk("OFFSET VAL is : %u, %u, %u, %d\n", filtered_xval, filtered_yval, filtered_zval, temp);
    // printk("OFFSET VAL is : %f, %f\n", roll, pitch) ;//, filtered_zval, temp);
    calib.acc_roll_angle_zero_offset = -1*(roll);               //save the roll angle to flash memory
    calib.acc_pitch_angle_zero_offset = -1*(pitch);             //save the pitch angle to flash memory

    // printk("OFFSET VAL is : %0.5f, %0.5f \n", roll, pitch) ;//, z_zero_offset); //, angle_sum/temp);
}

/**
 * @brief Read the values of roll and pitch values and save it local variable to be used in the program
 * 
 */
static void Readoffset_flash()
{
    calib_t calib_temp;
    calib_temp = CALIB_read();
    x_zero_offset = calib_temp.acc_roll_angle_zero_offset;
    y_zero_offset = calib_temp.acc_pitch_angle_zero_offset;

}

/**
 * @brief read the who am i register . Used to validate the state of i2c bus. 
 *        value of who am i register in IIM42351 is 0x6c
 * 
 * @return uint8_t 
 */
static uint8_t read_whoami()
{
    uint8_t return_val =0;
    int ret;
    ret = i2c_reg_read_byte(i2c_dev, ACC_ADR,0x75, &who_am_i);
    if (ret != 0 || who_am_i != 0x6c)
    {
        // printk("i2c error \n");
        return 0;
       
    }
    else if(who_am_i == 0x6c)
        return 1;
}

/**
 * @brief api to clear the i2c bus manually in case of stuck bus 
 * 
 */
static void cycle_i2c_CLK()
{
    nrf_gpio_cfg_output(46);
    nrf_gpio_pin_clear(46);
    nrf_gpio_cfg_output(11);
    nrf_gpio_pin_clear(11);
    for(int i = 0; i < 20; i++)
    {
        nrf_gpio_pin_toggle(46);
        nrf_gpio_pin_toggle(11);
        k_busy_wait(1000);
    }

}

/**
 * @brief Thread function for Angle thread. 
 * 
 */
void acc_angle_read()
{  
    int ret; 
    double x_axis= 0, y_axis= 0, z_axis= 0;
    double yaw_buffer[16], roll_buffer[16], pith_buffer[16];
    uint8_t count = 0;
    //Init the Moving average buffer to NULL
    // ACC_CALIBRATION();
    // ret =firstrun_prev_val_init();
    // if(ret <0) 

    //Read the offset from the flash
    Readoffset_flash();

    while(1)
    {
        // if(start_i2crecover)
        // {
        //     ret = i2c_recover_bus(&i2c_dev);
        //     printk("state of i2c bus is %d\n", ret);
        // }

        if(!accinit_failed)                                 // check if acc is initialized
        {
            if(pendulum_state && system_calibrated_flag)    //check pendulum state  & calibrated flag
            {
                if(read_whoami())                           // check for state of i2c bus
                {
                    ret = read_raw_val();
                    if(ret >0 )
                    {
                        x_axis = x_angle_calc() + x_zero_offset;
                        y_axis = y_angle_calc() + y_zero_offset;
                        double x_axis_1 = x_axis / 180* PI;
                        double y_axis_1 = y_axis / 180* PI;
                        double X_Y_angle = sqrt(tan(x_axis_1)*tan(x_axis_1) + tan(y_axis_1)*tan(y_axis_1));
                        X_Y_angle = atan(X_Y_angle);
                        X_Y_angle = X_Y_angle * 180 / PI;
                        // printk("offset x is  %0.5f offset y is , %0.5f, \t", x_zero_offset, y_zero_offset , X_Y_angle);
                        // printk("roll %0.5f pitch %0.5f,yaw%0.5f\n", x_axis, y_axis ,X_Y_angle);
                        z_axis = X_Y_angle;
                        // z_axis = 3.4;
                        if(z_axis>=3.1 && Outoflevel_blink_flag ==0)
                            count++;
                        else if (z_axis <=3.0 && Outoflevel_blink_flag ==1)
                            count++;
                        else
                            count =0;
                        // printf("x angle is %0.3f, y angle is %0.3f \n", x_axis, y_axis);
                        if(motorstatus_reg&0x80)                                // Check if motor is running
                        {
                            Outoflevel_blink_flag =0;
                            k_timer_stop(&OneHz_laserpulse);
                        }
                        if(pendulum_state && ((motorstatus_reg&0x80)^0x80))     //check for pendulum stat and motor status again
                        {
                            Outoflevel_blink(z_axis, count);
                        }
                        // printk("acc thread fine \n");
                        anglethread_sleep_time = 150;
                    }
                    else 
                    {
                    xgval = 0;
                    ygval = 0;
                    zgval = 0; 
                    }
                }
                else 
                {    
                    anglethread_sleep_time = 1000;      //Delay the process of i2c read
                    //  *(volatile uint32_t *)0x40003500 = 0;
                    // //*(volatile uint32_t *)0x40003FFC;
                    //  *(volatile uint32_t *)0x40003500 = 5;
                    // cycle_i2c_CLK();
                }
            }
        }
        else if(accinit_failed) //reinitalize acc becasue it was not initialized
        {
            if(read_whoami())
            {
                if(freefall_HWinitconfig() > 0)
                    accinit_failed =0;
                else 
                    accinit_failed =1;
            }
        }
        k_sleep(K_MSEC(anglethread_sleep_time));
       
    }
}


/**
 * @brief Thread funnction for tap_drop thread
 * Reads the INT register and calculate the fall distance basedon formuale 
 * to record 1m fall
 * 
 */
void tap_drop_read()
{
    uint16_t ff_time_data=0;
    uint32_t ff_time_ms =0, ff_distnace =0;
    int i2c_error = 0;
    int ret;

    while(1)
    {
            if(read_whoami() == 0)      //validate the i2c bus
                i2c_error =1;

            ret = i2c_reg_write_byte(i2c_dev, ACC_ADR,
                                        0x76, 0x00); 
            
            if(ret < 0)
                i2c_error = 1;
            ret = i2c_reg_read_byte(i2c_dev, ACC_ADR,
                                            0x38, &read_reg);
        
            if (ret)
            {
                // printf("Unable get WAI data. (err %i)\n", ret);
                i2c_error = 1;
                //return;
            }

            else if(read_reg >=2)
            {
                // k_timer_start(&dropblink_timer, K_MSEC(500),K_MSEC(500));
                ret = i2c_reg_read_byte(i2c_dev, ACC_ADR,
                                            0x31, &who_am_i);
                if(ret < 0)
                    i2c_error = 1;

                ff_time_data = who_am_i;
                ret = i2c_reg_read_byte(i2c_dev, ACC_ADR,
                                            0x32, &who_am_i);
                if(ret < 0)
                    i2c_error = 1;
                ff_time_data = who_am_i<<8 | ff_time_data; 
                
                if(!i2c_error)
                {
                    printk("distance1 is %u\n", ff_time_data);
                    ff_time_ms = (ff_time_data * 2);
                    printk("distance1 is %u\n", ff_time_ms);
                    ff_distnace = (9.81* ff_time_ms * ff_time_ms) /2;
                    printk("distance is %u\n", ff_distnace);
                    
                    if(((ff_distnace/1000000) >= 1) && ((ff_distnace/1000000) < 100))
                    { 
                        // flash_mem.Drop_detect_mem  = ff_distnace;
                        // FLASHVAR_update(&flash_mem.Drop_detect_mem, (uint32_t)ff_distnace);  
                        // FLASHVAR_write();
                        FLASHVAR_update(&flash_mem.Drop_detect_mem, (uint32_t)ff_distnace); 
                        DL_Inc_U32(&DL_log.total_count_drops_topples);
                        k_sem_give(&datalogwrite_semaphore);
                        gpio_pin_set_dt(&dropled, 1);
                        drop_flag = 1;
                        tapdropthread_sleep_time = 500;
                    }
                }
                else 
                    tapdropthread_sleep_time = 5000; // delay for i2c 

            }

        k_sleep(K_MSEC(tapdropthread_sleep_time));
    }
}

// ###########################################
// # THREAD DEFINATION
// ###########################################

/**
 * @brief ACC ANGLE READ THREAD
 * Thread to collect and calculate angles.
 * Runs only if pendulum is unlocked, lasers are running and motor not running
 * 
 */
K_THREAD_DEFINE(ACC_ANGLE_READ_THREAD, ACC_ANGLE_STACK_SIZE, acc_angle_read, NULL, NULL,
				NULL, ACC_ANGLE_THREAD_PRIORITY, 0, -1);
/**
 * @brief DROP READ THREAD
 * Thread that keep reading the INT register from acc to detect Drop
 * Lowest Priority thread. 
 * 
 */
K_THREAD_DEFINE(DROP_READ_THREAD, DROP_READ_STACK_SIZE, tap_drop_read, NULL, NULL,
				NULL, DROP_READ_PRIORITY, 0, -1);