/*
  Sample program for the HawkNL cross platform network library
  Copyright (C) 2001-2003 Phil Frisbie, Jr. (phil@hawksoft.com)
*/
/*
  This app opens up UDP/TCP sockets until the system cannot open up any more,
  or it hits the NL_MAX_INT_SOCKETS limit. On a Windows NT 4.0 server with
  256 MB of RAM, it can open up 64511 UDP sockets and 7926 connected
  TCP sockets, and on Windows 95 it can open up 252 UDP sockets.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nl.h"

static void printError(void)
{
    NLenum err = nlGetError();

    if(err == NL_SYSTEM_ERROR)
    {
        printf("System error: %u, %s\n", (unsigned int)err, nlGetSystemErrorStr(nlGetSystemError()));
    }
    else
    {
        printf("HawkNL error: %s\n", nlGetErrorStr(err));
    }
}

static void printErrorExit(void)
{
    printError();
    nlShutdown();
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    NLsocket    sock, serversock, s;
    NLaddress   address, serveraddr, *alladdr;
    NLint       group, count;
    NLbyte      string[NL_MAX_STRING_LENGTH];

    if(nlInit() == NL_FALSE)
        printErrorExit();

    printf("nlGetString(NL_VERSION) = %s\n", nlGetString(NL_VERSION));
    printf("nlGetString(NL_NETWORK_TYPES) = %s\n", nlGetString(NL_NETWORK_TYPES));

    if(nlSelectNetwork(NL_IP) == NL_FALSE)
        printErrorExit();

    /* list all the local addresses */
    printf("local addresses are:\n");
    alladdr = nlGetAllLocalAddr(&count);
    if(alladdr != NULL)
    {
        while(count-- > 0)
        {
            printf("  %s\n", nlAddrToString(alladdr++, string));
        }
    }
    printf("\n");
    printf("Testing TCP connected sockets\n\n");
    serversock = nlOpen(1258, NL_RELIABLE);
    nlListen(serversock);
    nlGetLocalAddr(serversock, &serveraddr);
    nlSetAddrPort(&serveraddr, 1258);
    group = nlGroupCreate();
    nlGroupAddSocket(group, serversock);
    while(1 == 1)
    {
        /* create a client socket */
        sock = nlOpen(0, NL_RELIABLE); /* let the system assign the port number */
        if(sock == NL_INVALID)
        {
            printError();
            break;
        }
        nlConnect(sock, &serveraddr);
        printf("Socket: %d\r", sock);
        if(nlPollGroup(group, NL_READ_STATUS, &s, 1, 1000) != 1)
        {
            printError();
            break;
        }
        sock = nlAcceptConnection(serversock);
        if(sock == NL_INVALID)
        {
            printError();
            break;
        }
        printf("Socket: %d\r", sock);
    }
    printf("\nOpened %d sockets\n", nlGetInteger(NL_OPEN_SOCKETS));
    nlShutdown();

    if(nlInit() == NL_FALSE)
        printErrorExit();
    if(nlSelectNetwork(NL_IP) == NL_FALSE)
        printErrorExit();

    printf("Testing UDP sockets\n\n");
    while(1 == 1)
    {
        /* create a client socket */
        sock = nlOpen(0, NL_UNRELIABLE); /* let the system assign the port number */
        if(sock == NL_INVALID)
        {
            printError();
            break;
        }
        nlGetLocalAddr(sock, &address);
        printf("Socket: %d, port: %hu\r", sock, nlGetPortFromAddr(&address));
    }
    printf("\nOpened %d sockets\n", nlGetInteger(NL_OPEN_SOCKETS));
    nlShutdown();
    return 0;
}

