#pragma once

#include "gpio.h"

typedef struct
{
	gpio_t cs;
	gpio_t sck;
	gpio_t mosi;
} spi_t;

void spi_init(spi_t spi);
void spi_write(spi_t spi, uint8_t *data, uint8_t length);
