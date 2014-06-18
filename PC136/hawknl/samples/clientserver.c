/*
  Sample program for the HawkNL cross platform network library
  Copyright (C) 2000-2002 Phil Frisbie, Jr. (phil@hawksoft.com)
*/
/*
  This app shows a multithreaded client/server app.

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

NLmutex printmutex;
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

#define MAX_CLIENTS 10
NLenum socktype = NL_RELIABLE_PACKETS;
NLushort serverport = 25000;


static void *mainServerLoop(void *s)
{
    NLsocket    sock = *(NLsocket *)s;
    NLsocket    client[MAX_CLIENTS];
    NLint       clientnum = 0;
    NLbyte      string[NL_MAX_STRING_LENGTH];
    NLint       group;
    
    memset(client, 0, sizeof(client));
    
    group = nlGroupCreate();
    while(1)
    {
        NLint       i, count;
        NLbyte      buffer[128];
        NLsocket    s[MAX_CLIENTS];
        NLsocket    newsock;

        /* check for shutdown */
        if(shutdown == NL_TRUE)
        {
            printf("SERVER: shutting down\n");
            return NULL;
        }
        /* check for a new client */
        newsock = nlAcceptConnection(sock);
        
        if(newsock != NL_INVALID)
        {
            NLaddress   addr;
            
            nlGetRemoteAddr(newsock, &addr);
            client[clientnum] = newsock;
            /* add socket to the group */
            nlGroupAddSocket(group, newsock);
			nlMutexLock(&printmutex);
            printf("SERVER:Client %d connected from %s on socket %d\n", clientnum,
                        nlAddrToString(&addr, string), newsock);
			nlMutexUnlock(&printmutex);
            clientnum++;
        }
        else
        {
            NLint err = nlGetError();

            if( err == NL_SYSTEM_ERROR || err == NL_NOT_LISTEN)
            {
                printf("\n\nServer shutdown!!!!!!!!!!!!!\n");
                printErrorExit();
            }
        }
        /* check for incoming messages */
        count = nlPollGroup(group, NL_READ_STATUS, s, MAX_CLIENTS, 0);
        if(count == NL_INVALID)
        {
            NLenum err = nlGetError();
            
            if(err == NL_SYSTEM_ERROR)
            {
                printf("nlPollGroup system error: %s\n", nlGetSystemErrorStr(nlGetSystemError()));
            }
            else
            {
                printf("nlPollGroup HawkNL error: %s\n", nlGetErrorStr(err));
            }
        }
        if(count > 0)
        {
			nlMutexLock(&printmutex);
            printf("\n\n!!!!!!!!!count = %d\n", count);
			nlMutexUnlock(&printmutex);
        }
        /* loop through the clients and read the packets */
        for(i=0;i<count;i++)
        {
            int readlen;

			nlMutexLock(&printmutex);
            while((readlen = nlRead(s[i], buffer, sizeof(buffer))) > 0)
            {
                buffer[127] = 0; /* null terminate the char string */
                printf("SERVER:socket %d sent %s\n", s[i], buffer);
                /* send to the whole group */
                nlWrite(group, buffer, strlen(buffer) + 1);
            }
			nlMutexUnlock(&printmutex);
            if(readlen == NL_INVALID)
            {
                NLenum err = nlGetError();

                if( err == NL_MESSAGE_END || err == NL_SOCK_DISCONNECT)
                {
                    nlGroupDeleteSocket(group, s[i]);
                    nlClose(s[i]);
					nlMutexLock(&printmutex);
                    printf("SERVER:socket %d closed\n", s[i]);
					nlMutexUnlock(&printmutex);
                    clientnum--;
                }
            }
        }
        nlThreadYield();
    }
    return 0;
}

static void *mainClientLoop(void *s)
{
    NLsocket    sock[MAX_CLIENTS];
    NLaddress   addr;
    NLint       i, count = 4;
	NLbyte		str[256];

    sleep(1);
    /* create the client sockets */
    for(i=0;i<MAX_CLIENTS;i++)
    {
        sock[i] = nlOpen(0, socktype);
        if(sock[i] == NL_INVALID)
        {
            printf("CLIENT: nlOpen error\n");
            return (void *)5;
        }
        /* now connect */
        nlGetLocalAddr(sock[i], &addr);
        nlSetAddrPort(&addr, serverport);
        if(!nlConnect(sock[i], &addr))
        {
            printf("CLIENT: nlConnect error\n");
        }
        printf("CLIENT %d connect to %s\n", i, nlAddrToString(&addr, str));
    }
    while(count-- > 0)
    {
        for(i=0;i<MAX_CLIENTS;i++)
        {
			nlMutexLock(&printmutex);
            sprintf(str, "Client %d says hello, hello", i);
            nlWrite(sock[i], str, strlen(str) + 1);
            sprintf(str, "... client %d out.", i);
            nlWrite(sock[i], str, strlen(str) + 1);
            printf("\n\nCLIENT %d received: ",i);
            while(nlRead(sock[i], str, sizeof(str)) > 0)
            {
                printf("\"%s\",", str);
            }
			nlMutexUnlock(&printmutex);
        }
        sleep(1);
    }
    /* create the client sockets */
	nlMutexLock(&printmutex);
    for(i=0;i<MAX_CLIENTS;i++)
    {
        nlClose(sock[i]);
        printf("CLIENT %d CLOSED\n", i);
    }
	nlMutexUnlock(&printmutex);
    sleep(1);
    return (void *)4;
}

int main(int argc, char **argv)
{
    NLsocket        serversock;
    NLthreadID      tid;
    NLenum          type = NL_IP;/* default network type */
    NLint           exitcode;

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
    }

    if(!nlSelectNetwork(type))
        printErrorExit();

	nlMutexInit(&printmutex);
    /* create the server socket */
    serversock = nlOpen(serverport, socktype); /* just a random port number ;) */
    
    if(serversock == NL_INVALID)
        printErrorExit();
    
    if(!nlListen(serversock))       /* let's listen on this socket */
    {
        nlClose(serversock);
        printErrorExit();
    }
    /* start the server thread */
    (void)nlThreadCreate(mainServerLoop, (void *)&serversock, NL_FALSE);
    /*now enter the client loop */
    tid = nlThreadCreate(mainClientLoop, NULL, NL_TRUE);
    nlThreadJoin(tid, (void **)&exitcode);
    printf("mainClientLoop exited with code %d\n", exitcode);
    shutdown = NL_TRUE;
    sleep(4);
    nlShutdown();
	nlMutexDestroy(&printmutex);
    return 0;
}

