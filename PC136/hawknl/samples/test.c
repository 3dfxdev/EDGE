#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nl.h"

#if defined WIN32 || defined WIN64
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define sleep(x)    Sleep(1000 * (x))
#endif

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

void mainServerLoop(NLsocket sock)
{
    NLsocket    client[MAX_CLIENTS];
    NLint       clientnum = 0;
    NLbyte      string[NL_MAX_STRING_LENGTH];

    memset(client, 0, sizeof(client));

    while(1)
    {
        NLint   i;
        NLbyte  buffer[128];

        /* check for a new client */
        NLsocket newsock = nlAcceptConnection(sock);

        if(newsock != NL_INVALID)
        {
            NLaddress   addr;

            nlGetRemoteAddr(newsock, &addr);
            client[clientnum] = newsock;
            printf("Client %d connected from %s\n", clientnum, nlAddrToString(&addr, string));
            clientnum++;
        }
        else
        {
            if(nlGetError() == NL_SYSTEM_ERROR)
            {
                printErrorExit();
            }
        }
        /* loop through the clients and read the packets */
        for(i=0;i<clientnum;i++)
        {
            if(nlRead(client[i], buffer, sizeof(buffer)) > 0)
            {
                NLint j;

                buffer[127] = 0; /* null terminate the char string */
                printf("Client %d sent %s\n", i, buffer);
                for(j=0;j<clientnum;j++)
                {
                    if(i != j)
                        nlWrite(client[j], buffer, strlen(buffer));
                }
            }
        }
        sleep(0);
    }
}

void mainClientLoop(NLsocket sock)
{
    while(1)
    {
        NLbyte  buffer[128];

        memset(buffer, 0, sizeof(buffer));
        gets(buffer);
        nlWrite(sock, buffer, strlen(buffer) + 1);
        while(nlRead(sock, buffer, sizeof(buffer)) > 0)
        {
            printf("%s\n", buffer);
        }
        sleep(0);
    }
}

int main(int argc, char **argv)
{
    NLboolean   isserver = NL_FALSE;
    NLsocket    serversock;
    NLsocket    clientsock;
    NLaddress   addr;
    NLbyte      server[] = "127.0.0.1:25000";
    NLenum      type = NL_UNRELIABLE; /* Change this to NL_RELIABLE for reliable connection */
    NLbyte      string[NL_MAX_STRING_LENGTH];

    if(!nlInit())
        printErrorExit();

    printf("nlGetString(NL_VERSION) = %s\n\n", nlGetString(NL_VERSION));
    printf("nlGetString(NL_NETWORK_TYPES) = %s\n\n", nlGetString(NL_NETWORK_TYPES));

    if(!nlSelectNetwork(NL_IP))
        printErrorExit();

    if(argc > 1)
    {
        if(!strcmp(argv[1], "-s")) /* server mode */
            isserver = NL_TRUE;
    }

    if(isserver)
    {
        /* create a server socket */
        serversock = nlOpen(25000, type); /* just a random port number ;) */

        if(serversock == NL_INVALID)
            printErrorExit();

        if(!nlListen(serversock))       /* let's listen on this socket */
        {
            nlClose(serversock);
            printErrorExit();
        }
        nlGetLocalAddr(serversock, &addr);
        printf("Server address is %s\n", nlAddrToString(&addr, string));
        mainServerLoop(serversock);
    }
    else
    {
        /* create a client socket */
        clientsock = nlOpen(0, type); /* let the system assign the port number */
        nlGetLocalAddr(clientsock, &addr);
        printf("our address is %s\n", nlAddrToString(&addr, string));

        if(clientsock == NL_INVALID)
            printErrorExit();
        /* create the NLaddress */
        nlStringToAddr(server, &addr);
        printf("Address is %s\n", nlAddrToString(&addr, string));
        /* now connect */
        if(!nlConnect(clientsock, &addr))
        {
            nlClose(clientsock);
            printErrorExit();
        }
        mainClientLoop(clientsock);
    }

    nlShutdown();
    return 0;
}

