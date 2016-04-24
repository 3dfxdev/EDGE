//----------------------------------------------------------------------------
//  EDGE2 MD5 Library
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2015 Isotope SoftWorks and Contributors.
//  Portions (C) GSP, LLC.
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
//----------------------------------------------------------------------------

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>

#define MSGBUFSIZE 4096
static char msgbuf[MSGBUFSIZE];

void I_Printf(const char *message,...)
{
	va_list argptr;
	char *string;
	char printbuf[MSGBUFSIZE];

	// clear the buffer
	memset (printbuf, 0, MSGBUFSIZE);

	string = printbuf;

	va_start(argptr, message);

	// Print the message into a text string
	vsprintf(printbuf, message, argptr);

	// Clean up \n\r combinations
	while (*string)
	{
		if (*string == '\n')
		{
			memmove(string + 2, string + 1, strlen(string));
			string[1] = '\r';
			string++;
		}
		string++;
	}

	// Send the message to the console.
	printf("%s", printbuf);

	va_end(argptr);
}


void I_Warning(const char *warning,...)
{
	va_list argptr;

	va_start(argptr, warning);
	vsprintf(msgbuf, warning, argptr);

	I_Printf("WARNING: %s", msgbuf);
	va_end(argptr);
}

void I_Debugf(const char *message,...)
{
	va_list argptr;

	va_start(argptr, message);
	vsprintf(msgbuf, message, argptr);

	I_Printf("DEBUG: %s", msgbuf);
	va_end(argptr);
}

void I_Error(const char *error,...)
{
	va_list argptr;

	va_start(argptr, error);
	vsprintf(msgbuf, error, argptr);
	va_end(argptr);
	
	fprintf(stderr, "ERROR: %s\n", msgbuf);
	fflush(stderr);

	exit(1);
}
