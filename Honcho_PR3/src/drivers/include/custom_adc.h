/*
*  Copyright (c) 2022, Stanley Black and Decker 
*    All rights reserved
* Author : Kavya Jain 
* Created on : 25 Nov 2022
*/

#ifndef CUSTOM_ADC_H
#define CUSTOM_ADC_H

#include <zephyr.h>
#include <sys/printk.h>
#include <drivers/adc.h>
#include <drivers/gpio.h>

#include "gpio_def.h"
#include "soc_laserblink.h"
#include "button_manager.h"
#include "system_manager.h"

#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || \
	!DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "No suitable devicetree overlay specified"
#endif


#define PACKADC_NUM_CHANNELS	2		//DT_PROP_LEN(DT_PATH(zephyr_user), io_channels)

/*
#if ADC_NUM_CHANNELS > 2
#error "Currently only 1 or 2 channels supported in this sample"
#endif

#if ADC_NUM_CHANNELS == 2 && !DT_SAME_NODE( \
	DT_PHANDLE_BY_IDX(DT_PATH(zephyr_user), io_channels, 0), \
	DT_PHANDLE_BY_IDX(DT_PATH(zephyr_user), io_channels, 1))
#error "Channels have to use the same ADC."
#endif
*/

#define ADC_NODE		DT_PHANDLE(DT_PATH(zephyr_user), io_channels)

/* Common settings supported by most ADCs */
#define ADC_RESOLUTION					12
#define ADC_GAIN						ADC_GAIN_1_6
#define ADC_REFERENCE					ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME	 		ADC_ACQ_TIME_DEFAULT

#ifdef CONFIG_ADC_NRFX_SAADC
#define ADC_INPUT_POS_OFFSET 			SAADC_CH_PSELP_PSELP_AnalogInput0
#else
#define ADC_INPUT_POS_OFFSET 			0
#endif

#define ADCREAD_THREAD_STACK_SIZE   		1024
#define ADCREAD_THREAD_PRIORITY     		5
#define LASERCURRENT_THREAD_STACK_SIZE   	1024
#define LASERCURRENT_THREAD_PRIORITY     	5

#define VBTAEN_NODE  					DT_ALIAS(vbaten)	
#define BATTTHEN_NODE					DT_ALIAS(battthen)
#define BATT_VOLATGE_75					2932		//2932 -> 18.2V
#define BATT_VOLATGE_50					2659		//2659 -> 16.5V
#define BATT_VOLATGE_25					2385		//2385 -> 14.8V
#define BATT_VOLATGE_10					2272		//2272 -> 14.1V
#define BATT_VOLATGE_12V				1938		//1938 -> 12V	
#define BATT_VOLATGE_9V					730			//730 -> 9V
#define HIGH_TEMP_RESISTANCE			2.3			//Set to 2.3 becasue the resistnce drop to 2.2K when hot pack is recorded
#define PLUMB1_LIMIT					340			//300mA *(4095 / (0.6*6)) 
#define PLUMB2_LIMIT					340			//300mA *(4095 / (0.6*6)) 			
#define LEVEL_LIMIT						398			//350mA *(4095 / (0.6*6)) 

static const struct device *dev_adc = DEVICE_DT_GET(ADC_NODE);
static const struct gpio_dt_spec Vbaten = GPIO_DT_SPEC_GET_OR(VBTAEN_NODE, gpios,		//set which pin by index value/default value
							      {0});	

static const struct gpio_dt_spec Battthen = GPIO_DT_SPEC_GET_OR(BATTTHEN_NODE, gpios,		//set which pin by index value/default value
							      {0});	

// extern bool lowvoltage_flag;
extern const k_tid_t PACK_ADC_READ_THREAD;
extern const k_tid_t LASER_CURRENT_READ_THREAD;
void custom_adc_init(void);
// void disable_battmeasurement();
// void adc_thread_time(int x);
void pack_adc_read(void);
void lasercurrent_adc_read(void);

#endif /* CUSTOM_ADC_H */