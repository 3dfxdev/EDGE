//------------------------------------------------------------------------
//  MISCELLANEOUS Definitions
//------------------------------------------------------------------------
// 
//  DEH_EDGE  Copyright (C) 2004  The EDGE Team
// 
//  This program is under the GNU General Public License.
//  It comes WITHOUT ANY WARRANTY of any kind.
//  See COPYING.txt for the full details.
//
//------------------------------------------------------------------------

#ifndef __MISC_HDR__
#define __MISC_HDR__

namespace Misc
{
	extern int init_ammo;
	/* NOTE: initial health is set in mobjinfo[MT_PLAYER] */

	extern int max_armour;
	extern int max_health;

	extern int green_armour_class;
	extern int blue_armour_class;
	extern int bfg_cells_per_shot;

	extern int soul_health;
	extern int soul_limit;
	extern int mega_health;  // and limit

	extern int monster_infight;

	// NOTE: we don't support the amounts given by cheats
	//       (God Mode Health, IDKFA Armor, etc).

	void AlterMisc(const char *misc_name, int value);
}

#endif /* __MISC_HDR__ */
