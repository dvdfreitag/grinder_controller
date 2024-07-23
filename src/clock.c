#include <stdint.h>

#include <avr/io.h>
#include <avr/interrupt.h>

#include "clock.h"

static volatile uint32_t timer0_millis = 0;

void clock_init(void)
{
	// Set timer 0 to Clear Timer mode
	//   CTC mode will clear the timer count at OCR0A
	TCCR0A |= (1 << WGM01);

	// Set timer 0 clock prescale factor to 64
	TCCR0B |= (1 << CS01) | (1 << CS00);

	// Set timer 0 compare to 250 => 16MHz / 64 / 250 = 1kHz
	OCR0A = 250;

	// Enable timer 0 compare interrupt
	TIMSK0 |= (1 << OCIE0A);
}

void delay_ms(uint32_t ms)
{
	// For every millisecond,
	for (uint32_t i = 0; i < ms; i++)
	{
		// ... delay 1000 microseconds
		delay_us(1000);
	}
}

void delay_us(uint32_t us)
{
	// If delay time is negligible,
	if (us <= 1)
	{
		// Nothing to do here
		//   It takes about 1us for this check and function overhead
		return;
	}

	// Magic incantation from Arduino @ 16MHz
	us = (us << 2) - 1;

	// Spinlock
	__asm__ __volatile__
	(
		"1:"			"\n\t"	// Create a label called 1
		"sbiw %0,1"		"\n\t"	// us -= 1
		"brne 1b"				// if (us != 0) goto 1
		: "=w" (us)
		: "0" (us)
	);
}

uint32_t millis(void)
{
	// Disable interrupts because a uint32_t takes
	//   multiple instructions to copy and we don't
	//   want the timer to update mid-copy

	// Copy CPU flags
	uint8_t sreg = SREG;
	// Disable interrupts
	cli();

	// Copy milliseconds counter
	uint32_t m = timer0_millis;

	// Restore CPU flags
	SREG = sreg;

	return m;
}

uint32_t micros(void)
{
	// Copy CPU flags
	uint8_t sreg = SREG;
	// Disable interrupts
	cli();

	// Convert to microseconds
	uint32_t us = timer0_millis * 1000;

	// Each timer count is 4us => 1kHz / 250 = 4us
	us += TCNT0 * 4;

	// Restore CPU flags
	SREG = sreg;

	return us;
}

// Timer 0 Compare Interrupt
ISR(TIMER0_COMPA_vect)
{
	// Increment milliseconds value at 1kHz
	timer0_millis += 1;
}
