/*!
 * @file mcu_config.h
 *
 *  Created on: Mar 13, 2022
 *      Author: Sally Dai
 *
 */

#ifndef MCU_CONFIG_H_
#define MCU_CONFIG_H_
/*****************************************************************************/
/* Include files */
/*****************************************************************************/
#include "main.h"
/*****************************************************************************/
/* Global pre-processor symbols/macros ('#define') */
/*****************************************************************************/
/*
MCU FRE
*/
#define MCU_CORE_CLK                         	(64ul)					//MHZ	
/*
SYS_TICK
*/
#define SYSTICK_TIMES	                        (100ul)																							//US
#define SYSTICK_TIMES_CNT                     (0XFFFFFFU&(MCU_CORE_CLK*SYSTICK_TIMES))
//lint -e(961) Violates MISRA 2004 Advisory Rule 19.7, Function-like macro defined. this is MCU lib - safe use
//lint -e(9026) Function-like macro
#define DISABLE_SYSTICK_INT()	                (SysTick->CTRL &= 0XFFFFFFFDU)
#define READ_SYSTICK_COUNTF()					        (SysTick->CTRL & 0X00010000U)
/*
IO outut set
*/
#define LED                             (GPIO4->DO_f.P4)      //Output LED
#define LED_ON()                        (LED = 1)
#define LED_OFF()                       (LED = 0)
#define LED_TOGGLE()                    (LED = ~LED)

#define SelfOn                          (GPIO1->DO_f.P0)       //Output Vdd power on
#define SelfOn_set()                    (SelfOn = 1)
#define SelfOn_reset()                  (SelfOn = 0)
#define SelfOn_toggle()                 (SelfOn = ~SelfOn)

#define BattSenseEnable                 (GPIO2->DO_f.P1)        //Output Enable Battery Temp and Voltage measurement
#define BattSenseEnable_set()           (BattSenseEnable = 1)
#define BattSenseEnable_reset()         (BattSenseEnable = 0)
#define BattSenseEnable_toggle()        (BattSenseEnable = ~BattSenseEnable) 
/*
UART
*/
#define UART_BAUD_FAST	                28800U  //TODO: change baud rate for Flash UART operations
#define UART_BAUD_SLOW	                  9600U  //TODO: change baud rate for Flash UART operations
//lint -e(834) Operator followed by operator is confusing
#define SET_BAUD(x)                     (x != 0 ? (UART1->DLR = 4000000/x) : 0);//__HIRC_64M/16
/*
TIMER
*/
#define GETCHAR_TIME_OUT                5000//4us/count for time0
/*
INTERRUPT
*/
//lint -e(961) Violates MISRA 2004 Advisory Rule 19.7, Function-like macro defined. this is MCU lib - safe use
//lint -e(9026) Function-like macro
#define    DI()                                __disable_irq()
#define    EI()                                 __enable_irq()
/*****************************************************************************/
/* Global function prototypes ('extern', definition in C source) */
/*****************************************************************************/
void BSP_MCU_Config(void);
void BSP_GPIOConfig(void);
void BSP_UART1Config(void);
void BSP_TMR0Config(void);
void BSP_Delay_Us(uint16_t us);
void BSP_Delay_Ms(uint16_t ms);
void xmitchar(const uint8_t ch);
void getChar(uint8_t* Char);
#endif /* MCU_CONFIG_H_ */
