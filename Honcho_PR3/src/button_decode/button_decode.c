// /*
//  * Copyright (c) 2022 Stanley Black and Decker
//  *
//  * Author: Swathi Thirunarayanan
//  */
// #include "button_decode.h"

// /**
//  * @brief Thread define.
//  * keypad_decode_thread is created with entry point at function keypad_decode
//  * 
//  */
// K_THREAD_DEFINE(keypad_decode_thread, KEYPAD_DECODE_STACKSIZE, keypad_decode, NULL, NULL,
// 				NULL, KEYPAD_DECODE_PRIORITY, 0, 0);

// // variables 
// uint16_t button_pressed = BUTTON_DEFAULT;
// volatile bool longPressReleaseFlag;

// /**
//  * @brief keypad_decode_cb
//  * keypad callback leading to keypad_decode_handler function.
//  */
// struct keypad_cb keypad_decode_cb = {
// 	.received = keypad_decode_handler,
// };

// /**
//  * @brief keypad_decode_handler
//  * Gets keypad data buffer and stores it in a 16bit variable keypad_data.
//  * Then Decodes button_pressed_value from keypad_data.
//  * 
//  * Note: 
//  * get_button_name() = gives the name of the button pressed in string format.
//  * is_long_press() = returns bool value determining if its a long press.
//  * @param data keypad data buffer (array) of size len bytes.
//  *             data[0] = LSB, data[len-1] = MSB.
//  * @param len "bytes" of data that is read from keypad.
//  */
// void keypad_decode_handler(uint8_t *data, int len)
// {
// 	uint16_t keypad_data = BUTTON_DEFAULT;
// 	for (int i = 0; i < len; i++)
// 	{
// 		keypad_data |= (data[i] << (8 * i));
// 	}
// 	button_pressed = get_button_pressed_value(keypad_data);
// 	printk("Button pressed : %s (%x)\n", get_button_name(button_pressed), button_pressed);
// 	printk("is it long press? %s", is_long_press(button_pressed) ? "true" : "false");
// }

// /**
//  * @brief keypad_decode
//  * This is the entry point for keypad_decode_thread.
//  * On start, it calls get_keypad_data() which initializes 
//  * callback and takes get_keypad_data_sem. When button is 
//  * pressed and spi data is received by keypad_attiny thread, 
//  * get_keypad_data_sem is released to initiate keypad_decode_handler. 
//  */
// void keypad_decode()
// {
// 	while (1)
// 	{
// 		get_keypad_data(&keypad_decode_cb);
// 		k_sleep(K_MSEC(1));
// 	}
// }