
#define F_CPU 1000000UL
#define BAUD 9600

#include <util/setbaud.h>
#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <util/delay.h>
#include <avr/sfr_defs.h>
#include <stdio.h>
#include "ringBuffer.h"

char output=0;
volatile char input=0;

volatile ringBuffer incoming;
volatile ringBuffer outgoing;
volatile int maxSize=0;
volatile int overflow=0;
volatile unsigned long long clock=0;	// time at last tic in seconds since the epoch / 2^32
volatile unsigned long long secondsPerPulse=(long long int)(1)<<35;
volatile unsigned long ticks=0; // ticks since last "reset";
volatile unsigned long long resetTime=0;	// last reset time in seconds since the epoch / 2^32
//volatile long long int time=0;



ISR(USART_RX_vect, ISR_BLOCK)
{
	// if (UCSR0A && _BV(RXC0)) // is this necessary?
	{
		char input=UDR0;
		putChar(&incoming, input);
	}

}

ISR(TIMER2_OVF_vect)
{
	clock += secondsPerPulse;
	ticks++;
	PINB |= _BV(PINB5);
}

void transmitChar(char data)
{
	int currentSize=size(&outgoing);
	if (currentSize > maxSize) maxSize=currentSize;
	if (isOverFlow(&outgoing)) overflow=1;
	do {} while (isFull(&outgoing)); // block until ready for a byte.
	putChar(&outgoing, data);
	UCSR0B |= (1 << UDRIE0);  // set (and trigger) interrupt.
}

static FILE mystdout = FDEV_SETUP_STREAM(transmitChar, NULL, _FDEV_SETUP_WRITE);

ISR(USART_UDRE_vect)
{
	if (!isEmpty(&outgoing)) UDR0=getChar(&outgoing);
	if (isEmpty(&outgoing)) UCSR0B &= ~(1 << UDRIE0); // have to check isEmpty() again as I may have emptied the buffer.
}

void clock_init(void)
{
	ASSR |= _BV(AS2);
	TCCR2A = 0 ; // normal counting mode.
	TCCR2B = _BV(CS22) | _BV(CS21) | _BV(CS20); // prescalar 1024
	TCNT2 = 1;
	while (ASSR & _BV(TCN2UB)) {};
	TIMSK2 = _BV(TOIE2);
}

void uart_init(void)
{
	UBRR0H =UBRRH_VALUE;
	UBRR0L =UBRRL_VALUE;

#if USE_2X
	UCSR0A |= _BV(U2X0);
#else
	UCSR0A &= ~(_BV(U2X0));
#endif

	UCSR0C = _BV(UCSZ01) | _BV(UCSZ00); /* 8-bit data */
	UCSR0B = _BV(RXEN0) | _BV(TXEN0);   /* Enable RX and TX */
}

//void uart_putchar(char c) {
  //  loop_until_bit_is_set(UCSR0A, UDRE0); /* Wait until data register empty. */
  //  UDR0 = c;
//}

unsigned long long getPreciseTime()
{
	unsigned long long result=0;
	// Atomic
	{
		result=clock; // clock at last tic
		result+=(TCNT2*secondsPerPulse)>>8;
	}
	return result;

}

unsigned long getTime()
{
	return getPreciseTime() >> 32;
}

unsigned long getTicks()
{
	return ticks;
}

// Called at the precise time specified by time in seconds (no fractions allowed).
void setTime(unsigned long time)
{
	// Atomic
	{
		unsigned char rtc=TCNT2;
		// What happens if interrupt has been generated here????
		// unsigned long long currentTime=clock+rtc*secondsPerPulse>>8;
		clock=((unsigned long long)time<<32) - ((rtc*secondsPerPulse)>>8);
		if (resetTime > 0 && ticks > 0)
		{
			secondsPerPulse=(clock-resetTime)/ticks;
		}
		else // reset
		{
			resetTime=clock;
			ticks=0;
		}
	}
}

void printBuffer(char * buffer)
{
	int i=0;
	while (buffer[i])
	{
		transmitChar(buffer[i]);
		++i;
	}
}

unsigned long receivedLong=0;

void accumulateInt(char currentChar)
{
	if ('0' <= currentChar && '9' >= currentChar)
	{
		receivedLong*=10;
		receivedLong+=currentChar-'0';
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
	ADMUX |= _BV(REFS0); // use AVcc
	ADMUX |= 5; // ADC5 I'm using PortA pin 5.
	// Enable the ADC
	ADCSRA |= _BV(ADEN);
	for (int i=0; i<numSamples; ++i)
	{
		for (int j=0; j<msDelay; ++j) _delay_ms(1);
		ADCSRA |= _BV(ADSC); // start a measurement
		loop_until_bit_is_clear(ADCSRA, ADSC);
		unsigned long reading = (ADCH << 8) | ADCL;
		fprintf(&mystdout, "reading: %i\n", reading);
		unsigned long temperature = 713 - (reading*100)/115;
		fprintf(&mystdout, "Temperature: %i\n", temperature);
		average += temperature;
		temperature = temperature*9/5+320;
		fprintf(&mystdout, "Fahrenheit: %i\n", temperature);
		if (reading < min) min = reading;
		if (reading > max) max = reading;
		fprintf(&mystdout, "reading: %i\n", reading);
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

int main(void) {
	char currentCommand=0;
	char currentInput=0;
	incoming.head=incoming.tail=0;
	outgoing.head=outgoing.tail=0;

	uart_init();
	clock_init();

	DDRB=_BV(DDB5);

	UCSR0B |= _BV(RXCIE0);
	sei();

	while (1)
	{
		// processing here
		//PINB |= _BV(PINB5);
		if (!isEmpty(&incoming))
		{
      char timeBuffer[10];
			char currentChar=getChar(&incoming);
			switch (currentCommand)
			{
				case 'E' :
					transmitChar(currentChar);
					//transmitChar(currentChar);
					break;
				case 'U' :
					if ('a' <= currentChar && 'z' >= currentChar) currentChar+='A' - 'a';
					transmitChar(currentChar);
					break;
				case 'L' :
					if ('A' <= currentChar && 'Z' >= currentChar) currentChar+='a' - 'A';
					transmitChar(currentChar);
					break;
				case 'S' : // Set Clock command
					if ('*' == currentChar)
					{
						setTime(receivedLong);
					}
					else accumulateInt(currentChar);
					break;
				case 'T':
					ultoa(getTicks(), timeBuffer, 10);
					printBuffer(timeBuffer);
					transmitChar('\n');
					break;
				case 'C' : // get clock command (I expect this to be a \n
					ultoa(getTime(), timeBuffer, 10);
					printBuffer(timeBuffer);
					transmitChar('\n');
					// send time to outout buffer.
					break;
				case 'F':
					// Get Calibration Frequency
					ltoa((secondsPerPulse >> 3)- (1ULL<<32), timeBuffer, 10);
					printBuffer(timeBuffer);
					transmitChar('\n');
					break;
				case 'Q':
					// Set Calibratin Frequency
					if ('\n' == currentChar)
					{
						long long int freq = (receivedLong+(1ULL<<32))<<3;
						secondsPerPulse = freq;
					}
					else accumulateInt(currentChar);
					break;
				case 'M':
					// Measure Temperature: expect Number of measurements, Delay betwee measurements
					// return Mean, Max, Min
					measure(currentChar);
					break;
					case 'B': // print and clear outgoing buffer stats
					fprintf(&mystdout, "Head: %i, tail: %i, maxSize: %i, overflow: %i\n",
						outgoing.head, outgoing.tail, maxSize, overflow);
					maxSize=0;
					overflow=0;
				case 0 :
					currentCommand=currentChar;
					receivedLong=0;
					break;
			}
			if ('\n' == currentChar) currentCommand=0;
		}
	}
}
