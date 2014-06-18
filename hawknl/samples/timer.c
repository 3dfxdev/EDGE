/*
  Sample program for the HawkNL cross platform network library
  Copyright (C) 2000-2002 Phil Frisbie, Jr. (phil@hawksoft.com)
*/
/*
 This app tests the timer functions
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined WIN32 || defined WIN64
#include <conio.h>
#endif
#include "nl.h"


static void printErrorExit(void)
{
    NLenum err = nlGetError();

    if(err == NL_SYSTEM_ERROR)
    {
        printf("System error: %s\n", nlGetSystemErrorStr(nlGetSystemError()));
    }
    else
    {
        printf("HawkNL error: %s\n", nlGetErrorStr(err));
    }
    nlShutdown();
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    NLtime  t;
    NLlong  curtime, lastime;
    NLint   count;
    NLlong  times[20];

    if(nlInit() == NL_FALSE)
        printErrorExit();

    printf("Testing the resolution of nlTime()\n\n");
    nlTime(&t);
    curtime = lastime = t.useconds;
    count = 20;
    while(count-- > 0)
    {
        while(lastime == curtime)
        {
            nlTime(&t);
            curtime = t.useconds;
        }
        if(lastime > curtime)
        {
            times[count] = (curtime - lastime + 1000000);
        }
        else
        {
            times[count] = (curtime - lastime);
        }
        lastime = curtime;
    }
    count = 20;
    while(count-- > 0)
    {
        printf("nlTime resolution is %d microseconds\n", times[count]);
        lastime = curtime;
    }

    nlShutdown();
    return 0;
}

