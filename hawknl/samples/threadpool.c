/*
  Sample program for the HawkNL cross platform network library
  Copyright (C) 2000-2002 Phil Frisbie, Jr. (phil@hawksoft.com)
*/
/*
  This app shows a multithreaded client/server app with thread pooling.

  Choose the network type to use with the command line:
  
  clientserver NL_IP

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
#define sleep(x)    Sleep(1000 * (x))
#else
#include <unistd.h>
#endif

#define MAX_CLIENTS     8
#define MAX_SERVER      4
#define MAX_CONNECTS    100

NLmutex     startmutex, socketmutex;
NLenum      socktype = NL_RELIABLE_PACKETS;
NLushort    serverport = 25000;
NLcond      servercond;
NLsocket    clientsock = NL_INVALID;
NLboolean   shutdown = NL_FALSE;


void printErrorExit(void)
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
    exit(1);
}

static void *mainServerLoop(void *t)
{
    NLint       thread = (NLint)t;
    NLenum      err;

    /* wait for start */
    nlMutexLock(&startmutex);
    nlMutexUnlock(&startmutex);
    nlThreadYield();
    /* wait on the condition forever */
    while(nlCondWait(&servercond, 0) == NL_TRUE)
    {
        NLbyte      string[NL_MAX_STRING_LENGTH];
        NLsocket    s;

        if(shutdown == NL_TRUE)
        {
            printf("SERVER: thread %d asked to shut down\n", thread);
            return NULL;
        }
        nlMutexLock(&socketmutex);
        if(clientsock == NL_INVALID)
        {
            printf("SERVER: thread %d, invalid client socket\n", thread);
            nlMutexUnlock(&socketmutex);
            continue;
        }
        s = clientsock;
        clientsock = NL_INVALID;
        nlMutexUnlock(&socketmutex);
        if(nlRead(s, string, sizeof(string)) > 0)
        {
            printf("SERVER: thread %d, processed client thread %d\n", thread, atoi(string));
            nlWrite(s, string, strlen(string) + 1);
        }
        else
        {
            printf("SERVER: thread %d, nlRead error\n", thread);
        }
        /* we are done, so close the socket */
        nlClose(s);
    }
    /* if we got here, there was a problem with the condition */
    /* or we are shutting down */

    err = nlGetError();

    if(err == NL_SYSTEM_ERROR)
    {
        printf("SERVER: thread %d, system error: %s\n", thread, nlGetSystemErrorStr(nlGetSystemError()));
    }
    else
    {
        printf("SERVER: thread %d, HawkNL error: %s\n", thread, nlGetErrorStr(err));
    }
    printf("SERVER: thread %d exiting\n", thread);
    return NULL;
}

static void *mainClientLoop(void *t)
{
    NLint       thread = (NLint)t;
    NLsocket    sock;
    NLaddress   addr;
    NLbyte		str[NL_MAX_STRING_LENGTH];
    
    while(1)
    {
        sock = nlOpen(0, socktype);
        if(sock == NL_INVALID)
        {
            printf("CLIENT: thread %d nlOpen error\n", thread);
            if(nlGetError() == NL_NO_NETWORK)
            {
                /* we are shutting down */
                break;
            }
            continue;
        }
        /* now connect */
        nlGetLocalAddr(sock, &addr);
        nlSetAddrPort(&addr, serverport);
        if(!nlConnect(sock, &addr))
        {
            printf("CLIENT: thread %d nlConnect error\n", thread);
            nlClose(sock);
            printf("CLIENT: thread %d closed\n", thread);
            break;
        }
        printf("CLIENT: thread %d connected to %s\n", thread, nlAddrToString(&addr, str));
        sprintf(str, "%d Client thread says hello", thread);
        nlWrite(sock, str, strlen(str) + 1);
        while(nlRead(sock, str, sizeof(str)) > 0)
        {
            printf("CLIENT: thread %d received \"%s\"\n", thread, str);
        }
        nlClose(sock);
        nlThreadYield();
    }
    printf("CLIENT: thread %d exiting\n", thread);
    return NULL;
}

int main(int argc, char **argv)
{
    NLsocket        serversock;
    NLenum          type = NL_IP;/* default network type */
    NLint           connects = 0;
    NLint           i;

    if(!nlInit())
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
        else if(strcmp(argv[1], "NL_IP") == 0)
        {
            type = NL_IP;
        }
    }

    if(!nlSelectNetwork(type))
        printErrorExit();

    nlEnable(NL_BLOCKING_IO);

    if(nlCondInit(&servercond) == NL_FALSE)
        printErrorExit();

	nlMutexInit(&startmutex);
	nlMutexInit(&socketmutex);

    /* create the server socket */
    serversock = nlOpen(serverport, socktype); /* just a random port number ;) */
    
    if(serversock == NL_INVALID)
        printErrorExit();
    
    if(!nlListen(serversock))       /* let's listen on this socket */
    {
        nlClose(serversock);
        printErrorExit();
    }
    /* create the server threads */
    nlMutexLock(&startmutex);
    for(i=0;i<MAX_SERVER;i++)
    {
        /* pass the thread number */
        (void)nlThreadCreate(mainServerLoop, (void *)i, NL_FALSE);
    }
    /* now release the threads */
    nlMutexUnlock(&startmutex);
    nlThreadYield();

    /* create the client threads */
    for(i=0;i<MAX_CLIENTS;i++)
    {
        /* pass the thread number */
        (void)nlThreadCreate(mainClientLoop, (void *)i, NL_FALSE);
    }

    printf("Testing nlCondSignal\n\n");
    /* main dispatch loop */
    while(connects < MAX_CONNECTS)
    {
        NLsocket newsock = nlAcceptConnection(serversock);
        
        if(newsock != NL_INVALID)
        {
            /* wake up one thread */
            printf("MAIN: waking up one thread for socket %d, connection %d\n", newsock, connects);
			nlMutexLock(&socketmutex);
            clientsock = newsock;
			nlMutexUnlock(&socketmutex);
            nlThreadYield();
            if(nlCondSignal(&servercond) == NL_FALSE)
            {
                printf("MAIN: nlCondSignal error\n");
                printErrorExit();
            }
            connects++;
        }
    }

    printf("\nTesting nlCondBroadcast\n\n");
    connects = 0;
    while(connects < MAX_CONNECTS)
    {
        NLsocket newsock = nlAcceptConnection(serversock);
        
        if(newsock != NL_INVALID)
        {
            /* wake up one thread */
            printf("MAIN: waking up all threads for socket %d, connection %d\n", newsock, connects);
			nlMutexLock(&socketmutex);
            clientsock = newsock;
			nlMutexUnlock(&socketmutex);
            nlThreadYield();
            if(nlCondBroadcast(&servercond) == NL_FALSE)
            {
                printf("MAIN: nlCondSignal error\n");
                printErrorExit();
            }
            connects++;
        }
    }
    /* shutdown the thread pool */
    printf("\nMAIN: Begin shutdown\n\n");
    shutdown = NL_TRUE;
    nlCondBroadcast(&servercond);
    /* this will cause the client threads to exit */
    printf("\nMAIN: Close serversock\n\n");
    nlClose(serversock);
    /* wait for all the threads to exit */
    sleep(4);
    printf("\nMAIN: Call nlShutdown\n\n");
    nlShutdown();
    printf("\nMAIN: nlShutdown finished\n\n");
	nlMutexDestroy(&startmutex);
	nlMutexDestroy(&socketmutex);
    nlCondDestroy(&servercond);
    return 0;
}

