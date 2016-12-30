

void setClock(char input);
void getTicks(char input);
void getClock(char input);
void getFrequency(char input);
void setFrequency(char input);
unsigned long long getPreciseTime(void);
void setTime(unsigned long time);
unsigned long getTime(void);
unsigned long getTickCount(void);
void repeatCommand(unsigned int period, unsigned int offset, unsigned int numberOfSamples, void(*command)(unsigned int index));
void runCommand(void);
