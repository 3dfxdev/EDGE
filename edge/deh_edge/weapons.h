//------------------------------------------------------------------------
//  WEAPON conversion
//------------------------------------------------------------------------
// 
//  DEH_EDGE  Copyright (C) 2004  The EDGE Team
// 
//  This program is under the GNU General Public License.
//  It comes WITHOUT ANY WARRANTY of any kind.
//  See COPYING.txt for the full details.
//
//------------------------------------------------------------------------

#ifndef __WEAPONS_HDR__
#define __WEAPONS_HDR__

#include "i_defs.h"
#include "info.h"

// Ammunition types defined.
typedef enum
{
    am_bullet,    // Pistol / chaingun ammo.
    am_shell,     // Shotgun / double barreled shotgun.
    am_cell,      // Plasma rifle, BFG.
    am_rocket,    // Missile launcher.

    NUMAMMO,

	am_noammo     // Fist / chainsaw
}
ammotype_e;

namespace Ammo
{
	extern int plr_max[4];
	extern int pickups[4];
}

// The defined weapons...
typedef enum
{
    wp_fist,
    wp_pistol,
    wp_shotgun,
    wp_chaingun,
    wp_missile,
    wp_plasma,
    wp_bfg,
    wp_chainsaw,
    wp_supershotgun,
                                                                                            
    NUMWEAPONS
}
weapontype_e;

extern bool weapon_modified[NUMWEAPONS];

namespace Weapons
{
	void Startup(void);
	void ConvertWEAP(void);
}

#endif /* __WEAPONS_HDR__ */
