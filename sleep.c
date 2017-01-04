#include <avr/sleep.h>
#include <avr/power.h>
#include "sleep.h"
#include "clock.h"

unsigned long sleepTime=0;
unsigned int cycles=0;

void setEarliestSleepTime(unsigned long clockTime)
{
  if (clockTime > sleepTime)
  {
    sleepTime = clockTime;
  }
}

void stayAwakeCycles(unsigned int requestedCycles)
{
  if (cycles < requestedCycles) cycles=requestedCycles;
}

char isEligibleToSleep(void)
{
  char result=getTime() > sleepTime && 0 == cycles;
  if (0 != cycles)
  {
    --cycles;
  }
  return result;
}

void gotoSleep(void)
{
  if (isEligibleToSleep())
  {
    set_sleep_mode(SLEEP_MODE_PWR_SAVE);
    sleep_enable();
    PORTB &= ~_BV(PINB5);
    sleep_mode();  // this really does it....
    PORTB |= _BV(PINB5);
    sleep_disable();
    //setEarliestSleepTime(getTime());
    cycles=100;
  }
}
