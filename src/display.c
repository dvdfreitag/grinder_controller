#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <avr/pgmspace.h>

#include "spi.h"
#include "display.h"

#define CS_PIN 10
#define SCK_PIN 11
#define MOSI_PIN 12

// List of MAX7219 opcodes
#define OP_NOOP 0
#define OP_DIGIT0 1
#define OP_DIGIT1 2
#define OP_DIGIT2 3
#define OP_DIGIT3 4
#define OP_DIGIT4 5
#define OP_DIGIT5 6
#define OP_DIGIT6 7
#define OP_DIGIT7 8
#define OP_DECODEMODE 9
#define OP_INTENSITY 10
#define OP_SCANLIMIT 11
#define OP_SHUTDOWN 12
#define OP_DISPLAYTEST 15

// Table to convert ASCII digits to the format the LED display expects
static const uint8_t table[] PROGMEM =
{
	0b01111110, 0b00110000, 0b01101101, 0b01111001, 0b00110011, 0b01011011, 0b01011111, 0b01110000,
	0b01111111, 0b01111011, 0b01110111, 0b00011111, 0b00001101, 0b00111101, 0b01001111, 0b01000111,
	0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b10000000, 0b00000001, 0b10000000, 0b00000000,
	0b01111110, 0b00110000, 0b01101101, 0b01111001, 0b00110011, 0b01011011, 0b01011111, 0b01110000,
	0b01111111, 0b01111011, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b01110111, 0b00011111, 0b00001101, 0b00111101, 0b01001111, 0b01000111, 0b00000000,
	0b00110111, 0b00000000, 0b00000000, 0b00000000, 0b00001110, 0b00000000, 0b00000000, 0b00000000,
	0b01100111, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00001000,
	0b00000000, 0b01110111, 0b00011111, 0b00001101, 0b00111101, 0b01001111, 0b01000111, 0b00000000,
	0b00110111, 0b00000000, 0b00000000, 0b00000000, 0b00001110, 0b00000000, 0b00010101, 0b00011101,
	0b01100111, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,
	0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000
};

// Software SPI config struct to define pins
static spi_t spi =
{
	.cs = CS_PIN,
	.sck = SCK_PIN,
	.mosi = MOSI_PIN,
};

static uint8_t get_value(char c, uint8_t decimal)
{
	// Convert character to int
	uint8_t index = (uint8_t)c;
	// Read value from table in PROGMEM with character as offset
	uint8_t value = pgm_read_byte_near(table + index);

	// If a decimal point is desired,
	if (decimal != 0)
	{
		// Set the upper bit
		value |= 0x80;
	}

	return value;
}

static void display_write(uint8_t opcode, uint8_t data)
{
	// Write an opcode to the display using supplied data
	uint8_t buffer[2] = {0};

	// Opcode must only be bottom 4 bits
	buffer[0] = opcode & 0x0F;
	// Set user-supplied data
	buffer[1] = data;

	// Write 2 bytes over Software SPI
	spi_write(spi, buffer, 2);
}

void display_init(void)
{
	// Initialize software SPI
	spi_init(spi);

	// Disable display test mode
	display_write(OP_DISPLAYTEST, 0x00);
	// Configure to write up to 8 digits
	display_write(OP_SCANLIMIT, 0x07);
	// Disable BCD decoding
	display_write(OP_DECODEMODE, 0x00);
	// Set maximum intensity
	display_write(OP_INTENSITY, 0x0F);
	// Clear display before enabling to clear garbage
	display_clear();
	// Enable display outputs
	display_write(OP_SHUTDOWN, 0x01);
}

void display_update(int32_t value)
{
	// ASCII buffer, 8 digits + null
	static char digits[9] = {0};
	// Assume value is positive, first digit is empty
	char prefix = ' ';

	// If value is negative,
	if (value < 0)
	{
		// Set first digit to negative sign
		prefix = '-';
		// Ensure value is positive
		value = -value;
	}

	// Convert value to ASCII:
	//  "   0.0042"
	//  "-  0.0042"
	//  "   1.0000"
	//  "-  1.0000"
	//  "  20.0000"
	//  "- 20.0000"
	//  " 123.4567"
	//  "-123.4567"
	// Format Specifiers:
	// %c => Print character
	// 0%04ld => Print 0.<value> padded to 4 with leading zeros
	// %07ld => Print <value> padded to 7 with leading spaces
	if (value < 10000)
	{
		// Special case:
		//   Value is < 1", only display one zero to the left of the decimal point
		snprintf(digits, sizeof(digits), "%c  0%04ld", prefix, value);
	}
	else
	{
		snprintf(digits, sizeof(digits), "%c%7ld", prefix, value);
	}

	// Digit opcode starts at 1
	for (uint8_t i = 1; i <= 8; i++)
	{
		// Update all digits in display,
		//   with a decimal point at position 4
		display_write(9 - i, get_value(digits[i - 1], i == 4));
	}
}

void display_clear(void)
{
	// Digit opcode starts at 1
	for (uint8_t i = 1; i <= 8; i++)
	{
		// Clear all segments from all digits
		display_write(i, 0);
	}
}
