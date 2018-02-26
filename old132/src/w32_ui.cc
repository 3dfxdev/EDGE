//----------------------------------------------------------------------------
//  EDGE Win32 UI System Code (Error messages etc...)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2009  The EDGE Team.
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
#include "w32_sysinc.h"

//
// I_MessageBox
//
// Generate a message box displaying a notice and 'OK' button.
//
void I_MessageBox(const char *_msg, const char *_title)
{
	MessageBox(NULL, _msg, _title,
			   MB_ICONEXCLAMATION | MB_OK |
			   MB_SYSTEMMODAL | MB_SETFOREGROUND);
}


