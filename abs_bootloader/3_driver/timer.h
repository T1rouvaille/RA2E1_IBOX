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
/** \file timer.h
**
** History:
** 
*****************************************************************************/
#ifndef __TIMER_H_
#define __TIMER_H_

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
 **TMR 模式
-----------------------------------------------------------------------------*/
#define		TMR_COUNT_CONTINUONS_MODE	(0x00UL)					/*??????*/
#define		TMR_COUNT_PERIOD_MODE		(0x01UL)	/*??????*/
	
#define		TMR_BIT_16_MODE				(0x00UL)					/*16???*/
#define		TMR_BIT_32_MODE				(0x01UL)	/*32???*/
/*----------------------------------------------------------------------------
 **TMR 时钟
-----------------------------------------------------------------------------*/
#define		TMR_CLK_DIV_1		(0x00UL)							/*1??*/
#define		TMR_CLK_DIV_16		(0x01UL)		/*16??*/
#define		TMR_CLK_DIV_256		(0x02UL)		/*256??*/

#define		TMR_CLK_SEL_APB		(0x00UL)		/*时钟源选择APB时钟*/
#define		TMR_CLK_SEL_HSI		(0x01UL)		/*时钟源选择HSI时钟*/
#define		TMR_CLK_SEL_LSI		(0x03UL)		/*时钟源选择LSI时钟*/



#ifdef __cplusplus
}
#endif

#endif /* __TIMER_H_ */

