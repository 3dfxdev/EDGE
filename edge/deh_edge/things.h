//------------------------------------------------------------------------
//  THING conversion
//------------------------------------------------------------------------
// 
//  DEH_EDGE  Copyright (C) 2004  The EDGE Team
// 
//  This program is under the GNU General Public License.
//  It comes WITHOUT ANY WARRANTY of any kind.
//  See COPYING.txt for the full details.
//
//------------------------------------------------------------------------

#ifndef __THINGS_HDR__
#define __THINGS_HDR__

#include "mobj.h"

namespace Things
{
	void Startup(void);

	void MarkThing(int mt_num);  // attacks too
	void ConvertTHING(void);

	void HandleFlags(const mobjinfo_t *info, int mt_num, int player);

	const char *GetSound(int sound_id);
	const char *GetSpeed(int speed);

	void AlterThing(int new_val);
}


#endif /* __THINGS_HDR__ */
