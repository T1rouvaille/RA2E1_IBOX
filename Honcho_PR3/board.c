/*
 * Copyright (c) 2018 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>
#include <soc.h>
#include <hal/nrf_gpio.h>

static int Early_init_LED(void)
{
    nrf_gpio_pin_cfg_output(16);
    nrf_gpio_pin_cfg_output(24);
    nrf_gpio_pin_cfg_output(23);
    nrf_gpio_pin_set(16);
    nrf_gpio_pin_clear(23);
    nrf_gpio_pin_set(24);
    
	return 0;
}

SYS_INIT(Early_init_LED, PRE_KERNEL_1, 0);