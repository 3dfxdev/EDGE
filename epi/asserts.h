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

#ifndef __EPI_ASSERT__
#define __EPI_ASSERT__

#include "errors.h"

// -------- the macros --------

// FIXME: document the macros here
//

#ifndef DEBUG
#define EPI_ASSERT(cond)  ((void) 0)

#elif __GNUC__ // FIXME: proper test for __func__
#define EPI_ASSERT(cond)  ((cond) ? (void)0 : 
	epi::Assert::fail("Assertion %s failed in %s (%s:%d)\n", #cond , __func__, __FILE__, __LINE__))
#else
#define EPI_ASSERT(cond)  ((cond) ? (void)0 :  \
	epi::Assert::fail("Assertion %s failed at %s:%d\n", #cond , __FILE__, __LINE__))
#endif  // NDEBUG

#ifndef DEBUG
#define EPI_ASSERT_MSG(cond, arglist)  ((void) 0)
#else
#define EPI_ASSERT_MSG(cond, arglist)  ((cond) ? (void)0 :  \
	epi::Assert::fail arglist )
#endif

#define EPI_NULL_CHECK(ptr)    EPI_ASSERT((ptr) != NULL)
#define EPI_ZERO_CHECK(value)  EPI_ASSERT((value) != 0)

// -------- the support code -------- 

namespace epi
{

namespace Assert
{
	void fail(const char *msg, ...);
	// throw an assertion exception with the given message.
}

}  // namespace epi

#endif  /* __EPI_ASSERT__ */
