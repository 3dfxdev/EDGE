//------------------------------------------------------------------------
//  STORAGE for modifications
//------------------------------------------------------------------------
// 
//  DEH_EDGE  Copyright (C) 2004  The EDGE Team
// 
//  This program is under the GNU General Public License.
//  It comes WITHOUT ANY WARRANTY of any kind.
//  See COPYING.txt for the full details.
//
//------------------------------------------------------------------------

#ifndef __STORAGE_HDR__
#define __STORAGE_HDR__

namespace Storage
{
	void Startup(void);
	void RememberMod(int *target, int value);
	void ApplyAll(void);
}

#endif /* __STORAGE_HDR__ */
