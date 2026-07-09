/*
 * mcu_config.c
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
//lint --e{966} ignore the head files not used by mcu_config.c
#include "main.h"
#include "uart.h"

static uint8_t Get_Comm_Mode(void);

uint8_t communication_mode = 0U;
/***********************************************************************************************************************
* Function Name: Get_Comm_Mode
* Description  : Read value from Flash address BL_HW_MODE_ADDRESS to RAM.
* Arguments    : None
* Return Value : 'S' or 'D'
***********************************************************************************************************************/
static uint8_t Get_Comm_Mode(void)
{
	uint32_t comm_mode = *((uint32_t*)BL_HW_MODE_ADDRESS);
	return (uint8_t)(comm_mode & 0xFF);
}

/***********************************************************************************************************************
* Function Name:
* Description  : 
* Arguments    : 
* Return Value : 
***********************************************************************************************************************/
//lint --e{9117} implicit conversion changes signedness
//lint --e{511} Size incompatibility in cast from 'unsigned long'
//lint --e{923} cast from from unsigned long to pointer  
void BSP_MCU_Config(void)
{		
	//Check if clock setting is 64MHz after bootloader code.
	if ((SYS->CLKCON & SYS_CLKCON_IRCSEL_Msk) != SYS_CLK_HSI_64M)
	{
		SYS->CLKCON = SYS_CLKSEL_WRITE_KEY | SYS_CLK_HSI_EN | SYS_CLK_HSI_64M;
	}
	SYS->CLKSEL = SYS_CLKSEL_WRITE_KEY | SYS_CLK_SEL_HSI;
	
	SYS->AHBCKDIV = 0U;			//HCLK = F-SYS
	
	SYS->APBCKDIV = 0U;			//PCLK = HCLK
	
	SYS->APBCKEN = 0xFFFFFFFFU;	//enable all Peripheral clock
#if(1)	
	/*pin config*/
	BSP_GPIOConfig(); 
#endif
	communication_mode = Get_Comm_Mode();
#if(1)
	/*UART config*/ 
	BSP_UART1Config();
#endif	
#if(1)
	/*timer config*/
	BSP_TMR0Config();
#endif	
}

/***********************************************************************************************************************
* Function Name:
* Description  : 
* Arguments    : 
* Return Value : 
***********************************************************************************************************************/
void BSP_GPIOConfig(void)
{
		SYS_DisableIOCFGProtect();
		SYS_DisableGPIO0Protect();
		SYS_DisableGPIO1Protect();
		SYS_DisableGPIO2Protect();
		SYS_DisableGPIO3Protect();
		SYS_DisableGPIO4Protect();
//lint --e{511} Size incompatibility in cast from 'unsigned long'
//lint --e{923}	cast from unsigned long to pointer
		GPIO0->DO = 0;
		GPIO1->DO = 0;
		GPIO2->DO = 0;
		GPIO3->DO = 0;
		GPIO4->DO = 0;

	//-----------------------------USER SET IO-------------------------------



#if(1)// OUTPUT

		//lint --e{712} ignore loss of precision (assignment). unsigned long to unsigned int
		SYS_SET_IOCFG(IOP44CFG,SYS_IOCFG_P44_GPIO);	   
	  P4_4_set_mode(OUTPUT_PP_GP);//LED
		
		SYS_SET_IOCFG(IOP21CFG,SYS_IOCFG_P21_GPIO);	   
	  P2_1_set_mode(OUTPUT_PP_GP);//Battery-Sense
		
		SYS_SET_IOCFG(IOP10CFG,SYS_IOCFG_P10_GPIO);	   
	  P1_0_set_mode(OUTPUT_PP_GP);//Latch-on
			
		SYS_SET_IOCFG(IOP40CFG,SYS_IOCFG_P40_SWDDAT2);
		SYS_SET_IOCFG(IOP00CFG,SYS_IOCFG_P00_SWDCLK2);

#endif


}
/***********************************************************************************************************************
 ** \brief	UART_UART1_Config
 **			
 ** \param [in] none
 ** \return  none
 ** \note	
***********************************************************************************************************************/

void BSP_UART1Config(void)
{
	const uint32_t  BuadRate = 9467U;
	/*
	(1)UARTx mode setting
	*/
	//lint --e{923} cast from unsigned long to ointer
	//lint --e{511} Size incompatibility in cast from 'unsigned long'
	UART1->LCR = (UART_WLS_8 << UART_LCR_WLS_Pos);
	UART1->DLR = 4000000/BuadRate; //__HIRC_64M/16/
	/*
	(2)UARTx enable clock
	*/	
	//SYS_EnablePeripheralClk(SYS_CLK_UART1_MSK);
	/*
	(3)UARTx IO setting
	*/
	P3_6_set_mode(INPUT);
	SYS_SET_IOCFG(IOP36CFG, SYS_IOCFG_P36_RXD1);    //RX GPIO SET - TH
	if(communication_mode == 0x44)	//'D' = 0x44
	{
		SYS_SET_IOCFG(IOP35CFG, SYS_IOCFG_P35_TXD1);    //TX GPIO SET	
	}
	else
	{
		; //nothing here for sigle wire. 
	}
}
//lint --e{921} char is defined as unsigned char
void getChar(uint8_t* Char)
{
	if(communication_mode == 0x44)	//'D' = 0x44
	{
		; //nothing here for dual wire. 
	}
	else
	{
		SYS_SET_IOCFG(IOP36CFG, SYS_IOCFG_P36_RXD1);    //RX GPIO SET - TH
	}
	while(1)
	{
		//lint --e{960} non switch break used
		if(((TMR0->RIS & TMR_RIS_RIS_Msk)? 1:0))		//if interrupt has timed out, exit communication
		{
			*Char = COMMAND_NACK;
			break;
		}

		if(UART1->LSR & UART_LSR_RXFE_Msk)					// Check for receive errors
		{
			//UART1->LSR = 0xFFFF;
			//lint --e{586} keyword 'continue' is deprecated
			continue;
		}
		if (UART1->LSR & UART_LSR_RDR_Msk)
		{			
			while(!(UART1->LSR & UART_LSR_RDR_Msk)) {;}
			*Char = (uint8_t)(UART1->RBR);
			 break;
		}
	}
}
void xmitchar(const uint8_t ch)
{
	if(communication_mode == 0x44)	//'D' = 0x44
	{
		; //nothing here for dual wire. 
	}
	else
	{
		SYS_SET_IOCFG(IOP36CFG, SYS_IOCFG_P36_TXD1);    //RX GPIO SET - TH
	}
	while(!(UART1->LSR & UART_LSR_THRE_Msk));	// check TDV, wait until TBUF is ready
	UART1->THR = ch;
	while(!(UART1->LSR & UART_LSR_THRE_Msk));
	while(!(UART1->LSR & UART_LSR_TEMT_Msk));
	return;
}
/***********************************************************************************************************************
* Function Name:
* Description  : 
* Arguments    : 
* Return Value : 
***********************************************************************************************************************/
void BSP_TMR0Config(void)
{
	TMR0->CON = ((0x0U << TMR_CON_TMREN_Pos) | 
				 (TMR_CLK_DIV_256 << TMR_CON_TMRPRE_Pos) |
				 (TMR_BIT_16_MODE << TMR_CON_TMRSZ_Pos) | 
				 (0x1U << TMR_CON_TMRMS_Pos));
	
	TMR0->LOAD = GETCHAR_TIME_OUT;
	
	TMR0->ICLR = 0x01;	//clear ISR flag
}
/***********************************************************************************************************************
* Function Name:
* Description  : 
* Arguments    : 
* Return Value : 
***********************************************************************************************************************/
//lint --e{522} Highest operation
//lint --e{950} Non-ANSI reserved word or construct
void BSP_Delay_Us(uint16_t us)
{
	uint16_t i;
	while(us--)
	{
		i = 8U;
		while(--i)
		{
			__NOP();
		}
	}
}

/***********************************************************************************************************************
* Function Name:
* Description  : 
* Arguments    : 
* Return Value : 
***********************************************************************************************************************/

void BSP_Delay_Ms(uint16_t ms)
{
	while(ms--)
	{ 
		BSP_Delay_Us(1000U); 			// 1ms
	}
}

