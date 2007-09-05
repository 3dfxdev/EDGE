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

#ifndef __G_SCRIPT_H__
#define __G_SCRIPT_H__


typedef enum
{
  RTS_OK = 0,
  RTS_ERROR = 1,
//  RTS_EOF = 2,
  RTS_FINISHED = 3,
}
rts_result_e;
//!!!! FIXME
#define RTS_EOF  RTS_FINISHED


typedef enum
{
  // this is simplified (EDGE supports all 5 skills separately)
  WNAP_Easy   = (1 << 0),
  WNAP_Medium = (1 << 1),
  WNAP_Hard   = (1 << 2),

  WNAP_SP     = (1 << 4),
  WNAP_Coop   = (1 << 5),
  WNAP_DM     = (1 << 6),
}
when_appear_e;

#define WNAP_SkillBits  (WNAP_Easy | WNAP_Medium | WNAP_Hard)
#define WNAP_ModeBits   (WNAP_SP   | WNAP_Coop   | WNAP_DM)


class thing_spawn_c
{
private:
  thing_spawn_c();

public:
  ~thing_spawn_c();

  bool ambush;
  bool has_z;

  std::string type;

  float x, y, z;
 
  float angle;  // degrees (0 - 360)
  
  int tag;         // 0 = none
  int when_appear; // 0 = not specified

public:
  static bool thing_spawn_c::MatchThing(std::string& line);
  // returns true if this line matches a thing spawner.

  static thing_spawn_c * ReadThing(std::string& line);
  // Create a new thing-spawn definition by parsing the line.
  // The first keyword will already have been checked
  // (SPAWN_THING etc).  Returns NULL if an error occurs.

  void WriteThing(FILE *fp);
  // write this thing-spawn into the given file.
};


class rad_trigger_c
{
private:
  rad_trigger_c(bool _rect);

public:
  ~rad_trigger_c();

public:
  bool is_rect;
  bool worldspawn;

  float mx, my, mz;  // mid point
  float rx, ry, rz;  // radii (rz < 0 means no height range)
 
  std::string name;  // empty = none

  int tag;           // 0 = none
  int when_appear;   // 0 = not specified

  // all script lines except stuff covered by the above fields.
  // Does not include the start and end markers.
  std::vector<std::string> lines;

  // for 'worldspawn' scripts only: all of the entities
  std::vector<thing_spawn_c *> things;

public:
  static bool MatchRadTrig(std::string& line);
  // returns true if this line begins a radius trigger
  // (i.e. first keyword in RADIUS_TRIGGER or RECT_TRIGGER).

  static rad_trigger_c * ReadRadTrig(FILE *fp, std::string& first);
  // read a new radius trigger from the file and returns it.
  // Returns NULL if an error occurs.  The parameter 'first'
  // contains the first line, which must have previously matched
  // using MatchRadTrig().

  void WriteRadTrig(FILE *fp);
  // write this script into the given file.

private:
  rts_result_e ParseLocation(const char *pos);

  rts_result_e ParseBody(FILE *fp);
  
  rts_result_e ParseCommand(std::string& line);
  // parse the given line of the trigger.  Returns RTS_OK for
  // normal lines, RTS_FINISHED for the end marker (after which
  // this function is never called again), or RTS_ERROR if a
  // problem occurred.

  static bool MatchEndTrig(std::string& line);
};


class section_c
{
public:
   section_c(int _kind);
  ~section_c();

public:
  enum kind_e
  {
    TEXT      = 0,
    START_MAP = 1,
    RAD_TRIG  = 2,
  };

  int kind;

  // --- TEXT ---
  
  std::vector<std::string> lines;

  // --- START_MAP ---
 
  std::string map_name;

  // pieces are everything except the START_MAP and END_MAP lines
  std::vector<section_c *> pieces;

  // --- RAD_TRIG ---
  
  rad_trigger_c *trig;

public:
  static bool MatchStartMap(std::string& line);
  // returns true if this line begins with START_MAP.

  static section_c * ReadStartMap(FILE *fp, std::string& first);
  // reads the whole map section (from START_MAP to END_MAP)
  // and returns a new section_c object, or NULL if an error
  // occurred.  The parameter 'first' contains the first line,
  // which must have previously matched with MatchStartMap().

  void WriteSection(FILE *fp);
  // write this section into the given file.

  void AddLine(std::string& line);

private:
  void WriteText(FILE *fp);
  void WriteStartMap(FILE *fp);

  rts_result_e ParseMapName(const char *pos);
  rts_result_e ParsePieces(FILE *fp);

  static bool MatchEndMap(std::string& line);
};


class script_c
{
private:
  script_c();

public:
  ~script_c();
 
public:
  std::vector<section_c *> bits;

public:
  static script_c *Load(FILE *fp);
  // load a whole RTS script file and return the new script_c.
  // If a serious error occurs, shows a dialog message and
  // eventually returns NULL.

  void Save(FILE *fp);
  // save the whole script into the given file.

private:
};


#endif // __G_SCRIPT_H__

//--- editor settings ---
// vi:ts=2:sw=2:expandtab
