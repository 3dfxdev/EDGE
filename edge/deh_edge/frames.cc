
#include "i_defs.h"
#include "frames.h"

#include "info.h"
#include "system.h"
#include "wad.h"


actioninfo_t action_info[NUMACTIONS] =
{
    { "NOTHING" },     // A_NULL
    { "LIGHT0" },      // A_Light0
    { "READY" },       // A_WeaponReady
    { "LOWER" },       // A_Lower
    { "RAISE" },       // A_Raise
    { "SHOOT" },       // A_Punch
    { "REFIRE" },      // A_ReFire
    { "SHOOT" },       // A_FirePistol
    { "LIGHT1" },      // A_Light1
    { "SHOOT" },       // A_FireShotgun
    { "LIGHT2" },      // A_Light2
    { "SHOOT" },       // A_FireShotgun2
    { "CHECKRELOAD" }, // A_CheckReload
    { "PLAYSOUND(DBOPN)" },  // A_OpenShotgun2
    { "PLAYSOUND(DBLOAD)" }, // A_LoadShotgun2
    { "PLAYSOUND(DBCLS)" },  // A_CloseShotgun2
    { "SHOOT" },      // A_FireCGun
    { "FLASH" },      // A_GunFlash
    { "SHOOT" },      // A_FireMissile
    { "SHOOT" },      // A_Saw
    { "SHOOT" },      // A_FirePlasma
    { "PLAYSOUND(BFG)" },      // A_BFGsound
    { "SHOOT" },      // A_FireBFG
    { "SPARE_ATTACK" },      // A_BFGSpray
    { "EXPLOSIONDAMAGE" },      // A_Explode
    { "MAKEPAINSOUND" },      // A_Pain
    { "PLAYER_SCREAM" },      // A_PlayerScream
    { "MAKEDEAD" },      // A_Fall
    { "MAKEOVERKILLSOUND" },      // A_XScream
    { "LOOKOUT" },      // A_Look
    { "CHASE" },      // A_Chase
    { "FACETARGET" },      // A_FaceTarget
    { "RANGE_ATTACK" },      // A_PosAttack
    { "MAKEDEATHSOUND" },      // A_Scream
    { "RANGE_ATTACK" },      // A_SPosAttack
    { "RESCHASE" },      // A_VileChase
    { "RANGEATTEMPTSND" },      // A_VileStart
    { "RANGE_ATTACK" },      // A_VileTarget
    { "EFFECTTRACKER" },      // A_VileAttack
    { "TRACKERSTART" },      // A_StartFire
    { "TRACKERFOLLOW" },      // A_Fire
    { "TRACKERACTIVE" },      // A_FireCrackle
    { "RANDOM_TRACER" },      // A_Tracer
    { "CLOSEATTEMPTSND" },      // A_SkelWhoosh
    { "CLOSE_ATTACK" },      // A_SkelFist
    { "RANGE_ATTACK" },      // A_SkelMissile
    { "RANGEATTEMPTSND" },      // A_FatRaise
    { "RESET_SPREADER" },      // A_FatAttack1
    { "RANGE_ATTACK" },      // A_FatAttack2
    { "RANGE_ATTACK" },      // A_FatAttack3
    { "NOTHING" },      // A_BossDeath
    { "RANGE_ATTACK" },      // A_CPosAttack
    { "REFIRE_CHECK" },      // A_CPosRefire
    { "COMBOATTACK" },      // A_TroopAttack
    { "CLOSE_ATTACK" },      // A_SargAttack
    { "COMBOATTACK" },      // A_HeadAttack
    { "COMBOATTACK" },      // A_BruisAttack
    { "RANGE_ATTACK" },      // A_SkullAttack
    { "WALKSOUND_CHASE" },      // A_Metal
    { "REFIRE_CHECK" },      // A_SpidRefire
    { "WALKSOUND_CHASE" },      // A_BabyMetal
    { "RANGE_ATTACK" },      // A_BspiAttack
    { "PLAYSOUND(HOOF)" },      // A_Hoof
    { "RANGE_ATTACK" },      // A_CyberAttack
    { "RANGE_ATTACK " },      // A_PainAttack
    { "SPARE_ATTACK" },      // A_PainDie
    { "NOTHING" },      // A_KeenDie
    { "MAKEPAINSOUND" },      // A_BrainPain
    { "BRAINSCREAM" },      // A_BrainScream
    { "BRAINDIE" },      // A_BrainDie
    { "NOTHING" },      // A_BrainAwake
    { "BRAINSPIT" },      // A_BrainSpit
    { "MAKEACTIVESOUND" },      // A_SpawnSound
    { "CUBETRACER" },      // A_SpawnFly
    { "BRAINMISSILEEXPLODE" }      // A_BrainExplode
};


int Frames::InitGroup(int first, char group)
{
	if (first == S_NULL)
		return 0;

	state_dyn[first].group  = group;
	state_dyn[first].gr_idx = 1;

	return 1;
}

bool Frames::SpreadGroups(void)
{
	// returns true if something was done
	bool changes = false;

	int i;

	for (i = 0; i < NUMSTATES; i++)
	{
		if (state_dyn[i].group == 0)
			continue;

		int next = states[i].nextstate;

		if (next == S_NULL)
			continue;

		if (state_dyn[next].group != 0)
			continue;

		state_dyn[next].group  = state_dyn[i].group;
		state_dyn[next].gr_idx = state_dyn[i].gr_idx + 1;
	}

	return changes;
}

bool Frames::CheckSpawnRemove(int first)
{
	if (first == S_NULL || state_dyn[first].group != 'S')
		return false;

	for (;;)
	{
		if (states[first].tics < 0)  // hibernation
			break;

		int prev_idx = state_dyn[first].gr_idx;

		first = states[first].nextstate;

		if (first == S_NULL)
			return true;

		char next_gr = state_dyn[first].group;
		int next_idx = state_dyn[first].gr_idx;

		if (next_gr != 'S' || next_idx != (prev_idx + 1))
			break;
	}

	return false;
}

void Frames::OutputState(int cur)
{
	assert(cur > 0);

	state_t *st = states + cur;

	WAD::Printf("    %s:%c:%d:%s:%s",
		sprnames[st->sprite], 65 + ((int) st->frame & 63), (int) st->tics,
		(st->frame >= 32768) ? "BRIGHT" : "NORMAL",
		action_info[st->action].ddf_name);
}

const char *Frames::GroupToName(char group, bool use_spawn)
{
	switch (group)
	{
		case 'S': return use_spawn ? "SPAWN" : "IDLE";
		case 'E': return "CHASE";
		case 'L': return "MELEE";
		case 'M': return "MISSILE";
		case 'P': return "PAIN";
		case 'D': return "DEATH";
		case 'X': return "OVERKILL";
		case 'R': return "RESPAWN";

		default:
			InternalError("GroupToName: BAD GROUP '%c'\n", group);
	}		

	return NULL;
}

void Frames::OutputGroup(int first, char group, bool use_spawn)
{
	if (first == S_NULL)
		return;

	assert(state_dyn[first].group != 0);

	// this allows group sharing (especially MELEE and MISSILE)
	char first_group = state_dyn[first].group;

	WAD::Printf("\n");
	WAD::Printf("STATES(%s) =\n", GroupToName(group, use_spawn));

	int cur = first;

	for (;;)
	{
		OutputState(cur);

		if (states[cur].tics < 0)  // go into hibernation
		{
			WAD::Printf(";\n");
			return;
		}

		int prev_idx = state_dyn[cur].gr_idx;

		cur = states[cur].nextstate;

		if (cur == S_NULL)
		{
			WAD::Printf(",#REMOVE;\n");
			return;
		}

		char next_gr = state_dyn[cur].group;
		int next_idx = state_dyn[cur].gr_idx;

		assert(next_gr != 0);

		// check if finished and DON'T need a redirector
		if (cur == first && (group == 'S' || group == 'E'))
		{
			WAD::Printf(";\n");
			return;
		}

		if (next_gr == first_group && next_idx == (prev_idx + 1))
		{
			WAD::Printf(",\n");
			continue;
		}

		// we must be finished, and we need redirection

		if (next_idx == 1)
			WAD::Printf(",#%s;\n", GroupToName(next_gr, use_spawn));
		else
			WAD::Printf(",#%s:%d;\n", GroupToName(next_gr, use_spawn), next_idx);

		return;
	}
}

