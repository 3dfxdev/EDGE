//----------------------------------------------------------------------------
//  EDGE OpenGL Rendering (BSP Traversal)
//----------------------------------------------------------------------------
//
//  Copyright (c) 1999-2010  The EDGE Team.
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

#include "system/i_defs.h"
#include "system/i_defs_gl.h"

#include <math.h>

#include "dm_data.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "g_game.h"
#include "m_bbox.h"
#include "p_local.h"
#include "p_mobj.h"
#include "p_pobj.h"
#include "r_defs.h"
#include "r_misc.h"
#include "r_modes.h"
#include "r_gldefs.h"
#include "r_colormap.h"
#include "r_effects.h"
#include "r_image.h"
#include "r_occlude.h"
#include "r_shader.h"
#include "r_sky.h"
#include "r_things.h"
#include "r_units.h"

#include "n_network.h"  // N_NetUpdate
#include "con_main.h"

#include <stdio.h>

//#define ZDOOM_OCCLUSION // Use screen space clipping instead of angles for 1d occlusion buffer, as zdoom does

#define DEBUG  0

#define FLOOD_DIST    1024.0f
#define FLOOD_EXPAND  128.0f

#define DOOM_YSLOPE       (0.525)
#define DOOM_YSLOPE_FULL  (0.625)

extern float G_CircularLerp(float start, float end, float value);

bool RGL_OcclusionTestLine(double x1, double y1, double x2, double y2, angle_t &sx1, angle_t &sx2);

static inline float SecLerpedCeil(sector_t *sec)
{
	return N_Interpolate(sec->lc_h, sec->c_h);
}

static inline float SecLerpedFloor(sector_t *sec)
{
	return N_Interpolate(sec->lf_h, sec->f_h);
}

static inline float ExFloorLerpedBottom(extrafloor_t *exf)
{
	return N_Interpolate(exf->last_bottom_h, exf->bottom_h);
}

static inline float ExFloorLerpedTop(extrafloor_t *exf)
{
	return N_Interpolate(exf->last_top_h, exf->top_h);
}

// #define DEBUG_GREET_NEIGHBOUR


DEF_CVAR(debug_hom, int, "c", 0);
DEF_CVAR(r_oldblend, int, "c", 1);
extern int r_stretchworld;

side_t *sidedef;
line_t *linedef;
sector_t *frontsector;
sector_t *backsector;

unsigned int root_node;


int detail_level = 2;
int use_dlights = 0;

int doom_fading = 1;

int fade_color;
int swirl_pass = 0;
bool thick_liquid = false;

float view_x_slope;
float view_y_slope;

// -ES- 1999/03/20 Different right & left side clip angles, for asymmetric FOVs.
angle_t clip_left, clip_right;
angle_t clip_scope;

mobj_t *view_cam_mo;

float view_expand_w;

extern float pixel_aspect;


static int checkcoord[12][4] =
{
  {BOXRIGHT, BOXTOP, BOXLEFT, BOXBOTTOM},
  {BOXRIGHT, BOXTOP, BOXLEFT, BOXTOP},
  {BOXRIGHT, BOXBOTTOM, BOXLEFT, BOXTOP},
  {0},
  {BOXLEFT, BOXTOP, BOXLEFT, BOXBOTTOM},
  {0},
  {BOXRIGHT, BOXBOTTOM, BOXRIGHT, BOXTOP},
  {0},
  {BOXLEFT, BOXTOP, BOXRIGHT, BOXBOTTOM},
  {BOXLEFT, BOXBOTTOM, BOXRIGHT, BOXBOTTOM},
  {BOXLEFT, BOXBOTTOM, BOXRIGHT, BOXTOP}
};



// colour of the player's weapon
extern int rgl_weapon_r;
extern int rgl_weapon_g;
extern int rgl_weapon_b;

extern float sprite_skew;

// common stuff

extern sector_t *frontsector;
extern sector_t *backsector;

static subsector_t *cur_sub;
static seg_t *cur_seg;

static bool solid_mode;

static std::list<drawsub_c *> drawsubs;

enum {
	E_NONE,
	E_POSDIR_GIZMO,
	E_CAMERA_GIZMO,
};

struct gizmo_t
{
	int type;
};

struct posdir_gizmo_t : public gizmo_t
{
	posdir_gizmo_t()
	{
		type = E_POSDIR_GIZMO;
	}

	float x, y, z, ax, ay, az, scale;
};

static void RGL_DrawGizmo(const gizmo_t *gizmo)
{
	glPushMatrix();
	{
		switch (gizmo->type)
		{
			case E_POSDIR_GIZMO:
			{
				const posdir_gizmo_t *camera = (const posdir_gizmo_t *)gizmo;

				glLineWidth(4.0f);
				glTranslatef(camera->x, camera->y, camera->z);
				glScalef(camera->scale, camera->scale, camera->scale);
				glBegin(GL_LINES);
				{
					glColor3f(1.0f, 0.0f, 0.0f);
					glVertex3f(0, 0, 0);
					glVertex3f(1, 0, 0);

					glColor3f(0.0f, 1.0f, 0.0f);
					glVertex3f(0, 0, 0);
					glVertex3f(0, 1, 0);

					glColor3f(0.0f, 0.0f, 1.0f);
					glVertex3f(0, 0, 0);
					glVertex3f(0, 0, 1);

					glColor3f(1.0f, 0.0f, 1.0f);
					glVertex3f(0, 0, 0);
					glVertex3f(camera->ax, camera->ay, camera->az);
				}
				glEnd();
				glLineWidth(1.0f);
			}
			break;

			case E_CAMERA_GIZMO:
			{
				//TODO: Render a MD3 model
				// LOad it via modeldef_c *LoadModelFromLump(int model_num) somewhere and draw how it is done in r_md2.cc...
			}
			break;

		default: break;
		}
	}
	glPopMatrix();
}

// Camera-man system implementation.
DEF_CVAR(camera_subdir, std::string, "c", "doom_ddf/cameras")	// CVAR for camera-man system data files subdirectory.

namespace cameraman
{
	//TODO: Different types of interpolation...
	//TODO: Debug view mode...
	//TODO: Free camera mode...
	//TODO: Disable HUD when camera-man is in use...
	//TODO: Disable camera movement on player's input when camera-man is in use...
	//TODO: Look-at mechanics for tracking moving targets...

#define MAX_COUNT 512

	typedef struct
	{
		char name[32];
		int valid;
		angle_t viewangle, viewvertangle;
		float x, y, z;
		float fov;
	} cameraman_t;

	enum {
		E_LERP,
		E_LOOKAT,
	};

	static int g_active = 0;
	static int g_drawHelpers = 0;
	static int g_count = 0;
	static int g_startId = -1, g_endId = -1;
	static int g_interpolationType = E_LERP;
	static float g_interpolationValue = 0.0f;
	static cameraman_t cameramen[MAX_COUNT];

	static const cameraman_t *Get(int id)
	{
		if (id > -1 && id < MAX_COUNT)
		{
			const cameraman_t *cam = (const cameraman_t *)cameramen + id;
			if (cam->valid > 0)
			{
				return cam;
			}
		}

		return NULL;
	}

	static cameraman_t *Lerp(cameraman_t *interp, int id0, int id1, float t)
	{
		if (interp)
		{
			const cameraman_t *cam0 = Get(id0);
			const cameraman_t *cam1 = Get(id1);

			if (t <= 0.0f)
			{
				interp->x = cam0->x;
				interp->y = cam0->y;
				interp->z = cam0->z;
				interp->fov = cam0->fov;
				interp->viewangle = cam0->viewangle;
				interp->viewvertangle = cam0->viewvertangle;
				interp->valid = 1;
				return interp;
			}

			if (t >= 1.0f)
			{
				interp->x = cam1->x;
				interp->y = cam1->y;
				interp->z = cam1->z;
				interp->fov = cam1->fov;
				interp->viewangle = cam1->viewangle;
				interp->viewvertangle = cam1->viewvertangle;
				interp->valid = 1;
				return interp;
			}

			if (cam0 != NULL && cam1 != NULL)
			{
				interp->x = cam0->x + t * (cam1->x - cam0->x);
				interp->y = cam0->y + t * (cam1->y - cam0->y);
				interp->z = cam0->z + t * (cam1->z - cam0->z);
				interp->fov = cam0->fov + t * (cam1->fov - cam0->fov);
				interp->viewangle = G_CircularLerp(cam0->viewangle, cam1->viewangle, t);
				interp->viewvertangle = G_CircularLerp(cam0->viewvertangle, cam1->viewvertangle, t);
				interp->valid = 1;
				return interp;
			}
		}
		return NULL;
	}

	static cameraman_t *LerpLookAt(cameraman_t *interp, int id0, int id1, float t)//????
	{
		if (interp)
		{
			interp = Lerp(interp, id0, id1, t);
			//TODO: Override the angels with the ones calculated from direction to target...
			return interp;
		}
		return NULL;
	}

	static cameraman_t *Update(cameraman_t *cam)
	{
		if (cam != NULL)
		{
			if (g_active > 0)
			{
				if (g_startId > -1 && g_endId > -1 && g_startId != g_endId)
				{
					switch (g_interpolationType)
					{
					case E_LOOKAT:
						return LerpLookAt(cam, g_startId, g_endId, g_interpolationValue);
						break;

					case E_LERP:
					default:
						return Lerp(cam, g_startId, g_endId, g_interpolationValue);
						break;
					}
				}
				else
				{
					return (cameraman_t *)Get(g_startId);
				}
			}
		}
		return NULL;
	}

	void Reset()
	{
		for (int i = 0; i < MAX_COUNT; ++i)
		{
			cameraman_t *cam = cameramen + i;
			cam->valid = 0;
			cam->viewangle = 0;
			cam->viewvertangle = 0;
			cam->x = 0.0f;
			cam->y = 0.0f;
			cam->z = 0.0f;
			cam->fov = 0.0f;
		}

		g_count = 0;
		g_startId = -1;
		g_endId = -1;
		g_interpolationValue = 0.0f;
	}

	void Serialize(int reading, const char *map)
	{
		SYS_ASSERT(map || currmap);

		std::string fileName("./");
		FILE *file = NULL;

		fileName = fileName.append(camera_subdir);
		fileName = fileName.append("/");
		fileName = fileName.append(map ? map : currmap->name.c_str());
		fileName = fileName.append(".camdata");

		CON_Printf("Serializing: %s\n", fileName.c_str());

		char body[5] = { 0 };

		if (reading > 0)
		{
            if ((file = fopen(fileName.c_str(), "rb")))
			{
				fread((void *)&g_count, sizeof(int), 1, file);
				fread((void *)body, sizeof(char), 5, file);
				fread((void *)cameramen, sizeof(cameraman_t), (size_t)g_count, file);
				fclose(file);
			}
			else
			{
				CON_Printf("File \'%s\' not found!", fileName.c_str());
			}
		}
		else // Only editing
		{
            if ((file = fopen(fileName.c_str(), "wb")))
			{
				fwrite((void *)&g_count, sizeof(int), 1, file);
				fwrite((void *)body, sizeof(char), 5, file);
				fwrite((void *)cameramen, sizeof(cameraman_t), (size_t)g_count, file);
				fclose(file);
			}
			else
			{
				CON_Printf("File \'%s\' not found!", fileName.c_str());
			}
		}
	}

	void Activate(int activate)
	{
		Reset();
		g_active = activate;
	}

	void Toggle()
	{
		if (g_active > 0)
			Activate(0);
		else
			Activate(1);
	}

	void ToggleHelpers()
	{
		if (g_drawHelpers > 0)
			g_drawHelpers = 0;
		else
			g_drawHelpers = 1;
	}

	int Add(float x, float y, float z, float ax, float ay, float fov, const char *name)
	{
		if (g_active > 0)
		{
			if (g_count < MAX_COUNT)
			{
				cameraman_t *cam = cameramen + g_count;
				cam->valid = 1;
				cam->x = x;
				cam->y = y;
				cam->z = z;
				cam->viewangle = G_CircularLerp(0, ay, 1.0f);
				cam->viewvertangle = G_CircularLerp(0, ax, 1.0f);
				cam->fov = fov;

				std::string sname = "camera" + std::to_string(g_count);

				if (!name)
					strncpy(cam->name, sname.c_str(), sname.size());
				else
					strncpy(cam->name, name, strlen(name));

				++g_count;
				return (g_count - 1);
			}
			else
			{
				for (int i = 0; i < g_count; ++i)
				{
					cameraman_t *cam = cameramen + i;
					if (cam->valid != 1)
					{
						cam->valid = 1;
						cam->x = x;
						cam->y = y;
						cam->z = z;
						cam->viewangle = G_CircularLerp(0, ay, 1.0f);
						cam->viewvertangle = G_CircularLerp(0, ax, 1.0f);
						cam->fov = fov;

						std::string sname = "camera" + std::to_string(g_count);

						if (!name)
							strncpy(cam->name, sname.c_str(), sname.size());
						else
							strncpy(cam->name, name, strlen(name)); //TODO: V814 https://www.viva64.com/en/w/v814/ Decreased performance. The 'strlen' function was called multiple times inside the body of a loop.
						return i;
					}
				}
			}
		}
		return -1;
	}

	int Remove(int id)
	{
		if (g_active > 0)
		{
			if (id > -1 && id < MAX_COUNT)
			{
				cameraman_t *cam = cameramen + id;
				cam->valid = 0;
				return id;
			}
		}
		return -1;
	}

	int SetLookAtTarget(int id, int target)//???
	{
		//TODO:
		return id;
	}

	int SetPosition(int id, float x, float y, float z)
	{
		if (g_active > 0)
		{
			if (id > -1 && id < MAX_COUNT)
			{
				cameraman_t *cam = cameramen + id;
				if (cam->valid > 0)
				{
					cam->x = x;
					cam->y = y;
					cam->z = z;
					return id;
				}
			}
		}
		return -1;
	}

	int SetAngles(int id, float ax, float ay)
	{
		if (g_active > 0)
		{
			if (id > -1 && id < MAX_COUNT)
			{
				cameraman_t *cam = cameramen + id;
				if (cam->valid > 0)
				{
					cam->viewangle = G_CircularLerp(0, ay, 1.0f);
					cam->viewvertangle = G_CircularLerp(0, ax, 1.0f);
					return id;
				}
			}
		}
		return -1;
	}

	int SetFov(int id, float fov)
	{
		if (g_active > 0)
		{
			if (id > -1 && id < MAX_COUNT)
			{
				cameraman_t *cam = cameramen + id;
				if (cam->valid > 0)
				{
					cam->fov = fov;
					return id;
				}
			}
		}
		return -1;
	}

	int IsActive()
	{
		return g_active;
	}

	int GetId(std::string name)
	{
		if (g_active > 0)
		{
			for (int i = 0; i < g_count; ++i)
			{
				cameraman_t *cam = cameramen + i;
				if (cam->valid && name == cam->name)
					return i;
			}
		}
		return -1;
	}

	int GetStartId()
	{
		return g_startId;
	}

	int GetEndId()
	{
		return g_endId;
	}

	void SetStartId(int id)
	{
		if (const cameraman_t *cam = Get(id))
		{
			g_startId = id;
		}
		else
		{
			g_startId = -1;
		}
	}

	void SetEndId(int id)
	{
		if (const cameraman_t *cam = Get(id))
		{
			g_endId = id;
		}
		else
		{
			g_endId = -1;
		}
	}

	void SetStep(float value)
	{
		g_interpolationValue = value;
	}

	void Print()
	{
		CON_Printf("Camera-man system info - Is active: %s\n", g_active ? "yes" : "no");
		CON_Printf("--------------------------------------------\n");

		if (g_count > 0 && g_active > 0)
		{
			for (int i = 0; i < g_count; ++i)
			{
				const cameraman_t *cam = (const cameraman_t *)cameramen + i;

				float va = ANG_2_FLOAT(cam->viewangle);
				float vav = ANG_2_FLOAT(cam->viewvertangle);

				// vav is from 0 to 2PI, while it should be from -PI/2 to PI/2
				vav = (vav > 1.5f * 180.0f) ? vav - 2.0f * 180.0f : vav;
				vav = (vav > 180.0f) ? 180.0f - vav : vav;
				vav = (vav > 180.0f * 0.5f) ? 180.0f * 0.5f - (vav - 180.0f * 0.5f) : vav;

				CON_Printf("[ID: %d, NAME: %s] Valid: %s, Pos: (%g, %g, %g), Vertical: %3.3f, Horizontal: %3.3f, FOV: %g\n",
					i, cam->name, cam->valid > 0 ? "yes" : "no", cam->x, cam->y, cam->z, vav, va, cam->fov);
			}
		}
		else
		{
			CON_Printf("\t%s...\n", g_active > 0 ? "No resident camera-man found" : "System is not active");
		}

		CON_Printf("--------------------------------------------\n");
	}

	void DrawHelpers()
	{
		if (g_count > 0 && g_active > 0 && g_drawHelpers > 0)
		{
			for (int i = 0; i < g_count; ++i)
			{
				const cameraman_t *cam = (const cameraman_t *)cameramen + i;

				if (!cam->valid)
					continue;

				float va = ANG_2_FLOAT(cam->viewangle) * (M_PI / 180.0f);
				float vav = ANG_2_FLOAT(cam->viewvertangle) * (M_PI / 180.0f);

				// vav is from 0 to 2PI, while it should be from -PI/2 to PI/2
				vav = (vav > 1.5f * M_PI) ? vav - 2.0f * M_PI : vav;
				vav = (vav > M_PI) ? M_PI - vav : vav;
				vav = (vav > M_PI * 0.5f) ? M_PI * 0.5f - (vav - M_PI * 0.5f) : vav;
				vav -= (0.5f * M_PI);
				vav *= -1.0f;

				float r = 2.0f;
				float ax = (float)(sin(vav) * cos(va));
				float ay = (float)(sin(vav) * sin(va));
				float az = (float)cos(vav);

				posdir_gizmo_t gizmo;
				gizmo.x = cam->x;
				gizmo.y = cam->y;
				gizmo.z = cam->z;
				gizmo.ax = r * ax;
				gizmo.ay = r * ay;
				gizmo.az = r * az;
				gizmo.scale = 20.0f;

				RGL_DrawGizmo(&gizmo);
			}
		}
	}
}

// ========= MIRROR STUFF ===========

#define MAX_MIRRORS  3

static inline void ClipPlaneHorizontalLine(GLdouble *p, const vec2_t& s,
	const vec2_t& e)
{
	p[0] = e.y - s.y;
	p[1] = s.x - e.x;
	p[2] = 0.0f;
	p[3] = e.x * s.y - s.x * e.y;
}

static inline void ClipPlaneEyeAngle(GLdouble *p, angle_t ang)
{
	vec2_t s, e;

	s.Set(viewx, viewy);

	e.Set(viewx + M_Cos(ang), viewy + M_Sin(ang));

	ClipPlaneHorizontalLine(p, s, e);
}


typedef struct mirror_info_s
{
	drawmirror_c *def;

	float xc, xx, xy;  // x' = xc + x*xx + y*xy
	float yc, yx, yy;  // y' = yc + x*yx + y*yy
	float zc, z_scale; // z' = zc + z*z_scale

	float xy_scale;

	angle_t tc;

public:
	void ComputeMirror()
	{
		seg_t *seg = def->seg;

		float sdx = seg->v2->x - seg->v1->x;
		float sdy = seg->v2->y - seg->v1->y;

		float len_p2 = seg->length * seg->length;

		float A = (sdx * sdx - sdy * sdy) / len_p2;
		float B = (sdx * sdy * 2.0)       / len_p2;

		xx = A; xy =  B;
		yx = B; yy = -A;

		xc = seg->v1->x * (1.0-A) - seg->v1->y * B;
		yc = seg->v1->y * (1.0+A) - seg->v1->x * B;

		// Turn(a) = mir_angle + (0 - (a - mir_angle))
		//         = 2 * mir_angle - a

		tc = seg->angle << 1;

		zc = 0;
		z_scale = 1.0f;
		xy_scale = 1.0f;
	}

	float GetAlong(const line_t *ld, float x, float y)
	{
		if (fabs(ld->dx) >= fabs(ld->dy))
			return (x - ld->v1->x) / ld->dx;
		else
			return (y - ld->v1->y) / ld->dy;
	}

	void ComputePortal()
	{
		seg_t  *seg   = def->seg;
		line_t *other = seg->linedef->portal_pair;

		SYS_ASSERT(other);

		float ax1 = seg->v1->x;
		float ay1 = seg->v1->y;

		float ax2 = seg->v2->x;
		float ay2 = seg->v2->y;


		// find corresponding coords on partner line
		float along1 = GetAlong(seg->linedef, ax1, ay1);
		float along2 = GetAlong(seg->linedef, ax2, ay2);

		float bx1 = other->v2->x - other->dx * along1;
		float by1 = other->v2->y - other->dy * along1;

		float bx2 = other->v2->x - other->dx * along2;
		float by2 = other->v2->y - other->dy * along2;


		// compute rotation angle
		tc = ANG180 + R_PointToAngle(0,0, other->dx,other->dy) - seg->angle;

		xx =  M_Cos(tc); xy = M_Sin(tc);
		yx = -M_Sin(tc); yy = M_Cos(tc);


		// scaling
		float a_len = seg->length;
		float b_len = R_PointToDist(bx1,by1, bx2,by2);

		xy_scale = a_len / MAX(1, b_len);

		xx *= xy_scale; xy *= xy_scale;
		yx *= xy_scale; yy *= xy_scale;


		// translation
		xc = ax1 - bx1 * xx - by1 * xy;
		yc = ay1 - bx1 * yx - by1 * yy;


		// heights
		float a_h = (seg->frontsector->c_h - seg->frontsector->f_h);
		float b_h = (other->frontsector->c_h - other->frontsector->f_h);

		z_scale = a_h / MAX(1, b_h);
		zc = seg->frontsector->f_h - other->frontsector->f_h * z_scale;
	}

	void Compute()
	{
		if (def->is_portal)
			ComputePortal();
		else
			ComputeMirror();
	}

	void Transform(float& x, float& y)
	{
		float tx = x, ty = y;

		x = xc + tx*xx + ty*xy;
		y = yc + tx*yx + ty*yy;
	}

	void Z_Adjust(float& z)
	{
		z = zc + z * z_scale;
	}

	void Turn(angle_t& ang)
	{
		ang = (def->is_portal) ? (ang - tc) : (tc - ang);
	}
}
mirror_info_t;


static mirror_info_t active_mirrors[MAX_MIRRORS];

int num_active_mirrors = 0;


void MIR_Coordinate(float& x, float& y)
{
	for (int i=num_active_mirrors-1; i >= 0; i--)
		active_mirrors[i].Transform(x, y);
}

void MIR_Height(float& z)
{
	for (int i=num_active_mirrors-1; i >= 0; i--)
		active_mirrors[i].Z_Adjust(z);
}

void MIR_Angle(angle_t &ang)
{
	for (int i=num_active_mirrors-1; i >= 0; i--)
		active_mirrors[i].Turn(ang);
}

float MIR_XYScale(void)
{
	float result = 1.0f;

	for (int i=num_active_mirrors-1; i >= 0; i--)
		result *= active_mirrors[i].xy_scale;

	return result;
}

float MIR_ZScale(void)
{
	float result = 1.0f;

	for (int i=num_active_mirrors-1; i >= 0; i--)
		result *= active_mirrors[i].z_scale;

	return result;
}

bool MIR_Reflective(void)
{
	if (num_active_mirrors == 0)
		return false;

	bool result = false;

	for (int i=num_active_mirrors-1; i >= 0; i--)
		if (! active_mirrors[i].def->is_portal)
			result = !result;

	return result;
}

static bool MIR_SegOnPortal(seg_t *seg)
{
	if (num_active_mirrors == 0)
		return false;

	if (seg->miniseg)
		return false;

	drawmirror_c *def = active_mirrors[num_active_mirrors-1].def;

	if (def->is_portal)
	{
		if (seg->linedef == def->seg->linedef->portal_pair)
			return true;
	}
	else // mirror
	{
		if (seg->linedef == def->seg->linedef)
			return true;
	}

	return false;
}

static void MIR_SetClippers()
{
#ifndef NO_CLIP_PLANES
	glDisable(GL_CLIP_PLANE0);
	glDisable(GL_CLIP_PLANE1);
	glDisable(GL_CLIP_PLANE2);
	glDisable(GL_CLIP_PLANE3);
	glDisable(GL_CLIP_PLANE4);
	glDisable(GL_CLIP_PLANE5);

	if (num_active_mirrors == 0)
		return;

	// setup planes for left and right sides of innermost mirror.
	// Angle clipping has ensured that for multiple mirrors all
	// later mirrors are limited to the earlier mirrors.

	mirror_info_t& inner = active_mirrors[num_active_mirrors-1];

	GLdouble  left_p[4];
	GLdouble right_p[4];

	ClipPlaneEyeAngle(left_p,  inner.def->left);
	ClipPlaneEyeAngle(right_p, inner.def->right + ANG180);

  	glEnable(GL_CLIP_PLANE0);
   	glEnable(GL_CLIP_PLANE1);

	glClipPlane(GL_CLIP_PLANE0, left_p);
  	glClipPlane(GL_CLIP_PLANE1, right_p);


	// now for each mirror, setup a clip plane that removes
	// everything that gets projected in front of that mirror.

	for (int i=0; i < num_active_mirrors; i++)
	{
		mirror_info_t& mir = active_mirrors[i];

		vec2_t v1, v2;

		v1.Set(mir.def->seg->v1->x, mir.def->seg->v1->y);
		v2.Set(mir.def->seg->v2->x, mir.def->seg->v2->y);

		for (int k = i-1; k >= 0; k--)
		{
			if (! active_mirrors[k].def->is_portal)
			{
				vec2_t tmp; tmp = v1; v1 = v2; v2 = tmp;
			}

			active_mirrors[k].Transform(v1.x, v1.y);
			active_mirrors[k].Transform(v2.x, v2.y);
		}

		GLdouble front_p[4];

		ClipPlaneHorizontalLine(front_p, v2, v1);

		glEnable(GL_CLIP_PLANE2 + i);

		glClipPlane(GL_CLIP_PLANE2 + i, front_p);
	}
#endif
}

static void MIR_Push(drawmirror_c *mir)
{
	SYS_ASSERT(mir);
	SYS_ASSERT(mir->seg);

	SYS_ASSERT(num_active_mirrors < MAX_MIRRORS);

	active_mirrors[num_active_mirrors].def = mir;
	active_mirrors[num_active_mirrors].Compute();

	num_active_mirrors++;

	MIR_SetClippers();
}

static void MIR_Pop()
{
	SYS_ASSERT(num_active_mirrors > 0);

	num_active_mirrors--;

	MIR_SetClippers();
}


float Slope_GetHeight(slope_plane_t *slope, float x, float y)
{
	// FIXME: precompute (store in slope_plane_t)
	float dx = slope->x2 - slope->x1;
	float dy = slope->y2 - slope->y1;

	float d_len = dx*dx + dy*dy;

	float along = ((x - slope->x1) * dx + (y - slope->y1) * dy) / d_len;

	return slope->dz1 + along * (slope->dz2 - slope->dz1);
}


static void DrawLaser(player_t *p)
{
	static int countdown = -1;
	static int last_time = -1;

	static const vec2_t octagon[7] =
	{
		{  0.0  , +1.0   },
		{  0.866, +0.5   },
		{  0.866, -0.5   },
		{  0.0  , -1.0   },
		{ -0.866, -0.5   },
		{ -0.866, +0.5   },
		{  0.0  , +1.0   }
	};

	if (countdown < 0)
	{
		if (! p->attackdown[0])
			return;

		countdown = 13;
	}

	if (countdown == 0)
	{
		if (p->attackdown[0])
			return;

		countdown = -1;
		return;
	}

	vec3_t s, e;

	s.Set(p->mo->x, p->mo->y, p->mo->z + 30.0);
	e.Set(p->mo->x, p->mo->y, p->mo->z + 30.0);

	float dist = 2000;

	float lk_cos = M_Cos(viewvertangle);
	float lk_sin = M_Sin(viewvertangle);

	s.x += viewsin * 6;
	s.y -= viewcos * 6;

	// view vector
	float vx = lk_cos * viewcos;
	float vy = lk_cos * viewsin;
	float vz = lk_sin;

	s.x += vx * 1;
	s.y += vy * 1;
	s.z += vz * 1;

	e.x += vx * dist;
	e.y += vy * dist;
	e.z += vz * dist;

	RGL_StartUnits(false);

	for (int pass=0; pass < 2; pass++)
	{
	local_gl_vert_t *glvert = RGL_BeginUnit(GL_TRIANGLE_STRIP, 14,
			ENV_NONE, 0, ENV_NONE, 0, pass,
			BL_CullBack | (pass?BL_Alpha:0));

	for (int kk=0; kk < 14; kk++)
	{
		memset(glvert+kk, 0, sizeof(local_gl_vert_t));

		float ity = 1.0 - fabs(countdown-7)/7.0;
		ity = sqrt(ity);

		glvert[kk].rgba[0] = 1.0 * ity;
		glvert[kk].rgba[1] = 0.1 * ity;
		glvert[kk].rgba[2] = 0.1 * ity;
		glvert[kk].rgba[3] = pass ? 0.3 : 0;

		glvert[kk].pos = ((kk & 1) == 0) ? s : e;

		float size = 4;
		if (pass==1) size += ((kk & 1) == 0) ? 1 : sqrt(dist);

		glvert[kk].pos.x += octagon[kk/2].x * size;
		glvert[kk].pos.z += octagon[kk/2].y * size;

		glvert[kk].edge = true;
	}

	RGL_EndUnit(14);
	}

	RGL_FinishUnits();

	if (leveltime != last_time)
	{
		last_time = leveltime;
		countdown--;
	}
}



typedef struct wall_plane_data_s
{
	vec3_t normal;
	divline_t div;

	int light;
	float R, G, B;
	int col[3];
	float trans;

	int cmx;

	drawthing_t *dlights;
	const image_c *image;

	float tx, tdx;
	float ty, ty_mul, ty_skew;


	vec2_t x_mat, y_mat;

	bool flood_emu;
	float emu_mx, emu_my;
}
wall_plane_data_t;


#if 0
static inline void Color_Std(local_gl_vert_t *v, int R, int G, int B, float alpha)
{
	v->col[0] = R / 255.0;
	v->col[1] = G / 255.0;
	v->col[2] = B / 255.0;
	v->col[3] = alpha;
}

static inline void Color_Rainbow(local_gl_vert_t *v, int R, int G, int B, float alpha)
{
	v->col[0] = MIN(1.0, R * ren_red_mul / 255.0);
	v->col[1] = MIN(1.0, G * ren_grn_mul / 255.0);
	v->col[2] = MIN(1.0, B * ren_blu_mul / 255.0);
	v->col[3] = alpha;
}

static inline void Color_Dimmed(local_gl_vert_t *v, int R, int G, int B, float mul)
{
	v->col[0] = mul * R / 255.0;
	v->col[1] = mul * G / 255.0;
	v->col[2] = mul * B / 255.0;
	v->col[3] = 1.0;
}

static inline void Color_White(local_gl_vert_t *v)
{
	v->col[0] = 1.0;
	v->col[1] = 1.0;
	v->col[2] = 1.0;
	v->col[3] = 1.0;
}

static inline void Color_Black(local_gl_vert_t *v)
{
	v->col[0] = 0.0;
	v->col[1] = 0.0;
	v->col[2] = 0.0;
	v->col[3] = 1.0;
}

static inline void Vertex_Std(local_gl_vert_t *v, const vec3_t *src, GLboolean edge)
{
	v->x = src->x;
	v->y = src->y;
	v->z = src->z;

	v->edge = edge;
}

static inline void Normal_Std(local_gl_vert_t *v, float nx, float ny, float nz)
{
	v->nx = nx;
	v->ny = ny;
	v->nz = nz;
}

static inline void TexCoord_Wall(local_gl_vert_t *v, int t,
		const divline_t *div, float tx0, float ty0,
		float tx_mul, float ty_mul)
{
	float along;

	if (fabs(div->dx) > fabs(div->dy))
	{
		SYS_ASSERT(0 != div->dx);
		along = (v->x - div->x) / div->dx;
	}
	else
	{
		SYS_ASSERT(0 != div->dy);
		along = (v->y - div->y) / div->dy;
	}

	v->s[t] = tx0 + along * tx_mul;
	v->t[t] = ty0 + v->z  * ty_mul;
}

static inline void TexCoord_Plane(local_gl_vert_t *v, int t,
		float tx0, float ty0, float image_w, float image_h,
		const vec2_t *x_mat, const vec2_t *y_mat)
{
	float rx = (tx0 + v->x) / image_w;
	float ry = (ty0 + v->y) / image_h;

	v->s[t] = rx * x_mat->x + ry * x_mat->y;
	v->t[t] = rx * y_mat->x + ry * y_mat->y;
}


static inline void TexCoord_Shadow(local_gl_vert_t *v, int t)
{
//#if 0
	float rx = (v->x + data->tx);
	float ry = (v->y + data->ty);

	v->s[t] = rx * data->x_mat.x + ry * data->x_mat.y;
	v->t[t] = rx * data->y_mat.x + ry * data->y_mat.y;
//#endif
}

static inline void TexCoord_FloorGlow(local_gl_vert_t *v, int t, float f_h)
{
	v->s[t] = 0.5;
	v->t[t] = (v->z - f_h) / 64.0;
}

static inline void TexCoord_CeilGlow(local_gl_vert_t *v, int t, float c_h)
{
	v->s[t] = 0.5;
	v->t[t] = (c_h - v->z) / 64.0;
}

static inline void TexCoord_WallGlow(local_gl_vert_t *v, int t,
		float x1, float y1, float nx, float ny)
{
	float dist = (v->x - x1) * nx + (v->y - y1) * ny;

	v->s[t] = 0.5;
	v->t[t] = dist / 192.0;
}

static inline void TexCoord_WallLight(local_gl_vert_t *v, int t)
{
}

static inline void TexCoord_PlaneLight(local_gl_vert_t *v, int t)
{
}
#endif

// Adapted from Quake 3 GPL release - Dasho (not used yet, but might be for future effects)
void CalcScrollTexCoords( float x_scroll, float y_scroll, vec2_t *texc )
{
	int i;
	float timeScale = leveltime / 100.0f;
	float adjustedScrollS, adjustedScrollT;

	adjustedScrollS = x_scroll * timeScale;
	adjustedScrollT = y_scroll * timeScale;

	// clamp so coordinates don't continuously get larger
	adjustedScrollS = adjustedScrollS - floor( adjustedScrollS );
	adjustedScrollT = adjustedScrollT - floor( adjustedScrollT );

	texc->x += adjustedScrollS;
	texc->y += adjustedScrollT;
}

// Adapted from Quake 3 GPL release - Dasho
void CalcTurbulentTexCoords( vec2_t *texc, vec3_t *pos )
{
	float now;
	float phase = 0;
	float frequency = thick_liquid ? 0.5 : 1.0;
	float amplitude = 0.05;

	now = ( phase + leveltime / 100.0f * frequency );

	if (swirling_flats == SWIRL_PARALLAX)
	{
		frequency *= 2;
		if (thick_liquid)
		{
			if (swirl_pass == 1)
			{
				texc->x = texc->x + r_sintable[(int)(((pos->x + pos->z) * 1.0/128 * 0.125 + now) * FUNCTABLE_SIZE) & (FUNCTABLE_MASK)] * amplitude;
				texc->y = texc->y + r_sintable[(int)((pos->y * 1.0/128 * 0.125 + now) * FUNCTABLE_SIZE) & (FUNCTABLE_MASK) ] * amplitude;
			}
			else
			{
				amplitude = 0;
				texc->x = texc->x - r_sintable[(int)(((pos->x + pos->z)* 1.0/128 * 0.125 + now) * FUNCTABLE_SIZE) & (FUNCTABLE_MASK)] * amplitude;
				texc->y = texc->y - r_sintable[(int)((pos->y * 1.0/128 * 0.125 + now) * FUNCTABLE_SIZE) & (FUNCTABLE_MASK)] * amplitude;
			}
		}
		else
		{
			if (swirl_pass == 1)
			{
				amplitude = 0.025;
				texc->x = texc->x + r_sintable[(int)(((pos->x + pos->z) * 1.0/128 * 0.125 + now) * FUNCTABLE_SIZE) & (FUNCTABLE_MASK)] * amplitude;
				texc->y = texc->y + r_sintable[(int)((pos->y * 1.0/128 * 0.125 + now) * FUNCTABLE_SIZE) & (FUNCTABLE_MASK) ] * amplitude;
			}
			else
			{
				amplitude = 0.015;
				texc->x = texc->x - r_sintable[(int)(((pos->x + pos->z)* 1.0/128 * 0.125 + now) * FUNCTABLE_SIZE) & (FUNCTABLE_MASK)] * amplitude;
				texc->y = texc->y - r_sintable[(int)((pos->y * 1.0/128 * 0.125 + now) * FUNCTABLE_SIZE) & (FUNCTABLE_MASK)] * amplitude;
			}
		}
	}
	else
	{
		texc->x = texc->x + r_sintable[(int)(((pos->x + pos->z)* 1.0/128 * 0.125 + now) * FUNCTABLE_SIZE) & (FUNCTABLE_MASK)] * amplitude;
		texc->y = texc->y + r_sintable[(int)((pos->y * 1.0/128 * 0.125 + now) * FUNCTABLE_SIZE) & (FUNCTABLE_MASK) ] * amplitude;
	}
}

typedef struct
{
	int v_count;
	const vec3_t *vert;

	GLuint tex_id;

	int pass;
	int blending;

	float R, G, B;
	float trans;

	divline_t div;

	float tx0, ty0;
	float tx_mul, ty_mul;

	vec3_t normal;

	bool mid_masked;
}
wall_coord_data_t;


static void WallCoordFunc(void *d, int v_idx,
		vec3_t *pos, float *rgb, vec2_t *texc,
		vec3_t *normal, vec3_t *lit_pos)
{
	const wall_coord_data_t *data = (wall_coord_data_t *)d;

	*pos    = data->vert[v_idx];
	*normal = data->normal;

	if (swirl_pass > 1)
	{
		rgb[0] = 1.0 / data->R;
		rgb[1] = 1.0 / data->G;
		rgb[2] = 1.0 / data->B;
	}
	else
	{
		rgb[0] = data->R;
		rgb[1] = data->G;
		rgb[2] = data->B;		
	}

	float along;

	// compute texture coord
	if (fabs(data->div.dx) > fabs(data->div.dy))
	{
		along = (pos->x - data->div.x) / data->div.dx;
	}
	else
	{
		along = (pos->y - data->div.y) / data->div.dy;
	}

	texc->x = data->tx0 + along  * data->tx_mul;
	texc->y = data->ty0 + pos->z * data->ty_mul;


	if (swirl_pass > 0)
		CalcTurbulentTexCoords(texc, pos);
	*lit_pos = *pos;
}


typedef struct
{
	int v_count;
	const vec3_t *vert;

	GLuint tex_id;

    drawthing_t *dlights;
	int pass;
	int blending;

	float R, G, B;
	float trans;

	float tx0, ty0;
	float image_w, image_h;

	vec2_t x_mat;
	vec2_t y_mat;

	vec3_t normal;

	slope_plane_t *slope;
}
plane_coord_data_t;


static void PlaneCoordFunc(void *d, int v_idx,
		vec3_t *pos, float *rgb, vec2_t *texc,
		vec3_t *normal, vec3_t *lit_pos)
{
	const plane_coord_data_t *data = (plane_coord_data_t *)d;

	*pos    = data->vert[v_idx];
	*normal = data->normal;

	if (swirl_pass > 1)
	{
		rgb[0] = 1.0 / data->R;
		rgb[1] = 1.0 / data->G;
		rgb[2] = 1.0 / data->B;
	}
	else
	{
		rgb[0] = data->R;
		rgb[1] = data->G;
		rgb[2] = data->B;		
	}

	float rx = (data->tx0 + pos->x) / data->image_w;
	float ry = (data->ty0 + pos->y) / data->image_h;

	texc->x = rx * data->x_mat.x + ry * data->x_mat.y;
	texc->y = rx * data->y_mat.x + ry * data->y_mat.y;

	if (swirl_pass > 0)
		CalcTurbulentTexCoords(texc, pos);
	*lit_pos = *pos;
}


static void DLIT_Wall(mobj_t *mo, void *dataptr)
{
	wall_coord_data_t *data = (wall_coord_data_t *)dataptr;

	// light behind the plane ?
	if (! mo->info->dlight[0].leaky && ! data->mid_masked)
	{
		float mx = mo->x;
		float my = mo->y;

		MIR_Coordinate(mx, my);

		float dist = (mx - data->div.x) * data->div.dy -
					 (my - data->div.y) * data->div.dx;

		if (dist < 0)
			return;
	}

	R_ColorMapUpdate(NULL, cur_sub->sector->lightcolor, cur_sub->sector->desaturation); // FIXME: shouldn't this use colormaps properly too?

	SYS_ASSERT(mo->dlight.shader);

	int blending = (data->blending & ~BL_Alpha) | BL_Add;

	mo->dlight.shader->WorldMix(GL_POLYGON, data->v_count, data->tex_id,
			data->trans, &data->pass, blending, data->mid_masked,
			data, WallCoordFunc);
}

static void GLOWLIT_Wall(mobj_t *mo, void *dataptr)
{
	wall_coord_data_t *data = (wall_coord_data_t *)dataptr;

	R_ColorMapUpdate(NULL, cur_sub->sector->lightcolor, cur_sub->sector->desaturation); // FIXME: shouldn't this use colormaps properly too?

	SYS_ASSERT(mo->dlight.shader);

	int blending = (data->blending & ~BL_Alpha) | BL_Add;

	mo->dlight.shader->WorldMix(GL_POLYGON, data->v_count, data->tex_id,
			data->trans, &data->pass, blending, data->mid_masked,
			data, WallCoordFunc);
}


static void DLIT_Plane(mobj_t *mo, void *dataptr)
{
	plane_coord_data_t *data = (plane_coord_data_t *)dataptr;

	// light behind the plane ?
	if (! mo->info->dlight[0].leaky)
	{
		float z = data->vert[0].z;

		if (data->slope)
			z += Slope_GetHeight(data->slope, mo->x, mo->y);

		if ((MO_MIDZ(mo) > z) != (data->normal.z > 0))
			return;
	}

	// NOTE: distance already checked in P_DynamicLightIterator

	R_ColorMapUpdate(NULL, cur_sub->sector->lightcolor, cur_sub->sector->desaturation); // FIXME: shouldn't this use colormaps properly too?

	SYS_ASSERT(mo->dlight.shader);

	int blending = (data->blending & ~BL_Alpha) | BL_Add;

	mo->dlight.shader->WorldMix(GL_POLYGON, data->v_count, data->tex_id,
			data->trans, &data->pass, blending, false /* masked */,
			data, PlaneCoordFunc);
}

static void GLOWLIT_Plane(mobj_t *mo, void *dataptr)
{
	plane_coord_data_t *data = (plane_coord_data_t *)dataptr;

	R_ColorMapUpdate(NULL, cur_sub->sector->lightcolor, cur_sub->sector->desaturation); // FIXME: shouldn't this use colormaps properly too?

	SYS_ASSERT(mo->dlight.shader);

	int blending = (data->blending & ~BL_Alpha) | BL_Add;

	mo->dlight.shader->WorldMix(GL_POLYGON, data->v_count, data->tex_id,
			data->trans, &data->pass, blending, false,
			data, PlaneCoordFunc);
}


#define MAX_EDGE_VERT  20



static inline void GreetNeighbourSector(float *hts, int& num,
		vertex_seclist_t *seclist)
{
	if (! seclist)
		return;

	for (int k=0; k < (seclist->num * 2); k++)
	{
		//TODO LERP ME
		sector_t *sec = sectors + seclist->sec[k/2];

		//float h = (k & 1) ? sec->c_h : sec->f_h;
		float h = (k & 1) ? SecLerpedCeil(sec) : SecLerpedFloor(sec);

		// does not intersect current height range?
		if (h <= hts[0]+0.1 || h >= hts[num-1]-0.1)
			continue;

		// check if new height already present, and at same time
		// find place to insert new height.

		int pos;

		for (pos = 1; pos < num; pos++)
		{
			if (h < hts[pos] - 0.1)
				break;

			if (h < hts[pos] + 0.1)
			{
				pos = -1; // already present
				break;
			}
		}

		if (pos > 0 && pos < num)
		{
			for (int i = num; i > pos; i--)
				hts[i] = hts[i-1];

			hts[pos] = h;

			num++;

			if (num >= MAX_EDGE_VERT)
				return;
		}
	}
}


typedef enum
{
	WTILF_IsExtra  = (1 << 0),
	WTILF_ExtraX   = (1 << 1),  // side of an extrafloor
	WTILF_ExtraY   = (1 << 2),  //

	WTILF_MidMask  = (1 << 4),  // the mid-masked part (gratings etc)

}
wall_tile_flag_e;


static void DrawWallPart(drawfloor_t *dfloor,
		                 float x1, float y1, float lz1, float lz2,
						 float x2, float y2, float rz1, float rz2,
		                 float tex_top_h, surface_t *surf,
						 const image_c *image,
						 bool mid_masked, bool opaque,
   						 float tex_x1, float tex_x2,
						 region_properties_t *props = NULL)
{
	// Note: tex_x1 and tex_x2 are in world coordinates.
	//       top, bottom and tex_top_h as well.

	if (! props)
		props = surf->override_p ? surf->override_p : dfloor->props;

	float trans = surf->translucency;

	SYS_ASSERT(image);

	// (need to load the image to know the opacity)
	GLuint tex_id = W_ImageCache(image, true, ren_fx_colmap);

	// ignore non-solid walls in solid mode (& vice versa)
	if ((trans < 0.99f || image->opacity >= OPAC_Masked) == solid_mode)
		return;


	// must determine bbox _before_ mirror flipping
	float v_bbox[4];

	M_ClearBox(v_bbox);
	M_AddToBox(v_bbox, x1, y1);
	M_AddToBox(v_bbox, x2, y2);

	MIR_Coordinate(x1, y1);
	MIR_Coordinate(x2, y2);

#if 0
	// CW: Seems to be handled by MIR_Coordinate?
	if (MIR_Reflective())
	{
		float tmp_x = x1; x1 = x2; x2 = tmp_x;
		float tmp_y = y1; y1 = y2; y2 = tmp_y;

   		tmp_x = tex_x1; tex_x1 = tex_x2; tex_x2 = tmp_x;
	}
#endif

	SYS_ASSERT(currmap);

	int lit_adjust = 0;

	// do the N/S/W/E bizzo...
	if (currmap->episode->lighting == LMODEL_Doom && props->lightlevel > 0)
	{
		if (cur_seg->v1->y == cur_seg->v2->y)
			lit_adjust -= 16;
		else if (cur_seg->v1->x == cur_seg->v2->x)
			lit_adjust += 16;
	}

	if (cur_seg->sidedef->lit_abs)
		lit_adjust += cur_seg->sidedef->light - props->lightlevel;
	else
		lit_adjust += cur_seg->sidedef->light;


	float total_w = IM_TOTAL_WIDTH(image);
	float total_h = IM_TOTAL_HEIGHT(image);

	/* convert tex_x1 and tex_x2 from world coords to texture coords */
	tex_x1 = (tex_x1 * surf->x_mat.x) / total_w;
	tex_x2 = (tex_x2 * surf->x_mat.x) / total_w;


	float tx0    = tex_x1;
	float tx_mul = tex_x2 - tex_x1;

	MIR_Height(tex_top_h);

	float ty_mul = surf->y_mat.y / (total_h * MIR_ZScale());
	float ty0    = IM_TOP(image) - tex_top_h * ty_mul;

#if (DEBUG >= 3)
	L_WriteDebug( "WALL (%d,%d,%d) -> (%d,%d,%d)\n",
		(int) x1, (int) y1, (int) top, (int) x2, (int) y2, (int) bottom);
#endif


	// -AJA- 2007/08/07: ugly code here ensures polygon edges
	//       match up with adjacent linedefs (otherwise small
	//       gaps can appear which look bad).

	float  left_h[MAX_EDGE_VERT]; int  left_num=2;
	float right_h[MAX_EDGE_VERT]; int right_num=2;

	left_h[0]  = lz1; left_h[1]  = lz2;
	right_h[0] = rz1; right_h[1] = rz2;

	if (solid_mode && !mid_masked)
	{
		GreetNeighbourSector(left_h,  left_num,  cur_seg->nb_sec[0]);
		GreetNeighbourSector(right_h, right_num, cur_seg->nb_sec[1]);

#if DEBUG_GREET_NEIGHBOUR
		SYS_ASSERT(left_num  <= MAX_EDGE_VERT);
		SYS_ASSERT(right_num <= MAX_EDGE_VERT);

		for (int k = 0; k < MAX_EDGE_VERT; k++)
		{
			if (k+1 < left_num)
			{
				SYS_ASSERT(left_h[k]  <= left_h[k+1]);
			}
			if (k+1 < right_num)
			{
				SYS_ASSERT(right_h[k] <= right_h[k+1]);
			}
		}
#endif
	}

	vec3_t vertices[MAX_EDGE_VERT * 2];

	int v_count = 0;

	for (int LI = 0; LI < left_num; LI++)
	{
		vertices[v_count].x = x1;
		vertices[v_count].y = y1;
		vertices[v_count].z = left_h[LI];

		MIR_Height(vertices[v_count].z);

		v_count++;
	}

	for (int RI = right_num-1; RI >= 0; RI--)
	{
		vertices[v_count].x = x2;
		vertices[v_count].y = y2;
		vertices[v_count].z = right_h[RI];

		MIR_Height(vertices[v_count].z);

		v_count++;
	}


	int blending;

	if (trans >= 0.99f && image->opacity == OPAC_Solid)
		blending = BL_NONE;
	else if (trans < 0.11f || image->opacity == OPAC_Complex)
		blending = BL_Masked;
	else
		blending = BL_Less;

	if (r_oldblend > 0)
	{
		if (trans < 0.99f || image->opacity == OPAC_Complex)
			blending |= BL_Alpha;
	}
	else
	{
		if (trans < 0.99f || image->opacity != OPAC_Solid)
			blending |= BL_Alpha;
	}

	// -AJA- 2006-06-22: fix for midmask wrapping bug
	if (mid_masked)
		blending |= BL_ClampY;


	wall_coord_data_t data;

	data.v_count = v_count;
	data.vert = vertices;

	data.R = data.G = data.B = 1.0f;

	data.div.x  = x1;
	data.div.y  = y1;
	data.div.dx = x2 - x1;
	data.div.dy = y2 - y1;

	data.tx0 = tx0;
	data.ty0 = ty0;
	data.tx_mul = tx_mul;
	data.ty_mul = ty_mul;

	// TODO: make a unit vector
	data.normal.Set( (y2-y1), (x1-x2), 0 );

	data.tex_id = tex_id;
	data.pass   = 0;
	data.blending = blending;
	data.trans = trans;
	data.mid_masked = mid_masked;
	
if (surf->image->liquid_type == LIQ_Thick)
		thick_liquid = true;
	else
		thick_liquid = false;

	if (surf->image->liquid_type > LIQ_None && swirling_flats > SWIRL_SMMU)
		swirl_pass = 1;

	abstract_shader_c *cmap_shader = R_GetColormapShader(props, lit_adjust);

	R_ColorMapUpdate(cmap_shader, cur_sub->sector->lightcolor, cur_sub->sector->desaturation);

	cmap_shader->WorldMix(GL_POLYGON, data.v_count, data.tex_id,
			trans, &data.pass, data.blending, data.mid_masked,
			&data, WallCoordFunc);

	if (surf->image->liquid_type > LIQ_None && swirling_flats == SWIRL_PARALLAX) // Kept as an example for future effects
	{
		data.tx0 = surf->offset.x + 25;
		data.ty0 = surf->offset.y + 25;
		swirl_pass = 2;
		data.blending = BL_Masked | BL_Alpha;
		data.trans = 0.33f;
		trans = 0.33f;
		cmap_shader->WorldMix(GL_POLYGON, data.v_count, data.tex_id,
					trans, &data.pass, data.blending, false,
					&data, WallCoordFunc);
	}
	
	if (use_dlights && ren_extralight < 250)
	{
		float bottom = MIN(lz1, rz1);
		float top    = MAX(lz2, rz2);

		P_DynamicLightIterator(v_bbox[BOXLEFT],  v_bbox[BOXBOTTOM], bottom,
							   v_bbox[BOXRIGHT], v_bbox[BOXTOP],    top,
							   DLIT_Wall, &data);

		P_SectorGlowIterator(cur_seg->frontsector,
				             v_bbox[BOXLEFT],  v_bbox[BOXBOTTOM], bottom,
							 v_bbox[BOXRIGHT], v_bbox[BOXTOP],    top,
							 GLOWLIT_Wall, &data);
	}
	swirl_pass = 0;
}

static void DrawSlidingDoor(drawfloor_t *dfloor, float c, float f,
						    float tex_top_h, surface_t *surf,
						    bool opaque, float x_offset)
{

	//TODO does it matter that this isn't interpolated?

	/* smov may be NULL */
	slider_move_t *smov = cur_seg->linedef->slider_move;

	float opening = smov ? smov->opening : 0;

	line_t *ld = cur_seg->linedef;

/// float im_width = IM_WIDTH(wt->surface->image);

	int num_parts = 1;
	if (cur_seg->linedef->slide_door->s.type == SLIDE_Center)
		num_parts = 2;

	// extent of current seg along the linedef
	float s_seg, e_seg;

	if (cur_seg->side == 0)
	{
		s_seg = cur_seg->offset;
		e_seg = s_seg + cur_seg->length;
	}
	else
	{
		e_seg = ld->length - cur_seg->offset;
		s_seg = e_seg - cur_seg->length;
	}

	for (int part = 0; part < num_parts; part++)
	{
		// coordinates along the linedef (0.00 at V1, 1.00 at V2)
		float s_along, s_tex;
		float e_along, e_tex;

		switch (cur_seg->linedef->slide_door->s.type)
		{
			case SLIDE_Left:
				s_along = 0;
				e_along = ld->length - opening;

				s_tex   = -e_along;
				e_tex   = 0;
				break;

			case SLIDE_Right:
				s_along = opening;
				e_along = ld->length;

				s_tex   = 0;
				e_tex   = e_along - s_along;
				break;

			case SLIDE_Center:
				if (part == 0)
				{
					s_along = 0;
					e_along = (ld->length - opening) / 2;

					e_tex   = ld->length / 2;
					s_tex   = e_tex - (e_along - s_along);
				}
				else
				{
					s_along = (ld->length + opening) / 2;
					e_along = ld->length;

					s_tex   = ld->length / 2;
					e_tex   = s_tex + (e_along - s_along);
				}
				break;

			default:
				I_Error("INTERNAL ERROR: unknown slidemove type!\n");
				return; /* NOT REACHED */
		}

		// limit sliding door coordinates to current seg
		if (s_along < s_seg)
		{
			s_tex  += (s_seg - s_along);
			s_along = s_seg;
		}
		if (e_along > e_seg)
		{
			e_tex  += (e_seg - e_along);
			e_along = e_seg;
		}

		if (s_along >= e_along)
			continue;

		float x1 = ld->v1->x + ld->dx * s_along / ld->length;
		float y1 = ld->v1->y + ld->dy * s_along / ld->length;

		float x2 = ld->v1->x + ld->dx * e_along / ld->length;
		float y2 = ld->v1->y + ld->dy * e_along / ld->length;

		s_tex += x_offset;
		e_tex += x_offset;

		DrawWallPart(dfloor, x1,y1,f,c, x2,y2,f,c, tex_top_h,
		             surf, surf->image, true, opaque, s_tex, e_tex);
	}
}


static void DrawTile(seg_t *seg, drawfloor_t *dfloor,
                    float lz1, float lz2, float rz1, float rz2,
                    float tex_z, int flags, surface_t *surf)
{
	// tex_z = texturing top, in world coordinates

	const image_c *image = surf->image;

	if (! image)
		image = W_ImageForHOMDetect();

	float tex_top_h = tex_z + surf->offset.y;
	float x_offset  = surf->offset.x;

	if (flags & WTILF_ExtraX)
	{
		x_offset += seg->sidedef->middle.offset.x;
	}
	if (flags & WTILF_ExtraY)
	{
		// needed separate Y flag to maintain compatibility
		tex_top_h += seg->sidedef->middle.offset.y;
	}

	bool opaque = (! seg->backsector) ||
		(surf->translucency >= 0.99f &&
		 image->opacity == OPAC_Solid);

	// check for horizontal sliders
	if ((flags & WTILF_MidMask) && seg->linedef->slide_door)
	{
		if (surf->image)
			DrawSlidingDoor(dfloor, lz2, lz1, tex_top_h, surf, opaque, x_offset);
		return;
	}

	float x1 = seg->v1->x;
	float y1 = seg->v1->y;
	float x2 = seg->v2->x;
	float y2 = seg->v2->y;

	float tex_x1 = seg->offset;
	float tex_x2 = tex_x1 + seg->length;

	tex_x1 += x_offset;
	tex_x2 += x_offset;

	DrawWallPart(dfloor,
		x1,y1, lz1,lz2,
		x2,y2, rz1,rz2, tex_top_h,
		surf, image, (flags & WTILF_MidMask) ? true : false,
		opaque, tex_x1, tex_x2, (flags & WTILF_MidMask) ?
		&seg->sidedef->sector->props : NULL);
}


static inline void AddWallTile( seg_t *seg, drawfloor_t *dfloor,
 							   surface_t *surf,
                               float z1, float z2,
							   float tex_z, int flags,
							   float f_min, float c_max)
{
	z1 = MAX(f_min, z1);
	z2 = MIN(c_max, z2);

	if (z1 >= z2 - 0.01)
		return;

	DrawTile(seg, dfloor, z1,z2, z1,z2, tex_z, flags, surf);
}

static inline void AddWallTile2( seg_t *seg, drawfloor_t *dfloor,
                                surface_t *surf,
                                float lz1, float lz2, float rz1, float rz2,
							    float tex_z, int flags)
{
	DrawTile(seg, dfloor, lz1,lz2, rz1,rz2, tex_z, flags, surf);
}

#define IM_HEIGHT_SAFE(im)  ((im) ? IM_HEIGHT(im) : 0)

static void ComputeWallTiles(seg_t *seg, drawfloor_t *dfloor, int sidenum, float f_min, float c_max)
{
	line_t *ld = seg->linedef;
	side_t *sd = ld->side[sidenum];
	sector_t *sec, *other;
	surface_t *surf;

	extrafloor_t *S, *L, *C;
	float floor_h;
	float tex_z;

	bool lower_invis = false;
	bool upper_invis = false;


	if (! sd)
		return;

	sec = sd->sector;
	other = sidenum ? ld->frontsector : ld->backsector;

	float secfh = SecLerpedFloor(sec);
	float secch = SecLerpedCeil(sec);

	if (! other)
	{
		if (! sd->middle.image && ! debug_hom)
			return;

		if (sec->f_slope || sec->c_slope)
		{
			float lz1 = secch + (sec->c_slope ? Slope_GetHeight(sec->c_slope, seg->v1->x, seg->v1->y) : 0);
			float rz1 = secch + (sec->c_slope ? Slope_GetHeight(sec->c_slope, seg->v2->x, seg->v2->y) : 0);

			float lz2 = secfh + (sec->f_slope ? Slope_GetHeight(sec->f_slope, seg->v1->x, seg->v1->y) : 0);
			float rz2 = secfh + (sec->f_slope ? Slope_GetHeight(sec->f_slope, seg->v2->x, seg->v2->y) : 0);

			AddWallTile2(seg, dfloor,
				&sd->middle, lz1, lz2, rz1, rz2,
				(ld->flags & MLF_LowerUnpegged) ?
				secfh + IM_HEIGHT_SAFE(sd->middle.image) : secch, 0);
			return;
		}
		AddWallTile(seg, dfloor,
			&sd->middle, secfh, secch,
			(ld->flags & MLF_LowerUnpegged) ?
			secfh + IM_HEIGHT_SAFE(sd->middle.image) : secch,
			0, f_min, c_max);
		return;
	}

	float slope_fh = secfh;
	if (sec->f_slope)
		slope_fh += MIN(sec->f_slope->dz1, sec->f_slope->dz2);
	float slope_ch = secch;
	if (sec->c_slope)
		slope_ch += MAX(sec->c_slope->dz1, sec->c_slope->dz2);

	float otherfh = SecLerpedFloor(other);
	float otherch = SecLerpedCeil(other);

	// handle lower, upper and mid-masker

	if (slope_fh < otherfh)
	{
		if (! sd->bottom.image && ! debug_hom)
		{
			lower_invis = true;
		}
		else if (other->f_slope)
		{
			float lz1 = slope_fh;
			float rz1 = slope_fh;

			float lz2 = otherfh + Slope_GetHeight(other->f_slope, seg->v1->x, seg->v1->y);
			float rz2 = otherfh + Slope_GetHeight(other->f_slope, seg->v2->x, seg->v2->y);

			AddWallTile2(seg, dfloor,
				&sd->bottom, lz1, lz2, rz1, rz2,
				(ld->flags & MLF_LowerUnpegged) ? secch : otherfh, 0);
		}
		else
		{
			AddWallTile(seg, dfloor,
				&sd->bottom, slope_fh, otherfh,
				(ld->flags & MLF_LowerUnpegged) ? secch : otherfh,
				0, f_min, c_max);
		}
	}

	if (slope_ch > otherch &&
		! (IS_SKY(sec->ceil) && IS_SKY(other->ceil)))
	{
		if (! sd->top.image && ! debug_hom)
		{
			upper_invis = true;
		}
		else if (other->c_slope)
		{
			float lz1 = otherch + Slope_GetHeight(other->c_slope, seg->v1->x, seg->v1->y);
			float rz1 = otherch + Slope_GetHeight(other->c_slope, seg->v2->x, seg->v2->y);

			float lz2 = slope_ch;
			float rz2 = slope_ch;

			AddWallTile2(seg, dfloor,
				&sd->top, lz1, lz2, rz1, rz2,
				(ld->flags & MLF_UpperUnpegged) ? secch :
				otherch + IM_HEIGHT_SAFE(sd->top.image), 0);
		}
		else
		{
			AddWallTile(seg, dfloor,
				&sd->top, otherch, slope_ch,
				(ld->flags & MLF_UpperUnpegged) ? secch :
				otherch + IM_HEIGHT_SAFE(sd->top.image),
				0, f_min, c_max);
		}
	}

	if (sd->middle.image)
	{
		float f1 = MAX(secfh, otherfh);
		float c1 = MIN(secch, otherch);

		float f2, c2;

		if (ld->flags & MLF_LowerUnpegged)
		{
			f2 = f1 + sd->midmask_offset;
			c2 = f2 + IM_HEIGHT(sd->middle.image);
		}
		else
		{
			c2 = c1 + sd->midmask_offset;
			f2 = c2 - IM_HEIGHT(sd->middle.image);
		}

		tex_z = c2;

		// hack for transparent doors
		{
			if (lower_invis) f1 = secfh;
			if (upper_invis) c1 = secch;
		}

		// hack for "see-through" lines (same sector on both sides)
		if (sec != other)
		{
			f2 = MAX(f2, f1);
			c2 = MIN(c2, c1);
		}

		if (c2 > f2)
		{
			AddWallTile(seg, dfloor, &sd->middle, f2, c2, tex_z, WTILF_MidMask, f_min, c_max);
		}
	}

	// -- thick extrafloor sides --

	// -AJA- Don't bother drawing extrafloor sides if the front/back
	//       sectors have the same tag (and thus the same extrafloors).
	//
	//TODO test extrafloor interpolation
	if (other->tag == sec->tag)
		return;

	floor_h = otherfh;

	S = other->bottom_ef;
	L = other->bottom_liq;

	while (S || L)
	{
		if (!L || (S && ExFloorLerpedBottom(S) < ExFloorLerpedBottom(L)))
		{
			C = S;  S = S->higher;
		}
		else
		{
			C = L;  L = L->higher;
		}

		SYS_ASSERT(C);

		// ignore liquids in the middle of THICK solids, or below real
		// floor or above real ceiling
		//
		float Cbottomh = ExFloorLerpedBottom(C);
		float Ctoph = ExFloorLerpedTop(C);

		if (Cbottomh < floor_h || Cbottomh > otherch)
			continue;

		if (C->ef_info->type & EXFL_Thick)
		{
			int flags = WTILF_IsExtra;

			// -AJA- 1999/09/25: Better DDF control of side texture.
			if (C->ef_info->type & EXFL_SideUpper)
				surf = &sd->top;
			else if (C->ef_info->type & EXFL_SideLower)
				surf = &sd->bottom;
			else
			{
				surf = &C->ef_line->side[0]->middle;

				flags |= WTILF_ExtraX;

				if (C->ef_info->type & EXFL_SideMidY)
					flags |= WTILF_ExtraY;
			}

			if (! surf->image && ! debug_hom)
				continue;

			tex_z = (C->ef_line->flags & MLF_LowerUnpegged) ?
				Cbottomh + IM_HEIGHT_SAFE(surf->image) : Ctoph;

			sector_t *csec = C->ef_line->frontsector;
			if (csec->f_slope || csec->c_slope)
			{
				float lz1 = Ctoph + (csec->c_slope ? Slope_GetHeight(csec->c_slope, seg->v1->x, seg->v1->y) : 0);
				float rz1 = Ctoph + (csec->c_slope ? Slope_GetHeight(csec->c_slope, seg->v2->x, seg->v2->y) : 0);

				float lz2 = Cbottomh + (csec->f_slope ? Slope_GetHeight(csec->f_slope, seg->v1->x, seg->v1->y) : 0);
				float rz2 = Cbottomh + (csec->f_slope ? Slope_GetHeight(csec->f_slope, seg->v2->x, seg->v2->y) : 0);

				AddWallTile2(seg, dfloor, surf, lz1, lz2, rz1, rz2, tex_z, 0);
			}
			else
				AddWallTile(seg, dfloor, surf, Cbottomh, Ctoph, tex_z, flags, Cbottomh, Ctoph);
		}

		floor_h = Ctoph;
	}
}



#define MAX_FLOOD_VERT  16

typedef struct
{
	int v_count;
	vec3_t vert[2 * (MAX_FLOOD_VERT + 1)];

	GLuint tex_id;
	int pass;

	float R, G, B;

	float plane_h;

	float tx0, ty0;
	float image_w, image_h;

	vec2_t x_mat;
	vec2_t y_mat;

	vec3_t normal;

	int piece_row;
	int piece_col;

	float h1, dh;
}
flood_emu_data_t;


static void FloodCoordFunc(void *d, int v_idx,
		vec3_t *pos, float *rgb, vec2_t *texc,
		vec3_t *normal, vec3_t *lit_pos)
{
	const flood_emu_data_t *data = (flood_emu_data_t *)d;

	*pos    = data->vert[v_idx];
	*normal = data->normal;

	rgb[0] = data->R;
	rgb[1] = data->G;
	rgb[2] = data->B;

	float along = (viewz - data->plane_h) / (viewz - pos->z);

	lit_pos->x = viewx + along * (pos->x - viewx);
	lit_pos->y = viewy + along * (pos->y - viewy);
	lit_pos->z = data->plane_h;

	float rx = (data->tx0 + lit_pos->x) / data->image_w;
	float ry = (data->ty0 + lit_pos->y) / data->image_h;

	texc->x = rx * data->x_mat.x + ry * data->x_mat.y;
	texc->y = rx * data->y_mat.x + ry * data->y_mat.y;
}

static void DLIT_Flood(mobj_t *mo, void *dataptr)
{
	flood_emu_data_t *data = (flood_emu_data_t *)dataptr;

	// light behind the plane ?
	if (! mo->info->dlight[0].leaky)
	{
		if ((MO_MIDZ(mo) > data->plane_h) != (data->normal.z > 0))
			return;
	}

	// NOTE: distance already checked in P_DynamicLightIterator

	SYS_ASSERT(mo->dlight.shader);

	float sx = cur_seg->v1->x;
	float sy = cur_seg->v1->y;

	float dx = cur_seg->v2->x - sx;
	float dy = cur_seg->v2->y - sy;

	int blending = BL_Add;

	for (int row=0; row < data->piece_row; row++)
	{
		float z = data->h1 + data->dh * row / (float)data->piece_row;

		for (int col=0; col <= data->piece_col; col++)
		{
			float x = sx + dx * col / (float)data->piece_col;
			float y = sy + dy * col / (float)data->piece_col;

			data->vert[col*2 + 0].Set(x, y, z);
			data->vert[col*2 + 1].Set(x, y, z + data->dh / data->piece_row);
		}

		R_ColorMapUpdate(NULL, cur_sub->sector->lightcolor, cur_sub->sector->desaturation); // FIXME: shouldn't this use colormaps properly too?

		mo->dlight.shader->WorldMix(GL_QUAD_STRIP, data->v_count,
				data->tex_id, 1.0, &data->pass, blending, false,
				data, FloodCoordFunc);
	}
}


static void EmulateFloodPlane(const drawfloor_t *dfloor,
	const sector_t *flood_ref, int face_dir, float h1, float h2)
{
	if (num_active_mirrors > 0)
		return;

	const surface_t *surf = (face_dir > 0) ? &flood_ref->floor :
		&flood_ref->ceil;

	if (! surf->image)
		return;

	// ignore sky and invisible planes
	if (IS_SKY(*surf) || surf->translucency < 0.01f)
		return;

	// ignore transparent doors (TNT MAP02)
	if (flood_ref->f_h >= flood_ref->c_h)
		return;

	// ignore fake 3D bridges (Batman MAP03)
	if (cur_seg->linedef &&
	    cur_seg->linedef->frontsector == cur_seg->linedef->backsector)
		return;

	const region_properties_t *props = surf->override_p ?
		surf->override_p : &flood_ref->props;


	SYS_ASSERT(props);

	flood_emu_data_t data;

	data.tex_id = W_ImageCache(surf->image, true, ren_fx_colmap);
	data.pass = 0;

	data.R = data.G = data.B = 1.0f;

	data.plane_h = (face_dir > 0) ? h2 : h1;

	data.tx0 = surf->offset.x;
	data.ty0 = surf->offset.y;
	data.image_w = IM_WIDTH(surf->image);
	data.image_h = IM_HEIGHT(surf->image);

	data.x_mat = surf->x_mat;
	data.y_mat = surf->y_mat;

	data.normal.Set(0, 0, face_dir);


	// determine number of pieces to subdivide the area into.
	// The more the better, upto a limit of 64 pieces, and
	// also limiting the size of the pieces.

	float piece_w = cur_seg->length;
	float piece_h = h2 - h1;

	int piece_col = 1;
	int piece_row = 1;

	while (piece_w > 16 || piece_h > 16)
	{
		if (piece_col * piece_row >= 64)
			break;

		if (piece_col >= MAX_FLOOD_VERT && piece_row >= MAX_FLOOD_VERT)
			break;

		if (piece_w >= piece_h && piece_col < MAX_FLOOD_VERT)
		{
			piece_w /= 2.0;
			piece_col *= 2;
		}
		else
		{
			piece_h /= 2.0;
			piece_row *= 2;
		}
	}

	SYS_ASSERT(piece_col <= MAX_FLOOD_VERT);

	float sx = cur_seg->v1->x;
	float sy = cur_seg->v1->y;

	float dx = cur_seg->v2->x - sx;
	float dy = cur_seg->v2->y - sy;
	float dh = h2 - h1;

	data.piece_row = piece_row;
	data.piece_col = piece_col;
	data.h1 = h1;
	data.dh = dh;

	int lit_adjust = 0;
	if (face_dir > 0)
	{
		if (cur_sub->sector->f_lit_abs)
			lit_adjust = cur_sub->sector->f_light - props->lightlevel;
		else
			lit_adjust = cur_sub->sector->f_light;
	}
	else
	{
		if (cur_sub->sector->c_lit_abs)
			lit_adjust = cur_sub->sector->c_light - props->lightlevel;
		else
			lit_adjust = cur_sub->sector->c_light;
	}

	abstract_shader_c *cmap_shader = R_GetColormapShader(props, lit_adjust);

	data.v_count = (piece_col+1) * 2;

	for (int row=0; row < piece_row; row++)
	{
		float z = h1 + dh * row / (float)piece_row;

		for (int col=0; col <= piece_col; col++)
		{
			float x = sx + dx * col / (float)piece_col;
			float y = sy + dy * col / (float)piece_col;

			data.vert[col*2 + 0].Set(x, y, z);
			data.vert[col*2 + 1].Set(x, y, z + dh / piece_row);
		}

#if 0  // DEBUGGING AIDE
		data.R = (64 + 190 * (row & 1)) / 255.0;
		data.B = (64 + 90 * (row & 2))  / 255.0;
#endif

		R_ColorMapUpdate(cmap_shader, cur_sub->sector->lightcolor, cur_sub->sector->desaturation);

		cmap_shader->WorldMix(GL_QUAD_STRIP, data.v_count,
				data.tex_id, 1.0, &data.pass, BL_NONE, false,
				&data, FloodCoordFunc);
	}

	if (use_dlights && solid_mode && ren_extralight < 250)
	{
		// Note: dynamic lights could have been handled in the row-by-row
		//       loop above (after the cmap_shader).  However it is more
		//       efficient to handle them here, and duplicate the striping
		//       code in the DLIT_Flood function.

		float ex = cur_seg->v2->x;
		float ey = cur_seg->v2->y;

		// compute bbox for finding dlights (use 'lit_pos' coords).
		float other_h = (face_dir > 0) ? h1 : h2;

		float along = (viewz - data.plane_h) / (viewz - other_h);

		float sx2 = viewx + along * (sx - viewx);
		float sy2 = viewy + along * (sy - viewy);
		float ex2 = viewx + along * (ex - viewx);
		float ey2 = viewy + along * (ey - viewy);

		float lx1 = MIN( MIN(sx,sx2), MIN(ex,ex2) );
		float ly1 = MIN( MIN(sy,sy2), MIN(ey,ey2) );
		float lx2 = MAX( MAX(sx,sx2), MAX(ex,ex2) );
		float ly2 = MAX( MAX(sy,sy2), MAX(ey,ey2) );

//		I_Debugf("Flood BBox size: %1.0f x %1.0f\n", lx2-lx1, ly2-ly1);

		P_DynamicLightIterator(lx1,ly1,data.plane_h, lx2,ly2,data.plane_h,
				               DLIT_Flood, &data);
	}
}


static void RGL_DrawSeg(drawfloor_t *dfloor, seg_t *seg)
{
	//
	// Analyses floor/ceiling heights, and add corresponding walls/floors
	// to the drawfloor.  Returns true if the whole region was "solid".
	//
	cur_seg = seg;

	SYS_ASSERT(!seg->miniseg && seg->linedef);

	// mark the segment on the automap
	seg->linedef->flags |= MLF_Mapped;

	frontsector = seg->front_sub->sector;
	backsector  = NULL;

	if (seg->back_sub)
		backsector = seg->back_sub->sector;

	side_t *sd = seg->sidedef;

	float f_min = dfloor->is_lowest  ? -32767.0 : dfloor->f_h;
	float c_max = dfloor->is_highest ? +32767.0 : dfloor->c_h;

#if (DEBUG >= 3)
	L_WriteDebug( "   BUILD WALLS %1.1f .. %1.1f\n", f_min, c1);
#endif

	// handle TRANSLUCENT + THICK floors (a bit of a hack)
	if (dfloor->c_ef && !dfloor->is_highest &&
		(dfloor->c_ef->ef_info->type & EXFL_Thick) &&
		(dfloor->c_ef->top->translucency < 0.99f))
	{
		c_max = dfloor->c_ef->top_h;
	}


	ComputeWallTiles(seg, dfloor, seg->side, f_min, c_max);


	// -AJA- 2004/04/21: Emulate Flat-Flooding TRICK
	if (! debug_hom && solid_mode && dfloor->is_lowest &&
		sd->bottom.image == NULL && cur_seg->back_sub)
	{
		float frontf = SecLerpedFloor(cur_seg->front_sub->sector);
		float backf = SecLerpedFloor(cur_seg->back_sub->sector);
		//frontf = N_Interpolate(cur_seg->front_sub->sector->lf_h, frontf);
		//backf = N_Interpolate(cur_seg->back_sub->sector->lf_h, backf);


		if (backf > frontf && backf < viewz)
		{
			EmulateFloodPlane(dfloor, cur_seg->back_sub->sector, +1,
				frontf,	backf);
		}
	}

	if (! debug_hom && solid_mode && dfloor->is_highest &&
		sd->top.image == NULL && cur_seg->back_sub)
	{
		float frontc = SecLerpedCeil(cur_seg->front_sub->sector);
		float backc = SecLerpedCeil(cur_seg->back_sub->sector);
		//frontc = N_Interpolate(cur_seg->front_sub->sector->lc_h, frontc);
		//backc = N_Interpolate(cur_seg->back_sub->sector->lc_h, backc);
		if (backc < frontc && backc > viewz)
		{
			EmulateFloodPlane(dfloor, cur_seg->back_sub->sector, -1,
				backc, frontc);
		}
	}
}


static void RGL_WalkBSPNode(unsigned int bspnum);


static void RGL_WalkMirror(drawsub_c *dsub, seg_t *seg,
						   angle_t left, angle_t right,
						   bool is_portal)
{
	drawmirror_c *mir = R_GetDrawMirror();
	mir->Clear(seg);

	mir->left  = viewangle + left;
	mir->right = viewangle + right;
	mir->is_portal = is_portal;

	dsub->mirrors.push_back(mir);

	// push mirror (translation matrix)
	MIR_Push(mir);

	subsector_t *save_sub = cur_sub;

	angle_t save_clip_L   = clip_left;
	angle_t save_clip_R   = clip_right;
	angle_t save_scope    = clip_scope;

	clip_left  = left;
	clip_right = right;
	clip_scope = left - right;

	// perform another BSP walk
	RGL_WalkBSPNode(root_node);

	cur_sub = save_sub;

	clip_left  = save_clip_L;
	clip_right = save_clip_R;
	clip_scope = save_scope;

	// pop mirror
	MIR_Pop();
}


//
// RGL_WalkSeg
//
// Visit a single seg of the subsector, and for one-sided lines update
// the 1D occlusion buffer.
//
static void RGL_WalkSeg(drawsub_c *dsub, seg_t *seg)
{
	// ignore segs sitting on current mirror
	if (MIR_SegOnPortal(seg))
		return;

	float sx1 = seg->v1->x;
	float sy1 = seg->v1->y;

	float sx2 = seg->v2->x;
	float sy2 = seg->v2->y;

#ifdef EXPERIMENTAL_REJECT
	// Reject lines not facing viewer
	{
		float pt1x = sx1 - viewx;
		float pt1y = sy1 - viewy;
		float pt2x = sx2 - viewx;
		float pt2y = sy2 - viewy;
		if (pt1y * (pt1x - pt2x) + pt1x * (pt2y - pt1y) >= 0)
			return;
	}

	// ignore segs sitting on current mirror
	if (MIR_SegOnPortal(seg))
		return;
#endif

	// when there are active mirror planes, segs not only need to
	// be flipped across them but also clipped across them.
	if (num_active_mirrors > 0)
	{
		for (int i=num_active_mirrors-1; i >= 0; i--)
		{
			active_mirrors[i].Transform(sx1, sy1);
			active_mirrors[i].Transform(sx2, sy2);

			if (! active_mirrors[i].def->is_portal)
			{
				float tmp_x = sx1; sx1 = sx2; sx2 = tmp_x;
				float tmp_y = sy1; sy1 = sy2; sy2 = tmp_y;
			}

			seg_t *clipper = active_mirrors[i].def->seg;

			divline_t div;

			div.x  = clipper->v1->x;
			div.y  = clipper->v1->y;
			div.dx = clipper->v2->x - div.x;
			div.dy = clipper->v2->y - div.y;

			int s1 = P_PointOnDivlineSide(sx1, sy1, &div);
			int s2 = P_PointOnDivlineSide(sx2, sy2, &div);

			// seg lies completely in front of clipper?
			if (s1 == 0 && s2 == 0)
				return;

			if (s1 != s2)
			{
				// seg crosses clipper, need to split it
				float ix, iy;

				P_ComputeIntersection(&div, sx1, sy1, sx2, sy2, &ix, &iy);

				if (s2 == 0)
					sx2 = ix, sy2 = iy;
				else
					sx1 = ix, sy1 = iy;
			}
		}
	}

#ifdef ZDOOM_OCCLUSION
	angle_t angle_L, angle_R;
	if (!RGL_OcclusionTestLine(sx1, sy1, sx2, sy2, angle_L, angle_R))
		return;
#else
	// angle_L = R_PointToAngle(sx1, sy1);
	//angle_t angle_R = R_PointToAngle(sx2, sy2);
	angle_t angle_L = R_PointToAngle(viewx, viewy, sx1, sy1);
	angle_t angle_R = R_PointToAngle(viewx, viewy, sx2, sy2);

	// Clip to view edges.

	angle_t span = angle_L - angle_R;

	// back side ?
	if (span >= ANG180)
		return;

	angle_L -= viewangle;
	angle_R -= viewangle;

	if (clip_scope != ANG180)
	{
		angle_t tspan1 = angle_L - clip_right;
		angle_t tspan2 = clip_left - angle_R;

		if (tspan1 > clip_scope)
		{
			// Totally off the left edge
			if (tspan2 >= ANG180)
				return;

			angle_L = clip_left;
		}

		if (tspan2 > clip_scope)
		{
			// Totally off the left edge
			if (tspan1 >= ANG180)
				return;

			angle_R = clip_right;
		}

		span = angle_L - angle_R;
	}

	// The seg is in the view range,
	// but not necessarily visible.

#if 1
	// check if visible
	if (span > (ANG1/4) && RGL_1DOcclusionTest(angle_R, angle_L))
	{
		return;
	}
#endif
#endif

	dsub->visible = true;

#ifdef ZDOOM_OCCLUSION
	if (seg->miniseg || angle_L == angle_R)
#else
	if (seg->miniseg || span == 0)
#endif
		return;

	if (num_active_mirrors < MAX_MIRRORS)
	{
		if (seg->linedef->flags & MLF_Mirror)
		{
			RGL_WalkMirror(dsub, seg, angle_L, angle_R, false);
			RGL_1DOcclusionSet(angle_R, angle_L);
			return;
		}
		else if (seg->linedef->portal_pair)
		{
			RGL_WalkMirror(dsub, seg, angle_L, angle_R, true);
			RGL_1DOcclusionSet(angle_R, angle_L);
			return;
		}
	}

	drawseg_c *dseg = R_GetDrawSeg();
	dseg->seg = seg;

	dsub->segs.push_back(dseg);


	sector_t *frontsector = seg->front_sub->sector;
	sector_t *backsector  = NULL;

	if (seg->back_sub)
		backsector = seg->back_sub->sector;

	// only 1 sided walls affect the 1D occlusion buffer

	if (seg->linedef->blocked)
	{
		RGL_1DOcclusionSet(angle_R, angle_L);
	}

	// --- handle sky (using the depth buffer) ---

//	bool upper_sky = false;
	bool lower_sky = false;

	if (backsector && IS_SKY(frontsector->floor) && IS_SKY(backsector->floor))
		lower_sky = true;

//	if (backsector && IS_SKY(frontsector->ceil) && IS_SKY(backsector->ceil))
//		upper_sky = true;

	if (lower_sky && frontsector->f_h < backsector->f_h)
	{
		RGL_DrawSkyWall(seg, frontsector->f_h, backsector->f_h);
	}

	if (IS_SKY(frontsector->ceil))
	{
		if (frontsector->c_h < frontsector->sky_h &&
			(! backsector || ! IS_SKY(backsector->ceil) ||
			backsector->f_h >= frontsector->c_h))
		{
			RGL_DrawSkyWall(seg, frontsector->c_h, frontsector->sky_h);
		}
		else if (backsector && IS_SKY(backsector->ceil))
		{
			float max_f = MAX(frontsector->f_h, backsector->f_h);

			if (backsector->c_h <= max_f && max_f < frontsector->sky_h)
			{
				RGL_DrawSkyWall(seg, max_f, frontsector->sky_h);
			}
		}
	}
	// -AJA- 2004/08/29: Emulate Sky-Flooding TRICK
	else if (! debug_hom && backsector && IS_SKY(backsector->ceil) &&
			 seg->sidedef->top.image == NULL &&
			 backsector->c_h < frontsector->c_h)
	{
		RGL_DrawSkyWall(seg, backsector->c_h, frontsector->c_h);
	}
}

//
// RGL_CheckBBox
//
// Checks BSP node/subtree bounding box.
// Returns true if some part of the bbox might be visible.
//
// Placed here to be close to RGL_WalkSeg(), which has similiar angle
// clipping stuff in it.
//

#ifdef ZDOOM_OCCLUSION // ZDoom's clipsegs adjusted to use fake angles in range 0-180

bool RGL_OcclusionTestLine(double x1, double y1, double x2, double y2, angle_t &sx2, angle_t &sx1)
{
	x1 -= viewx;
	y1 -= viewy;
	x2 -= viewx;
	y2 -= viewy;

	// Sitting on a line?
	double EQUAL_EPSILON = (1 / 65536.);
	if (y1 * (x1 - x2) + x1 * (y2 - y1) >= -EQUAL_EPSILON)
		return true;

	double rx1 = x1 * viewsin - y1 * viewcos;
	double rx2 = x2 * viewsin - y2 * viewcos;
	double ry1 = x1 * viewcos + y1 * viewsin;
	double ry2 = x2 * viewcos + y2 * viewsin;

	if (num_active_mirrors % 2 == 1)
	{
		double t = -rx1;
		rx1 = -rx2;
		rx2 = t;
		std::swap(ry1, ry2);
	}

	if (rx1 >= -ry1)
	{
		if (rx1 > ry1) return false;	// left edge is off the right side
		if (ry1 == 0) return false;
		sx1 = (angle_t)std::round(ANG180 + rx1 * ANG90 / ry1);
	}
	else
	{
		if (rx2 < -ry2) return false;	// wall is off the left side
		if (rx1 - rx2 - ry2 + ry1 == 0) return false;	// wall does not intersect view volume
		sx1 = ANG90;
	}

	if (rx2 <= ry2)
	{
		if (rx2 < -ry2) return false;	// right edge is off the left side
		if (ry2 == 0) return false;
		sx2 = (angle_t)std::round(ANG180 + rx2 * ANG90 / ry2);
	}
	else
	{
		if (rx1 > ry1) return false;	// wall is off the right side
		if (ry2 - ry1 - rx2 + rx1 == 0) return false;	// wall does not intersect view volume
		sx2 = ANG270;
	}

	// Find the first clippost that touches the source post
	//	(adjacent pixels are touching).
	return !RGL_1DOcclusionTest(sx1, sx2);
}

bool RGL_CheckBBox(float *bspcoord)
{
	static const int checkcoord[12][4] =
	{
		{ 3,0,2,1 },
		{ 3,0,2,0 },
		{ 3,1,2,0 },
		{ 0 },
		{ 2,0,2,1 },
		{ 0,0,0,0 },
		{ 3,1,3,0 },
		{ 0 },
		{ 2,0,3,1 },
		{ 2,1,3,1 },
		{ 2,1,3,0 }
	};

	int 				boxx;
	int 				boxy;
	int 				boxpos;

	double	 			x1, y1, x2, y2;
	angle_t				sx1, sx2;

	// Find the corners of the box
	// that define the edges from current viewpoint.
	if (viewx <= bspcoord[BOXLEFT])
		boxx = 0;
	else if (viewx < bspcoord[BOXRIGHT])
		boxx = 1;
	else
		boxx = 2;

	if (viewy >= bspcoord[BOXTOP])
		boxy = 0;
	else if (viewy > bspcoord[BOXBOTTOM])
		boxy = 1;
	else
		boxy = 2;

	boxpos = (boxy << 2) + boxx;
	if (boxpos == 5)
		return true;

	x1 = bspcoord[checkcoord[boxpos][0]];
	y1 = bspcoord[checkcoord[boxpos][1]];
	x2 = bspcoord[checkcoord[boxpos][2]];
	y2 = bspcoord[checkcoord[boxpos][3]];

	// check clip list for an open space

	return RGL_OcclusionTestLine(x1, y1, x2, y2, sx1, sx2);
}

#else
bool RGL_CheckBBox(float *bspcoord)
{
	if (num_active_mirrors > 0)
	{
		// a flipped bbox may no longer be axis aligned, hence we
		// need to find the bounding area of the transformed box.
		static float new_bbox[4];

		M_ClearBox(new_bbox);

		for (int p=0; p < 4; p++)
		{
			float tx = bspcoord[(p & 1) ? BOXLEFT   : BOXRIGHT];
			float ty = bspcoord[(p & 2) ? BOXBOTTOM : BOXTOP];

			MIR_Coordinate(tx, ty);

			M_AddToBox(new_bbox, tx, ty);
		}

		bspcoord = new_bbox;
	}

	int boxx, boxy;

	// Find the corners of the box
	// that define the edges from current viewpoint.
	if (viewx <= bspcoord[BOXLEFT])
		boxx = 0;
	else if (viewx < bspcoord[BOXRIGHT])
		boxx = 1;
	else
		boxx = 2;

	if (viewy >= bspcoord[BOXTOP])
		boxy = 0;
	else if (viewy > bspcoord[BOXBOTTOM])
		boxy = 1;
	else
		boxy = 2;

	int boxpos = (boxy << 2) + boxx;

	if (boxpos == 5)
		return true;

	float x1 = bspcoord[checkcoord[boxpos][0]];
	float y1 = bspcoord[checkcoord[boxpos][1]];
	float x2 = bspcoord[checkcoord[boxpos][2]];
	float y2 = bspcoord[checkcoord[boxpos][3]];

	// check clip list for an open space
	angle_t angle_L = R_PointToAngle(viewx, viewy, x1, y1);
	angle_t angle_R = R_PointToAngle(viewx, viewy, x2, y2);

	angle_t span = angle_L - angle_R;

	// Sitting on a line?
	if (span >= ANG180)
		return true;

	angle_L -= viewangle;
	angle_R -= viewangle;

	if (clip_scope != ANG180)
	{
		angle_t tspan1 = angle_L - clip_right;
		angle_t tspan2 = clip_left - angle_R;

		if (tspan1 > clip_scope)
		{
			// Totally off the left EDGE?
			if (tspan2 >= ANG180)
				return false;

			angle_L = clip_left;
		}

		if (tspan2 > clip_scope)
		{
			// Totally off the right EDGE?
			if (tspan1 >= ANG180)
				return false;

			angle_R = clip_right;
		}

		if (angle_L == angle_R)
			return false;
	}

	return ! RGL_1DOcclusionTest(angle_R, angle_L);
}
#endif

static void RGL_DrawPlane(drawfloor_t *dfloor, float h,
						  surface_t *surf, int face_dir)
{
	float orig_h = h;

	MIR_Height(h);

	int num_vert, i;

	if (! surf->image)
		return;

	// ignore sky
	if (IS_SKY(*surf))
		return;


	region_properties_t *props = dfloor->props;

	// more deep water hackitude
	if (cur_sub->deep_ref &&
		((face_dir > 0 && dfloor->prev_R == NULL) ||
		 (face_dir < 0 && dfloor->next_R == NULL)))
	{
		props = &cur_sub->deep_ref->props;
	}

	if (surf->override_p)
		props = surf->override_p;


	slope_plane_t *slope = NULL;

	if (face_dir > 0 && dfloor->is_lowest)
 		slope = cur_sub->sector->f_slope;

	if (face_dir < 0 && dfloor->is_highest)
 		slope = cur_sub->sector->c_slope;


	float trans = surf->translucency;

	// ignore invisible planes
	if (trans < 0.01f)
		return;

	// ignore non-facing planes
// -CW- 2017/02/14: if left in, skips extrafloor floors
//	if ((viewz > h) != (face_dir > 0) && !slope)
//		return;

	// ignore dud regions (floor >= ceiling)
	if (dfloor->f_h > dfloor->c_h && !slope)
		return;

	// ignore empty subsectors
	if (cur_sub->segs == NULL)
		return;


	// (need to load the image to know the opacity)
	GLuint tex_id = W_ImageCache(surf->image, true, ren_fx_colmap);

	// ignore non-solid planes in solid_mode (& vice versa)
	if ((trans < 0.99f || surf->image->opacity >= OPAC_Masked) == solid_mode)
		return;


	// count number of actual vertices
	seg_t *seg;
	for (seg=cur_sub->segs, num_vert=0; seg; seg=seg->sub_next, num_vert++)
	{
		/* no other code needed */
	}

	// -AJA- make sure polygon has enough vertices.  Sometimes a subsector
	// ends up with only 1 or 2 segs due to level problems (e.g. MAP22).
	if (num_vert < 3)
		return;

	if (num_vert > MAX_PLVERT)
		num_vert = MAX_PLVERT;

	vec3_t vertices[MAX_PLVERT];

	float v_bbox[4];

	M_ClearBox(v_bbox);

	int v_count = 0;

	for (seg=cur_sub->segs, i=0; seg && (i < MAX_PLVERT);
		 seg=seg->sub_next, i++)
	{
		if (v_count < MAX_PLVERT)
		{
			float x = seg->v1->x;
			float y = seg->v1->y;
			float z = h;
			int vi = seg->v1 - vertexes;

			// must do this before mirror adjustment
			M_AddToBox(v_bbox, x, y);

			if (face_dir > 0)
			{
				// floor - check vertex heights
				if (vi >= 0 && vi < numvertexes && zvertexes)
					if (zvertexes[vi].x > -1000000.0f)
						z = zvertexes[vi].x;

				// now check 3D slope
				if (dfloor->f_ef)
					slope = dfloor->f_ef->ef_line->frontsector->f_slope;
			}

			if (face_dir < 0)
			{
				// ceiling - check vertex heights
				if (vi >= 0 && vi < numvertexes && zvertexes)
					if (zvertexes[vi].y > -1000000.0f)
						z = zvertexes[vi].y;

				// now check 3D slope
				if (dfloor->c_ef)
					slope = dfloor->c_ef->ef_line->frontsector->c_slope;
			}

			// handle slope
			if (slope)
			{
				// use orig_h to prevent vertex height from affecting slope above it
				z = orig_h + Slope_GetHeight(slope, x, y);

				MIR_Height(z);
			}

			MIR_Coordinate(x, y);

			vertices[v_count].x = x;
			vertices[v_count].y = y;
			vertices[v_count].z = z;

			v_count++;
		}
	}


	int blending;

	if (trans >= 0.99f && surf->image->opacity == OPAC_Solid)
		blending = BL_NONE;
	else if (trans < 0.11f || surf->image->opacity == OPAC_Complex)
		blending = BL_Masked;
	else
		blending = BL_Less;

	if (r_oldblend > 0)
	{
		if (trans < 0.99f || surf->image->opacity == OPAC_Complex)
		{
			blending |= BL_Alpha;
		}
	}
	else if (trans < 0.99f || surf->image->opacity != OPAC_Solid)
			blending |= BL_Alpha;


	plane_coord_data_t data;

	data.v_count = v_count;
	data.vert = vertices;

	data.R = data.G = data.B = 1.0f;

	data.tx0 = surf->offset.x;
	data.ty0 = surf->offset.y;
	data.image_w = IM_WIDTH(surf->image);
	data.image_h = IM_HEIGHT(surf->image);

	data.x_mat = surf->x_mat;
	data.y_mat = surf->y_mat;

	float mir_scale = MIR_XYScale();
	data.x_mat.x /= mir_scale; data.x_mat.y /= mir_scale;
	data.y_mat.x /= mir_scale; data.y_mat.y /= mir_scale;

	data.normal.Set(0, 0, (viewz > h) ? +1 : -1);

	data.tex_id = tex_id;
	data.pass   = 0;
	data.blending = blending;
	data.trans = trans;
	data.slope = slope;
	
		if (surf->image->liquid_type == LIQ_Thick)
		thick_liquid = true;
			else
		thick_liquid = false;
	if (surf->image->liquid_type > LIQ_None && swirling_flats > SWIRL_SMMU)
		swirl_pass = 1;
		
	int lit_adjust = 0;
	if (face_dir > 0)
	{
		if (cur_sub->sector->f_lit_abs)
			lit_adjust = cur_sub->sector->f_light - props->lightlevel;
		else
			lit_adjust = cur_sub->sector->f_light;
	}
	else
	{
		if (cur_sub->sector->c_lit_abs)
			lit_adjust = cur_sub->sector->c_light - props->lightlevel;
		else
			lit_adjust = cur_sub->sector->c_light;
	}

	abstract_shader_c *cmap_shader = R_GetColormapShader(props, lit_adjust);

	R_ColorMapUpdate(cmap_shader, cur_sub->sector->lightcolor, cur_sub->sector->desaturation);

	cmap_shader->WorldMix(GL_POLYGON, data.v_count, data.tex_id,
			trans, &data.pass, data.blending, false /* masked */,
			&data, PlaneCoordFunc);

if (surf->image->liquid_type > LIQ_None && swirling_flats == SWIRL_PARALLAX) // Kept as an example for future effects
	{
		data.tx0 = surf->offset.x + 25;
		data.ty0 = surf->offset.y + 25;
		swirl_pass = 2;
		data.blending = BL_Masked | BL_Alpha;
		data.trans = 0.33f;
		trans = 0.33f;
		cmap_shader->WorldMix(GL_POLYGON, data.v_count, data.tex_id,
					trans, &data.pass, data.blending, false,
					&data, PlaneCoordFunc);
	}
	
	if (use_dlights && ren_extralight < 250)
	{
		P_DynamicLightIterator(v_bbox[BOXLEFT],  v_bbox[BOXBOTTOM], h,
				               v_bbox[BOXRIGHT], v_bbox[BOXTOP],    h,
							   DLIT_Plane, &data);

		P_SectorGlowIterator(cur_sub->sector,
				             v_bbox[BOXLEFT],  v_bbox[BOXBOTTOM], h,
				             v_bbox[BOXRIGHT], v_bbox[BOXTOP],    h,
							 GLOWLIT_Plane, &data);
	}
	swirl_pass = 0;

}

static inline void AddNewDrawFloor(drawsub_c *dsub, extrafloor_t *f_ef,
								   extrafloor_t *c_ef,
								   float f_h, float c_h, float top_h,
								   surface_t *floor, surface_t *ceil,
								   region_properties_t *props)
{
	drawfloor_t *dfloor;

	dfloor = R_GetDrawFloor();
	dfloor->Clear();

	dfloor->f_h   = f_h;
	dfloor->c_h   = c_h;
	dfloor->top_h = top_h;
	dfloor->floor = floor;
	dfloor->ceil  = ceil;
	dfloor->f_ef  = f_ef;
	dfloor->c_ef  = c_ef;
	dfloor->props = props;

	// link it in, height order

	dsub->floors.push_back(dfloor);

	// link it in, rendering order (very important)

	if (dsub->floors_R == NULL || f_h > viewz)
	{
		// add to head
		dfloor->next_R = dsub->floors_R;
		dfloor->prev_R = NULL;

		if (dsub->floors_R)
			dsub->floors_R->prev_R = dfloor;

		dsub->floors_R = dfloor;
	}
	else
	{
		// add to tail
		drawfloor_t *tail;

		for (tail = dsub->floors_R; tail->next_R; tail = tail->next_R)
		{ /* nothing here */ }

		dfloor->next_R = NULL;
		dfloor->prev_R = tail;

		tail->next_R = dfloor;
	}
}

//
// RGL_WalkPolyobject
//
// Visit a PolyObject anchor, collecting where walls need to be drawn.
//
static void RGL_WalkPolyobject(drawsub_c *dsub, mobj_t *mo)
{
	polyobj_t *po = P_GetPolyobject(mo->po_ix);
	if (!po)
	{
		I_Debugf("RGL_WalkPolyobject: No PO %d!\n", po->index); 
		//TODO: V522 https://www.viva64.com/en/w/v522/ Dereferencing of the null pointer 'po' might take place.
		return;
	}

	//I_Printf("RGL_WalkPolyobject: Adding render info for PO %d\n", po->index);
	seg_t *seg = po->segs;
	for (int i=0; i<po->count; i++, seg++)
	{
		//I_Printf("RGL_WalkPolyobject: Walking seg %p\n", po->segs[i]);
		RGL_WalkSeg(dsub, seg);
	}
}

//
// RGL_WalkSubsector
//
// Visit a subsector, and collect information, such as where the
// walls, planes (ceilings & floors) and things need to be drawn.
//
//TODO tapamn these should be where interpolation for elevators goes
static void RGL_WalkSubsector(int num)
{
	subsector_t *sub = &subsectors[num];
	seg_t *seg;
	mobj_t *mo;
	sector_t *sector;
	surface_t *floor_s;
	float floor_h;

	extrafloor_t *S, *L, *C, *F;

#if (DEBUG >= 1)
	L_WriteDebug( "\nVISITING SUBSEC %d (sector %d)\n\n", num, sub->sector - sectors);
#endif

	cur_sub = sub;
	sector = cur_sub->sector;

	drawsub_c *K = R_GetDrawSub();
	K->Clear(sub);

	// --- handle sky (using the depth buffer) ---
	//TODO? is sky plane interpolation important?
	// Nope! ^_^ ~CA~
	if (IS_SKY(cur_sub->sector->floor) && viewz > cur_sub->sector->f_h)
	{
		RGL_DrawSkyPlane(cur_sub, cur_sub->sector->f_h);
	}

	if (IS_SKY(cur_sub->sector->ceil) && viewz < cur_sub->sector->sky_h)
	{
		RGL_DrawSkyPlane(cur_sub, cur_sub->sector->sky_h);
	}

	// set the fog color
	fade_color = sector->fadecolor;

	// add in each extrafloor, traversing strictly upwards
	floor_s = &sector->floor;
	//floor_h = sector->f_h;
	floor_h = SecLerpedFloor(sector); //N_Interpolate(sector->lf_h, sector->f_h);
	F = NULL;

	S = sector->bottom_ef;
	L = sector->bottom_liq;

	// Handle the BOOMTEX flag (Boom compatibility)
	extrafloor_t *boom_ef = sector->bottom_liq ? sector->bottom_liq : sector->bottom_ef;

	if (boom_ef && (boom_ef->ef_info->type & EXFL_BoomTex))
		floor_s = &boom_ef->ef_line->frontsector->floor;


	float secceil = SecLerpedCeil(sector);

	while (S || L)
	{
		if (!L || (S && S->bottom_h < L->bottom_h))
		{
			C = S;  S = S->higher;
		}
		else
		{
			C = L;  L = L->higher;
		}

		SYS_ASSERT(C);

		// ignore liquids in the middle of THICK solids, or below real
		// floor or above real ceiling
		//
		//if (N_Interpolate(C->last_bottom_h, C->bottom_h) < floor_h || C->bottom_h > sector->c_h)
		float cboth = ExFloorLerpedBottom(C), ctoph = ExFloorLerpedTop(C);
		//float cboth = C->bottom_h;
		//float ctoph = C->top_h;
		//cboth = N_Interpolate(C->last_bottom_h, cboth);
		//ctoph = N_Interpolate(C->last_top_h, ctoph);

		if (cboth < floor_h || cboth > secceil)
			continue;

		bool de_f = (cur_sub->deep_ref && K->floors.size() == 0);

		AddNewDrawFloor(K, F, C,
			de_f ? cur_sub->deep_ref->f_h : floor_h,
			cboth, ctoph,
			de_f ? &cur_sub->deep_ref->floor : floor_s,
			C->bottom, C->p);

		F = C;
		floor_s = C->top;
		floor_h = ctoph;
	}

	// -AJA- 2004/04/22: emulate the Deep-Water TRICK (above too)
	bool de_f = (cur_sub->deep_ref && K->floors.size() == 0);
	bool de_c = (cur_sub->deep_ref != NULL);

	float ceilh = de_c ? SecLerpedCeil(cur_sub->deep_ref) : SecLerpedCeil(sector);
	//float ceilh = de_c ? cur_sub->deep_ref->c_h : sector->c_h;
	//ceilh = N_Interpolate(de_c ? cur_sub->deep_ref->lc_h : sector->lc_h, ceilh);

	AddNewDrawFloor(K, F, NULL,
		de_f ? cur_sub->deep_ref->f_h : floor_h,
		ceilh, ceilh,
		de_f ? &cur_sub->deep_ref->floor : floor_s,
		de_c ? &cur_sub->deep_ref->ceil : &sector->ceil, sector->p);

	K->floors[0]->is_lowest = true;
	K->floors[K->floors.size() - 1]->is_highest = true;

	// Handle each sprite in the subsector.  Must be done before walls,
	// since the wall code will update the 1D occlusion buffer.

	for (mo=cur_sub->thinglist; mo; mo=mo->snext)
	{
		if ((mo->typenum & ~3) != 9300)
			RGL_WalkThing(K, mo);
	}

	// Handle each polyobject in the subsector. Will update 1D occlusion buffer.
	for (mo=cur_sub->thinglist; mo; mo=mo->snext)
	{
		if ((mo->typenum == 9301) || (mo->typenum == 9302) || (mo->typenum == 9303))
			RGL_WalkPolyobject(K, mo);
	}

	// Handle each segment in the subsector. Will update 1D occlusion buffer.
	for (seg=sub->segs; seg; seg=seg->sub_next)
	{
		RGL_WalkSeg(K, seg);
	}

	// add drawsub to list (closest -> furthest)

	if (num_active_mirrors > 0)
		active_mirrors[num_active_mirrors-1].def->drawsubs.push_back(K);
	else
		drawsubs.push_back(K);
}


static void RGL_DrawSubsector(drawsub_c *dsub);


static void RGL_DrawSubList(std::list<drawsub_c *> &dsubs)
{
	// draw all solid walls and planes
	solid_mode = true;
	RGL_StartUnits(solid_mode);

	std::list<drawsub_c *>::iterator FI;  // Forward Iterator

	for (FI = dsubs.begin(); FI != dsubs.end(); FI++)
		RGL_DrawSubsector(*FI);

	RGL_FinishUnits();

	// draw all sprites and masked/translucent walls/planes
	solid_mode = false;
	RGL_StartUnits(solid_mode);

	std::list<drawsub_c *>::reverse_iterator RI;

	for (RI = dsubs.rbegin(); RI != dsubs.rend(); RI++)
		RGL_DrawSubsector(*RI);

	RGL_FinishUnits();

}


static void DrawMirrorPolygon(drawmirror_c *mir)
{
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	float alpha = 0.15 + 0.10 * num_active_mirrors;

	line_t *ld = mir->seg->linedef;
	SYS_ASSERT(ld);

	if (ld->special)
	{
		float R = RGB_RED(ld->special->fx_color) / 255.0;
		float G = RGB_GRN(ld->special->fx_color) / 255.0;
		float B = RGB_BLU(ld->special->fx_color) / 255.0;

		// looks better with reduced color in multiple reflections
		float reduce = 1.0f / (1 + 1.5 * num_active_mirrors);

		R *= reduce; G *= reduce; B *= reduce;

		glColor4f(R, G, B, alpha);
	}
	else
		glColor4f(1.0, 0.0, 0.0, alpha);

	float x1 = mir->seg->v1->x;
	float y1 = mir->seg->v1->y;
	float z1 = SecLerpedFloor(ld->frontsector);

	float x2 = mir->seg->v2->x;
	float y2 = mir->seg->v2->y;
	float z2 = SecLerpedCeil(ld->frontsector);

	MIR_Coordinate(x1, y1);
	MIR_Coordinate(x2, y2);

	glBegin(GL_POLYGON);

	glVertex3f(x1, y1, z1);
	glVertex3f(x1, y1, z2);
	glVertex3f(x2, y2, z2);
	glVertex3f(x2, y2, z1);

	glEnd();

	glDisable(GL_BLEND);
}

static void DrawPortalPolygon(drawmirror_c *mir)
{
	line_t *ld = mir->seg->linedef;
	SYS_ASSERT(ld);

	const surface_t *surf = &mir->seg->sidedef->middle;

	if (! surf->image || ! ld->special ||
		! (ld->special->portal_effect & PORTFX_Standard))
	{
		DrawMirrorPolygon(mir);
		return;
	}

	glDisable(GL_ALPHA_TEST);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// set texture
	GLuint tex_id = W_ImageCache(surf->image);

	glBindTexture(GL_TEXTURE_2D, tex_id);

	// set colour & alpha
	float alpha = ld->special->translucency * surf->translucency;

	float R = RGB_RED(ld->special->fx_color) / 255.0;
	float G = RGB_GRN(ld->special->fx_color) / 255.0;
	float B = RGB_BLU(ld->special->fx_color) / 255.0;

	glColor4f(R, G, B, alpha);

	// get polygon coordinates
	float x1 = mir->seg->v1->x;
	float y1 = mir->seg->v1->y;
	float z1 = SecLerpedFloor(ld->frontsector);

	float x2 = mir->seg->v2->x;
	float y2 = mir->seg->v2->y;
	float z2 = SecLerpedCeil(ld->frontsector);


	MIR_Coordinate(x1, y1);
	MIR_Coordinate(x2, y2);

	// get texture coordinates
	float total_w = IM_TOTAL_WIDTH( surf->image);
	float total_h = IM_TOTAL_HEIGHT(surf->image);

	float tx1 = mir->seg->offset;
	float tx2 = tx1 + mir->seg->length;

	float ty1 = 0;
	float ty2 = (z2 - z1);

	tx1 = tx1 * surf->x_mat.x / total_w;
	tx2 = tx2 * surf->x_mat.x / total_w;

	ty1 = ty1 * surf->y_mat.y / total_h;
	ty2 = ty2 * surf->y_mat.y / total_h;

	glBegin(GL_POLYGON);

	glTexCoord2f(tx1, ty1); glVertex3f(x1, y1, z1);
	glTexCoord2f(tx1, ty2); glVertex3f(x1, y1, z2);
	glTexCoord2f(tx2, ty2); glVertex3f(x2, y2, z2);
	glTexCoord2f(tx2, ty1); glVertex3f(x2, y2, z1);

	glEnd();

	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
}

static void RGL_DrawMirror(drawmirror_c *mir)
{
	// mark the segment on the automap
	mir->seg->linedef->flags |= MLF_Mapped;

	RGL_FinishUnits();

	MIR_Push(mir);
	{
		RGL_DrawSubList(mir->drawsubs);
	}
	MIR_Pop();

	if (mir->is_portal)
		DrawPortalPolygon(mir);
	else
		DrawMirrorPolygon(mir);

	solid_mode = true;
	RGL_StartUnits(solid_mode);
}


static void RGL_DrawSubsector(drawsub_c *dsub)
{
	subsector_t *sub = dsub->sub;

#if (DEBUG >= 1)
	L_WriteDebug("\nREVISITING SUBSEC %d\n\n", (int)(sub - subsectors));
#endif

	cur_sub = sub;

	if (solid_mode)
	{
		std::list<drawmirror_c *>::iterator MRI;

		for (MRI = dsub->mirrors.begin(); MRI != dsub->mirrors.end(); MRI++)
		{
			RGL_DrawMirror(*MRI);
		}
	}

	cur_sub = sub;

	drawfloor_t *dfloor;

	// handle each floor, drawing planes and things
	for (dfloor = dsub->floors_R; dfloor != NULL; dfloor = dfloor->next_R)
	{
		std::list<drawseg_c *>::iterator SEGI;

		for (SEGI = dsub->segs.begin(); SEGI != dsub->segs.end(); SEGI++)
		{
			RGL_DrawSeg(dfloor, (*SEGI)->seg);
		}

		RGL_DrawPlane(dfloor, dfloor->c_h, dfloor->ceil,  -1);
		RGL_DrawPlane(dfloor, dfloor->f_h, dfloor->floor, +1);

		if (! solid_mode)
		{
			RGL_DrawSortThings(dfloor);
  		}
	}
}

static void DoWeaponModel(void)
{
	player_t *pl = view_cam_mo->player;

	if (! pl)
		return;

	// clear the depth buffer, so that the weapon is never clipped
	// by the world geometry.  NOTE: a tad expensive, but I don't
	// know how any better way to prevent clipping -- the model
	// needs the depth buffer for overlapping parts of itself.

	// Actually, come to think of it, using this depth buffer might
	// give us some interesting things, for instance, in a game
	// like Hypertension, we can use this piece of code to detect if
	// a weapon will hit level geometry, and lower it, putting up like
	// some sort of "hands against the wall" effect, or even like a lower/
	// raise weapon _away_ from wall. Just an idea.

	glClear(GL_DEPTH_BUFFER_BIT);

	solid_mode = false;
	RGL_StartUnits(solid_mode);

	RGL_DrawWeaponModel(pl);

	RGL_FinishUnits();
}

//
// RGL_WalkBSPNode
//
// Walks all subsectors below a given node, traversing subtree
// recursively, collecting information.  Just call with BSP root.
//
static void RGL_WalkBSPNode(unsigned int bspnum)
{
	node_t *node;

	// Found a subsector?
	if (bspnum & NF_V5_SUBSECTOR)
	{
		RGL_WalkSubsector(bspnum & (~NF_V5_SUBSECTOR));
		return;
	}

	node = &nodes[bspnum];

	// Decide which side the view point is on.

	divline_t nd_div;

	nd_div.x  = node->div.x;
	nd_div.y  = node->div.y;
	nd_div.dx = node->div.x + node->div.dx;
	nd_div.dy = node->div.y + node->div.dy;

	MIR_Coordinate(nd_div.x,  nd_div.y);
	MIR_Coordinate(nd_div.dx, nd_div.dy);

	if (MIR_Reflective())
	{
		float tx = nd_div.x; nd_div.x = nd_div.dx; nd_div.dx = tx;
		float ty = nd_div.y; nd_div.y = nd_div.dy; nd_div.dy = ty;
	}

	nd_div.dx -= nd_div.x;
	nd_div.dy -= nd_div.y;

	int side = P_PointOnDivlineSide(viewx, viewy, &nd_div);
	//side = R_PointOnSide(viewx, viewy, &nd_div);

	// Recursively divide front space.
	if (RGL_CheckBBox(node->bbox[side]))
		RGL_WalkBSPNode(node->children[side]);



	// Recursively divide back space.
	if (RGL_CheckBBox(node->bbox[side ^ 1]))
		RGL_WalkBSPNode(node->children[side ^ 1]);
}

void RenderPolyBSPNode(unsigned int bspnum)
{
	while (!((size_t)bspnum & 1))  // Keep going until found a subsector
	{
		node_t *node;
#if 0

		// Found a subsector?
		if (bspnum & NF_V5_SUBSECTOR)
		{
			RGL_WalkSubsector(bspnum & (~NF_V5_SUBSECTOR));
			return;
		}
#endif // 0

		node = &nodes[bspnum];

		// Decide which side the view point is on.


		// Decide which side the view point is on.

		divline_t nd_div;

		nd_div.x = node->div.x;
		nd_div.y = node->div.y;
		nd_div.dx = node->div.x + node->div.dx;
		nd_div.dy = node->div.y + node->div.dy;

		MIR_Coordinate(nd_div.x, nd_div.y);
		MIR_Coordinate(nd_div.dx, nd_div.dy);

		if (MIR_Reflective())
		{
			float tx = nd_div.x; nd_div.x = nd_div.dx; nd_div.dx = tx;
			float ty = nd_div.y; nd_div.y = nd_div.dy; nd_div.dy = ty;
		}

		nd_div.dx -= nd_div.x;
		nd_div.dy -= nd_div.y;

		// Decide which side the view point is on.
		int side = P_PointOnDivlineSide(viewx, viewy, &nd_div);

		// Recursively divide front space (toward the viewer).
		//RenderPolyBSPNode(bsp->children[side]);
		RenderPolyBSPNode(node->children[side]);

		// Possibly divide back space (away from the viewer).
		side ^= 1;

		// It is not necessary to use the slower precise version here
		if (!RGL_CheckBBox(node->bbox[side]))
		{
			return;
		}

		bspnum = node->children[side];
	}

	RGL_WalkSubsector((bspnum - 1) & (~NF_V5_SUBSECTOR));
	//RGL_WalkSubsector((subsector_t *)(bspnum - 1));
}

//
// RGL_LoadLights
//
void RGL_LoadLights(void)
{

}

//
// RGL_RenderTrueBSP
//
// OpenGL BSP rendering.  Initialises all structures, then walks the
// BSP tree collecting information, then renders each subsector:
// firstly front to back (drawing all solid walls & planes) and then
// from back to front (drawing everything else, sprites etc..).
//
static void RGL_RenderTrueBSP(void)
{
	// clear extra light on player's weapon
	rgl_weapon_r = rgl_weapon_g = rgl_weapon_b = 0;

	FUZZ_Update();

	R2_ClearBSP();
	RGL_1DOcclusionClear();

	drawsubs.clear();

	player_t *v_player = view_cam_mo->player;

	// handle powerup effects
	RGL_RainbowEffect(v_player);


	RGL_SetupMatrices3D();

	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	// needed for drawing the sky
	RGL_BeginSky();

	// walk the bsp tree
	//
	RGL_WalkBSPNode(root_node);
	//RenderPolyBSPNode(root_node);

	RGL_FinishSky();

	RGL_DrawSubList(drawsubs);

	DoWeaponModel();

	glDisable(GL_DEPTH_TEST);

	cameraman::DrawHelpers();

	// now draw 2D stuff like psprites, and add effects
	RGL_SetupMatrices2D();

	if (v_player)
	{
		RGL_DrawWeaponSprites(v_player);

		RGL_ColourmapEffect(v_player);
		RGL_PaletteEffect(v_player);

		RGL_DrawCrosshair(v_player);
	}

#if (DEBUG >= 3)
	L_WriteDebug( "\n\n");
#endif
}

extern bool ff_shake[MAXPLAYERS];
extern int ff_frequency[MAXPLAYERS];
extern int ff_intensity[MAXPLAYERS];
extern int ff_timeout[MAXPLAYERS];

/// rendering view point based on the camera's location.
static void InitCamera(mobj_t *mo, bool full_height, float expand_w)
{
	cameraman::cameraman_t *pcman = NULL;
	cameraman::cameraman_t cman = { 0 };

	pcman = cameraman::Update(&cman);

	int t_fov = mo->player ? mo->player->telept_fov : 0;
	float fov = pcman == NULL ? CLAMP(5, r_fov + t_fov, 175) : pcman->fov;

	if (splitscreen_mode)
		fov = fov / 1.5;

	view_x_slope = tan(fov * M_PI / 360.0);

	view_y_slope = ((full_height) ? DOOM_YSLOPE_FULL : DOOM_YSLOPE) * ((r_stretchworld == 1) ? 1.2 : 1.0);

	viewiszoomed = false;

	if (player_t *hero = players[0])
	{
		hero->mo->visibility = (!pcman); //TODO: V601 https://www.viva64.com/en/w/v601/ The bool type is implicitly cast to the float type.
	}

	if (!pcman && mo->player && mo->player->zoom_fov > 0)
	{
		viewiszoomed = true;

		float new_slope = tan(mo->player->zoom_fov * M_PI / 360.0);

		view_y_slope *= new_slope / view_x_slope;
		view_x_slope  = new_slope;
	}

	// wide-screen adjustment
	view_expand_w = expand_w;

	view_x_slope *= view_expand_w;

	epi::vec3_c vp = mo->GetInterpolatedPosition();

	viewx = vp.x;
	viewy = vp.y;
	viewz = vp.z;
	//viewangle = mo->angle;
	viewangle = mo->GetInterpolatedAngle();

	// Only the view in that case...
	if (pcman != NULL)
	{
		viewx = pcman->x;
		viewy = pcman->y;
		viewz = pcman->z;
	}

	// -CW- shake screen
	if (!pcman && mo->player && ff_shake[mo->player->pnum] && ff_timeout[mo->player->pnum])
	{
		if (I_GetMillies() < ff_timeout[mo->player->pnum])
		{
			viewz += (float)ff_intensity[mo->player->pnum] / 10.0f * sin((double)(I_GetMillies()/ ff_frequency[mo->player->pnum]));
		}
		else
			ff_timeout[mo->player->pnum] = 0;
	}

	extern float N_GetInterpolater(void);
	float lerpv = N_GetInterpolater();
	if (!pcman)
	{
		if (mo->player)//t*b+(a-a*t)
	//viewz += mo->player->lastviewz + (mo->player->viewz - mo->player->lastviewz) * N_GetInterpolater();
			viewz += mo->player->viewz*lerpv + (mo->player->lastviewz - mo->player->lastviewz*lerpv);
		else
			viewz += mo->height * 9 / 10;
	}

	viewsubsector = mo->subsector;
	//viewvertangle = mo->vertangle;
	viewvertangle = mo->GetInterpolatedVertAngle();
	view_props = R_PointGetProps(viewsubsector, viewz);

	if (!pcman && mo->player)
	{
		viewvertangle += M_ATan(mo->player->kick_offset);

		if (! level_flags.mlook)
			viewvertangle = 0;

		// No heads above the ceiling
		if (viewz > mo->player->mo->ceilingz - 2)
			viewz = mo->player->mo->ceilingz - 2;

		// No heads below the floor, please
		if (viewz < mo->player->mo->floorz + 2)
			viewz = mo->player->mo->floorz + 2;
	}


	// do some more stuff
	if (pcman != NULL)
	{
		viewangle = G_CircularLerp(viewangle, pcman->viewangle, 1.0f);
		viewvertangle = G_CircularLerp(viewvertangle, pcman->viewvertangle, 1.0f);
	}

	viewsin = M_Sin(viewangle);
	viewcos = M_Cos(viewangle);

	float lk_sin = M_Sin(viewvertangle);
	float lk_cos = M_Cos(viewvertangle);

	viewforward.x = lk_cos * viewcos;
	viewforward.y = lk_cos * viewsin;
	viewforward.z = lk_sin;

	viewup.x = -lk_sin * viewcos;
	viewup.y = -lk_sin * viewsin;
	viewup.z =  lk_cos;

	// cross product
	viewright.x = viewforward.y * viewup.z - viewup.y * viewforward.z;
	viewright.y = viewforward.z * viewup.x - viewup.z * viewforward.x;
	viewright.z = viewforward.x * viewup.y - viewup.x * viewforward.y;


	// compute the 1D projection of the view angle
	angle_t oned_side_angle;
	{
		float k, d;

		// k is just the mlook angle (in radians)
		k = ANG_2_FLOAT(viewvertangle);
		if (k > 180.0) k -= 360.0;
		k = k * M_PI / 180.0f;

		sprite_skew = tan((-k) / 2.0);

		k = fabs(k);

		// d is just the distance horizontally forward from the eye to
		// the top/bottom edge of the view rectangle.
		d = cos(k) - sin(k) * view_y_slope;

		oned_side_angle = (d <= 0.01f) ? ANG180 : M_ATan(view_x_slope / d);
	}

	// setup clip angles
	if (oned_side_angle != ANG180)
	{
		clip_left  = 0 + oned_side_angle;
		clip_right = 0 - oned_side_angle;
		clip_scope = clip_left - clip_right;
	}
	else
	{
		// not clipping to the viewport.  Dummy values.
		clip_scope = ANG180;
		clip_left  = 0 + ANG45;
		clip_right = 0 - ANG45;
	}
}


void R_Render(int x, int y, int w, int h, mobj_t *camera,
              bool full_height, float expand_w)
{
	viewwindow_x = x;
	viewwindow_y = y;
	viewwindow_w = w;
	viewwindow_h = h;

	view_cam_mo = camera;

	// Load the details for the camera
	InitCamera(camera, full_height, expand_w);

	// Profiling
	framecount++;
	validcount++;

	RGL_RenderTrueBSP();
}
