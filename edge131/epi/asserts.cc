//----------------------------------------------------------------------------
//  EPI Assertions
//----------------------------------------------------------------------------
//
//  Copyright (c) 2004-2005  The EDGE Team.
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

#include "epi.h"
#include "asserts.h"
#include "errors.h"

#include <stdarg.h>
#include <stdio.h>

namespace epi
{
    
void Assert::fail(const char *msg, ...)
{
	char message_buf[1000];

	message_buf[sizeof(message_buf)-1] = 0;

	va_list argptr;

	va_start(argptr, msg);
	vsprintf(message_buf, msg, argptr);
	va_end(argptr);

	if (message_buf[sizeof(message_buf)-1] != 0)
		throw error_c(EPI_ERRGEN_MEMORYALLOCFAILED,
			"Assertion failure message was too long !!", true);

	// we don't know whether this is an EPI assertion or not, and our
	// interface would get very cluttered trying to include it, so we
	// just pass 'false' in the _epi parameter.

	throw error_c(EPI_ERRGEN_ASSERTION, message_buf, false);
}

};

