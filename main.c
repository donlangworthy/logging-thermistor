
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
#include "echo.h"
#include "clock.h"
#include "util.h"
#include "temperature.h"

char output=0;
volatile char input=0;

volatile ringBuffer incoming;
volatile ringBuffer outgoing;
volatile int maxSize=0;
volatile int overflow=0;
//volatile long long int time=0;

ISR(USART_RX_vect, ISR_BLOCK)
{
	// if (UCSR0A && _BV(RXC0)) // is this necessary?
	{
		char input=UDR0;
		putChar(&incoming, input);
	}

}

int transmitChar(char data, FILE *stream)
{
	int currentSize=size(&outgoing);
	if (currentSize > maxSize) maxSize=currentSize;
	if (isOverFlow(&outgoing)) overflow=1;
	do {} while (isFull(&outgoing)); // block until ready for a byte.
	putChar(&outgoing, data);
	UCSR0B |= (1 << UDRIE0);  // set (and trigger) interrupt.
	return 0;
}

FILE mystdout = FDEV_SETUP_STREAM(transmitChar, NULL, _FDEV_SETUP_WRITE);

void bufferStatus(char input);

void bufferStatus(char input)
{
	fprintf(&mystdout, "Head: %i, tail: %i, maxSize: %i, overflow: %i\n",
		outgoing.head, outgoing.tail, maxSize, overflow);
	maxSize=0;
	overflow=0;
}

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


void doNothing(char currentChar);

void doNothing(char currentChar)
{

}

typedef struct
{
	char commandLetter;
	void (*commandPointer)(char);
} CommandTarget;

CommandTarget commandTargets[]=
{
	{ .commandLetter='E', .commandPointer=echo},
	{ .commandLetter='U', .commandPointer=uppercase},
	{ .commandLetter='L', .commandPointer=lowercase},
	{ .commandLetter='S', .commandPointer=setClock},
	{ .commandLetter='T', .commandPointer=getTicks},
	{ .commandLetter='C', .commandPointer=getClock},
	{ .commandLetter='F', .commandPointer=getFrequency},
	{ .commandLetter='Q', .commandPointer=setFrequency},
	{ .commandLetter='M', .commandPointer=measure},
	{ .commandLetter='B', .commandPointer=bufferStatus},
	{ .commandLetter=0, .commandPointer=doNothing}
};


int main(void) {
	void (*currentCommand)(char)=0;
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
			char currentChar=getChar(&incoming);
			if (0 == currentCommand)
			{
				/// set set command here.
				currentCommand=doNothing;
				for (int i=0; i< sizeof commandTargets / sizeof commandTargets[0]; ++i)
				{
					if (commandTargets[i].commandLetter == currentChar)
					{
						currentCommand=commandTargets[i].commandPointer;
						break;
					}
				}
			} else {
				currentCommand(currentChar);
			}
			if ('\n' == currentChar)
			{
				currentCommand=0;
				receivedLong=0;
				receivedSign=0;
			}
		}
	}
}
