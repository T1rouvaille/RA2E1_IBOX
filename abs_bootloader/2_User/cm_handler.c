/*
 * cm_handler.c
 *
 *  Created on: Mar 15, 2022
 *      Author: Sally Dai
 *
 *	-------------------------------------------------------------------------------------------------------
 *
 *
 *
 *
 */
  //lint --e{966} ignore the head files not used by cm_handler.c
#include "main.h"
//lint --e{956} volatile will occur warning
uint8_t bank_state = 0U;

extern uint8_t g_FEE_Buffer[FEE_SIZE];
/*
 * Parameters(IN)   : void
 *
 * Return value     : void
 *
 * Description      : This function identify the address. the PC sends out 16bits adress, we use 32bits address. The address value needs to transfer to what we want.
 */
 //lint --e{952} could not be declared as const
void Set_Address(uReg32* ptraddr)
{
	//lint --e{960} use of non-numeric value is safe
	//lint --e{9117} implicit conversion changes signedness 
	xmitchar(COMMAND_SET_ADDRESS);
	ptraddr->Val[3]= 0U;			  //this upper most byte will always be 0x00 because that's the flash range
	ptraddr->Val[2]= 0U;				//just store 0 in the high 16bit bytes

	getChar (&ptraddr->Val[1]);		//get the low 16bits address. address number is 0x0000-0000~0x0000-7FFF
	getChar (&ptraddr->Val[0]);		
	ptraddr->Val32+= 0x0000U;		  //add the flash offset
//lint --e{9117} implicit conversion changes signedness 
	xmitchar(ETX);
}
//Send the whole eeprom
 //lint --e{952} could not be declared as const
//lint --e{818}
void Cmd_Read_EEPROM(uint8_t* data)
{
	//lint --e{960} use of non-numeric value is safe
	//lint --e{9117} implicit conversion changes signedness 
		xmitchar(COMMAND_READ_EEPROM);		//Echo cmd

 /* Datalog on EEPROM Chip */
	//lint --e{921} cast from unsigned int to unsigned int 
		xmitchar((uint8_t)(FEE_SIZE>>8U));		//number of bytes, high byte
//lint --e{921} cast from unsigned int to unsigned int 
		xmitchar((uint8_t)(FEE_SIZE&0xFFU));		//number of bytes, low byte


		uint16_t i;

/* Datalog on EEPROM Chip */
		for(i=0U; i < FEE_SIZE;i++)
		{
			xmitchar(data[i]);
		}
		//lint --e{9117} implicit conversion changes signedness 
		xmitchar(ETX);
		return;
}
//lint --e{952} *mem couldn't be declared as const
void Cmd_Load_EEPROM(uint8_t* mem)
{
	union num_bytes			//union for the number of bytes to receive
	{
		uint16_t tot_bytes;
		uint8_t byte_array[2];
	} rec_num_bytes;

	union CS				//union for the checksum
	{
		uint16_t totsum;
		uint8_t chk[2];
	} checksum;

	uint16_t i;
	uint8_t Protected_Page_Data[PROTECTED_PAGE_SIZE_0];

	Increment_EEP_Buffer(3U, 1U);
	//lint --e{960} use of non-numeric value is safe
	//lint --e{9117} implicit conversion changes signedness 
	xmitchar(COMMAND_LOAD_EEPROM);		//echo cmd

	getChar(&rec_num_bytes.byte_array[1]);	//Get the high byte for number of bytes
	getChar(&rec_num_bytes.byte_array[0]);	//Get the low byte

	checksum.totsum = 0U;

	for (i = 0U; i < (rec_num_bytes.tot_bytes); i++)
	{
		if (i < (PROTECTED_PAGE_START_0 + PROTECTED_PAGE_SIZE_0))
		{
			getChar(&Protected_Page_Data[i]);
			xmitchar(Protected_Page_Data[i]);
			checksum.totsum += Protected_Page_Data[i];
		}
		else if(i <  PROTECTED_PAGE_START_1)
		{
			getChar(&mem[i]);
			xmitchar(mem[i]);
			checksum.totsum += mem[i];
		}
		else 	//Receive the protected page in a temp variable, do not write to the EEPROM shadow array
		{
			getChar(&Protected_Page_Data[(i -  PROTECTED_PAGE_START_1)]);
			xmitchar(Protected_Page_Data[i -  PROTECTED_PAGE_START_1]);
			checksum.totsum += Protected_Page_Data[i -  PROTECTED_PAGE_START_1];
		}
	}

	xmitchar(checksum.chk[1]);	//checksum high byte
	xmitchar(checksum.chk[0]);	//checksum low byte
//lint --e{9117} implicit conversion changes signedness 
	xmitchar(ETX);		//End transmission

	FEE_Write();

	return;
}
/*
 * Parameters(IN)   : void
 *
 * Return value     : void
 *
 * Description      : This function read out the module's information.
 */
void Read_Version_Info(void)
{
	uReg32 	Data;
	uint16_t	cntr;
	uint16_t	i;
	uint8_t data[BYTES_PART_NUMBER + BYTES_PERSONALIZATION];
//lint --e{960} use of non-numeric value is safe
//lint --e{9117} implicit conversion changes signedness 
	xmitchar(COMMAND_READ_FLASH);
//lint -e(572) Excessive shift value 
	xmitchar(NUM_INFO_BYTES >> 8U);		//transmit number of bytes, high byte
	xmitchar(NUM_INFO_BYTES & 0xFFU);	//transmit number of bytes, low byte
	
		//send Module ID/ Bootloader version- 8 bytes
	for (i = 0U; i < BYTES_MODULE_ID; i++)
	{
		xmitchar(Module_ID[i]);
	}
		//send Part number, Personalization, and code version - 56 bytes
	FMC_Read_Array(FLASH_CONST_ADDRESS, (BYTES_PART_NUMBER + BYTES_PERSONALIZATION) >> 2U, data);

	for(cntr = 0U; cntr < ((BYTES_PART_NUMBER + BYTES_PERSONALIZATION)); cntr++)
	{
		{
			if (data[cntr] == 0U || data[cntr] == 0xFFU)	//replace 0 and blank w/ spaces
			{
				data[cntr] = ' ';
			}
				
		}
		xmitchar(data[cntr]);
	}
		//Send Code Version - 2 bytes
	//lint --e{923}	cast from unsigned long to pointer
	Data.Val32 = *((uint32_t*)VERSION_ADDRESS);
	xmitchar(Data.Val[0]);
	xmitchar(Data.Val[1]);

	//Send Code Verification - 2 bytes
	//lint --e{923}	cast from unsigned long to pointer
	Data.Val32 = *((uint32_t*)VERIFY_ADDRESS);
	xmitchar(Data.Val[0]);
	xmitchar(Data.Val[1]);
	
	uint8_t	Chip_ID[16];
	//lint --e{569} Loss of information (arg. no. 1) (29 bits to 16 bits)
	//lint --e{712} loss precision
  FMC_Read_Array(UCID_ADDRESS, BYTES_CHIP_ID>> 2U, Chip_ID);
	//Send Chip ID - 16 bytes
	for(cntr = 0U; cntr < (BYTES_CHIP_ID); cntr++)
	{		
		xmitchar( Chip_ID[cntr]);
	}
	
	//Send module revision - 1 byte
	xmitchar(Revision);
	//lint --e{960} use of non-numeric value is safe
	//lint --e{9117} implicit conversion changes signedness 
	xmitchar(ETX);
}
/*
 * Parameters(IN)   : void
 *
 * Return value     : void
 *
 * Description      : This function read out the bytes by reading adress directly.The adress value is important.
 *									:it should be the address that you want
 */
uint16_t Read_Flash_Page(const uReg32* Address, const uint8_t Tx_enable)
{
	uint8_t Data[PAGE_SIZE];		//register to hold the data for xmit
	uint8_t Rx_byte;
	uint16_t i;
	uint16_t checksum = 0U;
 
	if (Tx_enable)
	{
		//Catch some incoming bytes
		getChar(&Rx_byte);
		getChar(&Rx_byte);
	}
//lint -e(685) let adress is always >= 0
//lint -e(568)
	if ((Address->Val32 >= FLASH_START_ADDRESS) && (Address->Val32 <= FLASH_END_ADDRESS))	//Only read from valid Flash locations
	{
		if (Tx_enable)
		{
//lint --e{960} use of non-numeric value is safe
//lint --e{9117} implicit conversion changes signedness 
			xmitchar(COMMAND_READ_FLASH);			

			xmitchar(PAGE_SIZE >> 8U);	//transmit number of bytes, high byte
			xmitchar(PAGE_SIZE & 0xFFU);	//transmit number of bytes, low byte
		}
		//lint --e{569} Loss of information (arg. no. 1) (29 bits to 16 bits)
		//lint --e{712} loss precision
		FMC_Read_Array(Address->Val32, PAGE_SIZE >> 2U, Data);
		for (i = 0U; i < PAGE_SIZE; i++)
		{			
			checksum += Data[i];
			if (Tx_enable)
			{
				xmitchar(Data[i]);
			}
		}
	}
	if (Tx_enable)
	{
		//lint --e{511} Size incompatibility in cast from 'unsigned long'
		//lint --e{923}	cast from unsigned long to pointer
		SET_BAUD(UART_BAUD_SLOW);
	}		
	return checksum;
}
/*
 * Parameters(IN)   : void
 *
 * Return value     : void
 *
 * Description      : This function load the flash with page size.
 */
void Load_Flash_Page(const uReg32* Address)
{
	//lint --e{9146}  multiple declarators in a declaration
		union uReg16
	{
		uint16_t total;
		uint8_t	byte[2];
	}Rx_Bytes, RAM_Checksum;

	uint16_t Flash_Checksum = 0U;
	uint16_t cntr = 0U;
	uint8_t failed = 1U;
	uint8_t buf[PAGE_SIZE];

	//NVM_STATUS Page_Status;
//lint -e(685) let adress is always >= 0. START_ADDRESS maybe used by other condition
//lint -e(568)
	if ((Address->Val32 >= FLASH_START_ADDRESS) && (Address->Val32 <= FLASH_END_ADDRESS))
	{
		RAM_Checksum.total = 0U;

		getChar(&Rx_Bytes.byte[1]);		//get number of bytes, high byte
		getChar(&Rx_Bytes.byte[0]);		//get number of bytes, low byte

		//Read page into RAM
		for (cntr = 0U; cntr < Rx_Bytes.total; cntr++)
		{
			getChar(&buf[cntr]);
			RAM_Checksum.total += buf[cntr];
		}

		while (failed)
		{
			if(Rx_Bytes.total == 0U)
			{
				Erase_Flash_Page(Address->Val32);
				RAM_Checksum.total = 0xff00U;
			}
			else
			{
				//lint --e{569} Loss of information (arg. no. 1) (29 bits to 16 bits)
			  //lint --e{712} loss precision
				FMC_Write_Array(Address->Val32, PAGE_SIZE >> 2U, buf);
			}

			Flash_Checksum = Read_Flash_Page(Address,Tx_OFF);
//lint --e{734} Flash_Checksum^RAm_Checksum.total = 1 or 0
			failed = (Flash_Checksum ^ RAM_Checksum.total);
		}
//lint --e{960} use of non-numeric value is safe
//lint --e{9117} implicit conversion changes signedness 
		xmitchar('M');	//echo cmd

		xmitchar(0);	//send the high number of bytes
		xmitchar(2);	//send the low number  of bytes

		xmitchar(RAM_Checksum.byte[1]);	//send the high checksum byte
		xmitchar(RAM_Checksum.byte[0]);	//send the low checksum byte
	}
//lint --e{511} Size incompatibility in cast from 'unsigned long'
//lint --e{923}	cast from unsigned long to pointer
	SET_BAUD(UART_BAUD_SLOW);
}
/*
 * Parameters(IN)   : void
 *
 * Return value     : void
 *
 * Description      : This function shall call FEE write operation and recovery routine if it fails.
 *
 */
void FEE_Write(void)
{
	switch(bank_state)
	{
		case 0x00://B0 empty, B1 empty
		case 0x20://B0 invalid, B1 empty
		case 0x02://B0 empty, B1 invalid
		case 0x01://B0 empty, B1 valid
		case 0x43://B0 old, B1 new
		case 0x21://B0 invalid, B1 valid			
		{
			//transfer 4pcs 8bits to 32bits
			//lint --e{569} Loss of information (arg. no. 1) (29 bits to 16 bits)
			//lint --e{712} loss precision
			FMC_Write_Array(FMC_BANK0_START_ADRESS, (FEE_SIZE >> 2U), g_FEE_Buffer);//write buffer to bank0
			break;
		}			
		case 0x34://B0 new, B1 old
		case 0x10://B0 valid, B1 empty
		case 0x12://B0 valid, B1 invalid
		{
			//lint --e{569} Loss of information (arg. no. 1) (29 bits to 16 bits)
			//lint --e{712} loss precision
			FMC_Write_Array(FMC_BANK1_START_ADRESS, (FEE_SIZE >> 2U), g_FEE_Buffer);//write buffer to bank1
			break;
		}
		case 0x22://B0 invalid, B2 invalid
		{
			//transfer 4pcs 8bits to 32bits
			//lint --e{569} Loss of information (arg. no. 1) (29 bits to 16 bits)
			//lint --e{712} loss precision
			FMC_Write_Array(FMC_BANK0_START_ADRESS, (FEE_SIZE >> 2U), g_FEE_Buffer);
			FMC_Write_Array(FMC_BANK1_START_ADRESS, (FEE_SIZE >> 2U), g_FEE_Buffer);
			break; 
		}
		default:
		{
			break;
		}    			
	}
}
/*
 * Parameters(IN)  	: void
 *
 * Return value   	: void
 *
 * Description    	: This routine calls FEE read operation (with CRC check) and in case of failure, it calls recovery function.
 */
void FEE_Read(void)
{
	Get_Bank_Final_State(&bank_state);
	switch(bank_state)
	{
		case 0x00U://B0 empty, B1 empty
		case 0x20U://B0 invalid, B1 empty
		case 0x02U://B0 empty, B1 invalid
		case 0x22U://B0 invalid, B2 invalid
		{
			*g_FEE_Buffer = 0xFFU;
			break;
		}
		case 0x10U://B0 valid, B1 empty
		case 0x34U://B0 new, B1 old
		case 0x12U://B0 valid, B1 invalid		
		{
					//transfer 4pcs 8bits to 32bitsbuf
			//lint --e{569} Loss of information (arg. no. 1) (29 bits to 16 bits)
		  //lint --e{712} loss precision
			FMC_Read_Array(FMC_BANK0_START_ADRESS, (FEE_SIZE >> 2U), g_FEE_Buffer); // read out bank0 to buffer
			break; 
		}
		case 0x01://B0 empty, B1 valid
		case 0x21://B0 invalid, B1 valid
		case 0x43://B0 old, B1 new
		{
			//lint --e{569} Loss of information (arg. no. 1) (29 bits to 16 bits)
			//lint --e{712} loss precision
			FMC_Read_Array(FMC_BANK1_START_ADRESS, (FEE_SIZE>>2U), g_FEE_Buffer);// read out ban 1 to buffer
			break;
		}	
		default:
		{
			break;
		}
	}
}
/*
 * Parameters(IN)   : void
 *
 * Return value     : void
 *
 * Description      : After get the bank valid/invalid it needs to judge which bank is new.
 */
//lint --e{952} *state could not be declared as const
static void Get_Bank_Final_State(uint8_t* state)
{
	uint32_t bank0_start_count = 0U;
	uint32_t bank1_start_count = 0U;
	uint32_t bank0_end_count = 0U;
	uint32_t bank1_end_count = 0U;
	uint8_t bank0_state = 0U;
	uint8_t bank1_state = 0U;
	//lint --e{569} Loss of information (arg. no. 1) (29 bits to 16 bits)
	//lint --e{712} loss precision
	bank0_start_count = FMC_Read_Data(FMC_BANK0_START_ADRESS) & 0xFFFFFU;
	bank1_start_count = FMC_Read_Data(FMC_BANK1_START_ADRESS) & 0xFFFFFU;
	bank0_end_count = FMC_Read_Data(FMC_BANK0_END_ADRESS) >> 8U;
	bank1_end_count = FMC_Read_Data(FMC_BANK1_END_ADRESS) >> 8U;
	//get bank0, bank1 state
	if (bank0_start_count == bank0_end_count)
	{
		//lint --e{650} bank_start/end_count has treat as 24 bits.
		if (bank0_start_count == 0xFFFFFFU)
		{
			bank0_state = BANK_EMPTY;
		}
		else
		{
			bank0_state = BANK_VALID;
		}
	}
	else
	{
		bank0_state = BANK_INVALID;
	}
	if (bank1_start_count == bank1_end_count)
	{
		//lint --e{650} bank_start/end_count has treat as 24 bits.
		if (bank1_start_count == 0xFFFFFFU)
		{
			bank1_state = BANK_EMPTY;
		}
		else
		{
			bank1_state = BANK_VALID;
		}
	}
	else
	{
		bank1_state = BANK_INVALID;
	}
	
	if ((bank0_state == BANK_VALID) && (bank1_state == BANK_VALID))
	{
		if (bank0_start_count >= bank1_start_count)
		{
			bank0_state = BANK_NEW;//0x03
			bank1_state = BANK_OLD;//0x04
		}
		else
		{
			bank1_state = BANK_NEW;//0x03
			bank0_state = BANK_OLD;//0x04
		}
	}
	//get both B0, B1 state
	//lint --e{734} bank0_state <= 4 12bit to 8bit is safe
	*state = bank0_state << 4 | bank1_state;
}
static void Increment_EEP_Buffer(const uint8_t bytes, const uint8_t increment)
{
    uint8_t        i;
    uint32_t       temp        = 0U;
//lint --e{921} cast from unsigned int to unsigned int 
    const uint32_t max         = ((uint32_t)(1U << (8U * bytes)) - 1U);

    for (i = 0U; i < bytes; i++)
    {
			//lint --e{701} Shift left of signed quantity
        temp += (g_FEE_Buffer[i]) << (8U * i);
    } // End of for (i = 0U; i < bytes; i++)

    if (temp < max - increment)
    {
        temp += increment;
    }
    else
    {
        temp = max;
    }

    for (i = 0U; i < bytes; i++)
    {
      g_FEE_Buffer[i] = temp & 0xFFU; //[0][1][2] +1;
			g_FEE_Buffer[509U + i] = temp & 0xFFU; //[509][510][511]+1; 
      temp >>= 8U;
    } // End of for (i = 0U; i < bytes; i++)

    return;
}