//------------------------------------------------------------------------
//  QMIPTEX Main program
//------------------------------------------------------------------------
// 
//  Copyright (c) 2008  Andrew J Apted
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

#include "headers.h"

#include <time.h>


void FatalError(const char *message, ...)
{
	fprintf(stderr, "ERROR: ");

	va_list argptr;

	va_start(argptr, message);
	vfprintf(stderr, message, argptr);
	va_end(argptr);

	exit(9);
}


void ShowTitle(void)
{
	printf("\n");
	printf("**** QMIPWAD v0.02 ");
	printf(" (C) 2008 Andrew Apted ****\n");
	printf("\n");
}

void ShowUsage(void)
{
	printf("USAGE: qmipwad  [OPTIONS...]  FILES...  -o OUTPUT.wad\n");
	printf("\n");

	printf("OPTIONS:\n");
	printf("   none yet !\n");
	printf("\n");
}


int main(int argc, char **argv)
{
	// TODO

	ShowTitle();
	ShowUsage();

	return 0;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
