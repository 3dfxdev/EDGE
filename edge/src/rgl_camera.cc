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

void camera_c::FindAngles(epi::vec3_c _dir)
{
	float len_2d = sqrt(_dir.x * _dir.x + _dir.y * _dir.y);

#if 0
	if (len_2d < 0.001f)
	{
		// vector is directly upwards or downwards, do the best we can.
		// (don't change TURN angle, hopefully current value is better
		// than an abitrary choice).

		mlook = (_dir.z < 0) ? epi::angle_c(-89.9f) : epi::angle_c(89.9f);
		return;
	}
#endif

	turn  = epi::angle_c::FromVector(_dir.x, _dir.y);
	mlook = epi::angle_c::FromVector(len_2d, _dir.z);
}

void camera_c::Recalculate()
{
	// compute slopes
	epi::angle_c SideAng = FOV / 2;

	horiz_slope = SideAng.Tan();

	float aspect = ASPECT_RATIO *
		(float(viewwindowheight) / SCREENHEIGHT) /
		(float(viewwindowwidth)  / SCREENWIDTH);

	vert_slope = horiz_slope * aspect;

	// compute face direction, ensuring it has length == 1
	dir.z = mlook.Sin();

	float len_2d = mlook.Cos();

	dir.x = len_2d * turn.Cos();
	dir.y = len_2d * turn.Sin();

	// cone information
	float diagonal = sqrtf(horiz_slope * horiz_slope + vert_slope * vert_slope);

	cone_cos = Z_NEAR / sqrtf(Z_NEAR * Z_NEAR + diagonal * diagonal);
	cone_tan = diagonal / Z_NEAR;

	// plane vectors...
	epi::vec2_c tmp_A(-Z_NEAR, 0);
	tmp_A.Rotate(mlook);

	left_v  = epi::vec3_c(tmp_A.x,  horiz_slope, tmp_A.y);
	right_v = epi::vec3_c(tmp_A.x, -horiz_slope, tmp_A.y);

	epi::vec2_c tmp_B(-vert_slope, Z_NEAR);
	tmp_B.Rotate(mlook);

	top_v = epi::vec3_c(tmp_B.x, 0, tmp_B.y);

	epi::vec2_c tmp_C(-vert_slope, -Z_NEAR);
	tmp_C.Rotate(mlook);

	bottom_v = epi::vec3_c(tmp_C.x, 0, tmp_C.y);

	AdjustPlane(&left_v);
	AdjustPlane(&right_v);
	AdjustPlane(&top_v);
	AdjustPlane(&bottom_v);
}

void camera_c::AdjustPlane(epi::vec3_c *v)
{
	// rotate by the camera's turn angle, then scale the vector
	// to be unit length.

	epi::vec2_c base(v->x, v->y);
	base.Rotate(turn);
	
	v->x = base.x;
	v->y = base.y;

	*v /= v->Length();
}

void camera_c::LoadGLMatrices(bool translate) const
{
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

int camera_c::TestSphere(epi::vec3_c mid, float R) const
{
	// first step: check for sphere surrounding camera
	epi::vec3_c relat(mid - pos);
	
	if (relat * relat <= R * R)
		return HIT_PARTIAL;

	// second step: check if completely behind the near plane
	float along_axis = relat * dir;

	if (along_axis - Z_NEAR <= -R)
		return HIT_OUTSIDE;
	
	// third step: check sphere against the cone
	float perp_sphr = sqrtf(relat * relat - along_axis * along_axis);
	float perp_cone = along_axis * cone_tan;

	float dist = (perp_sphr - perp_cone) * cone_cos;

	if (dist >= R)
		return HIT_OUTSIDE;

	if (dist <= -R)
		return HIT_INSIDE;

	return HIT_PARTIAL;
}

int camera_c::TestBBox(const epi::bbox3_c *bb) const
{
	// first step: check for bbox surrounding camera
	if (bb->Contains(pos + dir))
		return HIT_PARTIAL;

	// second step: check  @@@ !!!!
	for (int pt_index = 0; pt_index < 8; pt_index++)
	{
	}

	return HIT_PARTIAL;  //!!!!! FIXME
}

//------------------------------------------------------------------------
//  TEST ROUTINES
//------------------------------------------------------------------------

#ifdef TEST_CAMERA_CLASS

void Test_CameraClass(void)
{
// !!! FIXME

}

#endif // TEST_CAMERA_CLASS

