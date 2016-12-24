#include "util.h"

unsigned long receivedLong=0;
char receivedSign=0;

void accumulateInt(char currentChar)
{
	if ('0' <= currentChar && '9' >= currentChar)
	{
		receivedLong*=10;
		receivedLong+=currentChar-'0';
	}
	else if ('-' == currentChar) {
		receivedSign=1;
	}
}

void printBuffer(char * buffer)
{
	int i=0;
	while (buffer[i])
	{
		transmitChar(buffer[i], NULL);
		++i;
	}
}
