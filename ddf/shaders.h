//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (Shaders)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2008  The EDGE Team.
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

#ifndef __DDF_SHADERS_H__
#define __DDF_SHADERS_H__

#include "../epi/utility.h"

#include "types.h"

typedef enum
{
	SNS_Fragment = 0,
	SNS_Vertex,
}
shader_namespace_e;

//
// -AJA- 2004/11/16 Images.ddf
//
typedef enum
{
	SHADR_File,         // load from a path (/shaders/glsl/.fp/.)
	SHADR_Lump,         // load from lump in a WAD

}
shaderdata_type_e;

typedef enum
{
	LSF_FRAG = 0, //fragment shader always comes first!
	LSF_VERT, // vertex shader can come second.
}
L_shader_format_e;

class shaderdef_c
{
public:
	shaderdef_c();
	~shaderdef_c() {};

public:
	void Default(void);
	void CopyDetail(const shaderdef_c &src);

	// Member vars....
	epi::strent_c name;
	shader_namespace_e belong; //either fragment or vertex

	shaderdata_type_e type; // File or LUMP

	epi::strent_c info;   // IMGDT_WadXXX, IMGDT_Package, IMGDT_File, IMGDT_Lump
	L_shader_format_e format;  // IMGDT_Lump, IMGDT_File (etc)


private:
	// disable copy construct and assignment operator
	explicit shaderdef_c(shaderdef_c &rhs) { }
	shaderdef_c& operator=(shaderdef_c &rhs) { return *this; }
};


// Our shaderdefs container
class shaderdef_container_c : public epi::array_c 
{
public:
	shaderdef_container_c() : epi::array_c(sizeof(shaderdef_c*)) {}
	~shaderdef_container_c() { Clear(); } 

private:
	void CleanupObject(void *obj);

public:
	int GetSize() {	return array_entries; } 
	int Insert(shaderdef_c *a) { return InsertObject((void*)&a); }
	shaderdef_c *operator[](int idx) { return *(shaderdef_c**)FetchObject(idx); } 

	// Search Functions
	shaderdef_c *Lookup(const char *refname, shader_namespace_e belong);
};

extern shaderdef_container_c shaderdefs;

bool DDF_ReadShaders(void *data, int size);

#endif  /*__DDF_SHADERS_H__*/

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
