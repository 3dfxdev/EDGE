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

// TODO: keep track of line numbers (for errors)

#include "headers.h"

#include "lib_util.h"
#include "main.h"

#include "g_script.h"


const char *Float_TmpStr(float val, int precision = 2)
{
  // produces a 'Human Readable' float output, which removes
  // trailing zeros for a nicer result.  For example: "123.00"
  // becomes "123".

  static char buffer[200];

  sprintf(buffer, "%1.*f", precision, val);

  if (! strchr(buffer, '.'))
    return buffer;

  char *pos = buffer + strlen(buffer) - 1;

  while (*pos == '0')
    *pos-- = 0;

  if (*pos == '.')
    *pos-- = 0;
      
  SYS_ASSERT(isdigit(*pos));

  return buffer;
}

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

  if (appear & WNAP_Hard)   strcat(buffer, ":4:5");
  if (appear & WNAP_Medium) strcat(buffer, ":3");
  if (appear & WNAP_Easy)   strcat(buffer, ":1:2");

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
    {
      // DIALOG "file read error: " + strerror(errno)
      return RTS_ERROR;
    }

    if (ch == '\n')
      return RTS_OK;

    if (ch == '\r')  // ignore CR within CR-LF pairs
      continue;

    line += (char) ch;
  }
}

static inline const char *skip_space(const char *X)
{
  while (*X && isspace(*X))
    X++;

  return X;
}

static inline const char *skip_word(const char *X)
{
  while (*X && ! isspace(*X))
    X++;

  return X;
}

static bool DDF_CompareWord(const char *pos, const char *word)
{
  // check whether the given 'word' matches at the current
  // string position 'pos'.  This is different that str(i)cmp
  // because we allow whitespace to follow the word.  We also
  // allow '_' in the word to be absent in the string.
  //
  // The comparison is NOT case-sensitive.

  for (;;)
  {
    bool pos_end = (! *pos) || isspace(*pos);

    if (! *word)
      return pos_end;

    if (pos_end)
      return false;  // not long enough

    if (toupper(*pos) == toupper(*word))
    {
      pos++; word++; continue;
    }

    if (*word == '_')
    {
      word++; continue;
    }

    return false;  // does not match
  }
}

static void SplitLine(const char *pos, std::vector<std::string> & words)
{
  words.resize(0);

  for (;;)
  {
    pos = skip_space(pos);

    if (! *pos)
      break;

    const char *pos2 = skip_word(pos);

    words.push_back(std::string(pos, (int)(pos2 - pos)));

    pos = pos2;
  }
}


//------------------------------------------------------------------------
//  THING SPAWN Stuff
//------------------------------------------------------------------------

thing_spawn_c::thing_spawn_c(bool _ambush) :
    ambush(_ambush ? 1 : 0), type(),
    x(0), y(0), z(FLOAT_UNSPEC), angle(FLOAT_UNSPEC),
    tag(INT_UNSPEC), when_appear(INT_UNSPEC),
    ddf_info(NULL)
{ }

thing_spawn_c::~thing_spawn_c()
{ }


void thing_spawn_c::WriteThing(FILE *fp)
{
  fprintf(fp, "    spawn_thing%s", ambush ? "_ambush" : "");

  fprintf(fp, " %s", type.c_str());

  fprintf(fp, " %s", Float_TmpStr(x));
  fprintf(fp, " %s", Float_TmpStr(y));

  if (angle != FLOAT_UNSPEC)
    fprintf(fp, " %s", Float_TmpStr(angle));

  if (z != FLOAT_UNSPEC)
  {
    if (angle == FLOAT_UNSPEC)
      fprintf(fp, " 0");

    fprintf(fp, " %s", Float_TmpStr(z));
  }

  if (tag != INT_UNSPEC)
    fprintf(fp, " TAG=%d", tag);

  // NOTE: EDGE 1.31 functionality
  if (when_appear != INT_UNSPEC)
    fprintf(fp, " WHEN=%s", WhenAppear_TmpStr(when_appear));

  fprintf(fp, "\n");
}

bool thing_spawn_c::MatchThing(std::string& line)
{
  const char *pos = line.c_str();

  pos = skip_space(pos);

  return DDF_CompareWord(pos, "spawn_thing") ||
         DDF_CompareWord(pos, "spawn_thing_ambush");
}

thing_spawn_c * thing_spawn_c::ReadThing(std::string& line)
{
  const char *pos = line.c_str();

  std::vector<std::string> words;

  SplitLine(pos, words);

  if (words.size() < 2)
  {
    // DIALOG "Malformed SPAWN_THING command"
    return NULL;
  }

  bool is_ambush = DDF_CompareWord(words[0].c_str(), "spawn_thing_ambush");

  thing_spawn_c *TH = new thing_spawn_c(is_ambush);

  TH->type = words[1];

  // handle keyword parameters

  while (words.size() >= 3)
  {
    if (! strchr(words.back().c_str(), '='))
      break;

    TH->ParseKeyword(words.back());

    words.pop_back();
  }

  if (words.size() >= 4)
  {
    TH->x = atof(words[2].c_str());
    TH->y = atof(words[3].c_str());
  }

  if (words.size() >= 5)
  {
    TH->angle = atof(words[4].c_str());
  }

  if (words.size() >= 6)
  {
    TH->z = atof(words[5].c_str());
  }

  return TH;
}

rts_result_e thing_spawn_c::ParseKeyword(std::string& word)
{
  const char *pos = word.c_str();
  const char *arg = strchr(pos, '=');

  SYS_ASSERT(arg);

  if (StrCaseCmpPartial(pos, "X=") == 0)
  {
    x = atof(arg+1);
    return RTS_OK;
  }
  if (StrCaseCmpPartial(pos, "Y=") == 0)
  {
    y = atof(arg+1);
    return RTS_OK;
  }
  if (StrCaseCmpPartial(pos, "Z=") == 0)
  {
    z = atof(arg+1);
    return RTS_OK;
  }

  if (StrCaseCmpPartial(pos, "ANGLE=") == 0)
  {
    angle = atof(arg+1);
    return RTS_OK;
  }

  if (StrCaseCmpPartial(pos, "TAG=") == 0)
  {
    tag = atoi(arg+1);
    return RTS_OK;
  }

  // Note: EDGE 1.31 functionality
  if (StrCaseCmpPartial(pos, "WHEN=") == 0)
  {
    when_appear = WhenAppear_Parse(arg+1);
    return RTS_OK;
  }

  LogPrintf("Unknown SPAWN_THING keyword: %s\n", pos);
  return RTS_ERROR;
}


//------------------------------------------------------------------------
//  RADIUS TRIGGER Stuff
//------------------------------------------------------------------------

rad_trigger_c::rad_trigger_c(bool _rect) :
    is_rect(_rect ? 1 : 0),
    mx(0),  my(0),  mz(FLOAT_UNSPEC),
    rx(-1), ry(-1), rz(FLOAT_UNSPEC),
    name(), tag(INT_UNSPEC), when_appear(INT_UNSPEC),
    lines(), worldspawn(false), things()
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
    fprintf(fp, "RECT_TRIGGER");

    fprintf(fp, " %s", Float_TmpStr(mx-rx));
    fprintf(fp, " %s", Float_TmpStr(my-ry));
    fprintf(fp, " %s", Float_TmpStr(mx+rx));
    fprintf(fp, " %s", Float_TmpStr(my+ry));
  }
  else
  {
    fprintf(fp, "RADIUS_TRIGGER");
        
    fprintf(fp, " %s", Float_TmpStr(mx));
    fprintf(fp, " %s", Float_TmpStr(my));
    fprintf(fp, " %s", Float_TmpStr(rx));
  }

  if (mz != FLOAT_UNSPEC && rz != FLOAT_UNSPEC && rz > 0)
  {
    fprintf(fp, "  %s", Float_TmpStr(mz-rz));
    fprintf(fp, "  %s", Float_TmpStr(mz+rz));
  }

  fprintf(fp, "\n");

  // write out each property we explicitly handle
  if (when_appear != INT_UNSPEC)
    fprintf(fp, "    when_appear %s\n", WhenAppear_TmpStr(when_appear));

  if (name.size() > 0)
    fprintf(fp, "    name %s\n", name.c_str());

  if (tag != INT_UNSPEC)
    fprintf(fp, "    tag %d\n", tag);

  // write the 'body' of the script
  if (worldspawn)
  {
    fprintf(fp, "    tagged_immediate\n");

    std::vector<thing_spawn_c *>::iterator TI;

    for (TI = things.begin(); TI != things.end(); TI++)
      if (*TI)
        (*TI)->WriteThing(fp);
  }
  else
  {
    std::vector<std::string>::iterator LI;

    for (LI = lines.begin(); LI != lines.end(); LI++)
      fprintf(fp, "%s\n", (*LI).c_str());
  }

  // that's all folks!
  fprintf(fp, "END_RADIUS_TRIGGER\n");
}

bool rad_trigger_c::MatchRadTrig(std::string& line)
{
  const char *pos = line.c_str();

  pos = skip_space(pos);

  return DDF_CompareWord(pos, "RADIUS_TRIGGER") ||
         DDF_CompareWord(pos, "RECT_TRIGGER");
}

bool rad_trigger_c::MatchEndTrig(std::string& line)
{
  const char *pos = line.c_str();

  pos = skip_space(pos);

  return DDF_CompareWord(pos, "END_RADIUS_TRIGGER");
}

rad_trigger_c * rad_trigger_c::ReadRadTrig(FILE *fp, std::string& first)
{
  const char *pos = first.c_str();

  pos = skip_space(pos);

  bool is_rect = DDF_CompareWord(pos, "RECT_TRIGGER");

  pos = skip_word(pos);

  rad_trigger_c *TRIG = new rad_trigger_c(is_rect);

  rts_result_e res = TRIG->ParseLocation(pos);

  if (res == RTS_OK)
  {
    res = TRIG->ParseBody(fp);
  }

  if (res == RTS_ERROR)
  {
    delete TRIG; return NULL;
  }

  return TRIG;
}

rts_result_e rad_trigger_c::ParseLocation(const char *pos)
{
  pos = skip_space(pos);

  if (is_rect)
  {
    float x1, y1, z1;
    float x2, y2, z2;

    int num = sscanf(pos, " %f %f %f %f %f %f ",
                     &x1, &y1, &x2, &y2, &z1, &z2);

    if (num < 4 || num == 5)
    {
      // DIALOG "bad position after RECT_TRIGGER"
      return RTS_ERROR;
    }

    mx = (x1 + x2) / 2; rx = fabs(x2 - x1) / 2;
    my = (y1 + y2) / 2; ry = fabs(y2 - y1) / 2;

    if (num > 4)
    {
      mz = (z1 + z2) / 2; rz = fabs(z2 - z1) / 2;
    }
  }
  else /* radius */
  {
    float z1, z2;

    int num = sscanf(pos, " %f %f %f %f %f ",
                     &mx, &my, &rx, &z1, &z2);

    if (num < 3 || num == 4)
    {
      // DIALOG "bad position after RADIUS_TRIGGER"
      return RTS_ERROR;
    }

    ry = rx;

    if (num > 3)
    {
      mz = (z1 + z2) / 2; rz = fabs(z2 - z1) / 2;
    }
  }

  return RTS_OK;
}

rts_result_e rad_trigger_c::ParseBody(FILE *fp)
{
  for (;;)
  {
    std::string line;
    
    rts_result_e res = ReadLine(fp, line);

    if (res == RTS_ERROR)
      return RTS_ERROR;

    if (res == RTS_EOF)
    {
      // DIALOG "Missing rest of trigger (EOF found)"
      return RTS_ERROR;
    }

    if (MatchEndTrig(line))
      break;  // all done

    res = ParseCommand(line);

    if (res == RTS_ERROR)
      return RTS_ERROR;
  }

  return RTS_OK;
}

rts_result_e rad_trigger_c::ParseCommand(std::string& line)
{
  const char *pos = line.c_str();

  pos = skip_space(pos);

  const char *args = skip_space(skip_word(pos));


  if (DDF_CompareWord(pos, "name"))
    return cmd_Name(args);

  if (DDF_CompareWord(pos, "tag"))
    return cmd_Tag(args);

  if (DDF_CompareWord(pos, "when_appear"))
    return cmd_WhenAppear(args);

  if (! worldspawn)
  {
    // ordinary command
    lines.push_back(line);
    return RTS_OK;
  }

  if (DDF_CompareWord(pos, "tagged_immediate"))
    return RTS_OK;  // ignore it

  if (thing_spawn_c::MatchThing(line))
  {
    thing_spawn_c * th = thing_spawn_c::ReadThing(line);

    if (! th)
      return RTS_ERROR;

    things.push_back(th);
    return RTS_OK;
  }

  LogPrintf("Ignoring extra command in worldspawn: %s\n", pos);
  return RTS_OK;
}

rts_result_e rad_trigger_c::cmd_Name(const char *args)
{
  const char *arg_end = skip_word(args);

  if (arg_end == args)  // FIXME: validate name
  {
    // DIALOG "bad or missing NAME in radius trigger"
    return RTS_ERROR;
  }

  name = std::string(args, (int)(arg_end - args));

  const char *name_c = name.c_str();

  if (strncmp(name_c, "worldspawn", 10) == 0 && isdigit(name_c[10]))
  {
    worldspawn = true;

    int kind = atoi(name_c+10);

    when_appear  = 0;
    when_appear |= (kind & 1) ? WNAP_Easy   : 0;
    when_appear |= (kind & 2) ? WNAP_Medium : 0;
    when_appear |= (kind & 4) ? WNAP_Hard   : 0;
  }

  return RTS_OK;
}

rts_result_e rad_trigger_c::cmd_Tag(const char *args)
{
  tag = atoi(args);

  if (tag <= 0)
  {
    // DIALOG "bad or missing TAG in radius trigger"
    tag = INT_UNSPEC;
    return RTS_ERROR;
  }

  return RTS_OK;
}

rts_result_e rad_trigger_c::cmd_WhenAppear(const char *args)
{
  if (worldspawn) // when_appear already calculated from the name
    return RTS_OK;
  
  const char *arg_end = skip_word(args);

  if (arg_end == args)
  {
    // DIALOG "Missing WHEN_APPEAR in radius trigger"
    return RTS_ERROR;
  }

  std::string temp_val(args, (int)(arg_end - args));

  when_appear = WhenAppear_Parse(temp_val.c_str());

  return RTS_OK;
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
  const char *pos = line.c_str();

  pos = skip_space(pos);

  return DDF_CompareWord(pos, "START_MAP");
}

bool section_c::MatchEndMap(std::string& line)
{
  const char *pos = line.c_str();

  pos = skip_space(pos);

  return DDF_CompareWord(pos, "END_MAP");
}

section_c * section_c::ReadStartMap(FILE *fp, std::string& first)
{
  const char *pos = first.c_str();

  pos = skip_space(pos);
  pos = skip_word(pos);

  section_c *MAP = new section_c(START_MAP);

  rts_result_e res = MAP->ParseMapName(pos);

  if (res == RTS_OK)
  {
    // invoke parsing code for all the contents
    res = MAP->ParsePieces(fp);
  }

  if (res == RTS_ERROR)
  {
    delete MAP; return NULL;
  }

  return MAP;
}

rts_result_e section_c::ParseMapName(const char *pos)
{
  pos = skip_space(pos);

  if (! *pos)
  {
    // DIALOG "missing map name after START_MAP"
    return RTS_ERROR;
  }

  int length = (int) (skip_word(pos) - pos);

  // TODO: verify map name is OK

  map_name = std::string(pos, length);

  return RTS_OK;
}

rts_result_e section_c::ParsePieces(FILE *fp)
{
  section_c *cur_piece = NULL;
 
  for (;;)
  {
    std::string line;
    
    rts_result_e res = ReadLine(fp, line);

    if (res == RTS_ERROR)
      return RTS_ERROR;

    if (res == RTS_FINISHED)
    {
      // DIALOG "Missing rest of trigger (EOF found)"
      return RTS_ERROR;
    }

    if (MatchEndMap(line))
      break;  // all done

    if (rad_trigger_c::MatchRadTrig(line))
    {
      rad_trigger_c * trig = rad_trigger_c::ReadRadTrig(fp, line);

      if (! trig)
        return RTS_ERROR;

      cur_piece = new section_c(RAD_TRIG);
      cur_piece->trig = trig;

      pieces.push_back(cur_piece);

      cur_piece = NULL;
      continue;
    }

    if (! cur_piece)
    {
      cur_piece = new section_c(section_c::TEXT);

      pieces.push_back(cur_piece);
    }

    cur_piece->AddLine(line);
  }

  return RTS_OK;
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
  script_c *SCR = new script_c();

  section_c *cur_bit = NULL;
 
  for (;;)
  {
    std::string line;
    
    rts_result_e res = ReadLine(fp, line);

    if (res == RTS_ERROR)
    {
      delete SCR;
      return NULL;
    }

    if (res == RTS_FINISHED)
      break;

    if (section_c::MatchStartMap(line))
    {
      cur_bit = section_c::ReadStartMap(fp, line);
      if (! cur_bit)
      {
        delete SCR;
        return NULL;
      }

      SCR->bits.push_back(cur_bit);

      cur_bit = NULL;
      continue;
    }

    if (! cur_bit)
    {
      cur_bit = new section_c(section_c::TEXT);

      SCR->bits.push_back(cur_bit);
    }

    cur_bit->AddLine(line);
  }

  return SCR;
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
