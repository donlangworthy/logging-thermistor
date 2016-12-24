#include <stdlib.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include "clock.h"
#include "util.h"

unsigned long long getPreciseTime();
void setTime(unsigned long time);
unsigned long getTickCount();

volatile unsigned long long clock=0;	// time at last tic in seconds since the epoch / 2^32
volatile unsigned long long secondsPerPulse=(long long int)(1)<<35;
volatile unsigned long ticks=0; // ticks since last "reset";
volatile unsigned long long resetTime=0;	// last reset time in seconds since the epoch / 2^32
char timeBuffer[10];

ISR(TIMER2_OVF_vect)
{
	clock += secondsPerPulse;
	ticks++;
	PINB |= _BV(PINB5);
}

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

void setClock(char input)
{
  if ('*' == input)
  {
    setTime(receivedLong);
  }
  else accumulateInt(input);
}

void getTicks(char input)
{
  ultoa(getTickCount(), timeBuffer, 10);
  printBuffer(timeBuffer);
  transmitChar('\n', NULL);
}

void getClock(char input)
{
  ultoa(getTime(), timeBuffer, 10);
  printBuffer(timeBuffer);
  transmitChar('\n', NULL);
}


void getFrequency(char input)
{
  ltoa((secondsPerPulse >> 3)- (1ULL<<32), timeBuffer, 10);
  printBuffer(timeBuffer);
  transmitChar('\n', NULL);
}


void setFrequency(char input)
{
  if ('\n' == input)
  {
    long long int freq = receivedLong;
    if (receivedSign)
    {
      freq*=-1;
    }
    freq = (freq+(1ULL<<32))<<3;
    secondsPerPulse = freq;
  }
  else accumulateInt(input);
}
unsigned long getTickCount()
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
