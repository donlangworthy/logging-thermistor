#define F_CPU 1000000UL

#include <stdlib.h>
#include <avr/io.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <util/delay.h>
#include <avr/sfr_defs.h>
#include "temperature.h"
#include "util.h"
#include "clock.h"

int numSamples=0;
int msDelay=0;
int offset=0;
char state='S';
char *readings=NULL;
unsigned long int firstReadingTime;
unsigned int numberOfReadings;
unsigned long int interval;
const int storageBias=15;

void measure(const char incoming)
{

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

void retrieveLog(const char incoming)
{
	if ('\n'== incoming)
	{
		fprintf(&mystdout, "firstReadingTime: %li, interval: %li, numberOfReadings: %i, ", firstReadingTime, interval, numberOfReadings);
		for (int i=0; i<numberOfReadings; ++i)
		{
			fprintf(&mystdout, "[%i]:%i ", i, readings[i]+storageBias);
		}
		fprintf(&mystdout, "\n");
	}
}

void measureAndLog(const char incoming)
{
	switch (state)
	{
		case 'S' :
			accumulateInt(incoming);
			if (' ' == incoming)
			{
				msDelay=receivedLong;
				receivedLong=0;
				state = 'O';
			}
			break;
		case 'O' :
			accumulateInt(incoming);
			if (' '==incoming)
			{
				offset=receivedLong;
				receivedLong=0;
				state = 'N';
			}
			break;
		case 'N':
			accumulateInt(incoming);
			if (' '==incoming)
			{
				numSamples=receivedLong;
				receivedLong=0;
				state='X';
			}
			break;
	}
	if ('\n' == incoming)
	{
		// set up logging here....
		state='S';
		fprintf(&mystdout, "measureAndLog(%i, %i, %i, f() )\n", msDelay, offset, numSamples);
		if (NULL != readings)
		{
			free(readings);
		}
		readings=malloc(sizeof(char)*numSamples);
		if (NULL != readings)
		{
			numberOfReadings=0;
			interval=msDelay;
			firstReadingTime=repeatCommand(msDelay, offset, numSamples, getOneMeasurement);
		}
	}
}

void getOneMeasurement(unsigned int index)
{
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
	ADCSRA |= _BV(ADSC); // start a measurement
	loop_until_bit_is_clear(ADCSRA, ADSC);
	// unsigned long reading = (ADCH << 8) | ADCL;
	unsigned long reading = ADCL;
	reading += ADCH << 8;
	unsigned long temperature = 713 - (reading*100)/115;
	PORTB &= ~_BV(PORTB4); // de-power circuit
	ADCSRA &= ~_BV(ADEN); // stop ADC
	if (NULL != readings)
	{
		int storageValue=temperature-storageBias;
		if (0>storageValue) storageValue=0;
		if (255<storageValue) storageValue=255;
		readings[index]=storageValue;
		numberOfReadings=index+1;
	}
	unsigned long myTime=getTime();
	fprintf(&mystdout, "Time: %li, Index: %3i, Temperature: %li\n", myTime, index, temperature);
}

void measureHardware(int numSamples, int msDelay)
{
	int min=1024;
	int max=0;
	int average=0;

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
 fprintf(&mystdout, "Average: %i\nMax: %i\nMin: %i\n",
		average, max, min);
}
