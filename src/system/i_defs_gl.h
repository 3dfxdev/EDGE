//----------------------------------------------------------------------------
//  EDGE2 System Specific Header for OpenGL
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2007-2009  The EDGE2 Team.
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
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------

#ifndef __SYSTEM_SPECIFIC_DEFS_OPENGL__
#define __SYSTEM_SPECIFIC_DEFS_OPENGL__

#include "GL/gl_load.h"
#include "i_sdlinc.h"

#if defined(LINUX) || defined(BSD)
#include <GL/gl.h>
#include <GL/glext.h>
#endif

///TODO: change this to #ifdef WIN32_GLEW and add a new
///      define, like WIN32_GLAD, for GLEWs successor..
#ifdef GLEWISDEAD
#define GLEW_STATIC  1
#include "GL\gl.h"
#include "GL\glext.h"
#endif

#ifdef MACOSX
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#endif

#ifdef DREAMCAST
#include <gl/GL.h>
#endif

#define USING_GL_TYPES 1

// use OpenGL to brighten or dark the display instead of SDL
#define GL_BRIGHTNESS

enum GLCompat
{
	CMPT_GL2,
	CMPT_GL2_SHADER,
	CMPT_GL3,
	CMPT_GL4
};

enum RenderFlags
{
	// [BB] Added texture compression flags.
	RFL_TEXTURE_COMPRESSION = 1,
	RFL_TEXTURE_COMPRESSION_S3TC = 2,

	RFL_SHADER_STORAGE_BUFFER = 4,
	RFL_BUFFER_STORAGE = 8,
	RFL_SAMPLER_OBJECTS = 16,

	RFL_NO_CLIP_PLANES = 32,

	RFL_INVALIDATE_BUFFER = 64,
	RFL_DEBUG = 128
};

struct RenderContext
{
	unsigned int flags;
	unsigned int maxuniforms;
	unsigned int maxuniformblock;
	unsigned int uniformblockalignment;
	int lightmethod;
	int buffermethod;
	float glslversion;
	int max_texturesize;
	char * vendorstring;
	bool legacyMode;
	bool es;

	int MaxLights() const
	{
		return maxuniforms >= 2048 ? 128 : 64;
	}
};

extern RenderContext gl;

#endif /* __SYSTEM_SPECIFIC_DEFS_OPENGL__ */


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
