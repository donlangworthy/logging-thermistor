#include <avr/sleep.h>
#include <avr/power.h>
#include "sleep.h"
#include "clock.h"

unsigned long sleepTime=0;


void setEarliestSleepTime(unsigned long clockTime)
{
  if (clockTime > sleepTime)
  {
    sleepTime = clockTime;
  }
}
char isEligibleToSleep()
{
  return getTime() > sleepTime;
}

void gotoSleep()
{
  if (isEligibleToSleep())
  {
    set_sleep_mode(SLEEP_MODE_PWR_SAVE);
    sleep_enable();
    PORTB &= ~_BV(PINB5);
    sleep_mode();  // this really does it....
    PORTB |= _BV(PINB5);
    sleep_disable();
    setEarliestSleepTime(getTime()+1);
  }
}
