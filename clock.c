#include <stdlib.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include "clock.h"
#include "util.h"
#include "sleep.h"

volatile unsigned long long clock=0;	// time at last tic in seconds since the epoch / 2^32
volatile unsigned long long secondsPerPulse=(long long int)(1)<<35;
volatile unsigned long ticks=0; // ticks since last "reset";
volatile unsigned long long resetTime=0;	// last reset time in seconds since the epoch / 2^32
char timeBuffer[20];

ISR(TIMER2_OVF_vect)
{
	clock += secondsPerPulse;
	ticks++;
	// PINB |= _BV(PINB5);
}

unsigned long long getPreciseTime(void)
{
	unsigned long long result=0;
	// Atomic
	{
		result=clock; // clock at last tic
		result+=(TCNT2*secondsPerPulse)>>8;
	}
	return result;
}

unsigned long getTime(void)
{
	//return getPreciseTime() >> 32;
	unsigned long result=clock >> 32;
	result += TCNT2 >> 5;
	return result;
}

void setClock(char input)
{
  if ('*' == input)
  {
    setTime(receivedLong);
		setEarliestSleepTime(getTime()+60);
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
unsigned long getTickCount(void)
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

unsigned int _period;
unsigned long int _timeToFire;
unsigned int _numberOfSamples;
unsigned int _numberOfLastSample;
void (*_callback)(unsigned int index)=NULL;

void repeatCommand(unsigned int period, unsigned int offset, unsigned int numberOfSamples, void(*command)(unsigned int index))
{
	_numberOfLastSample=0;
	_numberOfSamples=numberOfSamples;
	_period=period;
	_timeToFire=(getTime()/period*period+offset);
	while (getTime() > _timeToFire)
	{
		_timeToFire+=_period;
	}
	_callback=command;
	fprintf(&mystdout, "repeatCommand: currentTime:%li, _timeToFire: %li\n", getTime(), _timeToFire);
}

void runCommand(void)
{
	if (NULL != _callback)
	{
		if (getTime() > _timeToFire)
		{
			_callback(_numberOfLastSample);
			++_numberOfLastSample;
			_timeToFire+=_period;
			if (_numberOfLastSample>=_numberOfSamples)
			{
				_callback=NULL;
			}
		}
	}
}
