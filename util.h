#include <stdio.h>
extern unsigned long receivedLong;
extern char receivedSign;

void accumulateInt(char currentChar);
void printBuffer(char * buffer);
int transmitChar(char data, FILE* stream);
extern FILE mystdout;
