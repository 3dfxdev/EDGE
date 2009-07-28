//------------------------------------------------------------------------
//  Assertions
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2006-2009 Andrew Apted
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

#include "main.h"


assert_fail_c::assert_fail_c(const char *_msg)
{
	strncpy(message, _msg, sizeof(message));

	message[sizeof(message) - 1] = 0;
}

assert_fail_c::~assert_fail_c()
{
	/* nothing needed */
}

assert_fail_c::assert_fail_c(const assert_fail_c &other)
{
	strcpy(message, other.message);
}

assert_fail_c& assert_fail_c::operator=(const assert_fail_c &other)
{
	if (this != &other)
		strcpy(message, other.message);

	return *this;
}

//----------------------------------------------------------------------------

#define MSG_BUF_LEN  1024

void AssertFail(const char *msg, ...)
{
	static char buffer[MSG_BUF_LEN];

	va_list argptr;

	va_start(argptr, msg);
	vsnprintf(buffer, MSG_BUF_LEN-1, msg, argptr);
	va_end(argptr);

	buffer[MSG_BUF_LEN-2] = 0;

	fprintf(stderr, "Sorry, an internal error occurred.\n%s", buffer);
	exit(9);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
