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
	AF_EXPLODE   = (1 << 0),   // uses A_Explode
	AF_WEAPON_ST = (1 << 1),   // uses a weapon state
	AF_THING_ST  = (1 << 2)    // uses a thing state
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
	void MarkStateUsers(int state);

	void ResetAll(void); // also resets the slots and flags
	int  BeginGroup(int first, char group);
	void SpreadGroups(void);
	bool CheckSpawnRemove(int first);
	void OutputState(int cur);
	void OutputGroup(int first, char group);
	const char *GroupToName(char group, bool use_spawn);

	// debugging stuff
	void DebugRange(const char *kind, const char *entry);
}

#endif /* __FRAMES_HDR__ */
