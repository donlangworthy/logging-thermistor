

void setClock(char input);
void getTicks(char input);
void getClock(char input);
void getFrequency(char input);
void setFrequency(char input);
unsigned long long getPreciseTime();
void setTime(unsigned long time);
unsigned long getTime();
unsigned long getTickCount();
void repeatCommand(unsigned int period, unsigned int offset, unsigned int numberOfSamples, void(*command)(unsigned int index));
