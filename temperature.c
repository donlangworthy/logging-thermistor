#define F_CPU 1000000UL

#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <util/delay.h>
#include <avr/sfr_defs.h>
#include <stdio.h>
#include "temperature.h"
#include "util.h"

void measure(const char incoming)
{
	static int numSamples=0;
	static int msDelay=0;
	static char state='S';

	// printBuffer("in measure()\n");
	//fprintf(&mystdout, "measure(%c)\nnumSamples: %i, msDelay %i\n", incoming, numSamples, msDelay);
	// parse 2 ints, start measurements on \n
	if ('\n' == incoming)
	{
		//Take measurements here
		if (0 == numSamples) numSamples=1;
		measureHardware(numSamples, msDelay);
		numSamples=0;
		msDelay=0;
		state='S';
	} else
	{
		switch (state) {
			case 'S':
			  accumulateInt(incoming);
			  if (' '==incoming)
				{
					numSamples=receivedLong;
					state = 'D';
					receivedLong=0;
				}
				break;
			case 'D':
				accumulateInt(incoming);
				if (' '==incoming)
				{
					msDelay=receivedLong;
					receivedLong=0;
					state='M';
				}
				break;
		}
	}
}

void measureHardware(int numSamples, int msDelay)
{
	int min=1024;
	int max=0;
	int average=0;
	char xmitBuffer[32];

	// printBuffer("In measureHardware\n");
	// printf("measureHardware(%i, %i)\n", numSamples, msDelay);
	fprintf(& mystdout, "measureHardware(%i, %i)\n", numSamples, msDelay);

	// turn on voltabe to the measument circuit
	// I'm using PB4 to control the power to the circuit
	DDRB |= _BV(DDB4);
	PORTB |= _BV(PORTB4);
	// set ADC prescalar
	// use div8 as the prescalar 1mHz / 8 == 125kHz
	ADCSRA |= _BV(ADPS1) | _BV(ADPS0); // b011;
	// ADC reference voltage Vcc. I'm using full scale voltage divider
	// ADMUX |= _BV(REFS0); // use AVcc
	// ADMUX |= 5; // ADC5 I'm using PortA pin 5. //Check for actually setting to 5.
	ADMUX = _BV(REFS0) | 0x05; // Use AVcc for reference and pin 5.
	// Enable the ADC
	ADCSRA |= _BV(ADEN);
	for (int i=0; i<numSamples; ++i)
	{
		for (int j=0; j<msDelay; ++j) _delay_ms(1);
		ADCSRA |= _BV(ADSC); // start a measurement
		loop_until_bit_is_clear(ADCSRA, ADSC);
		// unsigned long reading = (ADCH << 8) | ADCL;
		unsigned long reading = ADCL;
		reading += ADCH << 8;
		fprintf(&mystdout, "reading: %li\n", reading);
			unsigned long temperature = 713 - (reading*100)/115;
		fprintf(&mystdout, "Temperature: %li\n", temperature);
		average += temperature;
		temperature = temperature*9/5+320;
		fprintf(&mystdout, "Fahrenheit: %li\n", temperature);
		if (reading < min) min = reading;
		if (reading > max) max = reading;
		fprintf(&mystdout, "reading: %li\n", reading);
	}
	average /= numSamples;

	PORTB &= ~_BV(PORTB4); // de-power circuit
	ADCSRA &= ~_BV(ADEN); // stop ADC

	// Send Results:
	printBuffer("Average: ");
	ultoa(average, xmitBuffer, 10);
	printBuffer(xmitBuffer);
	printBuffer("\nMax: ");
	ultoa(max, xmitBuffer, 10);
	printBuffer(xmitBuffer);
	printBuffer("\nMin: ");
	ultoa(min, xmitBuffer, 10);
	printBuffer(xmitBuffer);
	printBuffer("\n");
}
