//------------------------------------------------------------------------
//  Debugging support
//------------------------------------------------------------------------
//
//  Edge MultiPlayer Server (C) 2005  Andrew Apted
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

#include "includes.h"
#include "sys_debug.h"


#define DEBUGGING_FILE  "mp_debug.txt"

static FILE *debug_fp = NULL;

static NLmutex debug_mutex;

//
// DebugInit
//
void DebugInit(bool enable)
{
	if (enable)
	{
		debug_fp = fopen(DEBUGGING_FILE, "w");

		nlMutexInit(&debug_mutex);

		DebugPrintf("====== START OF MPSERVER DEBUG FILE ======\n\n");
	}
}

//
// DebugTerm
//
void DebugTerm(void)
{
	if (debug_fp)
	{
		DebugPrintf("\n====== END OF MPSERVER DEBUG FILE ======\n");

		fclose(debug_fp);
		debug_fp = NULL;

		nlMutexDestroy(&debug_mutex);
	}
}

//
// DebugPrintf
//
void DebugPrintf(const char *str, ...)
{
	if (debug_fp)
	{
		nlMutexLock(&debug_mutex);

		va_list args;

		va_start(args, str);
		vfprintf(debug_fp, str, args);
		va_end(args);

		fflush(debug_fp);

		nlMutexUnlock(&debug_mutex);
	}
}

