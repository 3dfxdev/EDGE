/*
  Sample program for the HawkNL cross platform network library
  Copyright (C) 2000-2002 Phil Frisbie, Jr. (phil@hawksoft.com)
*/
/*
  This sample app uses a non-blocking TCP socket to connect to a web server
  and download a file.

  Usage: getfile www.hawksoft.com /links.shtml c:\links.html
                |                |            |
                |server name     |full path   |local file
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "nl.h"

#if defined WIN32 || defined WIN64
#include <io.h>
#define open        _open
#define write       _write
#define close       _close
#define O_BINARY    _O_BINARY
#define O_CREAT     _O_CREAT
#define O_TRUNC     _O_TRUNC
#define O_RDWR      _O_RDWR
#define S_IWRITE    _S_IWRITE
#define S_IREAD     _S_IREAD
#else
#include <sys/io.h>
#define O_BINARY    0
#endif


static void printErrorExit(void)
{
    NLenum err = nlGetError();

    if(err == NL_SYSTEM_ERROR)
    {
        NLint e = nlGetSystemError();

        printf("System error: %s\n", nlGetSystemErrorStr(e));
    }
    else
    {
        printf("HawkNL error # %d: %s\n", err, nlGetErrorStr(err));
    }
    nlShutdown();
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    NLsocket    sock;
    NLaddress   addr;
    NLbyte      buffer[4096];
    int         f;
    NLint       count, total = 0;
    NLint       crfound = 0;
    NLint       lffound = 0;

    if (argc != 4)
    {
        printf("\nSyntax: getfile ServerName FullPath LocalFile\n");
        return 1;
    }

    if(nlInit() == NL_FALSE)
        printErrorExit();

    if(nlSelectNetwork(NL_IP) == NL_FALSE)
        printErrorExit();

    nlEnable(NL_SOCKET_STATS);

    nlGetAddrFromName(argv[1], &addr);

    /* use the standard HTTP port */
    nlSetAddrPort(&addr, 80);
    printf("Server address is %s\n\n", nlAddrToString(&addr, buffer));

    /* open the socket and connect to the server */
    sock = nlOpen(0, NL_RELIABLE);
    if(sock == NL_INVALID)
        printErrorExit();
    if(nlConnect(sock, &addr) == NL_FALSE)
    {
        printErrorExit();
    }

    printf("Connected\n");
    /* open the local file */
    f = open(argv[3], O_BINARY|O_CREAT|O_TRUNC|O_RDWR, S_IWRITE | S_IREAD);
    if(f < 0)
    {
        printf("Could not open local file\n");
        printErrorExit();
    }

    /* now let's ask for the file */
#ifdef TEST_GZIP
    /* this is for my own personal use to test compressed web pages */
    sprintf(buffer, "GET %s HTTP/1.1\r\nHost:%s\nAccept: */*\r\nAccept-Encoding: gzip\r\nUser-Agent: HawkNL sample program Getfile\r\n\r\n"
                    , argv[2], argv[1]);
#else
    sprintf(buffer, "GET %s HTTP/1.0\r\nHost:%s\nAccept: */*\r\nUser-Agent: HawkNL sample program Getfile\r\n\r\n"
                    , argv[2], argv[1]);
#endif
    while(nlWrite(sock, (NLvoid *)buffer, (NLint)strlen(buffer)) < 0)
    {
        if(nlGetError() == NL_CON_PENDING)
        {
            nlThreadYield();
            continue;
        }
        close(f);
        printErrorExit();
    }

    /* receive the file and write it locally */
    while(1 == 1)
    {
        count = nlRead(sock, (NLvoid *)buffer, (NLint)sizeof(buffer) - 1);
        if(count < 0)
        {
            NLint err = nlGetError();

            /* is the connection closed? */
            if(err == NL_MESSAGE_END)
            {
                break;
            }
            else
            {
                close(f);
                printErrorExit();
            }
        }
        total += count;
        if(count > 0)
        {
            /* parse out the HTTP header */
            if(lffound < 2)
            {
                int i;
                
                for(i=0;i<count;i++)
                {
                    if(buffer[i] == 0x0D)
                    {
                        crfound++;
                    }
                    else
                    {
                        if(buffer[i] == 0x0A)
                        {
                            lffound++;
                        }
                        else
                        {
                            /* reset the CR and LF counters back to 0 */
                            crfound = lffound = 0;
                        }
                    }
                    if(lffound == 2)
                    {
                        /* i points to the second LF */
                        /* NUL terminate the header string and print it out */
                        buffer[i] = buffer[i-1] = 0x0;
                        printf(buffer);

                        /* write out the rest to the file */
                        write(f, &buffer[i+1], count - i);

                        break;
                    }
                }
                if(lffound < 2)
                {
                    /* we reached the end of buffer, so print it out */
                    buffer[count + 1] = 0x0;
                    printf(buffer);
                }
            }
            else
            {
                write(f, buffer, count);
                printf("received %d bytes at %d bytes per second\r",
                    total, nlGetSocketStat(sock, NL_AVE_BYTES_RECEIVED));
            }
        }
    }

    close(f);
    nlShutdown();
    return 0;
}

