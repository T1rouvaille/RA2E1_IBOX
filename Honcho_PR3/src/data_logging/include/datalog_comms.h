/*!
 * @file datalog_comms.h
 *
 *  Created on: 16 Nov 2022
 *      Author: KXL1003A
 *
 */

#ifndef datalog_comms_H_
#define datalog_comms_H_
#include <Stdint.h>
#include <drivers/uart.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "utils.h"

#include <zephyr.h>
#include <device.h>
#include <drivers/gpio.h>

#include "system_manager.h"
#include "laser_control.h"
#include "acc_IIM42351.h"

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// @ DEFINES
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
#define RECEIVE_TIMEOUT 100
#define CR '\r'

// iBox comms chars
#define IBOX_COMPATABILITY_ENABLED 1
#define POLL_BYTE 'B'
#define SET_ADDR_BYTE 'D'
#define READ_EEPROM_BYTE 'F'
#define READ_FLASH_BYTE 'N'
#define IDENTIFY 'I'
#define LOAD_FLASH_BYTE 'M'
#define LOAD_EEPROM_BYTE 'E'
#define PACKET_COMMUNICATION '*'
#define ETX 0x03
#define COMMAND_NACK 0x01

// iBox comms size constants


// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// @ DATATYPES
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// @ FUNCTION PROTOTYPES
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

void DL_Send_DA_Msg(uint32_t init);
void DL_Send_PB_Msg(uint32_t init);
void DL_Send_LB_Msg(uint32_t init);
int DL_Comms_Init(void);
void DL_Send_Identify_Msg(void);
void DL_Send_Set_Addr_Msg(void);
void DL_Send_Read_FLASH_Msg(void);
void DL_Send_Read_EEPROM_Msg(void);
void DL_Calibration_Msg(void);

// cbs
void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data);

#define UARTTX_NODE	DT_ALIAS(uarttx)
#if !DT_NODE_HAS_STATUS(UARTTX_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif
static const struct gpio_dt_spec uarttx = GPIO_DT_SPEC_GET_OR(UARTTX_NODE, gpios, {0});		//here set which pin by index value/default value

#define UARTRX_NODE	DT_ALIAS(uartrx)
#if !DT_NODE_HAS_STATUS(UARTRX_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif
static const struct gpio_dt_spec uartrx = GPIO_DT_SPEC_GET_OR(UARTRX_NODE, gpios, {0});		//here set which pin by index value/default value
extern const struct device *uart;

// helper functs
void uart_write(char* msg);
void uart_write_time(uint32_t secs);
void uart_write_num(uint32_t num);
void uart_write_fault_hist(fault_t num);
void uart_tx_bit_bang(unsigned char val);
unsigned char uart_rx_bit_bang(void);
void uart_tx_bit_bang_fast(unsigned char val);
unsigned char uart_rx_bit_bang_fast(void);
void IBOX(void);
void Ibox_init(void);

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// @ GLOBAL VARS (PUBLIC)
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

extern   bool msg_got_poll;
extern   bool msg_disable_ibox_comms;

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// @ GLOBAL CONSTS (PUBLIC)
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

#endif /* datalog_comms_H_ */
