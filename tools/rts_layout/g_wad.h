//------------------------------------------------------------------------
//  WAD : Wad file read/write functions.
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

#ifndef __G_WAD_H__
#define __G_WAD_H__


// ---- Directory entry ----------

class lump_c
{
public:
   lump_c(const char *_name, int _pos, int _len);
  ~lump_c();

public:
  // name of lump
  const char *name;

  // offset/length of lump in wad
  int start;
  int length;

  // cached data of lump
  byte *data;
};


// ---- Level information ---------

class level_c
{
public:
   level_c(const char *_name);
  ~level_c();

public:
  const char *name;

  // the child lump list (includes marker)
  std::vector<lump_c *> children;
};


// ---- WAD header -----------

class wad_c
{
private:
   wad_c();

public:
  ~wad_c();

  enum wad_kind_e { IWAD, PWAD };

public:
  FILE *fp;

  // kind of wad file (-1 if not opened yet)
  int kind;

  // number of entries in directory (original)
  int num_entries;

  // offset to start of directory
  int dir_start;

  // directory
  std::vector<lump_c *> dir;

  // levels
  std::vector<level_c *> levels;

  // current level
  level_c *cur_level;

public:
  static wad_c *Load(const char *filename);
  // open the input wad file and read the contents into memory.

  bool CheckLevelName(const char *name);
  // returns true if the given level exists.

  bool FindLevel(const char *map_name);
  // find a particular level in the wad directory, and store the
  // reference in 'wad.current_level'.  Returns false if not
  // found.

  lump_c *FindLump(const char *name);
  // find the lump with the given name in the wad directory, and
  // return a reference to it.  Returns NULL if no such lump exists.

  lump_c *FindLumpInLevel(const char *name);
  // find the level lump with the given name in the current level, and
  // return a reference to it.  Returns NULL if no such lump exists.

  const byte * CacheLump(lump_c *lump);
  // loads the lump data into memory (caching it for future usage).
  // A pointer to the data is returned.  The data will have an
  // extra NUL (\0) byte on the end, for convenience when parsing.

private:
  bool ReadHeader();
  void ReadDirEntry();
  void ReadDirectory();

  void DetermineLevels();

  void ProcessDirEntry(lump_c *lump);

  void AddLevel(lump_c *lump);
};


#endif // __G_WAD_H__

//--- editor settings ---
// vi:ts=2:sw=2:expandtab
