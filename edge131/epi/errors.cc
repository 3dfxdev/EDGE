//----------------------------------------------------------------------------
//  EDGE Platform Interface Header
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
#include "errors.h"

#include <string.h>

namespace epi
{
    
// Constructor
error_c::error_c(int _code, const char* _info, bool _epi)
{
	InternalSet(_code, _info, _epi);
}

// Constructor (Copy)
error_c::error_c(const error_c &rhs)
{
	InternalSet(rhs.GetCode(), rhs.GetInfo(), rhs.IsEPIGenerated());
}

// Deconstructor
error_c::~error_c()
{
}        

// InternalSet (Private)
void error_c::InternalSet(int _code, const char* _info, bool _epi)
{
    code = _code;
	epi = _epi;
    
    if (_info)
    {
        if (strlen(_info)>=EPI_ERRINF_MAXSIZE)
	        strncpy(info, _info, EPI_ERRINF_MAXSIZE-1);
	    else
	    	strcpy(info, _info);
    }
    else
    {
	    info[0] = '\0';
    }
}

// Assignment operator
error_c& error_c::operator=(const error_c &rhs)
{
	if (this != &rhs)	// We are not assigning to self...
	{
		InternalSet(rhs.GetCode(), rhs.GetInfo(), rhs.IsEPIGenerated());
	}

	return *this;
}

}  // namespace epi

