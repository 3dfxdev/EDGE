//----------------------------------------------------------------------------
//  EDGE Data Definition File Codes (Switch textures)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2001  The EDGE Team.
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
// Switch Texture Setup and Parser Code
//

#include "i_defs.h"

#include "ddf_locl.h"
#include "ddf_main.h"
#include "dm_state.h"
#include "p_local.h"
#include "p_spec.h"
#include "z_zone.h"

#undef  DF
#define DF  DDF_CMD

static switchlist_t buffer_switch;
static switchlist_t *dynamic_switch;

static const switchlist_t template_switch =
{
  DDF_BASE_NIL,  // ddf
  "",    // name1;
  "",    // name2;
  sfx_None,  // on_sfx;
  sfx_None,  // off_sfx;
  BUTTONTIME, // time
  {{0,0}}  // cache
};

switchlist_t ** alph_switches = NULL;
int num_alph_switches = 0;

static stack_array_t alph_switches_a;

#undef  DDF_CMD_BASE
#define DDF_CMD_BASE  buffer_switch

static const commandlist_t switch_commands[] =
{
  DF("ON TEXTURE",  name1, DDF_MainGetInlineStr10),
  DF("OFF TEXTURE", name2, DDF_MainGetInlineStr10),
  DF("ON SOUND",  on_sfx,  DDF_MainLookupSound),
  DF("OFF SOUND", off_sfx, DDF_MainLookupSound),
  DF("TIME", time, DDF_MainGetTime),

  // -AJA- backwards compatibility cruft...
  DF("!SOUND", on_sfx, DDF_MainLookupSound),

  DDF_CMD_END
};


//
//  DDF PARSE ROUTINES
//

static boolean_t SwitchStartEntry(const char *name)
{
  int i;
  boolean_t replaces = false;

  if (name && name[0])
  {
    for (i=0; i < num_alph_switches; i++)
    {
      if (DDF_CompareName(alph_switches[i]->ddf.name, name) == 0)
      {
        dynamic_switch = alph_switches[i];
        replaces = true;
        break;
      }
    }
    
    // if found, adjust pointer array to keep newest entries at end
    if (replaces && i < (num_alph_switches-1))
    {
      Z_MoveData(alph_switches + i, alph_switches + i + 1, switchlist_t *,
          num_alph_switches - i);
      alph_switches[num_alph_switches - 1] = dynamic_switch;
    }
  }

  // not found, create a new one
  if (! replaces)
  {
    Z_SetArraySize(&alph_switches_a, ++num_alph_switches);
    
    dynamic_switch = alph_switches[num_alph_switches - 1];
    dynamic_switch->ddf.name = (name && name[0]) ? Z_StrDup(name) :
        DDF_MainCreateUniqueName("UNNAMED_SWITCH", num_alph_switches);
  }

  dynamic_switch->ddf.number = 0;

  // instantiate the static entry
  buffer_switch = template_switch;

  return replaces;
}

static void SwitchParseField(const char *field, const char *contents,
    int index, boolean_t is_last)
{
#if (DEBUG_DDF)  
  L_WriteDebug("SWITCH_PARSE: %s = %s;\n", field, contents);
#endif

  if (! DDF_MainParseField(switch_commands, field, contents))
    DDF_WarnError("Unknown switch.ddf command: %s\n", field);
}

static void SwitchFinishEntry(void)
{
  ddf_base_t base;
  
  if (!buffer_switch.name1[0])
    DDF_Error("Missing first name for switch.\n");

  if (!buffer_switch.name2[0])
    DDF_Error("Missing last name for switch.\n");
 
  if (buffer_switch.time <= 0)
    DDF_Error("Bad time value for switch: %d\n", buffer_switch.time);
   
  // transfer static entry to dynamic entry
  
  base = dynamic_switch->ddf;
  dynamic_switch[0] = buffer_switch;
  dynamic_switch->ddf = base;

  // Compute CRC.  In this case, there is no need, since switch
  // textures have zero impact on the game simulation.
  dynamic_switch->ddf.crc = 0;
}

static void SwitchClearAll(void)
{
  // safe here to delete all switches

  num_alph_switches = 0;
  Z_SetArraySize(&alph_switches_a, num_alph_switches);
}


void DDF_ReadSW(void *data, int size)
{
#if (DEBUG_DDF)
  int i;
#endif

  readinfo_t switches;

  switches.memfile = (char*)data;
  switches.memsize = size;
  switches.tag = "SWITCHES";
  switches.entries_per_dot = 2;

  if (switches.memfile)
  {
    switches.message = NULL;
    switches.filename = NULL;
    switches.lumpname = "DDFSWTH";
  }
  else
  {
    switches.message = "DDF_InitSwitches";
    switches.filename = "switch.ddf";
    switches.lumpname = NULL;
  }

  switches.start_entry  = SwitchStartEntry;
  switches.parse_field  = SwitchParseField;
  switches.finish_entry = SwitchFinishEntry;
  switches.clear_all    = SwitchClearAll;

  DDF_MainReadFile(&switches);

#if (DEBUG_DDF)
  L_WriteDebug("DDF_ReadSW: Switch List:\n");

  for (i = 0; i < num_alph_switches; i++)
  {
    L_WriteDebug("  Num: %d  ON: '%s'  OFF: '%s'\n",
        i, alph_switches[i]->name1, alph_switches[i]->name2);
  }
#endif
}

void DDF_SWInit(void)
{
  Z_InitStackArray(&alph_switches_a, (void ***)&alph_switches,
      sizeof(switchlist_t), 0);
}

void DDF_SWCleanUp(void)
{
  // nothing to do
}

