#include <stdint.h>

#include <avr/io.h>

#include "gpio.h"
#include "spi.h"
#include "clock.h"

void spi_init(spi_t spi)
{
	gpio_set_value(spi.cs, VAL_HIGH);
	gpio_direction(spi.cs, DIR_OUTPUT);
	gpio_direction(spi.sck, DIR_OUTPUT);
	gpio_direction(spi.mosi, DIR_OUTPUT);
}

void spi_write(spi_t spi, uint8_t *data, uint8_t length)
{
	gpio_set_value(spi.cs, VAL_LOW);

	for (uint8_t i = 0; i < length; i++)
	{
		uint8_t val = data[i];

		for (uint8_t j = 0; j < 8; j++)
		{
			gpio_value_t out = VAL_LOW;

			if ((val & 0x80) != 0)
			{
				out = VAL_HIGH;
			}

			gpio_set_value(spi.mosi, out);
			val <<= 1;

			gpio_set_value(spi.sck, VAL_HIGH);
			delay_us(10);
			gpio_set_value(spi.sck, VAL_LOW);
			delay_us(10);
		}
	}

	gpio_set_value(spi.cs, VAL_HIGH);
}
