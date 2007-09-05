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


const char *Indent_TmpStr(int spaces)
{
  static char buffer[100];

  SYS_ASSERT(spaces >= 0);
  SYS_ASSERT(spaces < (int)sizeof(buffer) - 4);

  memset(buffer, ' ', spaces);

  buffer[spaces] = 0;

  return buffer;
}

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

int WhenAppear_Parse(const char *info)
{
  int result = 0;

  const char *range = strstr(info, "-");

  if (range)
  {
    char low  = (range > info) ? range[-1] : 0;
    char high = (range[+1])    ? range[+1] : 0;

    if (low < '1' || low > '5' || high < '1' || high > '5' || low > high)
    {
      LogPrintf("Bad range in WHEN_APPEAR value: %s\n", info);
    }
    else
    {
      for (; low <= high; low++)
      {
        if (low <= '2') result |= WNAP_Easy;
        if (low == '3') result |= WNAP_Medium;
        if (low >= '4') result |= WNAP_Hard;
      }
    }
  }
  else /* no range */
  {
    if (strstr(info, "1")) result |= WNAP_Easy;
    if (strstr(info, "2")) result |= WNAP_Easy;
    if (strstr(info, "3")) result |= WNAP_Medium;
    if (strstr(info, "4")) result |= WNAP_Hard;
    if (strstr(info, "5")) result |= WNAP_Hard;
  }

  if (strstr(info, "SP") || strstr(info, "sp"))
    result |= WNAP_SP;

  if (strstr(info, "COOP") || strstr(info, "coop"))
    result |= WNAP_Coop;

  if (strstr(info, "DM") || strstr(info, "dm"))
    result |= WNAP_DM;

  // allow more human readable strings...

  if (info[0] == '!')
    result ^= WNAP_SkillBits | WNAP_ModeBits;

  if ((result & WNAP_SkillBits) == 0)
    result |= WNAP_SkillBits;

  if ((result & WNAP_ModeBits) == 0)
    result |= WNAP_ModeBits;

  return result;
}

static rts_result_e ReadLine(FILE *fp, std::string& line)
{
  line = "";

  for (;;)
  {
    int ch = fgetc(fp);

    if (ch == EOF)
      return (line.size() == 0) ? RTS_FINISHED : RTS_OK;

    if (ferror(fp))
      return RTS_ERROR;

    line += (char) ch;
  }
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


void thing_spawn_c::WriteThing(FILE *fp)
{
  fprintf(fp, "    spawn_thing%s", ambush ? "_ambush" : "");

  fprintf(fp, " %s", type.c_str());

  fprintf(fp, " %1.1f %1.1f", x, y);

  if (angle != 0 || has_z)
    fprintf(fp, " %1.0f", angle);

  if (has_z)
    fprintf(fp, " %1.1f", z);

  if (tag != 0)
    fprintf(fp, " TAG=%d", tag);

#if 0  // only available in EDGE 1.31 
  if (when_appear != 0)
    fprintf(fp, " WHEN=%s", WhenAppear_TmpStr(when_appear));
#endif

  fprintf(fp, "\n");
}

thing_spawn_c * thing_spawn_c::ReadThing(std::string& line)
{
  // TODO
}


//------------------------------------------------------------------------
//  RADIUS TRIGGER Stuff
//------------------------------------------------------------------------

rad_trigger_c::rad_trigger_c(bool _rect) :
    is_rect(_rect), worldspawn(false),
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


void rad_trigger_c::WriteRadTrig(FILE *fp)
{
  // write the start marker
  if (is_rect)
  {
    fprintf(fp, "RECT_TRIGGER %1.1f %1.1f %1.1f %1.1f",
            mx-rx, my-ry, mx+rx, my+ry);
  }
  else
  {
    fprintf(fp, "RADIUS_TRIGGER %1.1f %1.1f %1.1f", mx, my, rx);
  }

  if (rz > 0)
    fprintf(fp, "  %1.1f %1.1f", mz-rz, mz+rz);

  fprintf(fp, "\n");

  // write out each property we explicitly handle
  if (when_appear != 0)
    fprintf(fp, "    when_appear %s\n", WhenAppear_TmpStr(when_appear));

  if (name.size() > 0)
    fprintf(fp, "    name %s\n", name.c_str());

  if (tag != 0)
    fprintf(fp, "    tag %d\n", tag);

  // write the 'body' of the script
  if (worldspawn)
  {
    fprintf(fp, "    tagged_immediate\n");

    std::vector<std::string>::iterator LI;

    for (LI = lines.begin(); LI != lines.end(); LI++)
      fprintf(fp, "%s\n", (*LI).c_str());
  }
  else
  {
    std::vector<thing_spawn_c *>::iterator TI;

    for (TI = things.begin(); TI != things.end(); TI++)
      if (*TI)
        (*TI)->WriteThing(fp);
  }

  // that's all folks!
  fprintf(fp, "END_RADIUSTRIGGER\n");
}

bool rad_trigger_c::MatchRadTrig(std::string& line)
{
  return false; // TODO !!!
}

rad_trigger_c * rad_trigger_c::ReadRadTrig(FILE *fp, std::string& first)
{
  // TODO !!!
}


//------------------------------------------------------------------------
//  SECTION gunk
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

void section_c::WriteSection(FILE *fp)
{
  switch (kind)
  {
    case TEXT:
      WriteText(fp);
      break;

    case START_MAP:
      WriteStartMap(fp);
      break;

    case RAD_TRIG:
      trig->WriteRadTrig(fp);
      break;
  };
}

void section_c::WriteText(FILE *fp)
{
  std::vector<std::string>::iterator LI;

  for (LI = lines.begin(); LI != lines.end(); LI++)
    fprintf(fp, "%s\n", (*LI).c_str());
}

void section_c::WriteStartMap(FILE *fp)
{
  fprintf(fp, "START_MAP %s\n", map_name.c_str());
 
  std::vector<section_c *>::iterator PI;

  for (PI = pieces.begin(); PI != pieces.end(); PI++)
    if (*PI)
      (*PI)->WriteSection(fp);

  fprintf(fp, "END_MAP\n"); 
}

void section_c::AddLine(std::string& line)
{
  lines.push_back(line);
}

bool section_c::MatchStartMap(std::string& line)
{
  return false; // TODO !!!
}

section_c * section_c::ReadStartMap(FILE *fp, std::string& first)
{
  // TODO !!!
}


//------------------------------------------------------------------------
//  SCRIPT Stuff
//------------------------------------------------------------------------

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
  script_c *result = new script_c();

  section_c *cur_section = NULL;
 
  for (;;)
  {
    std::string line;
    
    rts_result_e res = ReadLine(fp, line);

    if (res == RTS_ERROR)
    {
      delete result;
      return NULL;
    }

    if (res != RTS_OK)
      break;

    if (section_c::MatchStartMap(line))
    {
      cur_section = section_c::ReadStartMap(fp, line);
      if (! cur_section)
      {
        delete result;
        return NULL;
      }

      result->bits.push_back(cur_section);

      cur_section = NULL;
      continue;
    }

    if (! cur_section)
    {
      cur_section = new section_c(section_c::TEXT);

      result->bits.push_back(cur_section);
    }

    cur_section->AddLine(line);
  }

  return result;
}

void script_c::Save(FILE *fp)
{
  std::vector<section_c *>::iterator BI;

  for (BI = bits.begin(); BI != bits.end(); BI++)
    if (*BI)
      (*BI)->WriteSection(fp);
}


//--- editor settings ---
// vi:ts=2:sw=2:expandtab
