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
#include "text.h"
#include "things.h"
#include "util.h"
#include "weapons.h"


#define MAX_LINE  512
#define MAX_TEXT_STR  1200

#define PRETTY_LEN  28


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

	void ReadBinarySound(int s_num)
	{
		Debug_PrintMsg("\n--- ReadBinarySound %d ---\n", s_num);

		if (file_error)
			FatalError("File error reading binary sprite table.\n");

		sfxinfo_t *sfx = S_sfx + s_num;

		GetRawInt();  // ignore sound name pointer
		GetRawInt();  // ignore singularity

		GetInt(O_SOUND, s_num, &sfx->priority);

		GetRawInt();  // ignore link pointer
		GetRawInt();  // ignore link pitch
		GetRawInt();  // ignore link volume

		GetRawInt();  // 
		GetRawInt();  // unused stuff
		GetRawInt();  // 
	}

	void ReadBinarySprite(int spr_num)
	{
		Debug_PrintMsg("\n--- ReadBinarySprite %d ---\n", spr_num);

		if (file_error)
			FatalError("File error reading binary sprite table.\n");

		GetRawInt();  // ignore sprite name pointer
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
			/* -AJA- NOTE WELL: the "- 1" here.  Testing confiras that the
			 * DeHackEd code omits the very last frame from the V1.666+
			 * binary format.  The V1.2 binary format is fine though.
			 */
			for (j = 0; j < NUMSTATES - 1; j++)
				ReadBinaryFrame(j);
		}

		if (doom_ver == 12)
		{
			// Note: this V1.2 sound/sprite handling UNTESTED.  I'm not even
			// sure that there exists any such DEH patch files.

			for (j = 1; j < SOUNDS_1_2; j++)
				ReadBinarySound(sound12to166[j]);

			for (j = 0; j < SPRITES_1_2; j++)
				ReadBinarySprite(sprite12to166[j]);
		}
		else
		{
			/* -AJA- NOTE WELL: we start at one, as DEH patches don't
			 * include the dummy entry.  More importantly the "- 1" here,
			 * the very last sound is "DSRADIO" which is omitted from the
			 * patch file.  Confirmed through testing.
			 */
			for (j = 1; j < NUMSFX - 1; j++)
				ReadBinarySound(j);

			for (j = 0; j < NUMSPRITES; j++)
				ReadBinarySprite(j);
		}

		// Next would be text strings.  Hard to convert.
	}

	//------------------------------------------------------------------------

	typedef enum
	{
		// patch format 5:
		DEH_THING, DEH_SOUND, DEH_FRAME, DEH_SPRITE, DEH_AMMO,
		DEH_WEAPON,  // DEH_TEXT not needed

		// patch format 6:
		DEH_PTR, DEH_CHEAT, DEH_MISC

		// boom extensions:
		// BEX_PAR, BEX_CODEPTR, BEX_STRING,
		// BEX_SOUND, BEX_MUSIC, BEX_HELPER
	}
	sectionkind_e;

	char line_buf[MAX_LINE+4];
	int  line_num;

	char *equal_pos;

	int active_section = -1;
	int active_obj = -1;

	const char *cur_txt_ptr;

	void GetNextLine(void)
	{
		int len = 0;
		equal_pos = NULL;

		for (;;)
		{
			int ch = fgetc(pat_fp);

			if (ch == EOF)
			{
				if (ferror(pat_fp))
					PrintWarn("Read error on input file.\n");

				break;
			}

			// end-of-line detection.  We support the following conventions:
			//    1. CR LF    (MSDOS/Windows)
			//    2. LF only  (Unix)
			//    3. CR only  (Macintosh)

			if (ch == '\n')
				break;

			if (ch == '\r')
			{
				ch = fgetc(pat_fp);

				if (ch != EOF && ch != '\n')
					ungetc(ch, pat_fp); 

				break;
			}

			if (len >= MAX_LINE)  // truncation mode
				continue;

			if (! equal_pos && (char)ch == '=')
				equal_pos = line_buf + len;

			line_buf[len++] = (char)ch;

			if (len == MAX_LINE)
				PrintWarn("Truncating very long line (#%d).\n", line_num);
		}

		line_buf[len] = 0;
		line_num++;
	}
	
	typedef struct
	{
		const char *name;
		int kind;
	}
	sectionname_t;

	const sectionname_t sections[] =
	{
		{ "Thing",   DEH_THING },
		{ "Sound",   DEH_SOUND },
		{ "Frame",   DEH_FRAME },
		{ "Sprite",  DEH_SPRITE },
		{ "Ammo",    DEH_AMMO },
		{ "Weapon",  DEH_WEAPON },
		{ "Pointer", DEH_PTR },
		{ "Cheat",   DEH_CHEAT },
		{ "Misc",    DEH_MISC },

		// ^Text is given special treatment.

		// boom extensions:
		//  "[PAR]",     BEX_PAR },
		//  "[CODEPTR]", BEX_CODEPTR },
		//  "[STRING]",  BEX_STRING },
		//  "[SOUND]",   BEX_SOUND },
		//  "[MUSIC]",   BEX_MUSIC },
		//  "[HELPER]",  BEX_HELPER },

		{ NULL, -1 }   // End sentinel
	};

	bool CheckNewSection(void)
	{
		if (line_buf[0] == '[')
			FatalError("BEX (Boom Extension) directives not supported.\n"
				"Line %d: %s\n", line_num, line_buf);

		int i;
		int obj_num;

		for (i = 0; sections[i].name; i++)
		{
			if (StrCaseCmpPartial(line_buf, sections[i].name) != 0)
				continue;

			int sec_len = strlen(sections[i].name);

			if (! isspace(line_buf[sec_len]))
				continue;

			if (sscanf(line_buf + sec_len, " %i ", &obj_num) != 1)
				continue;

			active_section = sections[i].kind;
			active_obj = obj_num;

			return true;
		}

		return false;
	}

	void ReadTextString(char *dest, int len)
	{
		assert(cur_txt_ptr);

		char *begin = dest;

		int start_line = line_num;

		while (len > 0)
		{
			if ((dest - begin) >= MAX_TEXT_STR)
				FatalError("Text string exceeds internal buffer length.\n"
					"[> %d characters, starting on line %d]\n",
					MAX_TEXT_STR, start_line);
			
			if (*cur_txt_ptr)
			{
				*dest++ = *cur_txt_ptr++;  len--;
				continue;
			}

			if (feof(pat_fp))
				FatalError("End of file while reading Text replacement.\n");

			GetNextLine();
			cur_txt_ptr = line_buf;

			*dest++ = '\n';  len--;
		}

		*dest = 0;
	}

	const char *PrettyTextString(const char *t)
	{
		static char buf[PRETTY_LEN*2 + 10];

		while (isspace(*t))
			t++;

		if (! *t)
			return "<<EMPTY>>";

		int len = 0;

		for (; *t && len < PRETTY_LEN; t++)
		{
			if (t[0] == t[1] && t[1] == t[2])
				continue;

			if (*t == '"')
			{
				buf[len++] = '\'';
			}
			else if (*t == '\n')
			{
				buf[len++] = '\\';
				buf[len++] = 'n';
			}
			else if ((unsigned char)*t < 32 || (unsigned char)*t >= 127)
			{
				buf[len++] = '?';
			}
			else
				buf[len++] = *t;
		}

		if (*t)
		{
			buf[len++] = '.';
			buf[len++] = '.';
			buf[len++] = '.';
		}

		buf[len] = 0;

		return buf;
	}

	void ProcessTextSection(int len1, int len2)
	{
		Debug_PrintMsg("TEXT REPLACE: %d %d\n", len1, len2);

		static char text_1[MAX_TEXT_STR+8];
		static char text_2[MAX_TEXT_STR+8];

		GetNextLine();

		cur_txt_ptr = line_buf;

		ReadTextString(text_1, len1);
		ReadTextString(text_2, len2);

		Debug_PrintMsg("- Before <%s>\n", text_1);
		Debug_PrintMsg("- After  <%s>\n", text_2);

		if (len1 == 4 && len2 == 4)
			if (TextStr::ReplaceSprite(text_1, text_2))
				return;

		if (len1 < 7 && len2 < 7)
		{
			if (Sounds::ReplaceSound(text_1, text_2))
				return;

			if (Sounds::ReplaceMusic(text_1, text_2))
				return;
		}

		if (TextStr::ReplaceString(text_1, text_2))
			return;

		PrintWarn("Cannot match text: \"%s\"\n",
			PrettyTextString(text_1));
	}

	void ProcessLine(void)
	{
		assert(active_section >= 0);

		Debug_PrintMsg("Section %d Object %d : <%s>\n", active_section,
			active_obj, line_buf);

		//!!! FIXME
	}

	void LoadDiff(void)
	{
		DetectMsg("text-based");

		doom_ver = 16;  // defaults
		patch_fmt = 5;  //

		line_num = 0;

		bool syncing = true;
		bool got_info = false;

		while (! feof(pat_fp))
		{
			GetNextLine();

			if (line_buf[0] == 0 || line_buf[0] == '#')
				continue;

			// Debug_PrintMsg("LINE %d: <%s>\n", line_num, line_buf);

			if (StrCaseCmpPartial(line_buf, "Doom version") == 0)
			{
				if (! equal_pos)
					FatalError("Badly formed directive !\nLine %d: %s\n",
						line_num, line_buf);

				doom_ver = (int)strtol(equal_pos+1, NULL, 10);

				if (! (doom_ver == 12 ||
					  (doom_ver >= 16 && doom_ver <= 21)))
				{
					FatalError("Unknown doom version found: V%d.%d\n",
						doom_ver / 10, (doom_ver+1000) % 10);
				}

				// I don't think the DeHackEd code supports this correctly
				if (doom_ver == 12)
					FatalError("Text patches for DOOM V1.2 are not supported.\n");
			}

			if (StrCaseCmpPartial(line_buf, "Patch format") == 0)
			{
				if (got_info)
					FatalError("Patch format is specified twice.\n");

				got_info = true;

				if (! equal_pos)
					FatalError("Badly formed directive !\nLine %d: %s\n",
						line_num, line_buf);

				patch_fmt = (int)strtol(equal_pos+1, NULL, 10);

				if (patch_fmt < 5 || patch_fmt > 6)
					FatalError("Unknown dehacked patch format found: %d\n",
						patch_fmt);

				VersionMsg();
			}

			if (StrCaseCmpPartial(line_buf, "include") == 0)
				FatalError("BEX (Boom Extension) INCLUDE directive is not supported.\n");

			if (StrCaseCmpPartial(line_buf, "Text") == 0 &&
				isspace(line_buf[4]))
			{
				int len1, len2;

				if (sscanf(line_buf + 4, " %i %i ", &len1, &len2) == 2 &&
					len1 > 1)
				{
					ProcessTextSection(len1, len2);
					syncing = true;
					continue;
				}
			}

			if (CheckNewSection())
			{
				syncing = false;
				continue;
			}

			if (! syncing)
			{
				ProcessLine();
			}
		}
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

