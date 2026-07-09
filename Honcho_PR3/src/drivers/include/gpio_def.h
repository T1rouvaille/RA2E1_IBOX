/*
*  Copyright (c) 2022, Stanley Black and Decker 
*    All rights reserved
* Author : Kavya Jain 
* Created on : 25 Nov 2022
*/

/**
 * @file
 * @brief Public API for button decode */
#ifndef GPIO_DEF_H_
#define GPIO_DEF_H_

/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Nordic UART Bridge Service (NUS) sample
 */
#include <zephyr.h>
#include <device.h>
#include <drivers/gpio.h>
#include <sys/util.h>
#include <sys/printk.h>
#include <inttypes.h>
#include <logging/log.h>

#include "custom_adc.h"

#define SLOPELED_NODE	DT_ALIAS(slopeled)
#if !DT_NODE_HAS_STATUS(SLOPELED_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif

#define DROPLED_NODE	DT_ALIAS(dropled)
#if !DT_NODE_HAS_STATUS(DROPLED_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif

#define BLELED_NODE	DT_ALIAS(bleled)
#if !DT_NODE_HAS_STATUS(BLELED_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif

#define SOC1_NODE DT_NODELABEL(soc1)
#if !DT_NODE_HAS_STATUS(SOC1_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif

#define SOC2_NODE DT_NODELABEL(soc2)
#if !DT_NODE_HAS_STATUS(SOC2_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif

#define SOC3_NODE DT_NODELABEL(soc3)
#if !DT_NODE_HAS_STATUS(SOC3_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif


static const struct gpio_dt_spec slopeled	= GPIO_DT_SPEC_GET_OR(SLOPELED_NODE, gpios,		//here set which pin by index value/default value
							      {0});								  
static const struct gpio_dt_spec dropled = GPIO_DT_SPEC_GET_OR(DROPLED_NODE, gpios,
							      {0});
static const struct gpio_dt_spec bleled = GPIO_DT_SPEC_GET_OR(BLELED_NODE, gpios,
							      {0});
static const struct gpio_dt_spec soc1 = GPIO_DT_SPEC_GET_OR(SOC1_NODE, gpios,
							      {0});
static const struct gpio_dt_spec soc2 = GPIO_DT_SPEC_GET_OR(SOC2_NODE, gpios,
							      {0});
static const struct gpio_dt_spec soc3 = GPIO_DT_SPEC_GET_OR(SOC3_NODE, gpios,
							      {0});


#define LaserbuckEN_NODE    DT_ALIAS(lbucken)
#if !DT_NODE_HAS_STATUS(LaserbuckEN_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif

#define MotorbuckEN_NODE	DT_ALIAS(mbucken)
#if !DT_NODE_HAS_STATUS(MotorbuckEN_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif

#define LdoEN_NODE	 DT_ALIAS(ldoen)
#if !DT_NODE_HAS_STATUS(LdoEN_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif

static const struct gpio_dt_spec LaserbuckEN = GPIO_DT_SPEC_GET_OR(LaserbuckEN_NODE, gpios,		//here set which pin by index value/default value
							      {0});								 

static const struct gpio_dt_spec MotorbuckEN = GPIO_DT_SPEC_GET_OR(MotorbuckEN_NODE, gpios,		//here set which pin by index value/default value
							      {0});								 

static const struct gpio_dt_spec SysldoEN = GPIO_DT_SPEC_GET_OR(LdoEN_NODE, gpios,		//here set which pin by index value/default value
							      {0});

/*******************************************************************************************************/
#define Laser_PS_ADC_NODE	DT_ALIAS(laserpsadc)
#if !DT_NODE_HAS_STATUS(MotorbuckEN_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif
#define PACK_ID_EN_NODE	DT_ALIAS(packiden)
#if !DT_NODE_HAS_STATUS(MotorbuckEN_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif
#define VBatt_coin_EN_NODE	DT_ALIAS(vbatcoinen)
#if !DT_NODE_HAS_STATUS(MotorbuckEN_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif
#define Laser_Current_NODE	DT_ALIAS(lasercurrent)
#if !DT_NODE_HAS_STATUS(MotorbuckEN_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif
#define Vbatt_coin_adc_NODE	DT_ALIAS(vbattcoinadc)
#if !DT_NODE_HAS_STATUS(MotorbuckEN_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif

static const struct gpio_dt_spec laserpsadc = GPIO_DT_SPEC_GET_OR(Laser_PS_ADC_NODE, gpios,		//here set which pin by index value/default value
							      {0});								 

static const struct gpio_dt_spec packiden = GPIO_DT_SPEC_GET_OR(PACK_ID_EN_NODE, gpios,		//here set which pin by index value/default value
							      {0});								 
static const struct gpio_dt_spec vbatcoinen = GPIO_DT_SPEC_GET_OR(VBatt_coin_EN_NODE, gpios,		//here set which pin by index value/default value
							      {0});								 

static const struct gpio_dt_spec lasercurrent = GPIO_DT_SPEC_GET_OR(Laser_Current_NODE, gpios,		//here set which pin by index value/default value
							      {0});								 
static const struct gpio_dt_spec vbattcoinadc = GPIO_DT_SPEC_GET_OR(Vbatt_coin_adc_NODE, gpios,		//here set which pin by index value/default value
							      {0});	

void led_init();
void control_init();
void led_off();
void bucks_off();
void enables_off();

#endif /* GPIO_DEF_H */