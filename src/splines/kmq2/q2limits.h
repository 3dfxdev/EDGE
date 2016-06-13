//----------------------------------------------------------------------------
//  EDGE2 Q2LIMITS
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2015 Isotope SoftWorks and Contributors.
//  Based on the Quake 2 GPL Source, (C) id Software.
//
//  Uses code from KMQuake II, (C) Mark "KnightMare" Shan.
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
#ifndef _Q2LIMITS_H_
#define _Q2LIMITS_H_

typedef enum {qfalse, qtrue}    qboolean;
typedef unsigned char byte;
typedef float vec_t;
typedef vec_t vec2_t[2];
typedef vec_t vec3_t[3];
typedef vec_t vec4_t[4];

typedef struct
{
	vec3_t		mins, maxs;
	vec3_t		origin;		// for sounds or lights
	float		radius;
	int			headnode;
	int			visleafs;		// not including the solid leaf 0
	int			firstface, numfaces;
} mmodel_t;

//from qshared.h
#define MAX_STRING_CHARS    1024    // max length of a string passed to Cmd_TokenizeString
#define MAX_STRING_TOKENS   80      // max tokens resulting from Cmd_TokenizeString
#define MAX_TOKEN_CHARS     128     // max length of an individual token

#define MAX_QPATH           64      // max length of a quake game pathname
#define MAX_OSPATH          256     // max length of a filesystem pathname

//from qfiles.h
#define	MAX_MAP_MODELS		400
#define	MAX_MAP_BRUSHES		/* 4000 */ 40000

#define	MAX_MAP_ENTSTRING	0x28000

#define	MAX_MAP_TEXINFO		3000	// was 8192
#define	MAX_MAP_AREAS		130
#define	MAX_MAP_AREAPORTALS	400
#define	MAX_MAP_PLANES		(10000*6)
#define	MAX_MAP_NODES		(12000*2)
#define	MAX_MAP_BRUSHSIDES	(17000*3)
#define	MAX_MAP_LEAFS		(13000*2)
#define	MAX_MAP_VERTS		12000
#define	MAX_MAP_FACES		12000
#define	MAX_MAP_LEAFFACES	12000	//not used?
#define	MAX_MAP_LEAFBRUSHES 	(16000*2)
#define	MAX_MAP_PORTALS		12000	//not used?
#define	MAX_MAP_EDGES		30000
#define	MAX_MAP_SURFEDGES	44000
#define	MAX_MAP_LIGHTING	0x60000	//Not used?
#define	MAX_MAP_VISIBILITY	0x37000

#endif
 