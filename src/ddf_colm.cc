//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (Colourmaps)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2004  The EDGE Team.
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
//----------------------------------------------------------------------------
//
// Colourmap handling.
//

#include "i_defs.h"

#include "dm_defs.h"
#include "dm_type.h"
#include "ddf_locl.h"
#include "m_random.h"
#include "z_zone.h"

#undef  DF
#define DF  DDF_CMD

static colourmap_t buffer_colmap;
static colourmap_t *dynamic_colmap;

static const colourmap_t template_colmap =
{
  DDF_BASE_NIL,  // ddf
  "",            // lump name
  0,             // start
  1,             // length
  (colourspecial_e)0,             // special
  {NULL, NULL, -1, 0, 0xFFFFFF}  // cache
};

void DDF_ColmapGetSpecial(const char *info, void *storage);

#define DDF_CMD_BASE  buffer_colmap

static const commandlist_t colmap_commands[] =
{
  DF("LUMP",    lump_name, DDF_MainGetInlineStr10),
  DF("START",   start,     DDF_MainGetNumeric),
  DF("LENGTH",  length,    DDF_MainGetNumeric),
  DF("SPECIAL", special,   DDF_ColmapGetSpecial),

  // -AJA- backwards compatibility cruft...
  DF("!PRIORITY", ddf, DDF_DummyFunction),

  DDF_CMD_END
};

colourmap_t ** ddf_colmaps;
int num_ddf_colmaps = 0;
int num_disabled_colmaps = 0;

static stack_array_t ddf_colmaps_a;


//
//  DDF PARSE ROUTINES
//

static bool ColmapStartEntry(const char *name)
{
  int i;
  bool replaces = false;

  if (name && name[0])
  {
    for (i=num_disabled_colmaps; i < num_ddf_colmaps; i++)
    {
      if (DDF_CompareName(ddf_colmaps[i]->ddf.name, name) == 0)
      {
        dynamic_colmap = ddf_colmaps[i];
        replaces = true;
        break;
      }
    }
    
    // if found, adjust pointer array to keep newest entries at end
    if (replaces && i < (num_ddf_colmaps-1))
    {
      Z_MoveData(ddf_colmaps + i, ddf_colmaps + i + 1, colourmap_t *,
          num_ddf_colmaps - i);
      ddf_colmaps[num_ddf_colmaps - 1] = dynamic_colmap;
    }
  }

  // not found, create a new one
  if (! replaces)
  {
    Z_SetArraySize(&ddf_colmaps_a, ++num_ddf_colmaps);

    dynamic_colmap = ddf_colmaps[num_ddf_colmaps-1];
    dynamic_colmap->ddf.name = (name && name[0]) ? Z_StrDup(name) :
        DDF_MainCreateUniqueName("UNNAMED_COLMAP", num_ddf_colmaps);
  }

  dynamic_colmap->ddf.number = 0;

  // instantiate the static entry
  buffer_colmap = template_colmap;

  return replaces;
}

static void ColmapParseField(const char *field, const char *contents,
    int index, bool is_last)
{
#if (DEBUG_DDF)  
  L_WriteDebug("COLMAP_PARSE: %s = %s;\n", field, contents);
#endif

  if (DDF_MainParseField(colmap_commands, field, contents))
    return;

  // handle properties
  if (index == 0 && DDF_CompareName(contents, "TRUE") == 0)
  {
    DDF_ColmapGetSpecial(field, NULL);  // FIXME FOR OFFSETS
    return;
  }

  DDF_WarnError("Unknown colmap.ddf command: %s\n", field);
}

static void ColmapFinishEntry(void)
{
  ddf_base_t base;
  
  if (buffer_colmap.start < 0)
  {
    DDF_WarnError("Bad START value for colmap: %d\n", buffer_colmap.start);
    buffer_colmap.start = 0;
  }
  
  if (buffer_colmap.length <= 0)
  {
    DDF_WarnError("Bad LENGTH value for colmap: %d\n", buffer_colmap.length);
    buffer_colmap.length = 1;
  }
  
  if (!buffer_colmap.lump_name || !buffer_colmap.lump_name[0])
    DDF_Error("Missing LUMP name for colmap.\n");

  // transfer static entry to dynamic entry
  
  base = dynamic_colmap->ddf;
  dynamic_colmap[0] = buffer_colmap;
  dynamic_colmap->ddf = base;

  // Compute CRC.  In this case, there is no need, since colourmaps
  // only affect rendering, they have zero effect on the game
  // simulation itself.
  dynamic_colmap->ddf.crc = 0;
}

static void ColmapClearAll(void)
{
  // not safe to delete colourmaps -- disable them

  num_disabled_colmaps = num_ddf_colmaps;
}


void DDF_ReadColourMaps(void *data, int size)
{
  readinfo_t colm_r;

  colm_r.memfile = (char*)data;
  colm_r.memsize = size;
  colm_r.tag = "COLOURMAPS";
  colm_r.entries_per_dot = 2;

  if (colm_r.memfile)
  {
    colm_r.message = NULL;
    colm_r.filename = NULL;
    colm_r.lumpname = "DDFCOLM";
  }
  else
  {
    colm_r.message = "DDF_InitColourMaps";
    colm_r.filename = "colmap.ddf";
    colm_r.lumpname = NULL;
  }

  colm_r.start_entry  = ColmapStartEntry;
  colm_r.parse_field  = ColmapParseField;
  colm_r.finish_entry = ColmapFinishEntry;
  colm_r.clear_all    = ColmapClearAll;

  DDF_MainReadFile(&colm_r);
}

void DDF_ColmapInit(void)
{
  Z_InitStackArray(&ddf_colmaps_a, (void ***)&ddf_colmaps, sizeof(colourmap_t), 0);
}

void DDF_ColmapCleanUp(void)
{
  /* nothing to do */
}

const colourmap_t *DDF_ColmapLookup(const char *name)
{
  int i;

  for (i = num_disabled_colmaps; i < num_ddf_colmaps; i++)
  {
    if (DDF_CompareName(ddf_colmaps[i]->ddf.name, name) == 0)
      return ddf_colmaps[i];
  }

  if (lax_errors)
    return ddf_colmaps[0];

  DDF_Error("DDF_ColmapLookup: No such colourmap '%s'\n", name);
  return NULL;
}

specflags_t colmap_specials[] =
{
    {"FLASH",  COLSP_NoFlash,  1},

    // -AJA- backwards compatibility cruft...
    {"!SKY",   0, 0},
    {NULL, 0, 0}
};

//
// DDF_ColmapGetSpecial
//
// Gets the colourmap specials.
//
void DDF_ColmapGetSpecial(const char *info, void *storage)
{
  int flag_value;

  switch (DDF_MainCheckSpecialFlag(info, colmap_specials, &flag_value,
      true, false))
  {
    case CHKF_Positive:
      buffer_colmap.special = (colourspecial_e)(buffer_colmap.special | flag_value);
      break;
    
    case CHKF_Negative:
      buffer_colmap.special = (colourspecial_e)(buffer_colmap.special & ~flag_value);
      break;

    case CHKF_User:
    case CHKF_Unknown:
      DDF_WarnError("DDF_ColmapGetSpecial: Unknown Special: %s", info);
      break;
  }
}

