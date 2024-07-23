#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "encoder.h"
#include "gpio.h"
#include "clock.h"

// Configure to use input pullups on the A/B signals
#define PULLUP_ENABLE 1

#define A_PIN D2
#define B_PIN D3

static uint8_t state = 0;
static int8_t position = 0;

//                           _______         _______
//               Pin1 ______|       |_______|       |______ Pin1
// negative <---         _______         _______         __      --> positive
//               Pin2 __|       |_______|       |_______|   Pin2

//	new	new	old	old
//	pin2	pin1	pin2	pin1	Result
//	----	----	----	----	------
//	0	0	0	0	no movement
//	0	0	0	1	+1
//	0	0	1	0	-1
//	0	0	1	1	+2  (assume pin1 edges only)
//	0	1	0	0	-1
//	0	1	0	1	no movement
//	0	1	1	0	-2  (assume pin1 edges only)
//	0	1	1	1	+1
//	1	0	0	0	+1
//	1	0	0	1	-2  (assume pin1 edges only)
//	1	0	1	0	no movement
//	1	0	1	1	-1
//	1	1	0	0	+2  (assume pin1 edges only)
//	1	1	0	1	-1
//	1	1	1	0	+1
//	1	1	1	1	no movement
static void update(void)
{
	// Get lower two bits of global state
	uint8_t s = state & 0x03;

	// Get the state of the A pin
	if (gpio_get_value(A_PIN) == VAL_HIGH)
	{
		s |= 0x04;
	}

	// Get the state of the B pin
	if (gpio_get_value(B_PIN) == VAL_HIGH)
	{
		s |= 0x08;
	}

	// It's a bit magic, we only increment on state E and D
	//   and that gives exactly 1 update per detent on the encoder
	if (s == 0x0E)
	{
		position += 1;
	}
	else if (s == 0x0D)
	{
		position -= 1;
	}

	// Update global state
	state = (s >> 2);
}

void encoder_init(void)
{
	#if PULLUP_ENABLE != 0
		// Set as input with internal pullup
		gpio_direction(A_PIN, DIR_INPUT_PULLUP);
		gpio_direction(B_PIN, DIR_INPUT_PULLUP);
	#else
		// Set as input with no pullup
		gpio_direction(A_PIN, DIR_INPUT);
		gpio_direction(B_PIN, DIR_INPUT);
	#endif

	// Small delay to let any RC filters charge
	delay_us(2000);

	// Get initial value for A pin
	if (gpio_get_value(A_PIN) == VAL_HIGH)
	{
		state |= 0x01;
	}

	// Get initial value for B pin
	if (gpio_get_value(B_PIN) == VAL_HIGH)
	{
		state |= 0x02;
	}

	// Configure A/B interrupt sense to CHANGE
	EICRA = (1 << ISC10) | (1 << ISC00);
	// Enable A/B interrupts
	EIMSK = (1 << INT1) | (1 << INT0);
}

int8_t encoder_read(void)
{
	// Copy CPU flags
	uint8_t sreg = SREG;
	// Disable interrupts
	cli();

	int8_t pos = position;
	position = 0;

	// Restore CPU flags
	SREG = sreg;

	return pos;
}

// A Interrupt
ISR(INT0_vect)
{
	// Update encoder state
	update();
}

// B Interrupt
ISR(INT1_vect)
{
	// Update encoder state
	update();
}
