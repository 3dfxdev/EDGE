//------------------------------------------------------------------------
//  Automatic locking
//------------------------------------------------------------------------
//
//  Edge MultiPlayer Server (C) 2004  Andrew Apted
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
#include "autolock.h"

autolock_c::autolock_c(NLmutex *_mutex) : mutex(_mutex)
{
	nlMutexLock(mutex);
}

autolock_c::~autolock_c()
{
	nlMutexUnlock(mutex);
	mutex = NULL;
}
