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

/****************************************************************************/
/*	include files
*****************************************************************************/
 //lint --e{966} ignore the head files not used by user_fmc.c
#include "main.h"
/****************************************************************************/
/*	Local pre-processor symbols/macros('#define')
*****************************************************************************/

/****************************************************************************/
/*	Global variable definitions(declared in header file with 'extern')
*****************************************************************************/

/****************************************************************************/
/*	Local type definitions('typedef')
*****************************************************************************/

/****************************************************************************/
/*	Local variable  definitions('static')
*****************************************************************************/

/****************************************************************************/
/*	Local function prototypes('static')
*****************************************************************************/

/****************************************************************************/
/*	Function implementation - global ('extern') and local('static')
*****************************************************************************/
enum
{
	FMC_READ_DATA_CMD = 1,
	FMC_WRITE_DATA_CMD = 2,
	FMC_PAGE_ERASE_CMD = 3,
};
/***********************************************************************************************************************
* Function Name:
* Description  : fmc_adress range 0~255
* Arguments    : 
* Return Value : 
***********************************************************************************************************************/
//lint --e{511}
//lint --e{569} ignore loss information
//lint --e{9117} implicit conversion changes signedness
//lint --e{572} precision ___ shifted right by___
//lint --e{598} precision ___ shifted left by___
//lint --e{923} cast from unsigned long to pointer
//lint --e{9130} bitwise operator '<<' applied to signed underlying type
void FMC_Write_Data(uint32_t fmc_adress, const uint32_t fmc_data)
{
	DI();
	/*
	*/
    
	fmc_adress <<= 2;
    
	FMC_UNLOCK();
	while(FMC_GET_BUSY_STATE()){;}
	FMC_ADDR(FMC_START_ADRESS + fmc_adress);
	
	FMC_CMD(FMC_PAGE_ERASE_CMD);
	while(FMC_GET_BUSY_STATE()){;}
	FMC_LOCK();
	
	/*
	*/
	FMC_UNLOCK();
	
	while(FMC_GET_BUSY_STATE()){;}
	FMC_ADDR(FMC_START_ADRESS + fmc_adress);
	FMC->DAT = fmc_data;
	
	FMC_CMD(FMC_WRITE_DATA_CMD);
	while(FMC_GET_BUSY_STATE()){;}

	
	FMC_LOCK();

	EI();


}
/***********************************************************************************************************************
* Function Name:
* Description  : 
* Arguments    : 
* Return Value : 
***********************************************************************************************************************/
uint32_t FMC_Read_Data(const uint32_t fmc_adress)
{	
	uint32_t Rdata;

	
	DI();
	FMC_UNLOCK();
	while(FMC_GET_BUSY_STATE()){;}
	FMC_ADDR(fmc_adress);
	FMC_CMD(FMC_READ_DATA_CMD);
	while(FMC_GET_BUSY_STATE()){;}

	Rdata = FMC->DAT;
	
	FMC_LOCK();
	EI();
	
	return Rdata;
}

/***********************************************************************************************************************
* Function Name:
* Description  : 
* Arguments    : 
* Return Value : 
***********************************************************************************************************************/
void FMC_Read_Array(const uint32_t start_address, const uint16_t Length, uint8_t *buf)
{	
	uint8_t i;
	uint32_t data;
	
	DI();
	FMC_UNLOCK();
	for(i=0; i < Length; i++)
	{
		while(FMC_GET_BUSY_STATE()){;}
		FMC_ADDR(i*4 + start_address);
		FMC_CMD(FMC_READ_DATA_CMD);
		while(FMC_GET_BUSY_STATE()){;}

		data = FMC->DAT;
		
		buf[i*4] = data & 0xFF;
		buf[i*4+1] = (data >> 8U) & 0xFF;
		buf[i*4+2] = (data >> 16U) & 0xFF;
		buf[i*4+3] = (data >> 24U) & 0xFF;
	}
	FMC_LOCK();
	EI();
}
/***********************************************************************************************************************
* Function Name:
* Description  : fmc_adress range 0~255
* Arguments    : 
* Return Value : 
***********************************************************************************************************************/

void FMC_Write_Array(const uint32_t pageAddr, const uint16_t Length, const uint8_t* buf)
{
	uint8_t i;
	DI(); 
    
	FMC_UNLOCK();

	/*erase page*/
	while(FMC_GET_BUSY_STATE()){;}
	FMC_ADDR(pageAddr);
	
	FMC_CMD(FMC_PAGE_ERASE_CMD);
	while(FMC_GET_BUSY_STATE()){;}
	
	FMC_LOCK();
	
	/*
	write page
	*/	
	
	FMC_UNLOCK();
	for(i=0; i < Length; i++)
	{
		while(FMC_GET_BUSY_STATE()){;}
		FMC_ADDR(pageAddr + i*4);
		FMC->DAT = buf[i*4] + (buf[i*4+1] << 8U) + (buf[i*4+2] << 16U) + (buf[i*4+3] << 24U);	
		
		FMC_CMD(FMC_WRITE_DATA_CMD);
		while(FMC_GET_BUSY_STATE()){;}
	}
	FMC_LOCK();
	EI();

}
void Erase_Flash_Page(const uint32_t pageAddr)
{
	DI(); 
    
	FMC_UNLOCK();

	/*erase page*/
	while(FMC_GET_BUSY_STATE()){;}
	FMC_ADDR(pageAddr);
	
	FMC_CMD(FMC_PAGE_ERASE_CMD);
	while(FMC_GET_BUSY_STATE()){;}
	
	FMC_LOCK();
}