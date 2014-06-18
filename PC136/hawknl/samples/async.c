/*
  Sample program for the HawkNL cross platform network library
  Copyright (C) 2000-2002 Phil Frisbie, Jr. (phil@hawksoft.com)
*/
/*
  Test the asynchronous calls nlGetNameFromAddrAsync and nlGetAddrFromNameAsync
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
#define sleep(x)    Sleep((DWORD)(1000 * (x)))
#else
#include <unistd.h>
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


static void printErrorExit(void)
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
    NLchar      name1[NL_MAX_STRING_LENGTH] = TEXT("www.hawksoft.com:80");
    NLchar      name2[NL_MAX_STRING_LENGTH] = TEXT("www.flipcode.com");
    NLchar      name3[NL_MAX_STRING_LENGTH] = TEXT("www.amd.com");
    NLchar      name4[NL_MAX_STRING_LENGTH] = TEXT("www.3dfx.com");
    NLchar      name5[NL_MAX_STRING_LENGTH] = TEXT("192.156.136.22");
    NLaddress   addr1, addr2, addr3, addr4, addr5;
    NLchar      ip1[] = TEXT("1.2.3.4");
    NLchar      ip2[] = TEXT("64.28.67.72:80");
    NLchar      ip3[] = TEXT("66.218.74.1");
    NLchar      ip4[] = TEXT("216.200.197.161");
    NLchar      ip5[] = TEXT("64.28.67.150");
    NLchar      string[NL_MAX_STRING_LENGTH];

    if(nlInit() == NL_FALSE)
        printErrorExit();

    _tprintf(TEXT("nlGetString(NL_VERSION) = %s\n\n"), nlGetString(NL_VERSION));

    if(nlSelectNetwork(NL_IP) == NL_FALSE)
        printErrorExit();

    _tprintf(TEXT("Resolve address from name, start:\n"));

    nlGetAddrFromNameAsync(name1, &addr1);
    nlGetAddrFromNameAsync(name2, &addr2);
    nlGetAddrFromNameAsync(name3, &addr3);
    nlGetAddrFromNameAsync(name4, &addr4);
    nlGetAddrFromNameAsync(name5, &addr5);

    while(1 == 1)
    {
        int count = 0;
        static int sec = 0;

        _tprintf(TEXT("%d second(s)\n"), sec);
        if(addr1.valid == NL_TRUE)
        {
            _tprintf(TEXT("%s resolves to %s\n"), name1, nlAddrToString(&addr1, string));
            count++;
        }
        if(addr2.valid == NL_TRUE)
        {
            _tprintf(TEXT("%s resolves to %s\n"), name2, nlAddrToString(&addr2, string));
            count++;
        }
        if(addr3.valid == NL_TRUE)
        {
            _tprintf(TEXT("%s resolves to %s\n"), name3, nlAddrToString(&addr3, string));
            count++;
        }
        if(addr4.valid == NL_TRUE)
        {
            _tprintf(TEXT("%s resolves to %s\n"), name4, nlAddrToString(&addr4, string));
            count++;
        }
        if(addr5.valid == NL_TRUE)
        {
            _tprintf(TEXT("%s resolves to %s\n"), name5, nlAddrToString(&addr5, string));
            count++;
        }
        if(count == 5)
        {
            /* we are done */
            break;
        }
        _tprintf(TEXT("\n"));
        sleep(1);
        sec++;
    }

    _tprintf(TEXT("Done\n\n"));
    _tprintf(TEXT("Resolve name from address, start:\n"));
    nlStringToAddr(ip1, &addr1);
    nlStringToAddr(ip2, &addr2);
    nlStringToAddr(ip3, &addr3);
    nlStringToAddr(ip4, &addr4);
    nlStringToAddr(ip5, &addr5);
    nlGetNameFromAddrAsync(&addr1, name1);
    nlGetNameFromAddrAsync(&addr2, name2);
    nlGetNameFromAddrAsync(&addr3, name3);
    nlGetNameFromAddrAsync(&addr4, name4);
    nlGetNameFromAddrAsync(&addr5, name5);

    while(1 == 1)
    {
        int count = 0;
        static int sec = 0;

        _tprintf(TEXT("%d second(s)\n"), sec);
        if(_tcslen(name1) > 0)
        {
            _tprintf(TEXT("%s resolves to %s\n"), ip1, name1);
            count++;
        }
        if(_tcslen(name2) > 0)
        {
            _tprintf(TEXT("%s resolves to %s\n"), ip2, name2);
            count++;
        }
        if(_tcslen(name3) > 0)
        {
            _tprintf(TEXT("%s resolves to %s\n"), ip3, name3);
            count++;
        }
        if(_tcslen(name4) > 0)
        {
            _tprintf(TEXT("%s resolves to %s\n"), ip4, name4);
            count++;
        }
        if(_tcslen(name5) > 0)
        {
            _tprintf(TEXT("%s resolves to %s\n"), ip5, name5);
            count++;
        }
        if(count == 5)
        {
            /* we are done */
            break;
        }
        _tprintf(TEXT("\n"));
        sleep(1);
        sec++;
    }
    _tprintf(TEXT("Done\n"));
    nlShutdown();
    return 0;
}

