//------------------------------------------------------------------------
//  PATCH Loading
//------------------------------------------------------------------------
//
//  DEH_EDGE  Copyright (C) 2004  The EDGE Team
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License (in COPYING.txt) for more details.
//
//------------------------------------------------------------------------
//
//  DEH_EDGE is based on:
//
//  +  DeHackEd source code, by Greg Lewis.
//  -  DOOM source code (C) 1993-1996 id Software, Inc.
//  -  Linux DOOM Hack Editor, by Sam Lantinga.
//  -  PrBoom's DEH/BEX code, by Ty Halderman, TeamTNT.
//
//------------------------------------------------------------------------

#include "i_defs.h"
#include "patch.h"

#include "ammo.h"
#include "convert.h"
#include "frames.h"
#include "info.h"
#include "mobj.h"
#include "sounds.h"
#include "system.h"
#include "things.h"
#include "util.h"
#include "weapons.h"


namespace Patch
{
	FILE *pat_fp;

	bool file_error;

	int patch_fmt;   /* 1 to 5 */
	int dhe_ver;     /* 12, 13, 20-24, 30 */
	int doom_ver;    /* 12, 16 to 21 */

	typedef enum
	{
		O_MOBJ, O_AMMO, O_WEAPON, O_FRAME, O_SOUND, O_SPRITE
	}
	object_kind_e;

	void DetectMsg(const char *kind)
	{
		PrintMsg("Detected %s patch file from DEHACKED v%d.%d\n",
			kind, dhe_ver / 10, dhe_ver % 10);
	}

	void VersionMsg(void)
	{
		PrintMsg("Patch format %d, for DOOM EXE %d.%d%s\n",
			patch_fmt, doom_ver / 10, doom_ver % 10,
			(doom_ver == 16) ? "66" : "");
	
		if (patch_fmt == 2)
			PrintMsg("Note: code-pointer changes not supported.\n");
		else if (patch_fmt == 2)
			PrintMsg("Note: text string and code-pointer changes not supported.\n");
	}

	int GetRawInt(void)
	{
		if (feof(pat_fp) || ferror(pat_fp))
			file_error = true;

		if (file_error)
			return -1;

		unsigned char raw[4];

		fread(raw, 1, 4, pat_fp);

		return ((int)raw[0]) +
		       ((int)raw[1] << 8) +
		       ((int)raw[2] << 16) +
		       ((int)(char)raw[3] << 24);
	}

	const char *ObjectName(int o_kind)
	{
		switch (o_kind)
		{
			case O_MOBJ:   return "thing";
			case O_AMMO:   return "ammo";
			case O_WEAPON: return "weapon";
			case O_FRAME:  return "frame";
			case O_SOUND:  return "sound";
			case O_SPRITE: return "sprite";

			default: InternalError("Illegal O_KIND: %d\n", o_kind);
		}

		return NULL;  // not reached
	}

	void MarkObject(int o_kind, int o_num)
	{
		Debug_PrintMsg("[%d] MODIFIED\n", o_num);

		switch (o_kind)
		{
			case O_MOBJ:   Things:: MarkThing(o_num);  break;
			case O_AMMO:   Ammo::   MarkAmmo(o_num);   break;
			case O_WEAPON: Weapons::MarkWeapon(o_num); break;
			case O_FRAME:  Frames:: MarkState(o_num);  break;
			case O_SOUND:  Sounds:: MarkSound(o_num);  break;

			case O_SPRITE: // not needed 
				break;

			default: InternalError("Illegal O_KIND: %d\n", o_kind);
		}
	}

	void GetInt(int o_kind, int o_num, int *dest)
	{
		int temp = GetRawInt();

		Debug_PrintMsg("Int: %d\n", temp);

		if (*dest == temp)
			return;

		(*dest) = temp;

		MarkObject(o_kind, o_num);
	}

	void GetFrame(int o_kind, int o_num, int *dest)
	{
		int temp = GetRawInt();

		Debug_PrintMsg("Frame: %d\n", temp);

		if (doom_ver == 12)
		{
			if (temp < 0 || temp >= FRAMES_1_2)
			{
				PrintWarn("Found illegal V1.2 frame number: %d\n", temp);
				return;
			}

			temp = frame12to166[temp];
		}

		if (temp < 0 || temp >= NUMSTATES)
		{
			PrintWarn("Found illegal frame number: %d\n", temp);
			return;
		}

		if (*dest == temp)
			return;

		(*dest) = temp;

		MarkObject(o_kind, o_num);
	}

	void GetSprite(int o_kind, int o_num, int *dest)
	{
		int temp = GetRawInt();

		Debug_PrintMsg("Sprite: %d\n", temp);

		if (doom_ver == 12)
		{
			if (temp < 0 || temp >= SPRITES_1_2)
			{
				PrintWarn("Found illegal V1.2 sprite number: %d\n", temp);
				return;
			}

			temp = sprite12to166[temp];
		}

		if (temp < 0 || temp >= NUMSPRITES)
		{
			PrintWarn("Found illegal sprite number: %d\n", temp);
			return;
		}

		if (*dest == temp)
			return;

		(*dest) = temp;

		MarkObject(o_kind, o_num);
	}

	void GetSound(int o_kind, int o_num, int *dest)
	{
		int temp = GetRawInt();

		Debug_PrintMsg("Sound: %d\n", temp);

		if (doom_ver == 12)
		{
			if (temp < 0 || temp >= SOUNDS_1_2)
			{
				PrintWarn("Found illegal V1.2 sound number: %d\n", temp);
				return;
			}

			temp = sound12to166[temp];
		}

		if (temp < 0 || temp >= NUMSFX)
		{
			PrintWarn("Found illegal sound number: %d\n", temp);
			return;
	    }

		if (*dest == temp)
			return;

		(*dest) = temp;

		MarkObject(o_kind, o_num);
	}

	void GetAmmoType(int o_kind, int o_num, int *dest)
	{
		int temp = GetRawInt();

		Debug_PrintMsg("AmmoType: %d\n", temp);

		if (temp < 0 || temp > 5)
		{
			PrintWarn("Found illegal ammo type: %d\n", temp);
			return;
	    }

		if (temp == 4)
			temp = 5;

		if (*dest == temp)
			return;

		(*dest) = temp;

		MarkObject(o_kind, o_num);
	}

	void ReadBinaryThing(int mt_num)
	{
		Debug_PrintMsg("\n--- ReadBinaryThing %d ---\n", mt_num);

		if (file_error)
			FatalError("File error reading binary thing table.\n");

		mobjinfo_t *mobj = mobjinfo + mt_num;

		GetInt  (O_MOBJ, mt_num, &mobj->doomednum);
		GetFrame(O_MOBJ, mt_num, &mobj->spawnstate);
		GetInt  (O_MOBJ, mt_num, &mobj->spawnhealth);
		GetFrame(O_MOBJ, mt_num, &mobj->seestate);
		GetSound(O_MOBJ, mt_num, &mobj->seesound);
		GetInt  (O_MOBJ, mt_num, &mobj->reactiontime);

		GetSound(O_MOBJ, mt_num, &mobj->attacksound);
		GetFrame(O_MOBJ, mt_num, &mobj->painstate);
		GetInt  (O_MOBJ, mt_num, &mobj->painchance);
		GetSound(O_MOBJ, mt_num, &mobj->painsound);
		GetFrame(O_MOBJ, mt_num, &mobj->meleestate);
		GetFrame(O_MOBJ, mt_num, &mobj->missilestate);
		GetFrame(O_MOBJ, mt_num, &mobj->deathstate);
		GetFrame(O_MOBJ, mt_num, &mobj->xdeathstate);
		GetSound(O_MOBJ, mt_num, &mobj->deathsound);

		GetInt  (O_MOBJ, mt_num, &mobj->speed);
		GetInt  (O_MOBJ, mt_num, &mobj->radius);
		GetInt  (O_MOBJ, mt_num, &mobj->height);
		GetInt  (O_MOBJ, mt_num, &mobj->mass);
		GetInt  (O_MOBJ, mt_num, &mobj->damage);
		GetSound(O_MOBJ, mt_num, &mobj->activesound);
		GetInt  (O_MOBJ, mt_num, &mobj->flags);

		if (doom_ver != 12)
			GetFrame(O_MOBJ, mt_num, &mobj->raisestate);
	}

	void ReadBinaryAmmo(void)
	{
		Debug_PrintMsg("\n--- ReadBinaryAmmo ---\n");

		if (file_error)
			FatalError("File error reading binary ammo table.\n");

		GetInt(O_AMMO, 0, Ammo::plr_max + 0);
		GetInt(O_AMMO, 1, Ammo::plr_max + 1);
		GetInt(O_AMMO, 2, Ammo::plr_max + 2);
		GetInt(O_AMMO, 3, Ammo::plr_max + 3);

		GetInt(O_AMMO, 0, Ammo::pickups + 0);
		GetInt(O_AMMO, 1, Ammo::pickups + 1);
		GetInt(O_AMMO, 2, Ammo::pickups + 2);
		GetInt(O_AMMO, 3, Ammo::pickups + 3);
	}

	void ReadBinaryWeapon(int wp_num)
	{
		Debug_PrintMsg("\n--- ReadBinaryWeapon %d ---\n", wp_num);

		if (file_error)
			FatalError("File error reading binary weapon table.\n");

		weaponinfo_t *weap = weapon_info + wp_num;

		GetAmmoType(O_WEAPON, wp_num, &weap->ammo);

		GetFrame(O_WEAPON, wp_num, &weap->upstate);
		GetFrame(O_WEAPON, wp_num, &weap->downstate);
		GetFrame(O_WEAPON, wp_num, &weap->readystate);
		GetFrame(O_WEAPON, wp_num, &weap->atkstate);
		GetFrame(O_WEAPON, wp_num, &weap->flashstate);
	}

	void ReadBinaryFrame(int st_num)
	{
		Debug_PrintMsg("\n--- ReadBinaryFrame %d ---\n", st_num);

		if (file_error)
			FatalError("File error reading binary frame table.\n");

		state_t *state = states + st_num;

		GetSprite(O_FRAME, st_num, &state->sprite);
		GetInt   (O_FRAME, st_num, &state->frame);
		GetInt   (O_FRAME, st_num, &state->tics);

		GetRawInt();  // ignore code-pointer
		
		GetFrame(O_FRAME, st_num, &state->nextstate);
		GetInt  (O_FRAME, st_num, &state->misc1);
		GetInt  (O_FRAME, st_num, &state->misc2);

		if (state->misc1 || state->misc2)
			PrintWarn("frame %d has non-zero misc fields.\n", st_num);
	}

	void LoadReallyOld(void)
	{
		char tempfmt = 0;

		fread(&tempfmt, 1, 1, pat_fp);

		if (tempfmt < 1 || tempfmt > 2)
			FatalError("Bad format byte in DeHackEd patch file.\n"
				"[Really old patch, format byte %d]\n", tempfmt);

		patch_fmt = (int)tempfmt;
		doom_ver = 12;
		dhe_ver = 11 + patch_fmt;

		DetectMsg("really old");
		VersionMsg();
	
		int j;

		for (j = 0; j < THINGS_1_2; j++)
			ReadBinaryThing(thing12to166[j]);

		ReadBinaryAmmo();

		for (j = 0; j < 8; j++)
			ReadBinaryWeapon(j);  // no need to convert

		if (patch_fmt == 2)
		{
			for (j = 0; j < FRAMES_1_2; j++)
				ReadBinaryFrame(frame12to166[j]);
		}
	}

	void LoadBinary(void)
	{
		char tempdoom = 0;
		char tempfmt  = 0;

		fread(&tempdoom, 1, 1, pat_fp);
		fread(&tempfmt,  1, 1, pat_fp);

		if (tempfmt == 3)
			FatalError("Doom 1.6 beta patches are not supported.\n");
		else if (tempfmt != 4)
			FatalError("Bad format byte in DeHackEd patch file.\n"
				"[Binary patch, format byte %d]\n", tempfmt);

		patch_fmt = 4;

		if (tempdoom != 12 && ! (tempdoom >= 16 && tempdoom <= 21))
			FatalError("Bad Doom release number in patch file !\n"
				"[Binary patch, release number %d]\n", tempdoom);

		doom_ver = (int)tempdoom;

		DetectMsg("binary");
		VersionMsg();

		int j;

		if (doom_ver == 12)
		{
			for (j = 0; j < THINGS_1_2; j++)
				ReadBinaryThing(thing12to166[j]);
		}
		else
		{
			for (j = 0; j < NUMMOBJTYPES; j++)
				ReadBinaryThing(j);
		}

		ReadBinaryAmmo();

		int num_weap = (doom_ver == 12) ? 8 : 9;

		for (j = 0; j < num_weap; j++)
			ReadBinaryWeapon(j);

		if (doom_ver == 12)
		{
			for (j = 0; j < FRAMES_1_2; j++)
				ReadBinaryFrame(frame12to166[j]);
		}
		else
		{
			/* -AJA- NOTE WELL: the '- 1' here.  Testing confiras that
			 * the DeHackEd code omits the very last frame in the V1.666+
			 * binary format.  The V1.2 binary format is fine though.
			 */
			for (j = 0; j < NUMSTATES - 1; j++)
				ReadBinaryFrame(j);
		}

		// FIXME: load sounds
		// FIXME: load sprites
	}

	void LoadDiff(void)
	{
		DetectMsg("text-based");

		doom_ver = 16;   // defaults
		patch_fmt = 5;  //

		// FIXME

		// I don't think the DHE code supports this correctly
		if (doom_ver == 12)
			FatalError("Text format patches for DOOM V1.2 are not supported.\n");

		//...
	}

	void LoadNormal(void)
	{
		char idstr[30];

		memset(idstr, 0, sizeof(idstr));

		fread(idstr, 1, 24, pat_fp);

		if (StrCaseCmp(idstr, "atch File for DeHackEd v") != 0)
			FatalError("File is not a DeHackEd patch file !\n");

		memset(idstr, 0, 4);

		fread(idstr, 1, 3, pat_fp);

		if (! isdigit(idstr[0]) || idstr[1] != '.' || ! isdigit(idstr[2]))
			FatalError("Bad version string in DeHackEd patch file.\n"
				"[String %s is not digit . digit]\n", idstr);

		dhe_ver = (idstr[0] - '0') * 10 + (idstr[2] - '0');

		if (dhe_ver < 20 || dhe_ver > 31)
			FatalError("This patch file has an incorrect version number !\n"
				"[Version %s]\n", idstr); 

		if (dhe_ver < 23)
			LoadBinary();
		else
			LoadDiff();
	}
}

void Patch::Load(const char *filename)
{
	// Note: always binary - we'll handle CR/LF ourselves
	pat_fp = fopen(filename, "rb");

	if (! pat_fp)
	{
		int err_num = errno;

		FatalError("Could not open file: %s\n[%s]\n", filename, strerror(err_num));
	}
	
	file_error = false;

	char tempver = 0;

	fread(&tempver, 1, 1, pat_fp);

	if (tempver == 12)
		LoadReallyOld();
	else if (tempver == 'P')
		LoadNormal();
	else	
		FatalError("File is not a DeHackEd patch file !\n");

	fclose(pat_fp);

	PrintMsg("\n");
}

