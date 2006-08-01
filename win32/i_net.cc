//----------------------------------------------------------------------------
//  EDGE Win32 Networking Code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2005  The EDGE Team.
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

#include "../i_defs.h"
#include "i_sysinc.h"

#ifdef USE_HAWKNL
#include "nl.h"
#endif

bool nonet = true;

//
// I_StartupNetwork
//
void I_StartupNetwork(void)
{
#ifdef USE_HAWKNL
	if (nlInit() != NL_TRUE)
	{
		I_Printf("HawkNL: Initialisation FAILED.\n");
		return;
	}

	nlHint(NL_REUSE_ADDRESS, NL_TRUE);
	nlHint(NL_TCP_NO_DELAY,  NL_TRUE);

    if (nlSelectNetwork(NL_IP) != NL_TRUE)
	{
		I_Printf("HawkNL: Select Network failed:\n%s", I_NetworkReturnError());
		nlShutdown();
		return;
	}

	I_Printf("HawkNL: Initialisation OK.\n");
	nonet = false;
#endif
}

//
// I_ShutdownNetwork
//
void I_ShutdownNetwork(void)
{
#ifdef USE_HAWKNL
	if (! nonet)
	{
		nonet = true;
		nlShutdown();
	}
#endif
}

//
// I_NetworkReturnError
//
const char *I_NetworkReturnError(void)
{
#ifdef USE_HAWKNL
	int err = nlGetError();

	if (err == NL_NO_ERROR)
		return "";

	static char err_buf[512];

	if (err == NL_SYSTEM_ERROR)
	{
		err = nlGetSystemError();
		sprintf(err_buf, "(System) %s", nlGetSystemErrorStr(err));
	}
	else
	{
		sprintf(err_buf, "%s", nlGetErrorStr(err));
	}

	// remove newlines and other rubbish
	for (int i = 0; err_buf[i] && (unsigned)i < sizeof(err_buf); i++)
	{
		if (! isprint(err_buf[i]))
			err_buf[i] = ' ';
	}

	return err_buf;

#else // USE_HAWKNL
	return "";
#endif
}

