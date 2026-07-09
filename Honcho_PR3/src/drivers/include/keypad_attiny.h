/*
 * Copyright (c) 2022 Stanley Black and Decker
 *
 * Author: Swathi Thirunarayanan
 */
/**
 * @file
 * @brief Public API for Keypad attiny which includes keypad_spi_interrupt and keypad_attiny_spi
 */
#ifndef KEYPAD_ATTINY_H_
#define KEYPAD_ATTINY_H_

#include "spi.h"
#include "motor_control.h"

// Thread defines
#define KEYPAD_STACKSIZE 500
#define KEYPAD_PRIORITY 4

// SPI CMD
#define SBD_SPI_AUTHORIZATION 0xD9

#define row_column_mask 0x3F0F    // assigns 1 to row and column bits

// Keypad Interrupt INT1
#define KEYPAD_INT_NODE DT_ALIAS(keypadint)
#if !DT_NODE_HAS_STATUS(KEYPAD_INT_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif

// // variable
// static const struct gpio_dt_spec keypad_spi_interrupt = GPIO_DT_SPEC_GET_OR(KEYPAD_INT_NODE, gpios, {0});
// static struct gpio_callback keypad_spi_interrupt_cb;
// extern volatile bool longPressReleaseFlag;

#define button_data_size 	2
extern struct k_fifo button_fifo;
extern const k_tid_t KEYPAD_ATTINY_THREAD;

// callbacks and handlers
struct keypad_cb
{
    void (*received)(uint8_t *data, int len);
};
void keypad_spi_handler(uint8_t *data, int len);
void keypad_interrupt_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins);

//thread function
void keypad_attiny();

// init functions
void keypad_init();
void keypad_interrupt_init(void);


#endif /* KEYPAD_ATTINY_H_ */