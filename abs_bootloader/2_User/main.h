/*!
 * @file main.h
 *
 *  Created on: Mar 13, 2022
 *      Author: Sally Dai
 *
 */

#ifndef MAIN_H_
#define MAIN_H_
/*****************************************************************************/
/* Include files */
/*****************************************************************************/

#include "cms32m5xxx.h"
#include "code_config.h"
#include "gpio.h"
#include "system.h"
#include "uart.h"
#include "timer.h"
#include "mcu_config.h"
#include "cm_handler.h"
#include "user_fmc.h"
/*****************************************************************************/
/* Global pre-processor symbols/macros ('#define') */
/*****************************************************************************/
#define SLOW 0
#define FAST 1

#define Tx_ON  1U
#define Tx_OFF 0U
//lint -e{960} Disallowed use of non-numeric value in a case label
#define COMMAND_NACK 			0x01
#define COMMAND_HANDSHAKE 		'B'
#define COMMAND_READ_EEPROM 	'F'
#define COMMAND_LOAD_EEPROM 	'E'
#define COMMAND_READ_FLASH 		'N'
#define COMMAND_LOAD_FLASH 		'M'
#define COMMAND_SET_ADDRESS 	'D'
#define COMMAND_CHANGE_BAUD 	'*'
#define ETX 					0x03
static void ResetReg(void);
#endif /* MAIN_H */
