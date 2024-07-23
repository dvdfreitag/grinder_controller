#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "uart.h"

static int uart_putchar(char c, FILE *stream)
{
	(void)stream;

	// Wait until transmit buffer is empty
	while ((UCSR0A & (1 << UDRE0)) == 0);
	// Transmit character
	UDR0 = c;

	return 0;
}

static FILE uart_str = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_RW);

void uart_init(uint32_t baud)
{
	// Enable double transmit speed
	UCSR0A = 1 << U2X0;

	// Write baud rate setting
	uint16_t baud_setting = (F_CPU / (8 * baud)) - 1;
	UBRR0H = (uint8_t)((baud_setting & 0xFF00) >> 8);
	UBRR0L = (uint8_t)(baud_setting & 0x00FF);

	// Configure N81
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);

	// Enable RX and TX
	UCSR0B |= (1 << RXEN0) | (1 << TXEN0);

	// Set STDOUT to use the uart
	stdout = &uart_str;
}
