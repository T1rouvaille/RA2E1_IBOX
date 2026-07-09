/*!
 * @file cm_handler.h
 *
 *  Created on: Mar 13, 2022
 *      Author: Sally Dai
 *
 */

#ifndef CM_HANDLER_H_
#define CM_HANDLER_H_
/*****************************************************************************/
/* Include files */
/*****************************************************************************/
#include "main.h"
/*****************************************************************************/
/* Global type definitions ('typedef') */
/*****************************************************************************/
//union to hold 32 bit data and address
typedef union tuReg32
{
	uint32_t Val32;

	struct
	{
		uint16_t LW;
		uint16_t HW;
	} Word;

	uint8_t Val[4];
} uReg32;

/*****************************************************************************/
/* Global pre-processor symbols/macros ('#define') */
/*****************************************************************************/

/*****************************************************************************/
/* Global function prototypes ('extern', definition in C source) */
/*****************************************************************************/
static void Get_Bank_Final_State(uint8_t* state);
static void Increment_EEP_Buffer(const uint8_t bytes, const uint8_t increment);

void Set_Address(uReg32* ptraddr);
void Cmd_Read_EEPROM(uint8_t* data);
void Cmd_Load_EEPROM(uint8_t* mem);
void Read_Version_Info(void);
uint16_t Read_Flash_Page(const uReg32* Address, const uint8_t Tx_enable);
void Load_Flash_Page(const uReg32* Address);
//void Set_Baud(uint8_t speed);
void FEE_Read(void);
void FEE_Write(void);
#endif /* CM_HANDLER_H_ */
