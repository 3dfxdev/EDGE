//------------------------------------------------------------------------
//  TEXT Strings
//------------------------------------------------------------------------
// 
//  DEH_EDGE  Copyright (C) 2004  The EDGE Team
// 
//  This program is under the GNU General Public License.
//  It comes WITHOUT ANY WARRANTY of any kind.
//  See COPYING.txt for the full details.
//
//------------------------------------------------------------------------

#ifndef __TEXT_HDR__
#define __TEXT_HDR__

typedef struct
{
	const char *orig_name;
	char *new_name;
}
spritename_t;

namespace TextStr
{
	// these return true if the string was found.
	bool ReplaceString(const char *before, const char *after);
	bool ReplaceSprite(const char *before, const char *after);
	bool ReplaceCheat(const char *deh_name, const char *str);

	const char *GetSprite(int spr_num);

	void SpriteDependencies(void);

	void ConvertLDF(void);
}

#endif /* __TEXT_HDR__ */
