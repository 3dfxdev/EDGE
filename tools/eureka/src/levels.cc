//------------------------------------------------------------------------
//  LEVEL LOADING ETC
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

#include "m_bitvec.h"
#include "m_dialog.h"
#include "m_game.h"
#include "levels.h"
#include "e_things.h"
#include "w_structs.h"
#include "w_file.h"
#include "w_io.h"
#include "w_wads.h"


// FIXME should be somewhere else
int NumWTexture;    /* number of wall textures */
char **WTexture;    /* array of wall texture names */

// FIXME all the flat list stuff should be put in a separate class
size_t NumFTexture;   /* number of floor/ceiling textures */
flat_list_entry_t *flat_list; // List of all flats in the directory

int MapMaxX = -32767;   /* maximum X value of map */
int MapMaxY = -32767;   /* maximum Y value of map */
int MapMinX = 32767;    /* minimum X value of map */
int MapMinY = 32767;    /* minimum Y value of map */
bool MadeChanges;   /* made changes? */
bool MadeMapChanges;    /* made changes that need rebuilding? */

char Level_name[WAD_NAME + 1];

y_file_name_t Level_file_name;
y_file_name_t Level_file_name_saved;




/*
 *  flat_list_entry_cmp
 *  Function used by qsort() to sort the flat_list_entry array
 *  by ascending flat name.
 */
static int flat_list_entry_cmp (const void *a, const void *b)
{
	return y_strnicmp (
			((const flat_list_entry_t *) a)->name,
			((const flat_list_entry_t *) b)->name,
			WAD_FLAT_NAME);
}


/*
   function used by qsort to sort the texture names
   */
static int SortTextures (const void *a, const void *b)
{
	return y_strnicmp (*((const char *const *)a), *((const char *const *)b),
			WAD_TEX_NAME);
}


/*
   read the texture names
*/
void ReadWTextureNames ()
{
	MDirPtr dir;
	int n;
	s32_t val;

	verbmsg ("Reading texture names\n");

	// Other iwads : "TEXTURE1" and possibly "TEXTURE2"
	{
		const char *lump_name = "TEXTURE1";
		s32_t *offsets = 0;
		dir = FindMasterDir (MasterDir, lump_name);
		if (dir != NULL)  // In theory it always exists, though
		{
			const Wad_file0 *wf = dir->wadfile;
			wf->seek (dir->dir.start);
			if (wf->error ())
			{
				warn ("%s: seek error\n", lump_name);
				// FIXME
			}
			wf->read_i32 (&val);
			if (wf->error ())
			{
				// FIXME
			}
			NumWTexture = (int) val + 1;
			/* read in the offsets for texture1 names */
			offsets = (s32_t *) GetMemory ((long) NumWTexture * 4);
			wf->read_i32 (offsets + 1, NumWTexture - 1);
			{
				// FIXME
			}
			/* read in the actual names */
			WTexture = (char **) GetMemory ((long) NumWTexture * sizeof (char *));
			WTexture[0] = (char *) GetMemory (WAD_TEX_NAME + 1);
			strcpy (WTexture[0], "-");
			for (n = 1; n < NumWTexture; n++)
			{
				WTexture[n] = (char *) GetMemory (WAD_TEX_NAME + 1);
				wf->seek (dir->dir.start + offsets[n]);
				if (wf->error ())
				{
					warn ("%s: seek error\n", lump_name);
					// FIXME
				}
				wf->read_bytes (WTexture[n], WAD_TEX_NAME);
				if (wf->error ())
				{
					// FIXME
				}
				WTexture[n][WAD_TEX_NAME] = '\0';
			}
			FreeMemory (offsets);
		}
		{
			dir = FindMasterDir (MasterDir, "TEXTURE2");
			if (dir)  /* Doom II has no TEXTURE2 */
			{
				const Wad_file0 *wf = dir->wadfile;
				wf->seek (dir->dir.start);
				if (wf->error ())
				{
					warn ("%s: seek error\n", lump_name);
					// FIXME
				}
				wf->read_i32 (&val);
				if (wf->error ())
				{
					// FIXME
				}
				/* read in the offsets for texture2 names */
				offsets = (s32_t *) GetMemory ((long) val * 4);
				wf->read_i32 (offsets, val);
				if (wf->error ())
				{
					// FIXME
				}
				/* read in the actual names */
				WTexture = (char **) ResizeMemory (WTexture,
						(NumWTexture + val) * sizeof (char *));
				for (n = 0; n < val; n++)
				{
					WTexture[NumWTexture + n] = (char *) GetMemory (WAD_TEX_NAME + 1);
					wf->seek (dir->dir.start + offsets[n]);
					if (wf->error ())
					{
						warn ("%s: seek error\n", lump_name);
						// FIXME
					}
					wf->read_bytes (WTexture[NumWTexture + n], WAD_TEX_NAME);
					if (wf->error ())
						; // FIXME
					WTexture[NumWTexture + n][WAD_TEX_NAME] = '\0';
				}
				NumWTexture += val;
				FreeMemory (offsets);
			}
		}
	}

	/* sort the names */
	qsort (WTexture, NumWTexture, sizeof (char *), SortTextures);
}



/*
   forget the texture names
   */

void ForgetWTextureNames ()
{
	int n;

	/* forget all names */
	for (n = 0; n < NumWTexture; n++)
		FreeMemory (WTexture[n]);

	/* forget the array */
	NumWTexture = 0;
	FreeMemory (WTexture);
}



/*
   read the flat names
   */

void ReadFTextureNames ()
{
	MDirPtr dir;
	int n;

	verbmsg ("Reading flat names");
	NumFTexture = 0;

	for (dir = MasterDir; (dir = FindMasterDir (dir, "F_START", "FF_START"));)
	{
		bool ff_start = ! y_strnicmp (dir->dir.name, "FF_START", WAD_NAME);
		MDirPtr dir0;
		/* count the names */
		dir = dir->next;
		dir0 = dir;
		for (n = 0; dir && y_strnicmp (dir->dir.name, "F_END", WAD_NAME)
				&& (! ff_start || y_strnicmp (dir->dir.name, "FF_END", WAD_NAME));
				dir = dir->next)
		{
			if (dir->dir.start == 0 || dir->dir.size == 0)
			{
				if (! (toupper (dir->dir.name[0]) == 'F'
							&& (dir->dir.name[1] == '1'
								|| dir->dir.name[1] == '2'
								|| dir->dir.name[1] == '3'
								|| toupper (dir->dir.name[1]) == 'F')
							&& dir->dir.name[2] == '_'
							&& (! y_strnicmp (dir->dir.name + 3, "START", WAD_NAME - 3)
								|| ! y_strnicmp (dir->dir.name + 3, "END", WAD_NAME - 3))))
					warn ("unexpected label \"%.*s\" among flats.\n",
							WAD_NAME, dir->dir.name);
				continue;
			}
			if (dir->dir.size != 4096)
				warn ("flat \"%.*s\" has weird size %lu."
						" Using 4096 instead.\n",
						WAD_NAME, dir->dir.name, (unsigned long) dir->dir.size);
			n++;
		}
		/* If FF_START/FF_END followed by F_END (mm2.wad), advance
		   past F_END. In fact, this does not work because the F_END
		   that follows has been snatched by OpenPatchWad(), that
		   thinks it replaces the F_END from the iwad. OpenPatchWad()
		   needs to be kludged to take this special case into
		   account. Fortunately, the only consequence is a useless
		   "this wad uses FF_END" warning. -- AYM 1999-07-10 */
		if (ff_start && dir && ! y_strnicmp (dir->dir.name, "FF_END", WAD_NAME))
			if (dir->next && ! y_strnicmp (dir->next->dir.name, "F_END", WAD_NAME))
				dir = dir->next;

		verbmsg (" FF_START/%s %d", dir->dir.name, n);
		if (dir && ! y_strnicmp (dir->dir.name, "FF_END", WAD_NAME))
			warn ("this wad uses FF_END. That won't work with Doom."
					" Use F_END instead.\n");
		/* get the actual names from master dir. */
		flat_list = (flat_list_entry_t *) ResizeMemory (flat_list,
				(NumFTexture + n) * sizeof *flat_list);
		for (size_t m = NumFTexture; m < NumFTexture + n; dir0 = dir0->next)
		{
			// Skip all labels.
			if (dir0->dir.start == 0
					|| dir0->dir.size == 0
					|| (toupper (dir0->dir.name[0]) == 'F'
						&& (dir0->dir.name[1] == '1'
							|| dir0->dir.name[1] == '2'
							|| dir0->dir.name[1] == '3'
							|| toupper (dir0->dir.name[1]) == 'F')
						&& dir0->dir.name[2] == '_'
						&& (! y_strnicmp (dir0->dir.name + 3, "START", WAD_NAME - 3)
							|| ! y_strnicmp (dir0->dir.name + 3, "END", WAD_NAME - 3))
					   ))
				continue;
			*flat_list[m].name = '\0';
			strncat (flat_list[m].name, dir0->dir.name, sizeof flat_list[m].name - 1);
			flat_list[m].wadfile = dir0->wadfile;
			flat_list[m].offset  = dir0->dir.start;
			m++;
		}
		NumFTexture += n;
	}

	verbmsg ("\n");

	/* sort the flats by names */
	qsort (flat_list, NumFTexture, sizeof *flat_list, flat_list_entry_cmp);

	/* Eliminate all but the last duplicates of a flat. Suboptimal.
	   Would be smarter to start by the end. */
	for (size_t n = 0; n < NumFTexture; n++)
	{
		size_t m = n;
		while (m + 1 < NumFTexture
				&& ! flat_list_entry_cmp (flat_list + n, flat_list + m + 1))
			m++;
		// m now contains the index of the last duplicate
		int nduplicates = m - n;
		if (nduplicates > 0)
		{
			memmove (flat_list + n, flat_list + m,
					(NumFTexture - m) * sizeof *flat_list);
			NumFTexture -= nduplicates;
			// Note that I'm too lazy to resize flat_list...
		}
	}
}


/*
 *  is_flat_name_in_list
 *  FIXME should use bsearch()
 */
int is_flat_name_in_list (const char *name)
{
	if (! flat_list)
		return 0;
	for (size_t n = 0; n < NumFTexture; n++)
		if (! y_strnicmp (name, flat_list[n].name, WAD_FLAT_NAME))
			return 1;
	return 0;
}


/*
   forget the flat names
   */

void ForgetFTextureNames ()
{
	NumFTexture = 0;
	FreeMemory (flat_list);
	flat_list = 0;
}


/*
 *  update_level_bounds - update Map{Min,Max}{X,Y}
 */
void update_level_bounds ()
{
	MapMaxX = -32767;
	MapMaxY = -32767;
	MapMinX =  32767;
	MapMinY =  32767;

	for (obj_no_t n = 0; n < NumVertices; n++)
	{
		int x = Vertices[n]->x;
		int y = Vertices[n]->y;

		if (x < MapMinX) MapMinX = x;
		if (x > MapMaxX) MapMaxX = x;
		if (y < MapMinY) MapMinY = y;
		if (y > MapMaxY) MapMaxY = y;
	}
}


/*
 *  levelname2levelno
 *  Used to know if directory entry is ExMy or MAPxy
 *  For "ExMy" (case-insensitive),  returns 10x + y
 *  For "ExMyz" (case-insensitive), returns 100*x + 10y + z
 *  For "MAPxy" (case-insensitive), returns 1000 + 10x + y
 *  E0My, ExM0, E0Myz, ExM0z are not considered valid names.
 *  MAP00 is considered a valid name.
 *  For other names, returns 0.
 */
int levelname2levelno (const char *name)
{
	const unsigned char *s = (const unsigned char *) name;
	if (toupper (s[0]) == 'E'
			&& isdigit (s[1])
			&& s[1] != '0'
			&& toupper (s[2]) == 'M'
			&& isdigit (s[3])
			&& s[3] != '0'
			&& s[4] == '\0')
		return 10 * dectoi (s[1]) + dectoi (s[3]);
	if (yg_level_name == YGLN_E1M10
			&& toupper (s[0]) == 'E'
			&& isdigit (s[1])
			&& s[1] != '0'
			&& toupper (s[2]) == 'M'
			&& isdigit (s[3])
			&& s[3] != '0'
			&& isdigit (s[4])
			&& s[5] == '\0')
		return 100 * dectoi (s[1]) + 10 * dectoi (s[3]) + dectoi (s[4]);
	if (toupper (s[0]) == 'M'
			&& toupper (s[1]) == 'A'
			&& toupper (s[2]) == 'P'
			&& isdigit (s[3])
			&& isdigit (s[4])
			&& s[5] == '\0')
		return 1000 + 10 * dectoi (s[3]) + dectoi (s[4]);
	return 0;
}


/*
 *  levelname2rank
 *  Used to sort level names.
 *  Identical to levelname2levelno except that, for "ExMy",
 *  it returns 100x + y, so that
 *  - f("E1M10") = f("E1M9") + 1
 *  - f("E2M1")  > f("E1M99")
 *  - f("E2M1")  > f("E1M99") + 1
 *  - f("MAPxy") > f("ExMy")
 *  - f("MAPxy") > f("ExMyz")
 */
int levelname2rank (const char *name)
{
	const unsigned char *s = (const unsigned char *) name;
	if (toupper (s[0]) == 'E'
			&& isdigit (s[1])
			&& s[1] != '0'
			&& toupper (s[2]) == 'M'
			&& isdigit (s[3])
			&& s[3] != '0'
			&& s[4] == '\0')
		return 100 * dectoi (s[1]) + dectoi (s[3]);
	if (yg_level_name == YGLN_E1M10
			&& toupper (s[0]) == 'E'
			&& isdigit (s[1])
			&& s[1] != '0'
			&& toupper (s[2]) == 'M'
			&& isdigit (s[3])
			&& s[3] != '0'
			&& isdigit (s[4])
			&& s[5] == '\0')
		return 100 * dectoi (s[1]) + 10 * dectoi (s[3]) + dectoi (s[4]);
	if (toupper (s[0]) == 'M'
			&& toupper (s[1]) == 'A'
			&& toupper (s[2]) == 'P'
			&& isdigit (s[3])
			&& isdigit (s[4])
			&& s[5] == '\0')
		return 1000 + 10 * dectoi (s[3]) + dectoi (s[4]);
	return 0;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
