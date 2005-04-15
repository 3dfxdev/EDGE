//----------------------------------------------------------------------------
//  EDGE OpenGL Rendering (Cameras)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2004-2005  The EDGE Team.
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

#include "i_defs.h"
#include "rgl_camera.h"

#include "dm_defs.h"
#include "dm_state.h"
#include "con_main.h"
#include "m_bbox.h"
#include "p_local.h"
#include "p_mobj.h"
#include "r_defs.h"
#include "r_main.h"
#include "r_state.h"
#include "r2_defs.h"
#include "rgl_defs.h"
#include "v_ctx.h"
#include "z_zone.h"

static const float ASPECT_RATIO = 200.0f / 320.0f;

camera_c::camera_c() : FOV(epi::angle_c::Ang90()), pos(), turn(), mlook()
{
	Recalculate();
}

camera_c::camera_c(const camera_c& other) : FOV(other.FOV), pos(other.pos),
	turn(other.turn), mlook(other.mlook)
{
	// Note: a tad inefficient, but keeps it simple
	Recalculate();
}

camera_c::~camera_c()
{
	// nothing to free here, move along...
}

void FindAngles(epi::vec3_c _dir)
{
	// !!!! FIXME: FindAngles
}

void Recalculate()
{
	// !!!! FIXME: Recalculate
}

void camera_c::LoadGLMatrices(bool translate) const
{
	// first compute slopes
	epi::angle_c SideAng = FOV / 2;

	float horiz_slope = SideAng.Tan();

	float aspect = ASPECT_RATIO *
		(float(viewwindowheight) / SCREENHEIGHT) /
		(float(viewwindowwidth)  / SCREENWIDTH);

	float vert_slope = horiz_slope * aspect;

	glMatrixMode(GL_PROJECTION);

	glLoadIdentity();
	glFrustum(-horiz_slope, horiz_slope, -vert_slope, vert_slope, Z_NEAR, Z_FAR);

	glMatrixMode(GL_MODELVIEW);

	glLoadIdentity();
	glRotatef(270.0f - mlook.Degrees(), 1.0f, 0.0f, 0.0f);
	glRotatef(90.0f  - turn.Degrees(), 0.0f, 0.0f, 1.0f);

	if (translate)
		glTranslatef(-pos.x, -pos.y, -pos.z);
}

int camera_c::TestSphere(const epi::vec3_c mid, float R) const
{
	return HIT_PARTIAL;  //!!!!! FIXME

#if 0

Cone-Sphere test:
-----------------

V = sphere.center - cone.apex_location
a = V * cone.direction_normal
b = a * cone.tan
c = sqrt( V*V - a*a )
d = c - b
e = d * cone.cos

now  if ( e >= sphere.radius ) , cull the sphere
else if ( e <=-sphere.radius ) , totally include the sphere
	else the sphere is partially included.


Occlusion tests:
----------------

1. Sphere against near plane
2. Sphere against cone
3. BBox against five planes [needed ??]

#endif
}

int camera_c::TestBBox(const epi::bbox3_c *bb) const
{
	return HIT_PARTIAL;  //!!!!! FIXME
}

