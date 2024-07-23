#include <stdint.h>
#include <stdio.h>

#include <avr/io.h>
#include <avr/interrupt.h>

#include "clock.h"
#include "encoder.h"
#include "gpio.h"
#include "uart.h"
#include "display.h"

// Ration of input pulses from the encoder to pulses
//   of the step output
#define OUTPUT_GAIN 4

// How fast the encoder value is polled
#define READ_UPDATE_MS 10
// How fast the buttons are polled, display is updated, and
//   the step output pulses are generated
#define WRITE_UPDATE_MS 100

// Full pulse length is 2 * PULSE_TIME_US
// 200kHz = 5us total pulse length
// 100kHz = 10us total pulse length
// Closest to 200kHz without exceeding would be
//   PULSE_TIME_US = 3, or 166.67kHz
#define PULSE_TIME_US 5
// Set to 0 to disable dwell timer
#define DWELL_ENABLE 1
// Number of microseconds to wait between each set of
//   OUTPUT_GAIN step output pulses
#define DWELL_TIME_US 100

// Encoder direction
//   Right positive => Turn clockwise to go up
//   Left positive => Turn counter-clockwise to go up
#define RIGHT_POSITIVE 0
#define LEFT_POSITIVE 1
#define ENCODER_DIRECTION RIGHT_POSITIVE

// Direction output logic level
//   Dir High => Direction output is high for positive steps
//   Dir Low => Direction output is low for positive steps
#define DIR_HIGH 0
#define DIR_LOW 1
#define DIRECTION_OUTPUT DIR_HIGH

// Set to 0 to disable flashing the onboard LED
#define LED_ENABLE 1
#define LED_PIN D13

// Increment settings in 0.0001"
#define INCREMENT_FINE 1
#define INCREMENT_COARSE 10

#define ZERO_PIN D4
#define COARSE_PIN D5
#define FINE_PIN D6

#define STEP_OUT_PIN A0
#define DIR_OUT_PIN A1

static int32_t position = 0;
static int32_t position_last = 0;
static uint8_t increment = INCREMENT_FINE;
static uint32_t read_time = 0;
static uint32_t write_time = 0;
static uint8_t button_state = 0;

static void handle_buttons(void)
{
	#define ZERO_BIT (1 << 0)
	#define COARSE_BIT (1 << 1)
	#define FINE_BIT (1 << 2)

	uint8_t state = 0;

	// If the button is pressed
	if (gpio_get_value(ZERO_PIN) == VAL_LOW)
	{
		// Copy the state
		state |= ZERO_BIT;

		// ...and we didn't already process the button input
		if ((button_state & ZERO_BIT) == 0)
		{
			// Do the thing, only once per press
			position = 0;
			printf("Zero Button Pressed\n");
		}
	}

	// Repeat for all user inputs

	if (gpio_get_value(COARSE_PIN) == VAL_LOW)
	{
		state |= COARSE_BIT;

		if ((button_state & COARSE_BIT) == 0)
		{
			increment = INCREMENT_COARSE;
			printf("Coarse Button Pressed\n");
		}
	}

	if (gpio_get_value(FINE_PIN) == VAL_LOW)
	{
		state |= FINE_BIT;

		if ((button_state & FINE_BIT) == 0)
		{
			increment = INCREMENT_FINE;
			printf("Fine Button Pressed\n");
		}
	}

	// Update the button state
	button_state = state;

	#undef ZERO_BIT
	#undef COARSE_BIT
	#undef FINE_BIT
}

static void handle_output(void)
{
	// If the position hasn't changed since we last checked,
	if (position == position_last)
	{
		// Nothing to do here
		return;
	}

	// Compute the change in position
	int8_t diff = position - position_last;
	// Update last position
	position_last = position;

	gpio_value_t direction;

	// Assume difference is positive
	#if DIRECTION_OUTPUT == DIR_HIGH
		direction = VAL_HIGH;
	#else
		direction = VAL_LOW;
	#endif

	// If the difference is negative, flip the direction level
	if (diff < 0)
	{
		#if DIRECTION_OUTPUT == DIR_HIGH
			direction = VAL_LOW;
		#else
			direction = VAL_HIGH;
		#endif

		// Ensure diff is always positive
		diff = -diff;
	}

	// Set the direction output
	gpio_set_value(DIR_OUT_PIN, direction);
	// Small delay to give the driver time to latch in the direction
	//   before step pulses start arriving
	delay_us(2);

	// Send out step pulses
	for (uint8_t i = 0; i < (uint8_t)diff; i++)
	{
		// In fine mode, the maximum diff is ~9
		//   Or, at most 36 pulses, or ~360us

		// In coarse mode, the maximum diff is ~90 (usually 80)
		//   Or, at most 360 pulses, or ~3.6ms

		// Bit bang step ouptut pulses with 50% duty cycle
		for (uint8_t j = 0; j < OUTPUT_GAIN; j++)
		{
			// Set step pin high
			PORTC |= 0x01;
			// Delay
			delay_us(PULSE_TIME_US);
			// Set step pin low
			PORTC &= 0xFE;
			// Delay
			delay_us(PULSE_TIME_US);
		}

		// Configurable dwell time to separate each group of 4 step pulses
		#if DWELL_ENABLE != 0
			delay_us(DWELL_TIME_US);
		#endif
	}
}

static void gpio_init(void)
{
	#if LED_ENABLE != 0
		gpio_direction(LED_PIN, DIR_OUTPUT);
	#endif

	gpio_direction(ZERO_PIN, DIR_INPUT);
	gpio_direction(COARSE_PIN, DIR_INPUT);
	gpio_direction(FINE_PIN, DIR_INPUT);

	gpio_set_value(STEP_OUT_PIN, VAL_LOW);
	gpio_direction(STEP_OUT_PIN, DIR_OUTPUT);
	gpio_set_value(DIR_OUT_PIN, VAL_LOW);
	gpio_direction(DIR_OUT_PIN, DIR_OUTPUT);
}

int main(void)
{
	// Setup Timer 0 for millis()/micros()
	clock_init();
	// Setup UART and attach printf()
	uart_init(9600);
	// Setup encoder inputs and interrupts
	encoder_init();
	// Detup LED display and software SPI
	display_init();
	// Configure I/Os
	gpio_init();

	// Reset globals
	position = 0;
	position_last = 0;
	increment = INCREMENT_FINE;
	read_time = millis();
	write_time = read_time;
	button_state = 0;

	// Display 0.0000
	display_update(position);

	// Enable interrupts
	sei();

	while (1)
	{
		// Get the current time
		uint32_t now = millis();

		// Check for a read update
		if ((now - read_time) > READ_UPDATE_MS)
		{
			// Update last time
			read_time = now;

			// Read encoder value
			int8_t value = encoder_read();
			// Local copy of increment
			int8_t inc = increment;

			// If the encoder value has changed since we last looked,
			if (value != 0)
			{
				// Negate increment depending on desired rotation direction
				#if ENCODER_DIRECTION == RIGHT_POSITIVE
					if (value < 0)
					{
						inc = -inc;
					}
				#else
					if (value > 0)
					{
						inc = -inc;
					}
				#endif

				// Increment position
				position += inc;
				// Update LED display
				display_update(position);
			}
		}

		// Check for a write update
		if ((now - write_time) > WRITE_UPDATE_MS)
		{
			// Update last time
			write_time = now;

			// Toggle LED if desired
			#if LED_ENABLE != 0
				gpio_toggle(LED_PIN);
			#endif

			// Handle user inputs
			handle_buttons();
			// Transmit step pulses to output
			handle_output();
		}
	}

	return 0;
}
