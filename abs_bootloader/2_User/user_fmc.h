/*!
 * @file user_app.h
 *
 *  Created on: Mar 13, 2022
 *      Author: Sally Dai
 *
 */

#ifndef USER_FMC_H_
#define USER_FMC_H_
/*****************************************************************************/
/* Include files */
/*****************************************************************************/
#include "main.h"
/*****************************************************************************/
/* Global pre-processor symbols/macros ('#define') */
/*****************************************************************************/
#define FMC_BUSY_BIT (1<<5)
//lint -e(961) Violates MISRA 2004 Advisory Rule 19.7, Function-like macro defined. this is MCU lib - safe use
//lint -e(9026) Function-like macro
//lint -e(9022) ignore unparenthesized macro parameter in definition of macro
#define FMC_UNLOCK()                       do{FMC->LOCK = 0x55AA6699;}while(0)
#define FMC_UNLOCK_CPU()                       do{FMC->LOCK = 0x55AA669A;}while(0)
#define FMC_LOCK()                           do{FMC->LOCK = 0xFFFFFFFF;}while(0)
#define FMC_GET_BUSY_STATE()       (FMC->CON & FMC_BUSY_BIT)
#define FMC_ADDR(addr)                  do{FMC->ADR = (addr);}while(0)
#define FMC_CMD(cmd)		         do{FMC->CMD = cmd;}while(0)
#define FMC_ERROR_ADRESS               1


#define FMC_START_ADRESS                0x1c000000U
#define FMC_BANK0_START_ADRESS          0x1c000000U
#define FMC_BANK0_END_ADRESS            0x1c0001FCU
#define FMC_BANK1_START_ADRESS          0x1c000200U
#define FMC_BANK1_END_ADRESS            0x1c0003FCU

#define FMC_BANK0_START_INDEX          0U
#define FMC_BANK0_END_INDEX            127U
#define FMC_BANK1_START_INDEX          128U
#define FMC_BANK1_END_INDEX            255U

#define BANK_EMPTY	       0x00U
#define BANK_VALID 			   0x01U
#define BANK_INVALID 		   0x02U
#define BANK_NEW 	         0x03U
#define BANK_OLD 	         0x04U
/*****************************************************************************/
/* Global type definitions ('typedef') */
/*****************************************************************************/
typedef struct
{
	uint32_t data_read[8];
	uint32_t data_write[8];
}USER_FMC_STR;

/*****************************************************************************/
/* Global variable declarations ('extern', definition in C source) */
/*****************************************************************************/

/*****************************************************************************/
/* Global function prototypes ('extern', definition in C source) */
/*****************************************************************************/
void FMC_Write_Data(uint32_t fmc_adress,const uint32_t fmc_data);
uint32_t FMC_Read_Data(const uint32_t fmc_adress);
void FMC_Read_Array(const uint32_t start_address, const uint16_t Length, uint8_t *buf);
void FMC_Write_Array(const uint32_t pageAddr, const uint16_t Length, const uint8_t* buf);
void Erase_Flash_Page(const uint32_t pageAddr);


#endif /* USER_FMC_H_ */
