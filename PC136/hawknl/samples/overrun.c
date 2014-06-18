/*
  Sample program for the HawkNL cross platform network library
  Copyright (C) 2000-2002 Phil Frisbie, Jr. (phil@hawksoft.com)
*/
/*
  Test various calls for buffer over-run problems
*/

/*
  To test UNICODE on Windows NT/2000/XP, define UNICODE and _UNICODE in your compiler
  settings and recompile HawkNL. Then uncomment both the defines below and compile
  this program.
*/
//#define _UNICODE
//#define UNICODE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nl.h"

#ifdef WINDOWS_APP
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#endif

#ifndef _INC_TCHAR
#ifdef _UNICODE
#define TEXT(x)    L##x
#define _tmain      wmain
#define _tprintf    wprintf
#define _stprintf   swprintf
#define _tcslen     wcslen
#ifdef WINDOWS_APP
#define _ttoi       _wtoi
#else /* !WINDOWS_APP*/
#define _ttoi       wtoi
#endif /* !WINDOWS_APP*/
#else /* !UNICODE */
#define TEXT(x)    x
#define _tmain      main
#define _tprintf    printf
#define _stprintf   sprintf
#define _tcslen     strlen
#endif /* !UNICODE */
#endif /* _INC_TCHAR */

static void printError(void)
{
    NLenum err = nlGetError();

    if(err == NL_SYSTEM_ERROR)
    {
        _tprintf(TEXT("System error: %s\n"), nlGetSystemErrorStr(nlGetSystemError()));
    }
    else
    {
        _tprintf(TEXT("HawkNL error: %s\n"), nlGetErrorStr(err));
    }
}

static void printErrorExit(void)
{
    printError();
    nlShutdown();
    exit(EXIT_FAILURE);
}

#if defined (_WIN32_WCE)
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
				   LPWSTR lpCmdLine, int nShowCmd )
#else
int _tmain(int argc, NLchar **argv)
#endif
{
    NLaddress   addr1;
    NLchar      string[NL_MAX_STRING_LENGTH];
    NLchar      string2[] = "123.456.789.000%s%s%s%s%s%s%s%s%s";

    if(nlInit() == NL_FALSE)
        printErrorExit();

    _tprintf(TEXT("nlGetString(NL_VERSION) = %s\n\n"), nlGetString(NL_VERSION));

    if(nlSelectNetwork(NL_IP) == NL_FALSE)
        printErrorExit();

    /* init the string improperly with NO null termination */
    memset(string, 'a', sizeof(string));

    _tprintf(TEXT("\nTest nlStringToAddr with unterminated string\n"));
    if(nlStringToAddr(string, &addr1) == NL_FALSE)
        printError();

    _tprintf(TEXT("\nTest nlGetAddrFromName with unterminated string\n"));
    if(nlGetAddrFromName(string, &addr1) == NL_FALSE)
        printError();

    _tprintf(TEXT("\nTest nlGetAddrFromNameAsync with unterminated string\n"));
    nlGetAddrFromNameAsync(string, &addr1);
        printError();

    _tprintf(TEXT("\nTest nlStringToAddr with string containing formating\n"));
    if(nlStringToAddr(string2, &addr1) == NL_FALSE)
        printError();

    _tprintf(TEXT("\nDone\n"));
    nlShutdown();
    return 0;
}

