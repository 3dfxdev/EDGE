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

#ifndef __AUTOLOCK_H__
#define __AUTOLOCK_H__

//
// Use the classic "initialisation is resource acquisition" pattern,
// here applied to HawkNL's mutexes.  The constructor locks the
// mutex, and the destructor unlocks it.  Main advantage: exception
// safety.  (Of course a bad compiler can fuck this up if it doesn't
// call destructors during the stack unwind).
//
class autolock_c
{
private:
	NLmutex *mutex;	
	
public:
	autolock_c(NLmutex *_mutex);
	~autolock_c();
};

#endif /* __AUTOLOCK_H__ */
