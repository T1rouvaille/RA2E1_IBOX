#include <sys/reboot.h>

#include "datalog_handler.h"
#include "datalog_comms.h"
#include "personalization.h"
#include "utils.h"

static const struct device *uart = DEVICE_DT_GET(DT_NODELABEL(uart1));
extern log_t log;

/*
 * Read characters from UART until line end is detected. Afterwards push the
 * data to the message queue.  Not super efficient since we use interrupt rx but polling Tx code but that shouldn't matter
 * for the application of jsut pulling service logs
 */
void serial_cb(const struct device *dev, void *user_data)
{
    static char byte_1,byte_2,byte_3,byte_4; // 4 byte msg holder, that's all we need 
	uint8_t c;

	if (!uart_irq_update(uart)) {
		return;
	}

	while (uart_irq_rx_ready(uart)) {

		uart_fifo_read(uart, &c, 1);

	    // assemble 4 byte msg ()
        byte_4 = byte_3; 
        byte_3 = byte_2;
        byte_2 = byte_1;
        byte_1 = c;
        // parse  msg

        // possibility of an iBox cmd
        if(byte_1 == IDENTIFY){
            DL_Send_Identify_Msg();
    //    }else if(byte_4 == SET_ADDR_BYTE && byte_1 == ETX){
        }else if(byte_1 == SET_ADDR_BYTE){
            DL_Send_Set_Addr_Msg();
        }else if(byte_1 == READ_EEPROM_BYTE){
            DL_Send_Read_EEPROM_Msg();
        }else if(byte_1 == READ_FLASH_BYTE){
            DL_Send_Read_FLASH_Msg();
        }else if( byte_1 == 'B'){
            uart_poll_out(uart, byte_1);
        }
	}
}
   
    

// ==== BINARY COMMANDS =====

// echo identify msg back, win bootloader is asking for ibox version
// 
void DL_Send_Identify_Msg(void){

    uart_poll_out(uart, IDENTIFY);
    uart_poll_out(uart, 0x04);
    uart_poll_out(uart, 0x45);
    uart_poll_out(uart, ETX);
}

// echo set addr msg back, win bootloader is asking for ibox version
//
void DL_Send_Set_Addr_Msg(void){

    uart_poll_out(uart, SET_ADDR_BYTE);
    uart_poll_out(uart, 0xff);
    uart_poll_out(uart, 0xff);
    uart_poll_out(uart, ETX);
}

// send personalization message
//
void DL_Send_Read_FLASH_Msg(void)
{
    uint32_t i,offset;
     // hipe encoding (Read_Version_Info)

    // echo COMMAND_READ_FLASH = 'N'
    // echo PAGE SZ MSB  (84)
    // echo PAGE SZ LSB

    // echo [8] module ID
    // echo [8] part #
    // echo [48] pers

    // echo ETX = 0x03
    static uint8_t data_str[88];
    data_str[0] = READ_FLASH_BYTE;
    data_str[1] = 0;
    data_str[2] = 84;
    //3-86 = data
	//send Module ID to Bootloader version- 7 bytes
    offset = 3;
	for (i=0;i < BYTES_MODULE_ID;i++) // test - removed byte
	{
        data_str[offset+i] = g_module_id[i];
	}
    offset+=BYTES_MODULE_ID;

    // this is a bit nuts... now send module id part num and the pers data
    // so kind of re-sending it????
    for (i=0;i < BYTES_MODULE_ID;i++) // test - removed byte
	{
        data_str[offset+i] = g_module_id[i];
	}
    offset+=BYTES_MODULE_ID;
    data_str[offset++] = ' ';

    for (i=0;i < BYTES_PART_NUMBER;i++)
	{
        data_str[offset+i] = g_tool_part_number[i];
	}
    offset+=BYTES_PART_NUMBER;

    for (i=0;i < BYTES_PERSONALIZATION;i++)
	{
        data_str[offset+i] = g_personalization[i];
	}
    offset+=BYTES_PERSONALIZATION;

    for (i=0;i < BYTES_CODE_VERSION;i++) // test - moved
	{
        data_str[offset+i] = g_code_version[i];
	}
    offset+=BYTES_CODE_VERSION;

    data_str[offset++] = SW_VERSION_MAJOR;
    data_str[offset++] = SW_VERSION_MINOR;
    // verify version, no use but kep for compatability
    data_str[offset++] = SW_VERSION_MAJOR;
    data_str[offset++] = SW_VERSION_MINOR;

    // serialization - cpu#
    // SERIALIZATION_BYTES;
    // composite data made up of cpu serial and MAC address
    uint32_t tmp = NRF_FICR->DEVICEID[0];
    memcpy(&data_str[offset+=4],&tmp,4); // cpu

    tmp = NRF_FICR->DEVICEID[1];
    memcpy(&data_str[offset+=4],&tmp,4);

    tmp = NRF_FICR->DEVICEADDR[0];
    memcpy(&data_str[offset+=4],&tmp,4); //mac

    tmp = NRF_FICR->DEVICEADDR[1]; 
    memcpy(&data_str[offset+=4],&tmp,4);

    data_str[offset++] = MODULE_REVISION;
    data_str[offset++] = ETX;

    if(offset == sizeof(data_str))
    {
        for(int i=0;i<sizeof(data_str);i++){ // poll out the tx data
            uart_poll_out(uart, data_str[i]);
        }
    }
    else{
        HALT_ON_ERROR // packet is mal-formed
    }	
}

// send log message
//
void DL_Send_Read_EEPROM_Msg(void){
    uint32_t offset=0;
    // echo COMMAND_READ_EEPROM = 'F'
    // echo FEE SZ MSB  (512) (was 256 for eeprom)
    // echo FEE SZ LSB

    // echo 512 bytes of data

    // echo ETX = 0x03

    //static uint8_t data_str[448]; //516 HiPe TODO: Make correcct size needs new module
    static uint8_t data_str[DL_LOG_SIZE + DL_HEADER_SIZE];  // act sz +4 
    data_str[offset++] = READ_EEPROM_BYTE;
    data_str[offset++] = DL_LOG_SIZE >> 8;
    data_str[offset++] = DL_LOG_SIZE & 0xff; //payload size only

    //3-514 = data 
    memcpy(&data_str[offset],(void *)&log,DL_LOG_SIZE); //err this produces a mem fault at the minutre - keep an eye!!!
    offset = sizeof(data_str)-1; // last char
    data_str[offset] = ETX;

    for(int i=0;i<sizeof(data_str);i++){ // poll out the tx data
        uart_poll_out(uart, data_str[i]);
    }
}



// init datalog comms interface
//
int DL_Comms_Init(void){

    // init uart
	//
	if (!device_is_ready(uart)) {
		printk("uart0 init error!\n"); 
    	return 1;
	}
    
//	if (uart_configure(uart, &uart_cfg) == -ENOSYS) {
//		return -ENOSYS;
//	}
	/* configure interrupt and callback to receive data */
	uart_irq_callback_user_data_set(uart, serial_cb, NULL); // use irq method, dma too slow for small packets
	uart_irq_rx_enable(uart);
 	
    // verify personalization byte sizes
    // if these are too small you'll get a memory corruption error
    // only get here if the strings have been mis-configured
    if(strlen(g_module_id) != BYTES_MODULE_ID)
    {
        printk("module id init error!\n"); 
        return 4;
    }

    if(strlen(g_tool_part_number) != BYTES_PART_NUMBER)
    {
        printk("part number init error!\n");
        return 5;
    }

    if(strlen(g_personalization) != BYTES_PERSONALIZATION)
    {
        printk("persolization init error!\n"); 
        return 6;
    }
    
    if(strlen(g_code_version) != BYTES_CODE_VERSION)
    {
        printk("code veraion init error!\n"); 
        return 7;
    }

    return 0;
}



// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// @@                                            @@      
// @@           UART HELPER FUNCTS               @@
// @@                                            @@      
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@


// helper functs for uart
// write uart as string rather than mem buffer
void uart_write(char* msg){
    uart_tx(uart, msg, strlen(msg), SYS_FOREVER_US);	
}


// ====  write time out - used for tty connection only ====


// write string "dddd : hh : mm : ss"
//
void uart_write_time(uint32_t secs){
    uint32_t days,hrs,mins;
    static char str_data[64];
    
    // calc times
    days = (secs / (24 * 3600));
    secs -= (secs / (24 * 3600)) * (24 * 3600);
    
    hrs = (secs / 3600);
    secs -= (secs / 3600) * 3600;
    mins = (secs / 60);
    secs -= (secs / 60) * 60;

    sprintf(str_data,"%04d:%02d:%02d:%02d",days,hrs,mins,secs);
    uart_write(str_data);
}

// write a num
// 
void uart_write_num(uint32_t num){
    static char str_data[64];
    
    sprintf(str_data,"%d",num);
    uart_write(str_data);
}

// write a fault history enumeration
// 
void uart_write_fault_hist(fault_t num){
       
    switch(num){
        case flt_none: uart_write("\n0 - No Error"); break;
        case flt_eoc: uart_write("\n1 - EOC error"); break;
        case flt_laser_flt: uart_write("\n2 - Laser Fault"); break;  
        case flt_overtemp: uart_write("\n2 - Overtemp Fault"); break;      
        default: ; break;

    }
}









/* === dustbin area === */



// dump personalization and log as a csv
// every time you call print off an item, set init to true to start at beginning
//
/*
void DL_Send_DA_Msg(uint32_t init){
    static uint32_t item=0;
    static uint32_t armed=false;

    // manage state of this funct
    if(init){
        item = 0;
        armed = true;
    }
    if(armed == false){
        return;
    }

    // if armed, print off the data line by line
    switch(item){
        case 0: uart_write("\nModule ID,  "); break;
        case 1: uart_write(g_module_id); break;
        case 2: uart_write("\nTool Part Numnber, "); break;
        case 3: uart_write(g_tool_part_number); break;
        case 4: uart_write("\nPersonalization, "); break;
        case 5: uart_write(g_personalization); break;
        case 6: uart_write("\nCode Version, "); break;
        case 7: uart_write(g_code_version); break;
        case 8: uart_write("\nRevision, "); break;
        case 9: uart_write(g_module_revision); break;

        case 10: uart_write("\n\nApplication Time Low (Time < 30 min), "); break;
        case 11: uart_write_num(log.app_time_bucket_0);  break;
        case 12: uart_write("\nApplication Time Medium (30min < Time 1hr), "); break;
        case 13: uart_write_num(log.app_time_bucket_1);  break;
        case 14: uart_write("\nApplication Time Medium (1hr < Time < 2hr), "); break;
        case 15: uart_write_num(log.app_time_bucket_2);  break;
        case 16: uart_write("\nApplication Time High (Time > 2hr), "); break;
        case 17: uart_write_num(log.app_time_bucket_3);  break;
        case 18: uart_write("\nTotal Count, "); break;
        case 19: uart_write_num(log.app_time_bucket_0 + log.app_time_bucket_1 + log.app_time_bucket_2 + log.app_time_bucket_3);  break;
        case 20: uart_write("\n\nButton Presses, "); break;
        case 21: uart_write_num(log.scan_toggle_cycles);  break;   
        case 22: uart_write("\n\nFault History:"); break;
        case 23: uart_write_fault_hist(log.fault_history[0]); break;
        case 24: uart_write_fault_hist(log.fault_history[1]); break;
        case 25: uart_write_fault_hist(log.fault_history[2]); break;
        case 26: uart_write_fault_hist(log.fault_history[3]); break;
        case 27: uart_write_fault_hist(log.fault_history[4]); break;
        case 28: uart_write_fault_hist(log.fault_history[5]); break;
        case 29: uart_write_fault_hist(log.fault_history[6]); break;
        case 30: uart_write_fault_hist(log.fault_history[7]); break;
        case 31: uart_write_fault_hist(log.fault_history[8]); break;
        case 32: uart_write_fault_hist(log.fault_history[9]); break;
        case 33: uart_write_fault_hist(log.fault_history[10]); break;
        case 34: uart_write_fault_hist(log.fault_history[11]); break;
        case 35: uart_write_fault_hist(log.fault_history[12]); break;
        case 36: uart_write_fault_hist(log.fault_history[13]); break;
        case 37: uart_write_fault_hist(log.fault_history[14]); break;
        case 38: uart_write_fault_hist(log.fault_history[15]); break;   
        default: armed=false;
                 uart_write("\n\n");
                 break;
        / * TODO: finish this function if it is required.  I don't think that we will go ahead with * /
        / * the whole tty logging outtut option - so I think this woll not get developed further but if we * /
        / * do then all the log elements need filling in * /
    }
    item++;    
}
kl - depreciated functionality*/

// A port of bootloader funct 'Read_Version_Info', this 
// transmits the same datablock over the uart (personalizaion etc)
//
/*
void DL_Send_PB_Msg(uint32_t init){
    static uint32_t item=0;
    static uint32_t armed=false;

    // manage state of this funct
    if(init){
        item = 0;
        armed = true;
    }
    if(armed == false){
        return;
    }

    // if armed, print off the data line by line  
    switch(item){
        case 0: uart_write("$PB"); break;
        case 1: DL_Send_Read_FLASH_Msg(); break;
        default: armed=false;
                 break;
    }
    item++;    
}kl depreciated functionality */

// A port of bootloader function 'Cmd_Read_EEPROM', this
// transmits the same datablock over the uart (datalog)
//
/*
void DL_Send_LB_Msg(uint32_t init)
{
    static uint32_t item=0;
    static uint32_t armed=false;

    // manage state of this funct
    if(init){
        item = 0;
        armed = true;
    }
    if(armed == false){
        return;
    }

    // if armed, print off the data line by line 
    switch(item)
    {
        case 0: uart_write("$LB"); break;
        case 1: DL_Send_Read_EEPROM_Msg(); break;
        default: armed=false;
                 break;
    }
    item++;    
}
kl depreciated functionality */ 