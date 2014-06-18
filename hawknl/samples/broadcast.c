/*
  Sample program for the HawkNL cross platform network library
  Copyright (C) 2000-2002 Phil Frisbie, Jr. (phil@hawksoft.com)
*/
/*
  This app shows how to broadcast on a LAN.

  Choose the network type to use with the command line:
  
  broadcast NL_IP

  The default is NL_IP. Valid network types are: NL_IP, NL_IPX,
  and NL_LOOP_BACK 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nl.h"

#if defined WIN32 || defined WIN64
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define sleep(x)    Sleep((DWORD)(1000 * (x)))
#else
#include <unistd.h>
#endif


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

static void mainTestLoop(NLsocket sock)
{
    while(1 == 1)
    {
        NLbyte      buffer[100];
        NLbyte      string[NL_MAX_STRING_LENGTH];
        NLaddress   addr;
        char        hello[] = "Hello";

        if(nlWrite(sock, (NLvoid *)hello, (NLint)sizeof(hello)) == NL_INVALID)
        {
            printErrorExit();
        }

        while(nlRead(sock, (NLvoid *)buffer, (NLint)sizeof(buffer)) > 0)
        {
            nlGetRemoteAddr(sock, &addr);
            buffer[99] = (NLbyte)0;
            printf("received %s from %s, packet #%d\n", buffer, nlAddrToString(&addr, string), nlGetInteger(NL_PACKETS_RECEIVED));
        }
        sleep(1);
    }
}

int main(int argc, char **argv)
{
    NLsocket    sock;
    NLaddress   addr, *alladdr;
    NLbyte      string[NL_MAX_STRING_LENGTH];
    NLenum      type = NL_IP; /* default network type */
    NLint       count;

    if(nlInit() == NL_FALSE)
        printErrorExit();

    printf("nlGetString(NL_VERSION) = %s\n\n", nlGetString(NL_VERSION));
    printf("nlGetString(NL_NETWORK_TYPES) = %s\n\n", nlGetString(NL_NETWORK_TYPES));

    if (argc == 2)
    {
        if(strcmp(argv[1], "NL_IPX") == 0)
        {
            type = NL_IPX;
        }
        else if(strcmp(argv[1], "NL_LOOP_BACK") == 0)
        {
            type = NL_LOOP_BACK;
        }
    }

    if(nlSelectNetwork(type) == NL_FALSE)
        printErrorExit();

    /* list all the local addresses */
    alladdr = nlGetAllLocalAddr(&count);
    if(alladdr != NULL)
    {
        printf("local %d addresses are:\n", count);
        while(count-- > 0)
        {
            printf("  %s\n", nlAddrToString(alladdr++, string));
        }
    }
    else
    {
        printErrorExit();
    }
    printf("\n");
    nlEnable(NL_SOCKET_STATS);
    /* enable reuse address to run two or more copies on one machine */
    nlHint(NL_REUSE_ADDRESS, NL_TRUE);

    /* create a client socket */
    sock = nlOpen(25000, NL_BROADCAST);

    if(sock == NL_INVALID)
        printErrorExit();

    nlGetLocalAddr(sock, &addr);
    printf("socket address is %s\n", nlAddrToString(&addr, string));
    mainTestLoop(sock);

    nlShutdown();
    return 0;
}

