/*!
 * @file main.h
 *
 *  Created on: Mar 13, 2022
 *      Author: Sally Dai
 *
 */

#ifndef CODE_CONFIG_H_
#define CODE_CONFIG_H_
/*****************************************************************************/
/* Include files */
/*****************************************************************************/
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
/*****************************************************************************/
/* Global pre-processor symbols/macros ('#define') */
/*****************************************************************************/
//********************* module DEFINES ****************************************
//   CHANGE THESE DEFINES TO COMPILE FOR DIFFERENT module TYPES

#define ABS_CM20V

#if defined(ABS_CM20V)

#define ABS_BOOTLOADER_ID	{'N', 'A', '2', '1', '5', '9', '2', '9'}

#define MODULE_REVISION 0x04
#define ID_FORMAT_8BYTES

//#define BL_HW_MODE	0xFF
#define BL_HW_MODE	0x53 //default to single wire communication mode. 'S' = 0x53
//#define BL_HW_MODE	0x44	//default to dual wire communication mode. 'D' = 0x44
#endif 

#if defined(ID_FORMAT_8BYTES)
#define BYTES_MODULE_ID			8U
#define BYTES_PART_NUMBER		8U
#define BYTES_PERSONALIZATION	52U // The code version size is lumped into this number
#else
#define BYTES_MODULE_ID			7
#define BYTES_PART_NUMBER		8
#define BYTES_PERSONALIZATION	48 // The code version size is lumped into this number
#endif

#define FEE_AVAILABLE

#if defined(ID_FORMAT_8BYTES)
	#define NUM_INFO_BYTES			  89U
	#define FLASH_CONST_ADDRESS		0x00000E00U
	#define VERSION_ADDRESS			  0x00000E3CU
	#define VERIFY_ADDRESS			  0x00007FFCU
	#define APP_END_ADDRESS		    0x00007FFFU ///the last 16byte for verify purpose.
	#define BYTES_SERIALIZATION		16U
#else
	#define NUM_INFO_BYTES			84
	#define FLASH_CONST_ADDRESS		0x10003800
	#define VERSION_ADDRESS			0x1000383C
	#define OPCODE_RESET_ADDRESS	0x10003904
	#define VERIFY_ADDRESS			0x1000F5FC
	#define FLASH_END_ADDRESS		0x1000F5FF ///< 0x10010FFF [End of flash] - 0x1A00 [FEE total size] = 0x1000E3FF
	#define BYTES_SERIALIZATION		16
#endif

#define FMC_APROM_BASE         0x1000			/*!< APROM  Base Address    */
#define PAGE_SIZE				       0x200U			//512 bytes per page
#define BL_HW_MODE_ADDRESS		 0x0DFC
#define FLASH_START_ADDRESS    0x00000000U
#define FLASH_END_ADDRESS      0x00007FFFU
#define UCID_ADDRESS			     0x18000000U	//Start of 32 byte unique ID
#define BYTES_CHIP_ID			     16U

#define	FEE_SIZE 					     512U

#define PROTECTED_PAGE_SIZE_0			(16U)
#define PROTECTED_PAGE_START_0 		(0U)

#define PROTECTED_PAGE_SIZE_1			(16U)
#define PROTECTED_PAGE_START_1		(FEE_SIZE - PROTECTED_PAGE_SIZE_1)

/*****************************************************************************/
/* Global variable declarations ('extern', definition in C source) */
/*****************************************************************************/
extern const char Module_ID[];
extern const char Revision;
#endif /* CODE_CONFIG_H_ */
