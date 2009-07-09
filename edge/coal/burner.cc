/*  Copyright (C) 1996-1997  Id Software, Inc.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

    See file, 'COPYING', for details.
*/


#define MAX_PRINTMSG  1024


// typedef int  bool;
// #define false   0
// #define true    1


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>
#include <assert.h>

#include <sys/signal.h>


#include "c_local.h"  // FIXME FIXME : INTERNAL !!!

#include "coal.h"


void Error(char *error, ...)
{
	va_list argptr;

	printf("************ ERROR ************\n");

	va_start(argptr,error);
	vprintf(error,argptr);
	va_end(argptr);
	
	printf("\n");

	exit(1);
}


//============================================================================



static int filelength(FILE *f)
{
	int pos = ftell(f);

	fseek(f, 0, SEEK_END);

	int end = ftell(f);

	fseek(f, pos, SEEK_SET);

	return end;
}


int LoadFile(char *filename, char **bufptr)
{
	FILE *f = fopen(filename, "rb");

	if (!f)
		Error("Error opening %s: %s", filename, strerror(errno));

	int length = filelength(f);
	*bufptr = (char *) malloc(length+1);

	if ( fread(*bufptr, 1, length, f) != (size_t)length)
		Error("File read failure");

	fclose(f);

	(*bufptr)[length] = 0;

	return length;
}


//============================================================================

/*
============
main
============
*/
int main(int argc, char **argv)
{
	char	*src2;
	char	filename[1024];

	int   k;

	if (argc <= 1 ||
	    (strcmp(argv[1], "-?") == 0) || (strcmp(argv[1], "-h") == 0) ||
		(strcmp(argv[1], "-help") == 0) || (strcmp(argv[1], "--help") == 0))
	{
		printf("USAGE: coal [OPTIONS] filename.qc ...\n");
		return 0;
	}

	PR_InitData();

	if (strcmp(argv[1], "-t") == 0)
	{
		PR_SetTrace(true);
		argv++; argc--;
	}


	PR_BeginCompilation();

// compile all the files
  for (k = 1; k < argc; k++)
	{
    if (argv[k][0] == '-')
      Error("Bad filename: %s\n", argv[k]);

		sprintf(filename, "%s", argv[k]);

		printf("compiling %s\n", filename);

		LoadFile(filename, &src2);

		if (!PR_CompileFile(src2, filename) )
			exit(1);

    // FIXME: FreeFile(src2);

	}

	if (! PR_FinishCompilation())
		Error("compilation errors");

	PR_ShowStats();


  // find 'main' function

  func_t main_func = PR_FindFunction("main");

  if (! (int)main_func)
    Error("No main function!\n");

//  fprintf(stderr, "numfunctions = %d  main = %d\n", numfunctions, (int)main_func);

  PR_ExecuteProgram(main_func);


  return 0;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
