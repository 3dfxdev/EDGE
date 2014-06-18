/*
  Sample program for the HawkNL cross platform network library
  Copyright (C) 2000-2002 Phil Frisbie, Jr. (phil@hawksoft.com)
*/
/*
 This app tests the swap and read/write buffer macros
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
    NLbyte      buffer[NL_MAX_STRING_LENGTH];
    NLint       count = 0;
    NLbyte      b = (NLbyte)99;
    NLushort    s = 0x1122;
    NLulong     l = 0x11223344;
    NLfloat     f = 12.3141592651f;
    NLdouble    d = 123.12345678901234;
    NLchar      string[NL_MAX_STRING_LENGTH] = TEXT("Hello");
    NLbyte      block[] = {9,8,7,6,5,4,3,2,1,0};

    if(nlInit() == NL_FALSE)
        printErrorExit();

    nlEnable(NL_BIG_ENDIAN_DATA);
    _tprintf(TEXT("nl_big_endian_data = %d\n"), nlGetBoolean(NL_BIG_ENDIAN_DATA));
    _tprintf(TEXT("nlGetString(NL_VERSION) = %s\n\n"), nlGetString(NL_VERSION));
    _tprintf(TEXT("nlGetString(NL_NETWORK_TYPES) = %s\n\n"), nlGetString(NL_NETWORK_TYPES));

    if(nlSelectNetwork(NL_IP) == NL_FALSE)
        printErrorExit();

    _tprintf(TEXT("Short number: %#x, "), s);
    s = nlSwaps(s);
    _tprintf(TEXT("swapped: %#x, "), s);
    s = nlSwaps(s);
    _tprintf(TEXT("swapped back: %#x\n"), s);

    _tprintf(TEXT("Long number: %#lx, "), l);
    l = nlSwapl(l);
    _tprintf(TEXT("swapped: %#lx, "), l);
    l = nlSwapl(l);
    _tprintf(TEXT("swapped back: %#lx\n"), l);

    _tprintf(TEXT("Float number: %.10f, "), f);
    f = nlSwapf(f);
    _tprintf(TEXT("swapped: %.10f, "), f);
    f = nlSwapf(f);
    _tprintf(TEXT("swapped back: %.10f\n"), f);

    _tprintf(TEXT("Double number: %.14f, "), d);
    d = nlSwapd(d);
    _tprintf(TEXT("swapped: %.24f, "), d);
    d = nlSwapd(d);
    _tprintf(TEXT("swapped back: %.14f\n"), d);
    _tprintf(TEXT("\n"));

    _tprintf(TEXT("write byte %d to buffer\n"), b);
    _tprintf(TEXT("write short %#x to buffer\n"), s);
    _tprintf(TEXT("write long %#lx to buffer\n"), l);
    _tprintf(TEXT("write float %f to buffer\n"), f);
    _tprintf(TEXT("write double %.14f to buffer\n"), d);
    _tprintf(TEXT("write string %s to buffer\n"), string);
    _tprintf(TEXT("write block %d%d%d%d%d%d%d%d%d%d to buffer\n"), block[0]
            , block[1], block[2], block[3], block[4], block[5], block[6]
            , block[7], block[8], block[9]);
    _tprintf(TEXT("\n"));
    writeByte(buffer, count, b);
    writeShort(buffer, count, s);
    writeLong(buffer, count, l);
    writeFloat(buffer, count, f);
    writeDouble(buffer, count, d);
    writeString(buffer, count, string);
    writeBlock(buffer, count, block, 10);

    /* reset count to zero to read from start of buffer */
    count = 0;

    readByte(buffer, count, b);
    readShort(buffer, count, s);
    readLong(buffer, count, l);
    readFloat(buffer, count, f);
    readDouble(buffer, count, d);
    readString(buffer, count, string);
    readBlock(buffer, count, block, 10);
    _tprintf(TEXT("read byte %d from buffer\n"), b);
    _tprintf(TEXT("read short %#x from buffer\n"), s);
    _tprintf(TEXT("read long %#lx from buffer\n"), l);
    _tprintf(TEXT("read float %f from buffer\n"), f);
    _tprintf(TEXT("read double %.14f from buffer\n"), d);
    _tprintf(TEXT("read string %s from buffer\n"), string);
    _tprintf(TEXT("read block %d%d%d%d%d%d%d%d%d%d from buffer\n"), block[0]
            , block[1], block[2], block[3], block[4], block[5], block[6]
            , block[7], block[8], block[9]);

    nlShutdown();
    return 0;
}

