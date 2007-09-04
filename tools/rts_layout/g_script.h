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
  RTS_FINISHED = 2,
}
rts_result_e;


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
 
  float angle;  // degrees 0-359
  
  int tag;
  int when_appear;

public:
  static thing_spawn_c * Parse(const char *line);
  // Create a new thing-spawn definition by parsing the line.
  // The first keyword will already have been checked
  // (SPAWN_THING etc).  Returns NULL if an error occurs.

  void Write(FILE *fp);
  // write this thing-spawn into the given file.
};


class rad_trigger_c
{
private:
  rad_trigger_c();

public:
  ~rad_trigger_c();

public:
  bool is_rect;
  bool worldspawn;

  float mx, my, mz;  // mid point
  float rx, ry, rz;  // radii (rz < 0 means no heights)
 
  std::string name;  // empty = none

  int tag;           // 0 = none
  int when_appear;   // 0 = not specified

  // all script lines except stuff covered by the above fields.
  // Includes the end marker but excludes the start marker.
  std::vector<std::string> lines;

  // for 'worldspawn' scripts only: all of the entities
  std::vector<thing_spawn_c *> things;

public:
  static rad_trigger_c * Create(const char *line);
  // create a new radius trigger.  The first keyword will already
  // have matched a valid type (RADIUS_TRIGGER or RECT_TRIGGER).
  // Returns NULL if an error occurs.

  rts_result_e Parse(const char *line);
  // parse another line of the trigger.  Returns RTS_OK for
  // normal lines, RTS_FINISHED for the end line (after which
  // Parse() is never called again), or RTS_ERROR if a problem
  // occurred.

  void Write(FILE *fp);
  // write this script into the given file.
};


class section_c
{
public:
   section_c(int _kind);
  ~section_c();

public:
  enum part_kind_e
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

  std::vector<section_c *> pieces;

  // --- RAD_TRIG ---
  
  rad_trigger_c *trig;
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
  // ....
};


#endif // __G_SCRIPT_H__

//--- editor settings ---
// vi:ts=2:sw=2:expandtab
