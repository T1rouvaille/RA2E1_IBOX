/*
 * Copyright (c) 2022 Stanley Black and Decker
 *
 * Author: Swathi Thirunarayanan
 */
#include "keypad_attiny.h"

/**
 * @brief Thread define.
 * keypad_attiny_thread is created with entry point at function keypad_attiny
 * 
 */
K_THREAD_DEFINE(KEYPAD_ATTINY_THREAD, KEYPAD_STACKSIZE, keypad_attiny, NULL, NULL,
                NULL, KEYPAD_PRIORITY, 0, -1);

/**
 * @brief Semaphore defines.
 * keypad_interrupt_sem: Taken after keypad initialization and it waits 
 * (non-blocking) until keypad interrupt is triggered on attiny keypress.
 * 
 * get_keypad_data_sem: Taken after keypad_cb is defined on get_keypad_data() being called
 * and it waits (non-blocking) until keypad_spi_cb (keypad data is read from spi) is handled.
 */
K_SEM_DEFINE(keypad_interrupt_sem, 0, 1);

static struct button_data_t {
	uint16_t  data;
	uint16_t len;
}*button_fifo_data;
volatile bool longPressReleaseFlag;
static const struct gpio_dt_spec keypad_spi_interrupt = GPIO_DT_SPEC_GET_OR(KEYPAD_INT_NODE, gpios, {0});
static struct gpio_callback keypad_spi_interrupt_cb;
// static struct button_data_t *button_fifo_data; 

// keypad callback variable
struct keypad_cb keypad_cb;
// volatile bool longPressReleaseFlag;
uint8_t keypad_data_buffer[2];
int keypad_len;

/**
 * @brief keypad_spi_cb
 * spi callback leading to keypad_spi_handler function.
 */
struct spi_cb keypad_spi_cb = {
    .received = keypad_spi_handler,
};

/**
 * @brief keypad_spi_handler 
 * Stores the keypad data read from spi into variable
 * Then it resumes (gives) get_keypad_data_sem which sends cb to keypad_decode_thread.
 * @param data keypad spi data buffer (array) of size len bytes.
 *             data[0] = LSB, data[len-1] = MSB
 * @param len "bytes" of data that is read from keypad
 */
void keypad_spi_handler(uint8_t *data, int len)
{
    
	button_fifo_data = k_malloc(sizeof(struct button_data_t));
	uint16_t keypad_data = 0x0000;	
    for (int i = 0; i < len; i++)
    {
         keypad_data_buffer[i] = data[i];
         keypad_data |= (data[i] << (8*i));
    }
    button_fifo_data->data = (keypad_data);
	button_fifo_data->len = len;
	k_fifo_put(&button_fifo, button_fifo_data);
    keypad_len = len;
    // k_free(button_fifo_data);
    // k_sem_give(&get_keypad_data_sem);
}

/**
 * @brief keypad_interrupt_handler
 * On both rising and falling edge this interrupt is triggered.
 * On rising edge keypad_interrupt_sem released thus enabling spi 
 * transmission to read keypad data.
 * 
 * On falling edge longPressReleaseFlag is set to true.
 * 
 * @param dev gpio device
 * @param cb gpio callback
 * @param pins gpio pin interrupt
 */
void keypad_interrupt_handler(const struct device *dev, struct gpio_callback *cb,
                                  uint32_t pins)
{
    // printk("INT1 from attiny is triggered\n");
    if (gpio_pin_get(keypad_spi_interrupt.port, keypad_spi_interrupt.pin))
    {
        k_sem_give(&keypad_interrupt_sem);
    }
    if (!(gpio_pin_get(keypad_spi_interrupt.port, keypad_spi_interrupt.pin)))
    {
        longPressReleaseFlag = true;
        // printk("Stopping motor command sent");
        motor_stop();
    }
}

/**
 * @brief keypad_attiny
 * This is the entry point for keypad_atiny_thread.
 * On start, it initializes keypad and takes keypad_interrupt_sem.
 * When button is pressed, keypad_interrupt_sem is released to initiate
 * spi read.
 */
void keypad_attiny()
{
    uint16_t err;
    keypad_init();
    printk("Buffer sent thread enter....\n");

    while(1)
    {
        k_sem_take(&keypad_interrupt_sem, K_FOREVER);
        err = spi_read_cmd(SBD_SPI_AUTHORIZATION, 2, &keypad_spi_cb);
        if (err)
        {
            printk("SPI failed\n");
        }
    }
}

/**
 * @brief keypad_init
 * initializes keypad gpio interrupt INT1 and keypad spi with callback.
 */
void keypad_init()
{
    keypad_interrupt_init();
    spi_init(&keypad_spi_cb);
}

/**
 * @brief keypad_interrupt_init
 * Initializes gpio interrupt which is triggered 
 * on both edges of keypad_spi_interrupt with 
 * callback  keypad_spi_interrupt_cb and 
 * handler keypad_interrupt_handler.
 */
void keypad_interrupt_init()
{
    int ret;

    if (!device_is_ready(keypad_spi_interrupt.port))
    {
        printk("Error: button device %s is not ready\n",
               keypad_spi_interrupt.port->name);
        return;
    }

    ret = gpio_pin_configure_dt(&keypad_spi_interrupt, GPIO_INPUT);
    if (ret != 0)
    {
        printk("Error %d: failed to configure %s pin %d\n",
               ret, keypad_spi_interrupt.port->name, keypad_spi_interrupt.pin);
        return;
    }

    ret = gpio_pin_interrupt_configure_dt(&keypad_spi_interrupt,
                                          GPIO_INT_EDGE_BOTH);
    if (ret != 0)
    {
        printk("Error %d: failed to configure interrupt on %s pin %d\n",
               ret, keypad_spi_interrupt.port->name, keypad_spi_interrupt.pin);
        return;
    }

    gpio_init_callback(&keypad_spi_interrupt_cb, keypad_interrupt_handler, BIT(keypad_spi_interrupt.pin));
    gpio_add_callback(keypad_spi_interrupt.port, &keypad_spi_interrupt_cb);
    // printk("Set up keypad spi interrupt INT1 at %s pin %d\n", keypad_spi_interrupt.port->name, keypad_spi_interrupt.pin);
}