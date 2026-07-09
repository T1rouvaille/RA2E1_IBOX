/*
*  Copyright (c) 2022, Stanley Black and Decker 
*    All rights reserved
* Author : Kavya Jain 
* Created on : 14 Sept 2023
*/

#include "calibration.h"

static struct nvs_fs fs_calib;
static struct nvs_fs fs_flashvar;
calib_t calib = {0};
flashvar_t flash_mem = {0};
volatile bool system_calibrated_flag = 0;

void FLASHVAR_init_var()
{
	memset(&flash_mem, 0, sizeof(flash_mem));
}
void CALIB_init_var()
{
    memset(&calib, 0, sizeof(calib));
}

int FLASHVAR_init_NVM()
{
	int rc = 0;
	struct flash_pages_info info;

	/* define the nvs file system by settings with:
	 *	sector_size equal to the pagesize,
	 *	6 sectors = 12K?
	 *	starting at FLASH_AREA_OFFSET(storage)
	 */
	fs_flashvar.flash_device = FLASH_AREA_DEVICE(VARIABLE_FLASH_NODE_LABEL);
	if (!device_is_ready(fs_flashvar.flash_device)) {
		printk("Flash device %s is not ready\n", fs_flashvar.flash_device->name);
		return 8;
	}
	fs_flashvar.offset = FLASH_AREA_OFFSET(laserstatus_area);//237568
	rc = flash_get_page_info_by_offs(fs_flashvar.flash_device, fs_flashvar.offset, &info);//0=ok
	if (rc) {
		printk("Unable to get page info\n");
		return 9;
	}
	fs_flashvar.sector_size = info.size;//size 4096  index 58  
	fs_flashvar.sector_count = 2U; // use all sectors in this DT
	printk("_________________________________________________________\n");
	printk("idx=%d sz=%d os=%d\n",info.index,info.size,info.start_offset);

	rc = nvs_mount(&fs_flashvar);
	if (rc) {
		printk("Flash Init failed\n");
		return 10;
	}

	/* LOG_ID is used to store a log, lets see if we can
	 * read it from flash, since we don't know the size read the
	 * maximum possible
	 */
	rc = nvs_read(&fs_flashvar, FLASHVAR_ID, &flash_mem, sizeof(flash_mem));
	printk("reading bytes=%d rc=%d\n",sizeof(flash_mem),rc); 
	if ((rc > 0)) { /* item was found, show it */
		// print out basic logged parameters
		printk("nvs meta data: start Idx: %d\n", FLASHVAR_ID);
        printk("Laser register is = %d, Drop detect reg is = %d\n",flash_mem.laser_status_mem, flash_mem.Drop_detect_mem);
		printk("_________________________________________________________\n");
		printk("\n");
	} else   {/* item was not found, add it */
		// clt r log
		FLASHVAR_init_var();
		// (void)nvs_write(&fs_flashvar, FLASHVAR_ID, &flash_mem, sizeof(flash_mem));
		printk("No log entry found, adding empty Flash variable at id %d, %d bytes\n", FLASHVAR_ID,sizeof(flash_mem));	
	}
	printk("Flash varibale area init complete\n");
	return 0;
}
int CALIB_init_NVM()
{
    int rc = 0;
	struct flash_pages_info info;

	/* define the nvs file system by settings with:
	 *	sector_size equal to the pagesize,
	 *	6 sectors = 12K?
	 *	starting at FLASH_AREA_OFFSET(storage)
	 */
	fs_calib.flash_device = FLASH_AREA_DEVICE(CALIB_FLASH_NODE_LABEL);
	if (!device_is_ready(fs_calib.flash_device)) {
		printk("Flash device %s is not ready\n", fs_calib.flash_device->name);
		return 8;
	}
	fs_calib.offset = FLASH_AREA_OFFSET(calib_area);//237568
	rc = flash_get_page_info_by_offs(fs_calib.flash_device, fs_calib.offset, &info);//0=ok
	if (rc) {
		printk("Unable to get page info\n");
		return 9;
	}
	fs_calib.sector_size = info.size;//size 4096  index 58  
	fs_calib.sector_count = 2U; // use all sectors in this DT
	printk("_________________________________________________________\n");
	printk("idx=%d sz=%d os=%d\n",info.index,info.size,info.start_offset);

	rc = nvs_mount(&fs_calib);
	if (rc) {
		printk("Flash Init failed\n");
		return 10;
	}

	/* LOG_ID is used to store a log, lets see if we can
	 * read it from flash, since we don't know the size read the
	 * maximum possible
	 */
	rc = nvs_read(&fs_calib, CALIB_ID, &calib, sizeof(calib));
	printk("reading bytes=%d rc=%d\n",sizeof(calib),rc); 
	if ((rc > 0)) { /* item was found, show it */
		// print out basic logged parameters
		printk("nvs meta data: start Idx: %d\n", CALIB_ID);
        printk("MAC_ADDR IS %d:%d:%d:%d:%d:%d\n",calib.mac_addr[0],calib.mac_addr[1],calib.mac_addr[2],
                                                                                       calib.mac_addr[3],calib.mac_addr[4],calib.mac_addr[5]);
        printk("roll_xoffset : %f, pitch_offset %f,", calib.acc_roll_angle_zero_offset, calib.acc_pitch_angle_zero_offset);
		printk("_________________________________________________________\n");
		printk("\n");
		system_calibrated_flag =1;
	} else   {/* item was not found, add it */
		// clt r log
		CALIB_init_var();
		printk("No log entry found, adding empty log at id %d, %d bytes\n", CALIB_ID,sizeof(calib));	
	}
	printk("Calibration area init complete\n");
	return 0;
}

calib_t CALIB_read()
{
	int rc = 0;
	rc = nvs_read(&fs_calib, CALIB_ID, &calib, sizeof(calib));
	return (calib_t)calib;

}
void FLASHVAR_write()
{
	int rc = nvs_write(&fs_flashvar, FLASHVAR_ID, &flash_mem, sizeof(flash_mem));
    if( rc < 0)
			printk("error writing to flash %d \n", rc);
		else
			printk("write successful for laser register\n");
}

void CALIB_write()
{
    int rc = nvs_write(&fs_calib, CALIB_ID, &calib, sizeof(calib));
    if( rc < 0)
			printk("error writing to flash %d \n", rc);
		else
			printk("write successful\n");//flash some LEDs

}

void FLASHVAR_update(uint32_t *item, uint32_t num)
{
	*item = num;
}

uint8_t FLASHVAR_laser_read()
{
	int rc = 0;
	rc = nvs_read(&fs_flashvar, FLASHVAR_ID, &flash_mem, sizeof(flash_mem));
	return (uint8_t)flash_mem.laser_status_mem;

}