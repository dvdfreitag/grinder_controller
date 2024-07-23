#pragma once

typedef enum
{
	D0,		// PD0 (RX)
	D1,		// PD1 (TX)
	D2,		// PD2
	D3,		// PD3
	D4,		// PD4
	D5,		// PD5
	D6,		// PD6
	D7,		// PD7
	D8,		// PB0
	D9,		// PB1
	D10,	// PB2
	D11,	// PB3 (MOSI)
	D12,	// PB4 (MISO)
	D13,	// PB5 (SCK)
	A0,		// PC0
	A1,		// PC1
	A2,		// PC2
	A3,		// PC3
	A4,		// PC4
	A5,		// PC5
} gpio_t;

typedef enum
{
	DIR_INPUT,
	DIR_INPUT_PULLUP,
	DIR_OUTPUT,
} gpio_direction_t;

typedef enum
{
	VAL_LOW = 0,
	VAL_HIGH = 1,
} gpio_value_t;

void gpio_direction(gpio_t gpio, gpio_direction_t direction);
void gpio_set_value(gpio_t gpio, gpio_value_t value);
gpio_value_t gpio_get_value(gpio_t gpio);
void gpio_toggle(gpio_t gpio);
