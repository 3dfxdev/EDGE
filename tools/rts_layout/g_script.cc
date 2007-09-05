//------------------------------------------------------------------------
//  SCRIPT Files (high-level)
//------------------------------------------------------------------------
//
//  RTS Layout Tool (C) 2007 Andrew Apted
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

#include "headers.h"

#include "lib_util.h"
#include "main.h"

#include "g_script.h"


const char *WhenAppear_TmpStr(int appear)
{
  static char buffer[200];
  
  buffer[0] = 0;

  if (appear & WNAP_Easy)   strcat(buffer, ":1:2");
  if (appear & WNAP_Medium) strcat(buffer, ":3");
  if (appear & WNAP_Hard)   strcat(buffer, ":4:5");

  if (appear & WNAP_SP)     strcat(buffer, ":sp");
  if (appear & WNAP_Coop)   strcat(buffer, ":coop");
  if (appear & WNAP_DM)     strcat(buffer, ":dm");

  SYS_ASSERT(strlen(buffer) >= 2);

  return buffer+1;
}


//------------------------------------------------------------------------
//  THING SPAWN Stuff
//------------------------------------------------------------------------

thing_spawn_c::thing_spawn_c( ) :
    ambush(false), has_z(false), type(),
    x(0), y(0), z(0), angle(0),
    tag(0), when_appear(0)
{ }

thing_spawn_c::~thing_spawn_c()
{ }


thing_spawn_c::thing_spawn_c * Parse(const char *line)
{
  // TODO
}

void thing_spawn_c::Write(FILE *fp)
{
  fprintf(fp, "        SPAWN_THING%s ", ambush ? "_AMBUSH" : "");

  fprintf(fp, "%s ", type.c_str());

  fprintf(fp, "%1.1f %1.1f ", x, y);

  if (angle >= 0)
    fprintf(fp, "%1.0f ", angle);

  if (has_z)
    fprintf(fp, "%1.1f ", z);

  if (tag > 0)
    fprintf(fp, "TAG=%d ", tag);

#if 0  // only available in EDGE 1.31 
  if (when_appear)
    fprintf(fp, "WHEN=%s ", WhenAppear_TmpStr(when_appear));
#endif

  fprintf(fp, "\n");
}


//------------------------------------------------------------------------
//  RADIUS TRIGGER Stuff
//------------------------------------------------------------------------


rad_trigger_c::rad_trigger_c( ) :
    is_rect(false), worldspawn(false),
    mx(0),  my(0),  mz(0),
    rx(-1), ry(-1), rz(-1),
    name(), tag(0), when_appear(0),
    lines(), things()
{ } 

rad_trigger_c::~rad_trigger_c()
{
  std::vector<thing_spawn_c *>::iterator TI;

  for (TI = things.begin(); TI != things.end(); TI++)
    if (*TI)
      delete (*TI);
} 


rad_trigger_c * rad_trigger_c::Create(const char *line)
{
  // TODO
}

rts_result_e rad_trigger_c::Parse(const char *line)
{
  // TODO
}

void rad_trigger_c::Write(FILE *fp)
{
  // TODO
}


//------------------------------------------------------------------------
//  SECTION and SCRIPT Stuff
//------------------------------------------------------------------------


section_c::section_c(int _kind) :
    kind(_kind), lines(), map_name(), pieces(), trig(NULL)
{ }

section_c::~section_c()
{
  std::vector<section_c *>::iterator PI;

  for (PI = pieces.begin(); PI != pieces.end(); PI++)
    if (*PI)
      delete (*PI);
}


script_c::script_c() : bits()
{ }

script_c::~script_c()
{
  std::vector<section_c *>::iterator BI;

  for (BI = bits.begin(); BI != bits.end(); BI++)
    if (*BI)
      delete (*BI);
}
 

script_c *script_c::Load(FILE *fp)
{
  // TODO
}

void script_c::Save(FILE *fp)
{
  // TODO
}


//--- editor settings ---
// vi:ts=2:sw=2:expandtab
