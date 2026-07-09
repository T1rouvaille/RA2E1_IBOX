/*
 * Copyright (c) 2022 Stanley Black and Decker
 *
 * Author: Swathi Thirunarayanan
 */
#include "spi.h"

// spi cs config
struct spi_cs_control spi_cs = {
    .gpio_pin = DT_GPIO_PIN(SPI1_NODE, cs_gpios),
    .gpio_dt_flags = GPIO_ACTIVE_LOW,
    .delay = 0,
};
// spi config
static struct spi_config spi_cfg = {
    .operation = SPI_WORD_SET(8) | SPI_TRANSFER_LSB,
    .frequency = 125000,
    .slave = 0,
    .cs = &spi_cs,
};

// spi variables
const struct device *spi_dev;
struct spi_cb spi_cb;
int spi_err = -1;

/**
 * @brief spi_init
 * initialise spi with parameters specified in spi_cfg.
 * spi is setup at master mode, LSB first transfer, 125kHz frequency with SPI1
 * @param cb spi callback with read_buffer and read_legth bytes as its parameters
 */
void spi_init(struct spi_cb *cb)
{
    spi_cb.received = cb->received;
    spi_cs.gpio_dev = device_get_binding(DT_GPIO_LABEL(SPI1_NODE, cs_gpios));
    if (spi_cs.gpio_dev == NULL)
    {
        printk("Could not get gpio device\n");
    }
    // else
    // {
    //     printk("GPIO device: %s\n", DT_GPIO_LABEL(SPI1_NODE, cs_gpios));
    // }

    spi_dev = device_get_binding(DT_LABEL(SPI1_NODE));
    if (spi_dev == NULL)
    {
        printk("Could not get %s device\n", DT_LABEL(SPI1_NODE));
        return;
    }
    // else
    //     printk("SPI Device: %s\n", DT_LABEL(SPI1_NODE));
}

/**
 * @brief spi_bytewrite
 * writes/sends one byte of data (cmd)
 *
 * @param data one byte of data that needs to be transfered.
 * @return int return spi_err code. 0 If successful in master mode.
 */
int spi_bytewrite(uint8_t data)
{
    uint8_t tx_buffer;
    tx_buffer = data;
    const struct spi_buf tx_buf = {
        .buf = &tx_buffer,
        .len = sizeof(tx_buffer)};
    const struct spi_buf_set tx = {
        .buffers = &tx_buf,
        .count = 1};

    return spi_transceive(spi_dev, &spi_cfg, &tx, NULL);
}

/**
 * @brief spi_byteread
 * receives/reads one byte of data and stores it in buffer
 * in the specified buffer_idx.
 * Can be used for multiple single byte reads.
 *
 * @param buffer receive/read buffer.
 * @param buffer_idx index position to fill data in buffer.
 * @return int return spi_err code. 0 If successful in master mode.
 */
int spi_byteread(uint8_t *buffer, int buffer_idx)
{
    uint8_t rx_buffer;
    struct spi_buf rx_buf = {
        .buf = &rx_buffer,
        .len = sizeof(rx_buffer),
    };
    const struct spi_buf_set rx = {
        .buffers = &rx_buf,
        .count = 1};
    spi_err = spi_transceive(spi_dev, &spi_cfg, NULL, &rx);
    if (spi_err)
    {
        printk("Error receiving button data byte SPI error: %d\n", spi_err);
        return spi_err;
    }
    // else
    // {
    //     printk("Button data byte received: %x\n", rx_buffer);
    // }
    buffer[buffer_idx] = rx_buffer;
    return spi_err;
}

/**
 * @brief spi_read_cmd
 * sends/writes one byte cmd through spi to slave device
 * and receives/reads rd_length bytes of data from slave device.
 * Finally, calls received function pointer in external file's spi cb with spi data.
 * Note:
 * - Uses multiple single-byteread to read rd_length bytes of data.
 * - spi transmission is LSB first master mode.
 * - read_data_buffer stores the received/read spi data.
 * - read_data_buffer[0] = lowest byte .... read_data_buffer[bytes-1] = highest byte
 * - read_data_buffer is then put in a spi callback for access across files
 *
 * @param cmd command to initiate read.
 * @param rd_length "bytes" of data that should be read from slave
 * @return int return spi_err code. 0 If successful in master mode.
 */
int spi_read_cmd(uint8_t cmd, int rd_length, struct spi_cb *cb)
{
    uint8_t read_data_buffer[rd_length];
    spi_cb.received = cb->received;
    for (int i = 0; i < rd_length; i++)
    {
        read_data_buffer[i] = 0x00;
    }

    spi_err = spi_bytewrite(cmd);
    if (spi_err)
    {
        printk("Error sending SBD Authorization. SPI error: %d\n", spi_err);
        return spi_err;
    }
    // else
    // {
    //     printk("SBD Authorization sent:\n");
    // }

    // eventhough spi tranfer is LSB first,
    // lowest byte is stored in read_data_buffer[0].... highest byte in read_data_buffer[bytes-1]
    for (int i = 0; i < rd_length; i++)
    {
        spi_err = spi_byteread(read_data_buffer, i);
        if (spi_err)
        {
            printk("Error receiving button data byte SPI error: %d\n", spi_err);
            return spi_err;
        }
        else
        {
            // printk("Button data byte received: \n");
        }
    }
    spi_cb.received(read_data_buffer, rd_length);
    return spi_err;
}