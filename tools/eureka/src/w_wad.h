//------------------------------------------------------------------------
//  WAD Reading / Writing
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

#ifndef __W_WAD_H__
#define __W_WAD_H__


class Wad_file;


class Lump_c
{
friend class Wad_file;

private:
	int offset;
	int length;

	Lump_c(int _ofs, int _len);

public:
	~Lump_c();

	int Length() const { return length; }

	
};


class Wad_file
{
private:
	FILE * fp;

	std::vector< Lump_c* > directory;

	Wad_file(FILE * file);

public:
	~Wad_file();

	static Wad_file * Open(const char *filename);


private:
	void ReadDirectory();
};


extern std::vector< Wad_file* > master_dir;


#endif  /* __W_WAD_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
