/*
 * Copyright (c) 2022 Stanley Black and Decker
 *
 * Author: Swathi Thirunarayanan
 */

/**
 * @file
 * @brief Public API for nordic <-> spi communication 
 */
#ifndef SPI_H_
#define SPI_H_

#include <drivers/spi.h>
#include <logging/log.h>

#define SPI1_NODE               DT_NODELABEL(spi1)

/** @brief Callback struct used by the my_service Service. */
struct spi_cb 
{
	void (*received)(uint8_t *data, int bytes);
};

extern const struct device *spi_dev;
void spi_init(struct spi_cb *cb);
int spi_bytewrite(uint8_t cmd);
int spi_byteread(uint8_t *buffer, int buffer_idx);
int spi_read_cmd(uint8_t cmd, int rd_length, struct spi_cb *cb);

#endif /* KEYPAD_ATTINY_SPI_H_ */