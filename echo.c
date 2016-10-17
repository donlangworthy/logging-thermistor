// Commands to Echo characters.
#include "echo.h"

void transmitChar(char data); // temporary


void echo(char input)
{
  transmitChar(input);
}

void uppercase(char input)
{
  if ('a' <= input && 'z' >= input) input+='A' - 'a';
  transmitChar(input);
}

void lowercase(char input)
{
  if ('A' <= input && 'Z' >= input) input+='a' - 'A';
  transmitChar(input);
}
