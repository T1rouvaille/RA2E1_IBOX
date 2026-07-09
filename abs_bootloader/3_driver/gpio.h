/*******************************************************************************
* Copyright (C) 2019 China Micro Semiconductor Limited Company. All Rights Reserved.
*
* This software is owned and published by:
* CMS LLC, No 2609-10, Taurus Plaza, TaoyuanRoad, NanshanDistrict, Shenzhen, China.
*
* BY DOWNLOADING, INSTALLING OR USING THIS SOFTWARE, YOU AGREE TO BE BOUND
* BY ALL THE TERMS AND CONDITIONS OF THIS AGREEMENT.
*
* This software contains source code for use with CMS
* components. This software is licensed by CMS to be adapted only
* for use in systems utilizing CMS components. CMS shall not be
* responsible for misuse or illegal use of this software for devices not
* supported herein. CMS is providing this software "AS IS" and will
* not be responsible for issues arising from incorrect user implementation
* of the software.
*
* This software may be replicated in part or whole for the licensed use,
* with the restriction that this Disclaimer and Copyright notice must be
* included with each copy of this software, whether used in part or whole,
* at all times.
*/

/*****************************************************************************/
/** \file gpio.h
**
** History:
** 
*****************************************************************************/
#ifndef __GPIO_H__
#define __GPIO_H__

#ifdef __cplusplus
extern "C"
{
#endif

/*****************************************************************************/
/* Include files */
/*****************************************************************************/
#include "cms32m5xxx.h"
/*****************************************************************************/
/* Global pre-processor symbols/macros ('#define') */
/*****************************************************************************/
/*----------------------------------------------------------------------------
 **GPIO PORT & PIN
-----------------------------------------------------------------------------*/
#define		GPIO_PIN_NUM_MAX	(0x08UL)
#define 	GPIO_PIN_0_MSK		(0x01UL)		/*GPIO Pin 0 mask*/
#define 	GPIO_PIN_1_MSK		(0x02UL)		/*GPIO Pin 1 mask*/
#define 	GPIO_PIN_2_MSK		(0x04UL)		/*GPIO Pin 2 mask*/
#define 	GPIO_PIN_3_MSK		(0x08UL)		/*GPIO Pin 3 mask*/		
#define 	GPIO_PIN_4_MSK		(0x10UL)		/*GPIO Pin 4 mask*/
#define 	GPIO_PIN_5_MSK		(0x20UL)		/*GPIO Pin 5 mask*/
#define 	GPIO_PIN_6_MSK		(0x40UL)		/*GPIO Pin 6 mask*/
#define 	GPIO_PIN_7_MSK		(0x80UL)		/*GPIO Pin 7 mask*/

#define 	GPIO_PIN_0			(0x00UL)		/*GPIO Pin 0 Num*/
#define 	GPIO_PIN_1			(0x01UL)		/*GPIO Pin 1 Num*/
#define 	GPIO_PIN_2			(0x02UL)		/*GPIO Pin 2 Num*/
#define 	GPIO_PIN_3			(0x03UL)		/*GPIO Pin 3 Num*/
#define 	GPIO_PIN_4			(0x04UL)		/*GPIO Pin 4 Num*/		
#define 	GPIO_PIN_5			(0x05UL)		/*GPIO Pin 5 Num*/
#define 	GPIO_PIN_6			(0x06UL)		/*GPIO Pin 6 Num*/
#define 	GPIO_PIN_7			(0x07UL)		/*GPIO Pin 7 Num*/

/*----------------------------------------------------------------------------
 **GPIO 模式 
-----------------------------------------------------------------------------*/
#define 	GPIO_MODE_INPUT						(0x00UL)		/*普通输入模式*/
#define 	GPIO_MODE_OUTPUT					(0x01UL)		/*推挽输出模式*/
#define 	GPIO_MODE_OPEN_DRAIN_WITHOUT_PULL_UP		(0x02UL)		/*不带上拉的开漏输出模式*/
#define 	GPIO_MODE_INPUT_WITH_PULL_UP		(0x03UL)		/*上拉输入模式*/
#define		GPIO_MODE_INPUT_WITH_PULL_DOWN		(0x04UL)		/*下拉输入模式*/

#define 	INPUT					    GPIO_MODE_INPUT
#define 	OUTPUT_PP_GP			GPIO_MODE_OUTPUT
#define 	OUTPUT_OD_GP			GPIO_MODE_OPEN_DRAIN_WITHOUT_PULL_UP
#define 	INPUT_PU			    GPIO_MODE_INPUT_WITH_PULL_UP
#define		INPUT_PD			    GPIO_MODE_INPUT_WITH_PULL_DOWN
//---- P1 Config -----
static inline void P1_0_set_mode(const uint32_t mode){
    GPIO1->PMS &= ~(0x7UL << (GPIO_PIN_0 * 4));
    GPIO1->PMS |= (mode << (GPIO_PIN_0 * 4));
}
//---- P2 Config -----
static inline void P2_1_set_mode(const uint32_t mode){
    GPIO2->PMS &= ~(0x7UL << (GPIO_PIN_1 * 4));
    GPIO2->PMS |= (mode << (GPIO_PIN_1 * 4));
}
//---- P3 Config -----
static inline void P3_6_set_mode(const uint32_t mode){
    GPIO3->PMS &= ~(0x7UL << (GPIO_PIN_6 * 4));
    GPIO3->PMS |= (mode << (GPIO_PIN_6 * 4));
}
//---- P4 Config -----
static inline void P4_4_set_mode(const uint32_t mode){
    GPIO4->PMS &= ~(0x7UL << (GPIO_PIN_4 * 4));
    GPIO4->PMS |= (mode << (GPIO_PIN_4 * 4));
}



#ifdef __cplusplus
}
#endif
#endif /* __GPIO_H__ */

