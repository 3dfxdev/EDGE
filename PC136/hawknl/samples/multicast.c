/*
  Sample program for the HawkNL cross platform network library
  Copyright (C) 2000-2002 Phil Frisbie, Jr. (phil@hawksoft.com)
*/
/*
  This sample app shows client and server Multicasting.

  For complete usage, use multicast -h
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "nl.h"

#if defined WIN32 || defined WIN64

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define sleep(x)    Sleep((DWORD)(1000 * (x)))
#else
#include <unistd.h>
#define stricmp strcasecmp
#endif

/* command line helper functions from HAWK game engine */
static int    ParamCount=0;
static char **ParamData=NULL;


static void initParam(int count, char **params)
{
	ParamCount=count;
	ParamData=params;
}

static NLboolean isParam(char *name)
{
	int i;
	int len;

	for(i=0;i<ParamCount;i++)
	{
		len = (int)strlen(ParamData[i]);
		if(stricmp(name,ParamData[i])==0)
			return NL_TRUE;
	}
	return NL_FALSE;
}

static int getIntParam(char *name)
{
	int i;

	for(i=1;i<ParamCount-1;i++) {
		if(stricmp(name,ParamData[i])==0)
			return(atoi(ParamData[i+1]));
	}
	return 0;
}

static char *getStrParam(char *name)
{
	int i;

	for(i=1;i<ParamCount-1;i++) {
		if(stricmp(name,ParamData[i])==0)
			return(ParamData[i+1]);
	}
	return NULL;
}

static float getFloatParam(char *name)
{
	int i;

	for(i=1;i<ParamCount-1;i++) {
		if(stricmp(name,ParamData[i])==0)
			return(float)(atof(ParamData[i+1]));
	}
	return 0;
}

/* main program starts here */

static void printUsage(void)
{
    printf("multicast [-s (run as server)] [-a multicast_address] [-p port] [-t ttl] [-i interval]\n\n");
    printf("multicast_address full range: 224.0.0.0 to 239.255.255.255\n");
    printf("            recomended range: 234.0.0.0 to 238.255.255.255\n");
    printf("                     default: 234.235.236.237\n\n");
    printf("port number: 1024 to 65535\n");
    printf("    default: 1258\n\n");
    printf("ttl value: 1 to 255\n");
    printf("recomended values:   1 for local LAN\n");
    printf("                    15 for a site\n");
    printf("                    63 for a region\n");
    printf("                   127 for world\n");
    printf("          default: 1\n");
    printf("DO NOT use a ttl higher than you need\n\n");
    printf("interval: 1 to 60 seconds\n");
    printf(" default: 10 seconds\n\n");
}

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

static void serverLoop(NLsocket sock, NLint interval)
{
    time_t t;

    while(1 == 1)
    {
        NLbyte      *buffer;

        (void)time(&t);
        buffer = ctime(&t);
        (void)nlWrite(sock, (NLvoid *)buffer, (NLint)strlen(buffer));
        printf("Sent server time: %s", buffer);

        sleep(interval);
    }
}

static void clientLoop(NLsocket sock)
{
    NLbyte      buffer[100];
    NLbyte      string[NL_MAX_STRING_LENGTH];
    NLaddress   addr;

    memset(buffer, 0, sizeof(buffer));
    while(1 == 1)
    {
        while(nlRead(sock, (NLvoid *)buffer, (NLint)sizeof(buffer)) > 0)
        {
            nlGetRemoteAddr(sock, &addr);
            buffer[99] = (NLbyte)0;
            printf("Time at %s is %s", nlAddrToString(&addr, string), buffer);
            memset(buffer, 0, sizeof(buffer));
        }
        sleep(0);
    }
}

int main(int argc, char **argv)
{
    NLboolean   isserver = NL_FALSE;
    NLsocket    sock;
    NLaddress   addr, maddr;
    NLbyte      string[NL_MAX_STRING_LENGTH];
    NLbyte      address[] = "234.235.236.237";
    NLushort    port = 1258;
    NLint       ttl = NL_TTL_LOCAL;
    NLint       interval = 10;
    NLaddress   *alladdr;
    NLint       count;

    printf("Multicast demo program using HawkNL library.\n");
    printf("Copyright 2000-2002 by Phil Frisbie\n");
    printf("For usage help, use multicast -h\n\n");

    if(nlInit() == NL_FALSE)
        printErrorExit();

    if(nlSelectNetwork(NL_IP) == NL_FALSE)
        printErrorExit();

    initParam(argc, argv);
    if(isParam("-h") == NL_TRUE)
    {
        printUsage();
        return 0;
    }
    if(isParam("-s") == NL_TRUE)
    {
        isserver = NL_TRUE;
    }
    if(isParam("-a") == NL_TRUE)
    {
        nlStringToAddr(getStrParam("-a"), &maddr);
    }
    else
    {
        nlStringToAddr(address, &maddr);
    }
    if(isParam("-p") == NL_TRUE)
    {
        NLushort p = (NLushort)getIntParam("-p");

        if(p < 1024 || p > 65535)
        {
            printf("error: invalid port, using default port of %u\n", port);
            nlSetAddrPort(&maddr, port);
        }
        else
        {
            nlSetAddrPort(&maddr, p);
        }
    }
    else
    {
        nlSetAddrPort(&maddr, port);
    }
    if(isParam("-t") == NL_TRUE)
    {
        NLint t = getIntParam("-t");

        if(t < 1 || t > 255)
        {
            printf("error: invalid ttl, using default ttl of %d\n", ttl);
        }
        else
        {
            ttl = t;
        }
    }
    if(isParam("-i") == NL_TRUE)
    {
        NLint i = getIntParam("-i");

        if(i < 1 || i > 60)
        {
            printf("error: invalid interval, using default interval of %d\n", interval);
        }
        else
        {
            interval = i;
        }
    }
    /* change TTL if you want to get out of your LAN */
    /* note that this call must be before nlConnect */
    nlHint(NL_MULTICAST_TTL, ttl);
    /* enable reuse address to run two or more copies on one machine */
    nlHint(NL_REUSE_ADDRESS, NL_TRUE);
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
    if(isserver == NL_TRUE)
    {

        /* create a socket */
        /* to just write, you can use port 0 */
        /* but if you want to read also, then use the */
        /* multicast port just like the client code below */
        sock = nlOpen(0, NL_UDP_MULTICAST);

        if(sock == NL_INVALID)
            printErrorExit();

        nlGetLocalAddr(sock, &addr);
        printf("our socket address is %s\n", nlAddrToString(&addr, string));

        if(nlConnect(sock, &maddr) == NL_FALSE)
        {
            nlClose(sock);
            printErrorExit();
        }
        printf("multicast address %s\n", nlAddrToString(&maddr, string));

        serverLoop(sock, interval);
    }
    else
    {
        /* create a socket */
        /* to receive, you must specify a port number */
        /* that matches the multicast address port number */
        sock = nlOpen(1258, NL_UDP_MULTICAST);

        if(sock == NL_INVALID)
            printErrorExit();

        nlGetLocalAddr(sock, &addr);
        printf("our address is %s\n", nlAddrToString(&addr, string));

        if(nlConnect(sock, &maddr) == NL_FALSE)
        {
            nlClose(sock);
            printErrorExit();
        }
        printf("multicast address %s\n", nlAddrToString(&maddr, string));

        clientLoop(sock);
    }
    return 0;
}

