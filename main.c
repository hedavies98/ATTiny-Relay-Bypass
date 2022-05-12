/*

ATTiny based relay controller written by Hayden Davies

Primarily designed for guitar pedal and associated applications this can be
used to drive bypass or other relays with LED indicators

The static LED is driven high along with the Active High Relay output,
the blinky LED operates in the same way but has a slow startup and will
blink slowly when the foot switch is held showing that it's in momentary mode

The Active High Relay has slightly lower output than other pins, a buffer is recommended
this output requires the use of the reset pin, once the correct fuse has been set you'll 
need HVSP to reset it.

Switch In is active low, no pull up resistor is necessary since we use the internal one
                   _______
Active Low  Relay-|o      |-Vcc
Active High Relay-|       |-Switch In
   Latching Relay-|       |-Static LED
              Gnd-|_______|-Blinky LED


This software is written to be clock agnostic, change F_CPU as required
*/

#define F_CPU 1000000UL
#define MOMENTARY_DELAY 400
#define LATCHING_TIME 3

// locations of state flags in the state variable
#define SP 4
#define SS 3
#define LS 2
#define PO 1
#define TS 0

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

// a nice variable to hold the state flags
// Should Pulse | Switch State | Last State | Pedal On | Timer State
// SP			| SS		   | LS         | PO       | TS
char state = 0b00000;

// store current brightness and if it's increasing or decreasing
unsigned short int brightness_control = 0;
int brighness_up = 1;

// counter for the latching relay output
int pulse_counter = 0;

// counter for the momentary delay
int intr_count = 0;

// use timer1 to debounce the switch, this interrupt will run all the switching logic
// this is also used to make the LED shimmer by incrementing/reseting a variable every loop
ISR(TIMER1_COMPA_vect) {
	// count {X} for the momentary delay
	if (intr_count == MOMENTARY_DELAY) {
		state |= (1 << TS);		// set the Timer State bit
		intr_count = 0;
	} else {
		intr_count++;
	}
	
	// set the last state bit equal to the current state
	if ((state >> SS) & 1) {
		state |= (1<<LS);
		} else {
		state &= ~(1<<LS);
	}
	
	// run through the FSM
	if((PINB & (1 << PINB2)) == 0) {										// state 1xxx
		state |= 1 << SS;
		if (((state >> LS) & 1) == 0) {										// state 10xx
			if(((state >> PO) & 1) == 0) {									// state 100x
				intr_count = 0;												// > set to state 1010
				state |= (1<<PO);											// > turn pedal on
				state |= (1<<SP);
				brightness_control = 0;
				state &= ~(1<<TS);
				TCNT0 = 0;
			} else {														// state 101x
				state &= ~(1<<PO);											// > turn pedal off
				state |= (1<<SP);
			}
		}
	} else {
		state &= ~(1 << SS);												// state 0xxx
		if ((state >> LS) & 1 && (state >> PO) & 1 && (state >> TS) & 1) {	// state 01111
			state &= ~(1 << PO);											// > turn pedal off
			state |= (1<<SP);
		}
	}
	
	// shimmery LED funtimes
	if (brighness_up) {
		if (brightness_control < 254) {
			brightness_control++;
		} else {
			brighness_up = 0;
		}
	} else {
		if (brightness_control > 10) {
			brightness_control--;
		} else {
			brighness_up = 1;
		}
	}
	
	// increment pulse counter until LATCHING_TIME has elapsed, then reset the counter and pulse
	if (((state >> SP) & 1) && pulse_counter < LATCHING_TIME) {
		pulse_counter++;
	} else {
		pulse_counter=0;
		state &= ~(1<<SP);
	}
}

static inline void initTimer1(void) {
	TCCR1 |= (1 << CTC1);                               // clear timer on compare match
	TCCR1 |= (1 << CS12) | (1 << CS11) | (1 << CS10);	// clock prescaler 64
	OCR1C = 0.001 * (F_CPU/64.0);                       // compare match value to trigger interrupt every 1ms
	TIMSK |= (1 << OCIE1A);                             // enable output compare match interrupt
	sei();											    // enable interrupts
}

static inline void initTimer0(void) {
	TCCR0A = 1 << COM0A1 | 1 << WGM00;	// set timer for PWM
	TCCR0B=0x00;
	TCCR0B |= (1<<CS00);
}

int main (void)
{
	DDRB |= (1<<PB0) | (1<<PB3) | (1<<PB5) |	// set out outputs
			(1<<PB1) | (1<<PB4);
	DDRB &= ~(1<<PB2);							// and our input
	PORTB |= (1<<PB2);							// use the input pullup
	OCR0A = 0;									// start the LED off
	
	initTimer0();
	initTimer1();
	
	while (1) {
		if ((state >> PO) & 1) {				// check if pedal is on and set the LED accordingly
			PORTB |= (1<<PB3) | (1<<PB1);
			PORTB &= ~(1<<PB5);
			if((state >> SS) & 1) {				// if the button is held make the LED shimmer to indicate it's in momentary mode...
				OCR0A = brightness_control;
			} else {
				OCR0A = 255;					// ...if the button isn't held just make it fully on
			}
		} else {
			OCR0A = 0;							// pedal off = LED off
			PORTB &= ~(1<<PB3);
			PORTB &= ~(1<<PB1);
			PORTB |= (1<<PB5);
		}
		
		// send a pulse to the latching relay if necessary 
		if(pulse_counter != 0) {
			PORTB |= (1<<PB4);
		} else {
			PORTB &= ~(1<<PB4);
		}
	}
	return 1;
}