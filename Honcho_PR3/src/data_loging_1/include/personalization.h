/*!
 * @file Datalog_Handler.h
 *
 *  Created on: 11 Nov 2022
 *      Author: KXL1003A
 *
 */

#ifndef personalization_H_
#define personalization_H_

#define BYTES_MODULE_ID 7
//                  [-------]
//char* g_module_id = "N677739"; // identify module TODO: use correct module
char* g_module_id = "N777777"; 

#define BYTES_PART_NUMBER 8
//                         [--------]
//char* g_tool_part_number = "NA261933"; // identify tool sw TODO: use correct tool part number
char * g_tool_part_number = "NA777777"; // identify tool sw TODO: use correct tool part number

#define BYTES_PERSONALIZATION 36
// SW_PURPOSE + SW_DATE + SW_TOOL + _SW_BUILD + SW_Volt_And_Module * SW_MARKET + ( SW_TOOL_VERSION originally I've taken it out)
//                        [----------++++++++++----------++++++]
char* g_personalization = "Test 16Nov Rotary Laser M1 20VHE US ";

#define BYTES_CODE_VERSION 4
//                     [----]
char* g_code_version = "v1.0"; 

// ==== MODULE INFO =====
// may not apply to this tool?
#define BYTES_VERSIONS 2 // selects sml I think
#define SW_VERSION_MAJOR 0x13 // platform version just leave as is
#define SW_VERSION_MINOR 0x32
char g_software_version[3] = {SW_VERSION_MAJOR,SW_VERSION_MINOR,0};
#define MODULE_REVISION 0x04
char g_module_revision[2] = {MODULE_REVISION,0}; //copied from HiPe, err just leave

#define SERIALIZATION_BYTES 16



#endif /* personalization_H_ */