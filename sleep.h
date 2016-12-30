void setEarliestSleepTime(unsigned long clockTime);
char isEligibleToSleep(void);
void gotoSleep(void);
void stayAwakeSeconds(unsigned int timeToStayAwayke);
void stayAwakeCycles(unsigned int cyclesToStayAwake);
void repeatCommand(unsigned int period, unsigned int offset, unsigned int number, void(*)(unsigned int index));
