//------------------------------------------------------------------------
//  PATCH Loading
//------------------------------------------------------------------------
// 
//  DEH_EDGE  Copyright (C) 2004  The EDGE Team
// 
//  This program is under the GNU General Public License.
//  It comes WITHOUT ANY WARRANTY of any kind.
//  See COPYING.txt for the full details.
//
//------------------------------------------------------------------------

#ifndef __PATCH_HDR__
#define __PATCH_HDR__

namespace Patch
{
	extern char line_buf[];
	extern int line_num;

	extern int active_obj;
	extern int patch_fmt;

	void Load(const char *filename);
}

#endif /* __PATCH_HDR__ */
