//------------------------------------------------------------------------
//  SAVING TO PWAD
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2009 Andrew Apted
//  Copyright (C) 1997-2003 André Majorel et al
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//------------------------------------------------------------------------
//
//  Based on Yadex which incorporated code from DEU 5.21 that was put
//  in the public domain in 1994 by Raphaël Quinet and Brendon Wyber.
//
//------------------------------------------------------------------------

#include "main.h"

#include "m_dialog.h"
#include "editsave.h"
#include "m_game.h"
#include "levels.h"
#include "w_file.h"
#include "w_list.h"


#if 1
extern int RedrawMap;
#endif


/*
   ask for a filename
*/
void InputFileName (int x0, int y0, const char *prompt, size_t maxlen,
		char *filename)
{
	char *got_name = fl_file_chooser("Select output file", "*.wad", filename);

	if (! got_name || strlen(got_name) == 0)
	{
		filename[0] = '\0'; /* return an empty string */
		return;
	}

	strcpy(filename, got_name);
}


/*
 *  save_save_as
 *  Save the current level into a pwad.
 *  
 *  If neither <level_name> nor <file_name> are NULL and the
 *  level does not come from the iwad and <save_as> is not
 *  set, there is no interaction at all. The level is saved
 *  silently into <file_name> as <level_name>.
 *
 *  Else, the user is first prompted for a level name and a
 *  file name (in that order). The level name is given a
 *  default value of "E1M1" in Doom/Heretic mode or "MAP01"
 *  in Doom II/Hexen/Strife mode. The file name is given a
 *  default value of "LEVEL.wad" where LEVEL is the level
 *  name entered previously, lowercased.  Upon return from
 *  the function, <level_name> and <file_name> are set
 *  accordingly.
 *
 *  This function is called when the user hits [q], [F2] or
 *  [F3]. In the latter case, <save_as> is set to true.
 *
 *  Note: <level_name> being NULL means that the level name
 *  is not known, which is the case when using the "c"
 *  (create) command.  When using the "e" (edit) command,
 *  the level name is always known, since the level data
 *  comes from an iwad or pwad. The same goes for
 *  <file_name>.
 *
 *  Upon return from the function, <Level> is updated.
 *
 *  FIXME this should be a method of the Editwin class.
 */

#if 0
bool save_save_as (bool prompt)
{
	static char l[WAD_NAME + 1];  // "static" to avoid memory shortages
	static y_file_name_t f; // "static" to avoid memory shortages

	if (! CheckStartingPos ())
		return;

	// Fill in the level name
	if (*Level_name)
		al_scps (l, Level_name, sizeof l - 1);
	else
	{
		prompt = true;
		if (yg_level_name == YGLN_MAP01)
			strcpy (l, "map01");
		else
		{
			strcpy (l, "e1m1");
			if (yg_level_name != YGLN_E1M1 && yg_level_name != YGLN_E1M10)
				nf_bug ("Bad yg_level_name %d", (int) yg_level_name);
		}
	}

	// Fill in the file name
	if (*Level_file_name)
		al_scps (f, file_name, sizeof f - 1);
	else
	{
		prompt = true;
		al_scpslower (f, l, sizeof f - 1);
		al_saps (f, ".wad", sizeof f - 1);
	}

	// Create the dialog
	Entry2 e ("Save level as...",
			"Level %*S\nFile %+*s\n", sizeof l - 1, l, sizeof f - 1, f);

try_again :

	// If necessary, prompt for new level name and file name
	if (prompt || ! fncmp (level_name, MainWad))
	{
		int r = e.loop ();
		if (! r)  // Cancelled by user
			return false;  // Didn't save
	}

	printf ("Saving as \"%s\" into \"%s\"\n", l, f);
	//if (! ok_to_use_weird_level_name (l))
	//  goto try_again;
	//if (! ok_to_overwrite (f))
	//  goto try_again;

	// If file already exists, ask for confirmation.
	if (al_fnature (l) == 1)
	{
		bool ok = Confirm (-1, -1, "This file already exists. Saving to it will"
				" overwrite _everything_",
				"else it might contain, including other levels or lumps !");
		if (! ok)
		{
			prompt = true;
			goto try_again;
		}
	}

	// Try to save
	int r = SaveLevelData (...);
	if (r)
	{
		Notify (-1, -1, "Could not save to file", strerror (errno));
		prompt = true;
		goto try_again;
	}

	// Successfully saved
	Level = FindMasterDir (levelname);  // FIXME useless ?
	al_scps (Level_name,            l, sizeof Level_name            - 1);
	al_scps (Level_file_name,       f, sizeof Level_file_name       - 1);
	al_scps (Level_file_name_saved, f, sizeof Level_file_name_saved - 1);
	return true;  // Did save
}
#else  /* OLD_SAVE_METHOD */


/*
   get the name of the new wad file (returns NULL on Esc)
   */

char *GetWadFileName (const char *levelname)
{
#define BUFSZ 79
	char *outfile = (char *) GetMemory (BUFSZ + 1);

	/* get the file name */
	// If no name, find a default one
	if (! levelname)
	{
		if (yg_level_name == YGLN_E1M1 || yg_level_name == YGLN_E1M10)
			levelname = "E1M1";
		else if (yg_level_name == YGLN_MAP01)
			levelname = "MAP01";
		else
		{
			nf_bug ("Bad ygd_level_name %d, using E1M1.", (int) yg_level_name);
			levelname = "E1M1";
		}
	}

	if (true)
		///???        ! Level
		///???      || ! Level->wadfile
		///???      || ! fncmp (Level->wadfile->filename, MainWad))
	{
		strcpy(outfile, levelname);
		//!!!!!! FIXME  strlower(outfile);
		strcat (outfile, ".wad");
	}
	else
		strcpy (outfile, Level->wadfile->filename);

#if 0
	do
		InputFileName (-1, -1, "Name of the new wad file:", BUFSZ, outfile);
	while (! fncmp (outfile, MainWad));

	/* escape */
	if (outfile[0] == '\0')
	{
		FreeMemory (outfile);
		return 0;
	}
	/* if the wad file already exists, rename it to "*.bak" */
	Wad_file *wf;
	for (wad_list.rewind (); wad_list.get (wf);)
		if (fncmp (outfile, wf->filename) == 0)
		{
			verbmsg ("wf->filename: %s\n", wf->filename); // DEBUG
			verbmsg ("wf->fp        %p\n", wf->fp);   // DEBUG
			verbmsg ("outfile       %s\n", outfile);    // DEBUG
			al_fdrv_t drv;
			al_fpath_t path;
			al_fbase_t base;

			al_fana (wf->filename, drv, path, base, 0);
			sprintf (wf->filename, "%s%s%s.bak", drv, path, base);
			verbmsg ("setting wf->filename to %s\n", wf->filename);  // DEBUG
			/* Need to close, then reopen: problems with SHARE.EXE */
			verbmsg ("closing %p\n", wf->fp);       // DEBUG
			fclose (wf->fp);
			verbmsg ("renaming %s -> %s\n", outfile, wf->filename); // DEBUG
			if (rename (outfile, wf->filename) != 0)
			{
				verbmsg ("removing %s\n", wf->filename);  // DEBUG
				if (remove (wf->filename) != 0 && errno != ENOENT)
				{
					char buf1[81];
					char buf2[81];
					y_snprintf (buf1, sizeof buf1, "Could not delete \"%.64s\"",
							wf->filename);
					y_snprintf (buf2, sizeof buf2, "(%.64s)", strerror (errno));
					Notify (-1, -1, buf1, buf2);
					return 0;
				}
				verbmsg ("renaming %s -> %s\n", outfile, wf->filename);  // DEBUG
				if (rename (outfile, wf->filename))
				{
					char buf1[81];
					char buf2[81];
					y_snprintf (buf1, sizeof buf1, "Could not rename \"%.64s\"",
							outfile);
					y_snprintf (buf2, sizeof buf2, "as \"%.64s\" (%.64s)",
							wf->filename, strerror (errno));
					Notify (-1, -1, buf1, buf2);
					return 0;
				}
			}
			verbmsg ("opening %s\n", wf->filename); // DEBUG
			wf->fp = fopen (wf->filename, "rb");
			if (wf->fp == 0)
			{
				char buf1[81];
				char buf2[81];
				y_snprintf (buf1, sizeof buf1, "Could not reopen \"%.64s\"",
						wf->filename);
				y_snprintf (buf2, sizeof buf2, "(%.64s)", strerror (errno));
				Notify (-1, -1, buf1, buf2);
				return 0;
			}
			verbmsg ("wf->filename: %s\n", wf->filename); // DEBUG
			verbmsg ("wf->fp        %p\n", wf->fp);   // DEBUG
			verbmsg ("outfile       %s\n", outfile);    // DEBUG
			break;
		}
	return outfile;
#endif
}
#endif

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
