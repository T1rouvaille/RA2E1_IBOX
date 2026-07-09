/*!
 * @file Datalog_Handler.c
 *
 *  Created on: Nov 11
 *      Author: KXL1003A
 */
#include "datalog_handler.h"


log_t DL_log = {0};
static struct nvs_fs fs;

/**
 * Update Application Time counter in EEPROM.
 * This adds 1 count to proper app time bucket.
 * Called when exiting the RUNNING state.
 * 
 * Where: Call just before tool shuts down
 *        run_time should increment as app runs
 * 
 * Tested: 31 Mar 2022
 * Modified:  31 Mar 2022
 */
// void DL_Log_App_Time(uint32_t run_time)
// {
// 	if(run_time < (LOG_TICKS_PER_MIN * 30)) // 1s increment
// 	{
// 		DL_Inc_U32(&DL_log.app_time_bucket_0);
// 	}
// 	else if(run_time < (LOG_TICKS_PER_MIN * 60))
// 	{
// 		DL_Inc_U32(&DL_log.app_time_bucket_1);
// 	}
// 	else if(run_time < (LOG_TICKS_PER_MIN * 120))
// 	{
// 		DL_Inc_U32(&DL_log.app_time_bucket_2);
// 	}
// 	else
// 	{
// 		DL_Inc_U32(&DL_log.app_time_bucket_3);
// 	}

// }


/**
 * @brief Update the time logs. New logs should be put here to update every 1s.
 * Check the state of the tool... and increment the corresponding time bucket.
 * 
 * Where: Place this in a 1s loop
 * 
 * Tested: 31 Mar 2022
 * Modified:  31 Mar 2022
 */

void DL_Update_Buckets(/* pass data to funct containing time bucket info*/)
{

	// enum #s index fault msgs in the xml file

	// run this before shutdown, the funct needs to know what the shutdown reason was
	// it will then log it
	//
	// if ... low power
	//		DL_Inc_U32(corresponding power bucket)
	// else if ... med power
	//		DL_Inc_U32(corresponding power bucket)
	// else if ... high power
	//		DL_Inc_U32(corresponding power bucket)
	//      

}// END DL_Update_Timers


// function to prevent theoretical log item overflow
//
// Where: In application code e.g. catch button press or other tool event
//
void DL_Inc_U32(uint32_t *item){
	
	if(*item != 0xffffffff)
	{
		(*item)++;
	}
}

// function to prevent theoretical log item overflow
//
void DL_Add_U32(uint32_t *item, uint32_t num){
	
	if((*item + num) > *item)
	{
		(*item)+=num;
	}
}

// function to prevent theoretical log item overflow
//
void DL_Inc_U16(uint16_t *item){
	
	if(*item != 0xffff)
	{
		(*item)++;
	}
}

// function to prevent theoretical log item overflow
//
void DL_Add_U16(uint16_t *item, uint16_t num){
	
	if((*item + num) > *item)
	{
		(*item)+=num;
	}
}

/**
 * @brief Pushes a Tool_State (Fault Record) onto the Fault History.
 *
 * First shift the existing history down on element, overwritting the last element.  Then push the
 * new tool state on to the history.
 * 
 * Modified: December 5, 2014
 * Tested: _____________
 */
//  void DL_Increment_History( fault_t fault )
// {
//     for(int i=DL_FAULT_HISTORY_SIZE-1;i>0;i--){
// 		DL_log.fault_history[i] = DL_log.fault_history[i-1];
// 	}
// 	DL_log.fault_history[0]=(uint8_t)fault;

// } // End of static void DL_Increment_History(void)


/**
 * @brief Increment the shutdown counters if the flag has been set.
 * 
 * Where: Call once just before tool shutdown
 * 
 * Tested: 31 Mar 2022
 * Modified:  31 Mar 2022
 */

// void DL_Update_Shutdown_Log(/*pass vars holding shutdown fault flags*/ )
// {
// 	fault_t faultmask=flt_none;
	
// 	// enum #s index fault msgs in the xml file

// 	// example mindless increment.  I neality your code is here na dit looks for faults
// 	faultmask = DL_log.fault_history[0]+1;
// 	if(faultmask == flt_last)
// 	{
// 		faultmask = flt_none;
// 	}

// 	// run this before shutdown, the funct needs to know what the shutdown reason was
// 	// it will then DL_log it
// 	//
// 	// if ... fault
// 	//		DL_Inc_U32(corresponding shutdown counter)
// 	//      faultmask = fault
// 	//
// 	// if ... any fault detected
// 	//      Increment fault history buffer
// 	// else
// 	//      Do not inc fault history buffer


// 	// if a fault exists then increment the history
// 	if(faultmask != flt_none){
// 		DL_Increment_History((uint32_t)faultmask);
// 	}
// }



// Initialize an empty log
//
void DL_Init_Log(void)
{
	memset(&DL_log,0,sizeof(DL_log));
	DL_log.check_word = DL_CHECK_WORD;
}


int DL_Init_NVM(void){
	int rc = 0;
	struct flash_pages_info info;

	/* define the nvs file system by settings with:
	 *	sector_size equal to the pagesize,
	 *	6 sectors = 12K?
	 *	starting at FLASH_AREA_OFFSET(storage)
	 */
	fs.flash_device = FLASH_AREA_DEVICE(STORAGE_NODE_LABEL);
	if (!device_is_ready(fs.flash_device)) {
		printk("Flash device %s is not ready\n", fs.flash_device->name);
		return 8;
	}
	// printk("Flash device %s is not ready\n", fs.flash_device->name);
	fs.offset = FLASH_AREA_OFFSET(datalog_area);//237568
	rc = flash_get_page_info_by_offs(fs.flash_device, fs.offset, &info);//0=ok
	if (rc) {
		printk("Unable to get page info\n");
		return 9;
	}
	fs.sector_size = info.size;//size 4096  index 58  
	fs.sector_count = 6U; // use all sectors in this DT
	printk("_________________________________________________________\n");
	printk("idx=%d sz=%d os=%d\n",info.index,info.size,info.start_offset);

	rc = nvs_mount(&fs);
	if (rc) {
		printk("Flash Init failed %d\n",rc);
		// rc = nvs_clear(&fs);
		// if(rc)
		// {
		// 	printk("Cannot clear %d\n", rc);
		// }
		// else	
		// 	sys_reboot(0);
		// return 10;
	}

	/* LOG_ID is used to store a DL_log, lets see if we can
	 * read it from flash, since we don't know the size read the
	 * maximum possible
	 */
	
	if(DL_LOG_SIZE != sizeof(DL_log)) // catch DL_log sz errors
	{
		printk("Log structure size error!\n"); 
		return 11;
	}
	rc = nvs_read(&fs, LOG_ID, &DL_log, sizeof(DL_log));
	printk("reading bytes=%d rc=%d\n",sizeof(DL_log),rc); 
	if ((rc > 0) && (DL_log.check_word == DL_CHECK_WORD)) { /* item was found, show it */
		// print out basic DL_logged parameters
		printk("nvs meta data: start Idx: %d,check word: %x\n", LOG_ID, DL_log.check_word);
		printk("app time 0: %d\n", /*DL_log.app_time_bucket_0,*/DL_log.total_runtime);
		// printk("switch presses: %d\n", DL_log.scan_toggle_cycles);
		printk("_________________________________________________________\n");
		printk("\n");
	} else   {/* item was not found, add it */
		// clt r log
		DL_Init_Log();
		printk("No log entry found, adding empty log at id %d, %d bytes\n", LOG_ID,sizeof(DL_log));	
	}
	printk("Datalog area init complete\n");
	return 0;
}

// write the datalog to nvs
//
void DL_Write(void){
		int err;
		// printk("Reached here in sysof 45 \n");
		err = nvs_write(&fs, LOG_ID, &DL_log, sizeof(DL_log));
		// printk("Reached here in sysof 46 \n");
		if( err < 0)
			printk("error writing to flash %d \n", err);
		else
			printk("write successful\n");
}