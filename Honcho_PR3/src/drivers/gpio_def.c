/*
 * Copyright (c) 2022 Stanley Black and Decker
 *
 * Author: 
 */

/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Nordic UART Bridge Service (NUS) sample
 */
#include "gpio_def.h" 
 
/**
 * @brief led init 
 * Initializes the led pin drivers and configures led as output high
 * 
 */
void led_init() {
	int ret;

	if (!device_is_ready(slopeled.port)) {
		//printk("Error: button device %s is not ready\n",
		//       pwrled.port->name);
		return;
	}
	if (!device_is_ready(dropled.port)) {
		//printk("Error: button device %s is not ready\n",
		//       slopeled.port->name);
		return;
	}
	
	if (!device_is_ready(bleled.port)) {
		//printk("Error: button device %s is not ready\n",
		//       bleled.port->name);
		return;
	}

	ret = gpio_pin_configure_dt(&slopeled, GPIO_OUTPUT_HIGH);		//here set initial state
	if (ret != 0) {
		//printk("Error %d: failed to configure %s pin %d\n",
		//       ret, pwrled.port->name, pwrled.pin);
		return;
	}
	ret = gpio_pin_configure_dt(&dropled, GPIO_OUTPUT_HIGH);
	if (ret != 0) {
		//printk("Error %d: failed to configure %s pin %d\n",
		//       ret, slopeled.port->name, slopeled.pin);
		return;
	}
	
	ret = gpio_pin_configure_dt(&bleled, GPIO_OUTPUT_HIGH);
	if (ret != 0) {
		//printk("Error %d: failed to configure %s pin %d\n",
		//       ret, bleled.port->name, bleled.pin);
		return;
	}

	ret = gpio_pin_configure_dt(&soc1, GPIO_OUTPUT_HIGH);
	if (ret != 0) {
		//printk("Error %d: failed to configure %s pin %d\n",
		//       ret, bleled.port->name, bleled.pin);
		return;
	}

	ret = gpio_pin_configure_dt(&soc2, GPIO_OUTPUT_HIGH);
	if (ret != 0) {
		//printk("Error %d: failed to configure %s pin %d\n",
		//       ret, bleled.port->name, bleled.pin);
		return;
	}

	ret = gpio_pin_configure_dt(&soc3, GPIO_OUTPUT_HIGH);
	if (ret != 0) {
		//printk("Error %d: failed to configure %s pin %d\n",
		//       ret, bleled.port->name, bleled.pin);
		return;
	}
}

/**
 * @brief control init
 * Initiallizes all the enable pins to default state.
 * 
 */

void control_init() {
	int ret;

	if (!device_is_ready(LaserbuckEN.port)) {
		printk("Error: button device %s is not ready\n",
		      LaserbuckEN.port->name);
		return;
	}

	ret = gpio_pin_configure_dt(&LaserbuckEN, GPIO_OUTPUT_LOW);		//here set initial state
	if (ret != 0)
	{
		return;
	}

    if (!device_is_ready(MotorbuckEN.port)) {
		printk("Error: button device %s is not ready\n",
		      MotorbuckEN.port->name);
		return;
	}

	ret = gpio_pin_configure_dt(&MotorbuckEN, GPIO_OUTPUT_LOW);		//here set initial state
	if (ret != 0) 
	{
		return;
	}

	ret = gpio_pin_configure_dt(&SysldoEN, GPIO_OUTPUT_HIGH);		//here set initial state
	if (ret != 0) 
	{
		return;
	}

/***************************************************************************************************************/
	ret = gpio_pin_configure_dt(&laserpsadc, GPIO_OUTPUT_LOW);		//here set initial state
	if (ret != 0) 
	{
		return;
	}
	ret = gpio_pin_configure_dt(&packiden, GPIO_OUTPUT_LOW);		//here set initial state
	if (ret != 0) 
	{
		return;
	}
	ret = gpio_pin_configure_dt(&vbatcoinen, GPIO_OUTPUT_LOW);		//here set initial state
	if (ret != 0) 
	{
		return;
	}
	ret = gpio_pin_configure_dt(&lasercurrent, GPIO_OUTPUT_LOW);		//here set initial state
	if (ret != 0) 
	{
		return;
	}
	ret = gpio_pin_configure_dt(&vbattcoinadc, GPIO_OUTPUT_LOW);		//here set initial state
	if (ret != 0) 
	{
		return;
	}

}

/**
 * @brief led off
 * helper function to turn off all the leds 
 * 
 */

void led_off()
{
	int ret;
	ret = gpio_pin_set_dt(&slopeled, 0);
	ret = gpio_pin_set_dt(&dropled, 0);
	ret = gpio_pin_set_dt(&bleled, 0);
	ret = gpio_pin_set_dt(&soc1, 0);
	ret = gpio_pin_set_dt(&soc2, 0);
	ret = gpio_pin_set_dt(&soc3, 0);
}

/**
 * @brief bucks off
 * helper function to turn off all Bucks 
 * 
 */
void bucks_off()
{
	int ret; 
	ret = gpio_pin_set_dt(&LaserbuckEN, 0);
	ret = gpio_pin_set_dt(&MotorbuckEN, 0);
}

/**
 * @brief enables off
 * helper function to turn off adc measurement on pack.
 * 
 */
void enables_off()
{
	int ret;
	ret = gpio_pin_set_dt(&Vbaten, 0);
	ret = gpio_pin_set_dt(&Battthen, 0);
	// ret = gpio_pin_set_dt(&coin)
}