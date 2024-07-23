#include <stdint.h>
#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "gpio.h"

static volatile uint8_t *get_dir(gpio_t gpio)
{
	switch (gpio)
	{
		case D0:	// PD0 (RX)
			// Fallthrough
		case D1:	// PD1 (TX)
			// Fallthrough
		case D2:	// PD2
			// Fallthrough
		case D3:	// PD3
			// Fallthrough
		case D4:	// PD4
			// Fallthrough
		case D5:	// PD5
			// Fallthrough
		case D6:	// PD6
			// Fallthrough
		case D7:	// PD7
		{
			return &DDRD;
		}

		case D8:	// PB0
			// Fallthrough
		case D9:	// PB1
			// Fallthrough
		case D10:	// PB2
			// Fallthrough
		case D11:	// PB3 (MOSI)
			// Fallthrough
		case D12:	// PB4 (MISO)
			// Fallthrough
		case D13:	// PB5 (SCK)
		{
			return &DDRB;
		}

		case A0:	// PC0
			// Fallthrough
		case A1:	// PC1
			// Fallthrough
		case A2:	// PC2
			// Fallthrough
		case A3:	// PC3
			// Fallthrough
		case A4:	// PC4
			// Fallthrough
		case A5:	// PC5
		{
			return &DDRC;
		}

		default:
		{
			return NULL;
		}
	}
}

static volatile uint8_t *get_port_read(gpio_t gpio)
{
	switch (gpio)
	{
		case D0:	// PD0 (RX)
			// Fallthrough
		case D1:	// PD1 (TX)
			// Fallthrough
		case D2:	// PD2
			// Fallthrough
		case D3:	// PD3
			// Fallthrough
		case D4:	// PD4
			// Fallthrough
		case D5:	// PD5
			// Fallthrough
		case D6:	// PD6
			// Fallthrough
		case D7:	// PD7
		{
			return &PIND;
		}

		case D8:	// PB0
			// Fallthrough
		case D9:	// PB1
			// Fallthrough
		case D10:	// PB2
			// Fallthrough
		case D11:	// PB3 (MOSI)
			// Fallthrough
		case D12:	// PB4 (MISO)
			// Fallthrough
		case D13:	// PB5 (SCK)
		{
			return &PINB;
		}

		case A0:	// PC0
			// Fallthrough
		case A1:	// PC1
			// Fallthrough
		case A2:	// PC2
			// Fallthrough
		case A3:	// PC3
			// Fallthrough
		case A4:	// PC4
			// Fallthrough
		case A5:	// PC5
		{
			return &PINC;
		}

		default:
		{
			return NULL;
		}
	}
}

static volatile uint8_t *get_port_write(gpio_t gpio)
{
	switch (gpio)
	{
		case D0:	// PD0 (RX)
			// Fallthrough
		case D1:	// PD1 (TX)
			// Fallthrough
		case D2:	// PD2
			// Fallthrough
		case D3:	// PD3
			// Fallthrough
		case D4:	// PD4
			// Fallthrough
		case D5:	// PD5
			// Fallthrough
		case D6:	// PD6
			// Fallthrough
		case D7:	// PD7
		{
			return &PORTD;
		}

		case D8:	// PB0
			// Fallthrough
		case D9:	// PB1
			// Fallthrough
		case D10:	// PB2
			// Fallthrough
		case D11:	// PB3 (MOSI)
			// Fallthrough
		case D12:	// PB4 (MISO)
			// Fallthrough
		case D13:	// PB5 (SCK)
		{
			return &PORTB;
		}

		case A0:	// PC0
			// Fallthrough
		case A1:	// PC1
			// Fallthrough
		case A2:	// PC2
			// Fallthrough
		case A3:	// PC3
			// Fallthrough
		case A4:	// PC4
			// Fallthrough
		case A5:	// PC5
		{
			return &PORTC;
		}

		default:
		{
			return NULL;
		}
	}
}

static uint8_t get_pin_bit(gpio_t gpio)
{
	switch (gpio)
	{
		case D0:	// PD0 (RX)
		{
			return (1 << 0);
		}

		case D1:	// PD1 (TX)
		{
			return (1 << 1);
		}

		case D2:	// PD2
		{
			return (1 << 2);
		}

		case D3:	// PD3
		{
			return (1 << 3);
		}

		case D4:	// PD4
		{
			return (1 << 4);
		}

		case D5:	// PD5
		{
			return (1 << 5);
		}

		case D6:	// PD6
		{
			return (1 << 6);
		}

		case D7:	// PD7
		{
			return (1 << 7);
		}

		case D8:	// PB0
		{
			return (1 << 0);
		}

		case D9:	// PB1
		{
			return (1 << 1);
		}

		case D10:	// PB2
		{
			return (1 << 2);
		}

		case D11:	// PB3 (MOSI)
		{
			return (1 << 3);
		}

		case D12:	// PB4 (MISO)
		{
			return (1 << 4);
		}

		case D13:	// PB5 (SCK)
		{
			return (1 << 5);
		}

		case A0:	// PC0
		{
			return (1 << 0);
		}

		case A1:	// PC1
		{
			return (1 << 1);
		}

		case A2:	// PC2
		{
			return (1 << 2);
		}

		case A3:	// PC3
		{
			return (1 << 3);
		}

		case A4:	// PC4
		{
			return (1 << 4);
		}

		case A5:	// PC5
		{
			return (1 << 5);
		}

		default:
		{
			return 0;
		}
	}
}

void gpio_direction(gpio_t gpio, gpio_direction_t direction)
{
	volatile uint8_t *dir = get_dir(gpio);
	volatile uint8_t *out = get_port_write(gpio);
	uint8_t bit = get_pin_bit(gpio);

	if ((dir == NULL) || (out == NULL) || (bit == 0))
	{
		return;
	}

	uint8_t sreg = SREG;
	cli();

	switch (direction)
	{
		case DIR_INPUT:
		{
			// Clear direction bit
			*dir &= ~bit;
			// Clear output bit
			*out &= ~bit;

			break;
		}

		case DIR_INPUT_PULLUP:
		{
			// Clear direction bit
			*dir &= ~bit;
			// Set output bit
			*out |= bit;

			break;
		}

		case DIR_OUTPUT:
		{
			// Set direction bit
			*dir |= bit;

			break;
		}
	}

	// Restore CPU flags
	SREG = sreg;
}

void gpio_set_value(gpio_t gpio, gpio_value_t value)
{
	volatile uint8_t *out = get_port_write(gpio);
	uint8_t bit = get_pin_bit(gpio);

	if ((out == NULL) || (bit == 0))
	{
		return;
	}

	uint8_t sreg = SREG;
	cli();

	switch (value)
	{
		case VAL_LOW:
		{
			// Clear output bit
			*out &= ~bit;
			break;
		}

		case VAL_HIGH:
		{
			// Set output bit
			*out |= bit;
			break;
		}
	}

	// Restore CPU flags
	SREG = sreg;
}

gpio_value_t gpio_get_value(gpio_t gpio)
{
	volatile uint8_t *in = get_port_read(gpio);
	uint8_t bit = get_pin_bit(gpio);

	if ((in == NULL) || (bit == 0))
	{
		return VAL_LOW;
	}

	uint8_t sreg = SREG;
	cli();

	uint8_t value = *in & bit;

	// Restore CPU flags
	SREG = sreg;

	if (value == bit)
	{
		return VAL_HIGH;
	}

	return VAL_LOW;
}

void gpio_toggle(gpio_t gpio)
{
	volatile uint8_t *out = get_port_write(gpio);
	uint8_t bit = get_pin_bit(gpio);

	if ((out == NULL) || (bit == 0))
	{
		return;
	}

	uint8_t sreg = SREG;
	cli();

	// Toggle output
	*out ^= bit;

	// Restore CPU flags
	SREG = sreg;
}
