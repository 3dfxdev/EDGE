//------------------------------------------------------------------------
// SYSTEM : System specific code
//------------------------------------------------------------------------
//
//  GL-Friendly Node Builder (C) 2000-2004 Andrew Apted
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//------------------------------------------------------------------------

#include "system.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>
#include <assert.h>


#define FATAL_COREDUMP  0

#define DEBUG_ENABLED   0
#define DEBUGGING_FILE  "deh_debug.log"
#define DEBUG_ENDIAN  0


bool quiet_mode = false;

static int cpu_big_endian = 0;

static bool disable_progress = false;
static int progress_shown;

static char message_buf[1024];

#if DEBUG_ENABLED
static FILE *debug_fp = NULL;
#endif


static void Debug_Startup(void);
static void Debug_Shutdown(void);
static void Endian_Startup(void);


//
// System_Startup
//
void System_Startup(void)
{
	setbuf(stdout, NULL);

	progress_shown = -1;

#ifdef UNIX  
	// no whirling baton if stderr is redirected
	if (! isatty(2))
		disable_progress = true;
#endif
	
	Debug_Startup();
	Endian_Startup();
}

//
// System_Shutdown
//
void System_Shutdown(void)
{
	ClearProgress();

	Debug_Shutdown();
}

//
// FileExists
//
bool FileExists(const char *filename)
{
	FILE *fp = fopen(filename, "rb");

	if (fp)
	{
		fclose(fp);
		return true;
	}

	return false;
}

const char *StrUpper(const char *name)
{
	static char up_buf[512];

	assert(strlen(name) < sizeof(up_buf) - 1);

	char *dest = up_buf;

	while (*name)
		*dest++ = toupper(*name++);

	*dest = 0;

	return up_buf;
}

/* -------- text output code ----------------------------- */

//
// PrintMsg
//
void PrintMsg(const char *str, ...)
{
	va_list args;

	va_start(args, str);
	vsprintf(message_buf, str, args);
	va_end(args);

	printf("%s", message_buf);
	fflush(stdout);

#if DEBUG_ENABLED
	Debug_PrintMsg("> %s", message_buf);
#endif
}

//
// PrintWarn
//
void PrintWarn(const char *str, ...)
{
	va_list args;

	va_start(args, str);
	vsprintf(message_buf, str, args);
	va_end(args);

	if (quiet_mode)
	{
		printf("Warning: %s", message_buf);
		fflush(stdout);
	}

#if DEBUG_ENABLED
	Debug_PrintMsg("> Warning: %s", message_buf);
#endif
}

//
// FatalError
//
void FatalError(const char *str, ...)
{
	va_list args;

	va_start(args, str);
	vsprintf(message_buf, str, args);
	va_end(args);

  	printf("\nError: %s\n\n", message_buf);
	fflush(stdout);

#if (FATAL_COREDUMP && defined(UNIX))
	raise(SIGSEGV);
#endif

	exit(5);
}

//
// InternalError
//
void InternalError(const char *str, ...)
{
	va_list args;

	va_start(args, str);
	vsprintf(message_buf, str, args);
	va_end(args);

  	printf("\nINTERNAL ERROR: %s\n\n", message_buf);
	fflush(stdout);

#if (FATAL_COREDUMP && defined(UNIX))
	raise(SIGSEGV);
#endif

	exit(5);
}

//
// ClearProgress
//
void ClearProgress(void)
{
	fprintf(stderr, "                \b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");

	progress_shown = -1;
}

//
// ShowProgress
//
void ShowProgress(int count, int limit)
{
	if (disable_progress)
		return;

	int perc = count * 100 / limit;

	if (perc == progress_shown)
		return;

	fprintf(stderr, "--%3d%%--\b\b\b\b\b\b\b\b", perc);

	progress_shown = perc;
}


/* -------- debugging code ----------------------------- */

static void Debug_Startup(void)
{
#if DEBUG_ENABLED
	debug_fp = fopen(DEBUGGING_FILE, "w");

	if (! debug_fp)
		PrintMsg("Unable to open DEBUG FILE: %s\n", DEBUGGING_FILE);

	Debug_PrintMsg("=== START OF DEBUG FILE ===\n");
#endif
}

static void Debug_Shutdown(void)
{
#if DEBUG_ENABLED
	if (debug_fp)
	{
		Debug_PrintMsg("=== END OF DEBUG FILE ===\n");

		fclose(debug_fp);
		debug_fp = NULL;
	}
#endif
}

//
// Debug_PrintMsg
//
void Debug_PrintMsg(const char *str, ...)
{
#if DEBUG_ENABLED
  if (debug_fp)
  {
    va_list args;

    va_start(args, str);
    vfprintf(debug_fp, str, args);
    va_end(args);

    fflush(debug_fp);
  }
#else
  (void) str;
#endif
}


/* -------- endian code ----------------------------- */

//
// Endian_Startup
//
// Parts inspired by the Yadex endian.cc code.
//
static void Endian_Startup(void)
{
	volatile union
	{
		unsigned char mem[32];
		unsigned int val;
	}
	u_test;

	/* sanity-check type sizes */

	int size_8  = sizeof(unsigned char);
	int size_16 = sizeof(unsigned short);
	int size_32 = sizeof(unsigned int);

	if (size_8 != 1)
		FatalError("Sanity check failed: sizeof(uint8) = %d", size_8);

	if (size_16 != 2)
		FatalError("Sanity check failed: sizeof(uint16) = %d", size_16);

	if (size_32 != 4)
		FatalError("Sanity check failed: sizeof(uint32) = %d", size_32);

	/* check endianness */

	memset((unsigned int *) u_test.mem, 0, sizeof(u_test.mem));

	u_test.mem[0] = 0x70;  u_test.mem[1] = 0x71;
	u_test.mem[2] = 0x72;  u_test.mem[3] = 0x73;

#if DEBUG_ENDIAN
	Debug_PrintMsg("Endianness magic value: 0x%08x\n", u_test.val);
#endif

	if (u_test.val == 0x70717273)
		cpu_big_endian = 1;
	else if (u_test.val == 0x73727170)
		cpu_big_endian = 0;
	else
		FatalError("Sanity check failed: weird endianness (0x%08x)", u_test.val);

#if DEBUG_ENDIAN
	Debug_PrintMsg("Endianness = %s\n", cpu_big_endian ? "BIG" : "LITTLE");

	Debug_PrintMsg("Endianness check: 0x1234 --> 0x%04x\n", 
			(int) Endian_U16(0x1234));

	Debug_PrintMsg("Endianness check: 0x11223344 --> 0x%08x\n", 
			Endian_U32(0x11223344));
#endif
}

//
// Endian_U16
//
unsigned short Endian_U16(unsigned short x)
{
	if (cpu_big_endian)
		return (x >> 8) | (x << 8);
	else
		return x;
}

//
// Endian_U32
//
unsigned int Endian_U32(unsigned int x)
{
	if (cpu_big_endian)
		return (x >> 24) | ((x >> 8) & 0xff00) |
			((x << 8) & 0xff0000) | (x << 24);
	else
		return x;
}

