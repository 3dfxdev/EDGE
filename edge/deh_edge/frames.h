//------------------------------------------------------------------------
//  FRAME Handling
//------------------------------------------------------------------------
// 
//  DEH_EDGE  Copyright (C) 2004  The EDGE Team
// 
//  This program is under the GNU General Public License.
//  It comes WITHOUT ANY WARRANTY of any kind.
//  See COPYING.txt for the full details.
//
//------------------------------------------------------------------------

#ifndef __FRAMES_HDR__
#define __FRAMES_HDR__

typedef enum
{
	AF_EXPLODE    = (1 << 0),   // uses A_Explode
	AF_KEENDIE    = (1 << 1),   // uses A_KeenDie
	AF_LOOK       = (1 << 2),   // uses A_Look
	AF_SPREAD     = (1 << 3),   // uses A_FatAttack1/2/3

	AF_FLASH      = (1 << 5),   // weapon will go into flash state
	AF_MAKEDEAD   = (1 << 6),   // action needs an extra MAKEDEAD state

	AF_WEAPON_ST  = (1 << 10),  // uses a weapon state
	AF_THING_ST   = (1 << 11)   // uses a thing state
}
actflags_e;

namespace Frames
{
	typedef enum
	{
		RANGE = 0, COMBAT = 1, SPARE = 2
	}
	atkmethod_e;

	extern const char *attack_slot[3];
	extern int act_flags;

	void Startup(void);

	void MarkState(int st_num);
	void StateDependencies(void);

	void AlterFrame(int new_val);
	void AlterPointer(int new_val);

	void ResetAll(void); // also resets the slots and flags
	int  BeginGroup(int first, char group);
	void SpreadGroups(void);
	bool CheckSpawnRemove(int first);
	bool CheckWeaponFlash(int first);
	void OutputState(char group, int cur);
	void OutputGroup(int first, char group);
	const char *GroupToName(char group, bool use_spawn);

	// debugging stuff
	void DebugRange(const char *kind, const char *entry);
}

#endif /* __FRAMES_HDR__ */
