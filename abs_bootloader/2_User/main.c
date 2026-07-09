/*
 * Main.c
 *
 *  Created on: Mar 15, 2022
 *      Author: Sally Dai
 *
 *
 *      Module configuration can be switched by choosing #define XXX in globals.h
 *      The configuration loads different pins for UART and I2C and switches the USIC channel used.
 *
 *      The Bootloader lives from 0x0000_7000 to 0x0000_7FFF and does not use any interrupts.
 *      The BOOT sector need to config by the Write8 firstly(similar fuse).
 *      The Application lives from 0x0000_0000 to 0x0000_6FFF.       
 *
 *      Optimization must be set to size to fit in the 1 sectors
 *
 *      Multiple baud rates for "FAST" operations have been calculated and can be selected in UART.h
 *
 *
 *
 *	REVISION HISTORY:
 *
 *	AUTHOR	DATE		COMMENTS
 *	-------------------------------------------------------------------------------------------------------
 *
 *
 *
 *
 */
 //lint --e{966} ignore the head files not used by main.c
#include "main.h"
#include "user_fmc.h"
//const char Module_ID[] = ABS_BOOTLOADER_ID;
//lint --e{960} Disallowed use of non-numeric value in a case label
//const char Revision = MODULE_REVISION;
const char __attribute__((section(".ARM.__at_0x0DF0"))) Module_ID[] = ABS_BOOTLOADER_ID;
const char __attribute__((section(".ARM.__at_0x0DF8"))) Revision = MODULE_REVISION;
const char __attribute__((section(".ARM.__at_0x0DFC"))) BL_HW_Mode = BL_HW_MODE;

uReg32 Address; 
uint8_t g_FEE_Buffer[FEE_SIZE];
uint32_t *p_ram_stack;
uint32_t wrong_address;
uint8_t i = 0;
volatile uint8_t Read_Data[4];
volatile uint32_t Data;
uint8_t  STACK_FILL_VALUE = 0;
uint32_t *p_stack_pointer;
int main()
{ 
	BSP_MCU_Config();
	//Setup self on 
	//lint --e{511} Size incompatibility in cast from 'unsigned long' (4 bytes) to 'struct SYS_T *'
	//lint --e{9117} implicit conversion changes signedness
	//lint --e{923} cast from unsigned long to pointer
	SelfOn_set();
	uint8_t command = 0;
	uint8_t EXIT = 0U;
	uint8_t read_eeprom = 0U;
	uint8_t HP_LED_on = 0U;
	uint8_t HP_LED_cntr = 0U;
	//turn on read BATT
	BattSenseEnable_set();	
	
	BSP_Delay_Ms(5U); //Delay 5ms
	TMR0->CON |= TMR_CON_TMREN_Msk;
	LED_ON();
	p_ram_stack = (uint32_t *)0x0E00;
	p_stack_pointer = (uint32_t *)0x7FFF;
	uint8_t buf[PAGE_SIZE];
	uint8_t Data[PAGE_SIZE];
	while(1)
	{

					for (uint16_t cntr = 0U; cntr < 512; cntr++)
					{
						buf[cntr] = STACK_FILL_VALUE;
					}
					FMC_Write_Array(0x1000, PAGE_SIZE >> 2U, buf);
					FMC_Read_Array(0x1000, PAGE_SIZE >> 2U, Data);
			
					for (uint16_t j = 0U; j < 512; j++)
					{
						if((STACK_FILL_VALUE != Data[j]))
						{
							wrong_address = 0x1000 + (j);
						}	
					}
					STACK_FILL_VALUE++;		

	}
	while (!EXIT)	
	{ 

		// Read external EEPROM.
		if (read_eeprom == 1U)
		{
			FEE_Read();

			read_eeprom = 2U;

		} // End of if (read_eeprom == 1)
		
		getChar(&command);
		if (HP_LED_on)
		{
			if (HP_LED_cntr++ < 1U)
			{
				LED_ON();
			}
			else
			{
				LED_OFF();
			}
			if(HP_LED_cntr >= 10U)
			{
				HP_LED_cntr = 0U;
			}
			//lint -e{960} Disallowed use of non-numeric value in a case label
			if (command != COMMAND_HANDSHAKE)
			{
				LED_OFF();
			}
		}
		
		switch(command)
		{
			//lint --e{960} Disallowed use of non-numeric value in a case label
			case COMMAND_CHANGE_BAUD:
			{
				xmitchar(COMMAND_CHANGE_BAUD);
				//BSP_Delay_Ms(5); //Delay 10ms
				SET_BAUD(UART_BAUD_FAST);
				//BSP_Delay_Ms(10); //Delay 10ms				
				break;
			}
			case COMMAND_HANDSHAKE:	//Received B
			{
				xmitchar(COMMAND_HANDSHAKE);			//reply with a B
				if (!read_eeprom)
				{
					read_eeprom = 1U;	//valid handshake, get eeprom
				}
				LED_ON();				
				HP_LED_on = 1U;
				// stop and clear timer
				TMR0->CON &= ~(TMR_CON_TMREN_Msk);
				break;
			}
			case COMMAND_READ_EEPROM:	//Received F
			{
				Cmd_Read_EEPROM(g_FEE_Buffer);
				break;
			}
			case COMMAND_LOAD_EEPROM://Received F
			{
				Cmd_Load_EEPROM(g_FEE_Buffer);
				break;					
			}				
			case COMMAND_READ_FLASH:
			{
				if ((Address.Val[1] == 0xFFU) && (Address.Val[0] == 0xFFU))
				{
					Read_Version_Info();
				}
				else
				{
					//lint -e(534) Ignoring return value of function. 
					Read_Flash_Page(&Address, Tx_ON);
				}
				break;
			}
			case COMMAND_SET_ADDRESS:
			{
				Set_Address(&Address);
				break;
			}
			case COMMAND_LOAD_FLASH:
			{
				Load_Flash_Page(&Address);
				break;
			}
			case COMMAND_NACK:
			{
				EXIT = 1;
				break;
			}
			default:				//bad char or junk
			{

				break;
			}
		}//end switch
	}	//end while(!EXIT)
	LED_OFF();
//lint --e{712} Loss precision 
	ResetReg();
//lint --e{569} Loss of information (assignment) (31 bits to 16 bits)
//	FMC->LOCK = 0x55AA6699;//FMC_UnLock();
//	FMC->CON = 0x00000010;//boot from aprom
//	FMC->LOCK = 0;//FMC_Lock();						
//	SYS_ResetCPU();
	__set_MSP(*(__IO uint32_t*) FMC_APROM_BASE);
	((void (*)()) (*(volatile unsigned long *)(FMC_APROM_BASE+0x04)))();//Jump to APP
	/* Trap the CPU */
	while(1)
	{
		;
	}
}
static void ResetReg(void)
{
	//lint --e{511} Size incompatibility in cast from 'unsigned long' (4 bytes) to 'struct SYS_T *'
	//lint --e{9117} implicit conversion changes signedness
	//lint --e{923} cast from unsigned long to pointer
  SYS->IOP36CFG = 0x0;
	SYS->IOP35CFG = 0x0;
	UART1->DLR = 0x0;
	UART1->IER = 0x0;
	UART1->FCR = 0x0;
	UART1->LCR = 0x0;
	TMR0->CON = 0x0;
	TMR0->LOAD = 0x0;
}
/*
	NVIC Remap
*/
void NMI_Handler (void)
{
	((void (*)()) (*(volatile unsigned long *)(FMC_APROM_BASE+0x08)))();	
}

void HardFault_Handler(void)
{
	((void (*)()) (*(volatile unsigned long *)(FMC_APROM_BASE+0x0C)))();	
}

void SVC_Handler(void)
{
	((void (*)()) (*(volatile unsigned long *)(FMC_APROM_BASE+0x2C)))();	
}

void PendSV_Handler(void)
{
	((void (*)()) (*(volatile unsigned long *)(FMC_APROM_BASE+0x38)))();	
}

void SysTick_Handler(void)
{
	((void (*)()) (*(volatile unsigned long *)(FMC_APROM_BASE+0x3C)))();	
}

void GPIO0_IRQHandler(void)
{
	((void (*)()) (*(volatile unsigned long *)(FMC_APROM_BASE+0x40+0*4)))();
}

void GPIO1_IRQHandler(void)
{
	((void (*)()) (*(volatile unsigned long *)(FMC_APROM_BASE+0x40+1*4)))();
}

void GPIO2_IRQHandler(void)
{
	((void (*)()) (*(volatile unsigned long *)(FMC_APROM_BASE+0x40+2*4)))();
}

void GPIO3_IRQHandler(void)
{
	((void (*)()) (*(volatile unsigned long *)(FMC_APROM_BASE+0x40+3*4)))();
}

void GPIO4_IRQHandler(void)
{
	((void (*)()) (*(volatile unsigned long *)(FMC_APROM_BASE+0x40+4*4)))();
}

void GPIO5_IRQHandler(void)
{
	((void (*)()) (*(volatile unsigned long *)(FMC_APROM_BASE+0x40+5*4)))();
}

void CCP_IRQHandler(void)
{
	((void (*)()) (*(volatile unsigned long *)(FMC_APROM_BASE+0x40+6*4)))();
}

void ADC0_IRQHandler(void)
{
	((void (*)()) (*(volatile unsigned long *)(FMC_APROM_BASE+0x40+7*4)))();
}

void WWDT_IRQHandler(void)
{
	((void (*)()) (*(volatile unsigned long *)(FMC_APROM_BASE+0x40+9*4)))();
}

void EPWM_IRQHandler(void)
{
	((void (*)()) (*(volatile unsigned long *)(FMC_APROM_BASE+0x40+10*4)))();
}

void ADCB_IRQHandler(void)
{
	((void (*)()) (*(volatile unsigned long *)(FMC_APROM_BASE+0x40+12*4)))();
}

void ACMP_IRQHandler(void)
{
	((void (*)()) (*(volatile unsigned long *)(FMC_APROM_BASE+0x40+13*4)))();
}

void UART0_IRQHandler(void)
{
	((void (*)()) (*(volatile unsigned long *)(FMC_APROM_BASE+0x40+15*4)))();
}

void UART1_IRQHandler(void)
{
	((void (*)()) (*(volatile unsigned long *)(FMC_APROM_BASE+0x40+16*4)))();
}

void TIMER0_IRQHandler(void)
{
	((void (*)()) (*(volatile unsigned long *)(FMC_APROM_BASE+0x40+19*4)))();
}

void TIMER1_IRQHandler(void)
{
	((void (*)()) (*(volatile unsigned long *)(FMC_APROM_BASE+0x40+20*4)))();
}

void TIMER2_IRQHandler(void)
{
	((void (*)()) (*(volatile unsigned long *)(FMC_APROM_BASE+0x40+21*4)))();
}

void TIMER3_IRQHandler(void)
{
	((void (*)()) (*(volatile unsigned long *)(FMC_APROM_BASE+0x40+22*4)))();
}

void WDT_IRQHandler(void)
{
	((void (*)()) (*(volatile unsigned long *)(FMC_APROM_BASE+0x40+23*4)))();
}

void I2C_IRQHandler(void)
{
	((void (*)()) (*(volatile unsigned long *)(FMC_APROM_BASE+0x40+24*4)))();
}

void SSP_IRQHandler(void)
{
	((void (*)()) (*(volatile unsigned long *)(FMC_APROM_BASE+0x40+26*4)))();
}

void SYS_CHK_IRQHandler(void)
{
	((void (*)()) (*(volatile unsigned long *)(FMC_APROM_BASE+0x40+31*4)))();
}

