//----------------------------------------------------------------------------
//  EDGE Main Rendering Organisation Code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2004  The EDGE Team.
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
//
// -KM- 1998/09/27 Dynamic Colourmaps
//

#include "i_defs.h"
#include "r_main.h"

#include "dm_defs.h"
#include "dm_state.h"
#include "e_net.h"
#include "gui_main.h"
#include "m_argv.h"
#include "m_bbox.h"
#include "m_fixed.h"
#include "m_misc.h"
#include "m_menu.h"
#include "p_local.h"
#include "r_local.h"
#include "r_sky.h"
#include "r_vbinit.h"
#include "r_view.h"
#include "r2_defs.h"
#include "rgl_defs.h"

#include "st_stuff.h"
#include "v_colour.h"
#include "w_wad.h"
#include "wp_main.h"
#include "z_zone.h"

#define RESDELAY_MAXLOOP 18081979

viewbitmap_t *rightvb;

// the view and viewbitmap that the rendering system currently is set up for
view_t *curview = NULL;
viewbitmap_t *curviewbmp = NULL;

// -ES- 1999/03/14 Dynamic Field Of View
// Fineangles in the viewwidth wide window.
angle_t FIELDOFVIEW = 2048;

// The used aspect ratio. A normal texel will look aspect_ratio*4/3
// times wider than high on the monitor
static const float aspect_ratio = 200.0f / 320.0f;

// the extreme angles of the view
// -AJA- FIXME: these aren't angle_t (32 bit angles).
angle_t topangle;
angle_t bottomangle;
angle_t rightangle;
angle_t leftangle;

float leftslope;
float rightslope;
float topslope;
float bottomslope;

int viewwidth;
int viewheight;
int viewwindowx;
int viewwindowy;
int viewwindowwidth;
int viewwindowheight;

int vb_w;
int vb_h;
int vb_pitch;

angle_t viewangle = 0;
angle_t viewvertangle = 0;

angle_t normalfov, zoomedfov;
bool viewiszoomed = false;

// increment every time a check is made
int validcount = 1;

// -KM- 1998/09/27 Dynamic colourmaps
// -AJA- 1999/07/10: Updated for colmap.ddf.
const colourmap_t *effect_colourmap;
float effect_strength;
bool effect_infrared;

// -ES- 1999/03/19 rename from center to focus
float focusxfrac;
float focusyfrac;

// -ES- 1999/03/14 Added these. Unit Scale is used for things one distunit away.
float x_distunit;
float y_distunit;

// just for profiling purposes
int framecount;
int linecount;

subsector_t *viewsubsector;
region_properties_t *view_props;

float viewx;
float viewy;
float viewz;

float viewcos;
float viewsin;

player_t *viewplayer;

camera_t *camera = NULL;

camera_t *background_camera = NULL;
mobj_t *background_camera_mo = NULL;

viewbitmap_t *screenvb;

//
// precalculated math tables
//

// -ES- 1999/03/20 Different right & left side clip angles, for asymmetric FOVs.
angle_t leftclipangle, rightclipangle;
angle_t clipscope;

// The viewangletox[viewangle + FINEANGLES/4] lookup
// maps the visible view angles to screen X coordinates,
// flattening the arc to a flat projection plane.
// There will be many angles mapped to the same X. 
// -ES- 1999/05/22 Made Dynamic
int *viewangletox;

// The xtoviewangleangle[] table maps a screen pixel
// to the lowest viewangle that maps back to x ranges
// from leftangle to rightangle.
// -ES- 1998/08/20 Explicit init to NULL
angle_t *xtoviewangle = NULL;

// -KM- 1998/09/27 Dynamic colourmaps
// -ES- 1999/06/13 BPP-specific colourmaps
// -AJA- 1999/07/10: Un-BPP-ised the lighting tables, for colmap.ddf.

lighttable_t zlight[LIGHTLEVELS][MAXLIGHTZ];
lighttable_t scalelight[LIGHTLEVELS][MAXLIGHTSCALE];

// bumped light from gun blasts
int extralight;

angle_t viewanglebaseoffset;
angle_t viewangleoffset;

void (*colfunc) (void);
void (*basecolfunc) (void);
void (*fuzzcolfunc) (void);
void (*transcolfunc) (void);
void (*spanfunc) (void);
void (*trans_spanfunc) (void);

wipeinfo_t *telept_wipeinfo = NULL;
int telept_starttic;
int telept_active = 0;
static void R_Render_Standard(void)
{
	R_RenderViewBitmap(screenvb);
	if (telept_active)
		telept_active = !WIPE_DoWipe(&screenvb->screen, &screenvb->screen,
		&screenvb->screen, leveltime - telept_starttic, telept_wipeinfo);
}

//
// R_PointToAngle
//
// To get a global angle from cartesian coordinates,
// the coordinates are flipped until they are in
// the first octant of the coordinate system, then
// the y (<=x) is scaled and divided by x to get a
// tangent (slope) value which is looked up in the
// tantoangle[] table.
//
angle_t R_PointToAngle(float x1, float y1, float x, float y)
{
	x -= x1;
	y -= y1;

	if ((x == 0) && (y == 0))
		return 0;

	if (x >= 0)
	{
		// x >=0
		if (y >= 0)
		{
			// y>= 0

			if (x > y)
			{
				// octant 0
				return M_ATan(y / x);
			}
			else
			{
				// octant 1
				return ANG90 - 1 - M_ATan(x / y);
			}
		}
		else
		{
			// y<0
			y = -y;

			if (x > y)
			{
				// octant 8
				// -ACB- 1999/09/27 Fixed MSVC Compiler warning
				return 0 - M_ATan(y / x);
			}
			else
			{
				// octant 7
				return ANG270 + M_ATan(x / y);
			}
		}
	}
	else
	{
		// x<0
		x = -x;

		if (y >= 0)
		{
			// y>= 0
			if (x > y)
			{
				// octant 3
				return ANG180 - 1 - M_ATan(y / x);
			}
			else
			{
				// octant 2
				return ANG90 + M_ATan(x / y);
			}
		}
		else
		{
			// y<0
			y = -y;

			if (x > y)
			{
				// octant 4
				return ANG180 + M_ATan(y / x);
			}
			else
			{
				// octant 5
				return ANG270 - 1 - M_ATan(x / y);
			}
		}
	}
}

//
// R_PointToDist
//
//
//
float R_PointToDist(float x1, float y1, float x2, float y2)
{
	angle_t angle;
	float dx;
	float dy;
	float temp;
	float dist;

	dx = (float)fabs(x2 - x1);
	dy = (float)fabs(y2 - y1);

	if (dx == 0.0f)
		return dy;
	else if (dy == 0.0f)
		return dx;

	if (dy > dx)
	{
		temp = dx;
		dx = dy;
		dy = temp;
	}

	angle = M_ATan(dy / dx) + ANG90;

	// use as cosine
	dist = dx / M_Sin(angle);

	return dist;
}

//
// R_ScaleFromGlobalAngle
//
// Returns the texture mapping scale
// for the current line (horizontal span)
// at the given angle.
//
// rw_distance & rw_normalangle must be calculated first.
//
float R_ScaleFromGlobalAngle(angle_t visangle)
{
	float scale;
	angle_t anglea;
	angle_t angleb;
	float cosa;
	float cosb;
	float num;
	float den;

	// Note: anglea is the difference between the given angle `visangle'
	// and the view angle.  We calculate the distance from the camera to
	// the seg line (below), and by triangulating we can determine the
	// distance from the view plane to the point on the seg (along a line
	// parallel to the view angle).
	//
	// Namely: z = hyp * cos(anglea).
	//
	// Thus: scale = 1.0 / z.
	//
	// Since the angle lies in the view's cliprange (clipping has
	// already occurred), the cosine will always be positive.

	anglea = visangle - viewangle;
	cosa = M_Cos(anglea);

	// Note: angleb is the difference between the given angle `visangle'
	// and the seg line's normal.  We know the distance along the normal
	// (rw_distance), thus by triangulating we can calculate the
	// distance from the camera to the seg line at the given angle,
	//
	// Namely: hyp = rw_distance / cos(angleb).
	//
	// Since the seg line faces the camera (back faces have already been
	// culled), the cosine will always be positive.

	angleb = visangle - rw_normalangle;
	cosb = M_Cos(angleb);

	num = y_distunit * cosb;
	den = rw_distance * cosa;

	if (den > 0)
	{
		scale = num / den;

		if (scale > 64)
			scale = 64;
		else if (scale < 1/256.0f)
			scale = 1/256.0f;
	}
	else
		scale = 64;

	return scale;
}

//
// InitTextureMapping
//
static void InitTextureMapping(void)
{
	// this should be called between each view change (or it could be removed)
	leftclipangle = xtoviewangle[0];
	rightclipangle = xtoviewangle[viewwidth];
	clipscope = leftclipangle - rightclipangle;
}

//
// InitLightTables
//
// Inits the zlight and scalelight tables. Only done once, at startup.
// -ES- 1999/06/13 Made separate 8- and 16-bit versions of tables
// -AJA- 1999/07/10: Unseparated them, for colmap.ddf.
//
static void InitLightTables(void)
{
	int i;
	int j;
	int level;
	float startmap;
	float scale;

	// Calculate the light levels to use
	//  for each level / distance combination &
	//  for each level / scale combination.
	// -AJA- 1999/07/10: Reworked for colmap.ddf.

	for (i = 0; i < LIGHTLEVELS; i++)
	{
		startmap = ((LIGHTLEVELS - 1 - i) * 2) * 256.0f / LIGHTLEVELS;

		for (j = 0; j < MAXLIGHTZ; j++)
		{
			scale = 16 * 160.0f / (j + 1);
			level = (int)(startmap - scale);

			if (level < 0)
				level = 0;
			if (level > 255)
				level = 255;

			zlight[i][j] = 255 - level;
		}

		for (j = 0; j < MAXLIGHTSCALE; j++)
		{
			level = (int)(startmap - j * 192.0f / MAXLIGHTSCALE);

			if (level < 0)
				level = 0;
			if (level > 255)
				level = 255;

			scalelight[i][j] = 255 - level;
		}
	}
}

// Can currently only be R_Render_Standard.
void (*R_Render) (void) = R_Render_Standard;

//
// R_SetViewSize
//
// Do not really change anything here,
// because it might be in the middle of a refresh.
//
// The change will take effect next refresh.

bool setsizeneeded;
int setblocks;
bool setresfailed = false;
int use_3d_mode;

void R_SetViewSize(int blocks)
{
	setsizeneeded = true;

	setblocks = blocks;
}

static void InitViews(viewbitmap_t * vb, float xoffset)
{
	const char *s;
	aspect_t *a;

	// -ES- 1999/03/19 Use focusx & focusy, for asymmetric fovs.
	focusxfrac = leftslope * viewwidth / (leftslope - rightslope) + xoffset;

	// Unit scale at distance distunit.
	x_distunit = viewwidth / (leftslope - rightslope);

	a = R_CreateAspect(vb,
		x_distunit, y_distunit,
		focusxfrac,
		topslope, bottomslope,
		viewwidth, viewheight);

	// first of all, create the psprite view, but only if viewanglebaseoffset == 0
	if (viewanglebaseoffset == 0)
		R_CreateView(vb, a, 0, 0, camera, VRF_PSPR, 100);

	s = M_GetParm("-screencomp");
	if (s)
	{
		screencomposition = atoi(s);
	}

	// now create the "real" views...
	screencomplist[screencomposition].routine(vb);
}

//
// R_ExecuteSetViewSize
//
void R_ExecuteSetViewSize(void)
{
	float slopeoffset;
	int sbar_height;

	setsizeneeded = false;
	redrawsbar = true;

	sbar_height = FROM_200(ST_HEIGHT);

	if (setblocks == 11 || background_camera_mo)
	{
		viewwidth  = SCREENWIDTH;
		viewheight = SCREENHEIGHT;
	}
	else
	{
		viewwidth  = (setblocks * SCREENWIDTH / 10) & ~7;

		// Fixes 400x300 etc.
		if (setblocks == 10)
			viewheight = SCREENHEIGHT - sbar_height;
		else
			viewheight = (setblocks * (SCREENHEIGHT - sbar_height) / 10) & ~7;
	}

	viewwindowwidth  = viewwidth;
	viewwindowheight = viewheight;
	viewwindowx = (SCREENWIDTH - viewwindowwidth) / 2;

	if (viewwidth == SCREENWIDTH)
		viewwindowy = 0;
	else
		viewwindowy = (SCREENHEIGHT - sbar_height - viewheight) / 2;

	leftslope = M_Tan(leftangle << ANGLETOFINESHIFT);
	rightslope = M_Tan(rightangle << ANGLETOFINESHIFT);

	slopeoffset = M_Tan((FIELDOFVIEW / 2) << ANGLETOFINESHIFT) * aspect_ratio;
	slopeoffset = slopeoffset * viewwindowheight / viewwindowwidth;
	slopeoffset = slopeoffset * SCREENWIDTH / SCREENHEIGHT;

	topslope = slopeoffset;
	bottomslope = -slopeoffset;

	y_distunit = viewwindowheight / (topslope - bottomslope);
	focusyfrac = viewwindowheight * topslope / (topslope - bottomslope);

	if (use_3d_mode && SCREENBITS == 16)
	{
		static camera_t *leftc, *rightc;

		if (screenvb)
			R_DestroyViewBitmap(screenvb);

		if (rightvb)
			R_DestroyViewBitmap(rightvb);

		rightvb = R_CreateViewBitmap(viewwindowwidth, viewwindowheight, BPP, NULL, 0, 0);
		screenvb = R_CreateViewBitmap(viewwindowwidth, viewwindowheight, BPP, main_scr, viewwindowx, viewwindowy);

		R_InitVB_3D_Right(rightvb, screenvb);

		if (!leftc)
		{
			leftc = R_CreateCamera();
			R_InitCamera_3D_Left(leftc);
			rightc = R_CreateCamera();
			R_InitCamera_3D_Right(rightc);
		}

		camera = leftc;
		InitViews(screenvb, 4.0f);
		camera = rightc;
		InitViews(rightvb, -4.0f);

		R_InitVB_3D_Left(rightvb, screenvb);
	}
	else
	{
		// -AJA- FIXME: cameras should be renewed when starting a new
		//       level (since there will be new mobjs).

		// -AJA- 1999/10/22: background cameras.  This code really sucks
		//       arse, needs improving.
		if (background_camera_mo && !background_camera)
		{
			background_camera = R_CreateCamera();
			R_InitCamera_StdObject(background_camera, background_camera_mo);
			camera = background_camera;
		}

		if (!camera || (background_camera && !background_camera_mo))
		{
			camera = R_CreateCamera();
			R_InitCamera_StdPlayer(camera);
			background_camera = NULL;
		}

		if (screenvb)
			R_DestroyViewBitmap(screenvb);

		screenvb = R_CreateViewBitmap(viewwindowwidth, viewwindowheight, BPP, 
			main_scr, 0, 0); /// viewwindowx, viewwindowy);

		InitViews(screenvb, 0);
	}

	R_SetActiveViewBitmap(screenvb);

	pspritescale = (float)(viewwindowwidth / 320.0f);
	pspriteiscale = (float)(320.0f / viewwindowwidth);
	pspritescale2 = ((float)SCREENHEIGHT / SCREENWIDTH) * viewwindowwidth / 200.0f;
	pspriteiscale2 = ((float)SCREENWIDTH / SCREENHEIGHT) * 200.0f / viewwindowwidth;

}

//
// R_SetFOV
//
// Sets the specified field of view
//
void R_SetFOV(angle_t fov)
{
	// can't change fov to angle below 5 or above 175 deg (approx). Round so
	// that 5 and 175 are allowed for sure.
	if (fov < ANG90 / 18)
		fov = ANG90 / 18;
	if (fov > ((ANG90 + 17) / 18) * 35)
		fov = ANG90 / 18 * 35;

	setsizeneeded = true;

	fov = fov >> ANGLETOFINESHIFT;  // convert to fineangle format

	leftangle = fov / 2;
	rightangle = (fov/2)*-1; // -ACB- 1999/09/27 Fixed MSVC Compiler Problem
	FIELDOFVIEW = leftangle - rightangle;
}

void R_SetNormalFOV(angle_t newfov)
{
	menunormalfov = (newfov - ANG45 / 18) / (ANG45 / 9);
	cfgnormalfov = (newfov + ANG45 / 90) / (ANG180 / 180);
	normalfov = newfov;
	if (!viewiszoomed)
		R_SetFOV(normalfov);
}
void R_SetZoomedFOV(angle_t newfov)
{
	menuzoomedfov = (newfov - ANG45 / 18) / (ANG45 / 9);
	cfgzoomedfov = (newfov + ANG45 / 90) / (ANG180 / 180);
	zoomedfov = newfov;
	if (viewiszoomed)
		R_SetFOV(zoomedfov);
}

//
// R_ChangeResolution
//
// Makes R_ExecuteChangeResolution execute at start of next refresh.
//
bool changeresneeded = false;
static screenmode_t setMode;

void R_ChangeResolution(int width, int height, int depth, bool windowed)
{
	changeresneeded = true;
	setsizeneeded = true;  // need to re-init some stuff

	setMode.width    = width;
	setMode.height   = height;
	setMode.depth    = depth;
	setMode.windowed = windowed;

	L_WriteDebug("R_ChangeResolution: Trying %dx%dx%d (%s)\n", 
		width, height, depth, windowed?"windowed":"fullscreen");
}

//
// R_ExecuteChangeResolution
//
// Do the resolution change
//
// -ES- 1999/04/05 Changed this to work with the viewbitmap system
//
static bool DoExecuteChangeResolution(void)
{
	static bool init_rend = false;
	changeresneeded = false;

	SCREENWIDTH  = setMode.width;
	SCREENHEIGHT = setMode.height;
	SCREENBITS   = setMode.depth;
	SCREENWINDOW = setMode.windowed;

	// -ACB- 1999/09/20
	// parameters needed for I_SetScreenMode - returns false on failure
	if (! I_SetScreenSize(&setMode))
	{
		// wait one second before changing res again, gfx card doesn't like to
		// switch mode too rapidly.
		int count, t, t1, t2;

		count = 0;
		t1 = I_GetTime();
		t2 = t1 + TICRATE;

		// -ACB- 2004/02/21 If the app is minimised, the clock won't tic. Handle this.
		do 
		{ 
			t = I_GetTime();

			if (t == t1)
				count++;

			if (count > RESDELAY_MAXLOOP)
				break;
		}
		while (t2 >= t);

		return false;
	}

	// -ES- 2000-08-14 The resolution is obviously available.
	// In some situations, it might however not yet be available in the
	// screenmode list.
	V_AddAvailableResolution(&setMode);

	V_InitResolution();

	// -AJA- 1999/07/01: Setup colour tables.
	V_InitColour();

	if (! init_rend)
	{
#ifdef USE_GL
		RGL_Init();
#else
		R2_Init();
#endif

		init_rend = true;
	}

	/// -AJA- FIXME: clean up (move into R2_NewScreenSize)
	{
		int i;

		Z_Resize(columnofs, int, SCREENWIDTH);
		Z_Resize(ylookup, byte *, SCREENHEIGHT);

		for (i=0; i < SCREENWIDTH; i++)
			columnofs[i] = i * BPP;

		for (i=0; i < SCREENHEIGHT; i++)
			ylookup[i] = main_scr->data + i * main_scr->pitch;
	}

	vctx.NewScreenSize(SCREENWIDTH, SCREENHEIGHT, BPP);

	// -ES- 1999/08/29 Fixes the garbage palettes, and the blank 16-bit console
	V_SetPalette(PALETTE_NORMAL, 0);
	V_ColourNewFrame();

	// -ES- 1999/08/20 Update GUI (resize stuff etc)
	GUI_InitResolution();

	// re-initialise various bits of GL state
#ifdef USE_GL
	RGL_SoftInit();
	RGL_SoftInitUnits();	// -ACB- 2004/02/15 Needed to sort some vars lost in res change
	W_ResetImages();
#endif

	graphicsmode = true;

	// -AJA- 1999/07/03: removed PLAYPAL reference.
	return true;
}

void R_ExecuteChangeResolution(void)
{
	int i, j, idx;
	screenmode_t wantedMode;
	screenmode_t oldMode;
	screenmode_t defaultMode = { DEFAULTSCREENWIDTH, DEFAULTSCREENHEIGHT, DEFAULTSCREENBITS, DEFAULTSCREENWINDOW };

	wantedMode = setMode;

	oldMode.width    = SCREENWIDTH;
	oldMode.height   = SCREENHEIGHT;
	oldMode.depth    = SCREENBITS;
	oldMode.windowed = SCREENWINDOW;

	// -ACB- 1999/09/21 false on failure - logical.
	if (DoExecuteChangeResolution())
	{
		setresfailed = false;
		return;
	}

	setresfailed = true;

	setMode = oldMode;
	setsizeneeded = true;

	if (DoExecuteChangeResolution())
		return;

	// couldn't even reset to old resolution. Perhaps they were the same.
	// Try the default as second last resort.  Do a major loop over
	// windowing flag, e.g. so if all fullscreen modes fail we switch to
	// trying the windowing ones.

	for (j=0; j < 2; j++, SCREENWINDOW = !SCREENWINDOW)
	{
		defaultMode.windowed = SCREENWINDOW;
		idx = V_FindClosestResolution(&defaultMode, false, false);

		if (idx == -1)
			continue;

		setMode = scrmode[idx];
		setsizeneeded = true;

		I_Warning("Requested mode not available.  Trying %dx%dx%dc %s...\n",
			setMode.width, setMode.height, 1 << setMode.depth,
			setMode.windowed ? "(Windowed)" : "(Fullscreen)");

		if (DoExecuteChangeResolution())
			return;

		// Should not happen, that mode was in the avail list. 
		// Last ditch effort: try all other modes in avail list.

		for (i=0; i < numscrmodes; i++)
		{
			if (scrmode[i].windowed != SCREENWINDOW)
				continue;

			setMode = scrmode[i];
			setsizeneeded = true;

			// ignore ones we've already tried
			if (i == idx)
				continue;

			if (V_CompareModes(&setMode, &wantedMode) == 0)
				continue;

			if (V_CompareModes(&setMode, &oldMode) == 0)
				continue;

			I_Warning("Requested mode not available.  Trying %dx%dx%dc %s...\n",
				setMode.width, setMode.height, 1 << setMode.depth,
				setMode.windowed ? "(Windowed)" : "(Fullscreen)");

			if (DoExecuteChangeResolution())
				return;
		}
	}

	// Ouch. Nothing worked! Quit.

	I_Error(DDF_LanguageLookup("ModeSelErrT"), SCREENWIDTH, SCREENHEIGHT, 
		1 << SCREENBITS);
}

//
// R_Init
//
// Called once at startup, to initialise some rendering stuff.
//
bool R_Init(void)
{
	R_SetViewSize(screenblocks);
	I_Printf(".");
	InitLightTables();
	I_Printf(".");
	I_Printf(".");
	R_SetNormalFOV((angle_t)(cfgnormalfov * (angle_t)((float)ANG45 / 45.0f)));
	I_Printf(".");
	R_SetZoomedFOV((angle_t)(cfgzoomedfov * (angle_t)((float)ANG45 / 45.0f)));
	I_Printf(".");
	R_SetFOV(normalfov);
	I_Printf(".");
	framecount = 0;

	return true;
}

//
// R_PointInSubsector
//
subsector_t *R_PointInSubsector(float x, float y)
{
	node_t *node;
	int side;
	int nodenum;

	nodenum = root_node;

	while (!(nodenum & NF_SUBSECTOR))
	{
		node = &nodes[nodenum];
		side = P_PointOnDivlineSide(x, y, &node->div);
		nodenum = node->children[side];
	}

	return &subsectors[nodenum & ~NF_SUBSECTOR];
}

//
// R_PointGetProps
//
region_properties_t *R_PointGetProps(subsector_t *sub, float z)
{
	extrafloor_t *S, *L, *C;
	float floor_h;

	// traverse extrafloors upwards

	floor_h = sub->sector->f_h;

	S = sub->sector->bottom_ef;
	L = sub->sector->bottom_liq;

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

		DEV_ASSERT2(C);

		// ignore liquids in the middle of THICK solids, or below real
		// floor or above real ceiling
		//
		if (C->bottom_h < floor_h || C->bottom_h > sub->sector->c_h)
			continue;

		if (z < C->top_h)
			return C->p;

		floor_h = C->top_h;
	}

	// extrafloors were exhausted, must be top area
	return sub->sector->p;
}

int telept_effect = 0;
int telept_flash = 1;
int telept_reverse = 0;

void R_StartFading(int start, int range)
{
	telept_wipeinfo = WIPE_InitWipe(&screenvb->screen, 0, 0,
		&screenvb->screen, 0, 0, true,
		&screenvb->screen, 0, 0, false,
		viewwindowwidth, viewwindowheight, telept_wipeinfo,
		range, telept_reverse?true:false, (wipetype_e)telept_effect);

	telept_active = true;
	telept_starttic = start + leveltime;
}

//
// R_SetupFrame
//
// -ES- 1999/07/21 Exported most of this one to the camera section of
// r_vbinit.c
//
static void SetupFrame(camera_t * camera, view_t * v)
{
	// init all the globals
	R_CallCallbackList(camera->frame_start);

	// do some more stuff
	viewsin = M_Sin(viewangle);
	viewcos = M_Cos(viewangle);

	framecount++;
	validcount++;
}

//
// R_RenderView
//
void R_RenderViewBitmap(viewbitmap_t * vb)
{
	view_t *v;
	aspect_t *a;

	colfunc = basecolfunc = R_DrawColumn;
	fuzzcolfunc = R_DrawFuzzColumn;
	transcolfunc = R_DrawTranslatedColumn;
	spanfunc = R_DrawSpan;
	trans_spanfunc = R_DrawTranslucentSpan;

	R_CallCallbackList(vb->frame_start);

	R_SetActiveViewBitmap(vb);

	for (v = vb->views; v; v = v->vbnext)
	{
		a = v->aspect;

		SetupFrame(v->camera, v);

		R_AspectChangeY(a, a->y_distunit, M_Tan(viewvertangle));

		R_CallCallbackList(v->frame_start);

		R_SetActiveView(v);

		// we don't need to do anything if the view is invisible
		// we still have to call frame_end though, in case it would be used for
		// some sort of cleanup after frame_start, or if it would change something
		// in the view.
		if (v->screen.width > 0 && v->screen.height > 0)
		{
			if (v->renderflags & VRF_VIEW)
			{
				// -ES- FIXME: Clean up renderflags stuff.
				// Each view should have its own renderer, which can be truebsp,
				// non-truebsp or psprite. Needs cleanup of code.

				// check for new console commands.
				E_NetUpdate();

#ifdef USE_GL
				RGL_RenderTrueBSP();
#else
				R2_RenderTrueBSP();
#endif
				// Check for new console commands.
				E_NetUpdate();
			}
#ifndef USE_GL
			if (v->renderflags & VRF_PSPR && v->camera->view_obj->player)
			{
				// Draw the player sprites.
				// -ES- 1999/05/27 Moved psprite code here.
				focusyfrac = (float)viewheight / 2;

				R2_DrawPlayerSprites(v->camera->view_obj->player);
			}
#endif
		}
		R_CallCallbackList(v->frame_end);
		R_CallCallbackList(v->camera->frame_end);
	}

	R_CallCallbackList(vb->frame_end);
}

//
// R_SetActiveViewBitmap
//
// Changes the view bitmap to draw to.
// Currently supports:
// identically sized viewbmps
//
void R_SetActiveViewBitmap(viewbitmap_t * vb)
{
	if (curviewbmp == vb)
		return;

	curviewbmp = vb;

	vb_h = vb->screen.height;
	vb_w = vb->screen.width;

	// only need to re-init if pitch has changed.
	if (vb->screen.pitch != vb_pitch)
	{
		vb_pitch = vb->screen.pitch;
		I_PrepareAssembler();
	}
}

//
// R_SetActiveView
//
// Changes the view to draw.
//
void R_SetActiveView(view_t * v)
{
	aspect_t *a;

	curview = v;

	a = v->aspect;

	// error if not the appropriate vb is used. Could call
	// R_SetActiveViewBitmap instead, but you should always do that manually
	// before calling this one.
	if (v->parent != curviewbmp)
		I_Error("R_SetActiveView: The wrong viewbitmap is used!");

	///  columnofs = v->columnofs;
	///  ylookup = v->ylookup;

	viewwidth = v->screen.width;
	viewheight = v->screen.height;

	focusxfrac = a->focusxfrac - (float)v->aspect_x;
	focusyfrac = a->focusyfrac - (float)v->aspect_y;

	viewangletox = v->viewangletox;
	xtoviewangle = v->xtoviewangle;

	distscale = v->distscale;

	yslope = v->yslope;

	topslope = a->topslope - (a->topslope - a->bottomslope) * v->aspect_y / a->maxheight;
	bottomslope = a->topslope - (a->topslope - a->bottomslope) * (v->aspect_y + viewheight) / a->maxheight;

#ifndef USE_GL
	topslope += a->fakefocusslope;
	bottomslope += a->fakefocusslope;
#endif

	x_distunit = a->x_distunit;
	y_distunit = a->y_distunit;

	// This is no more time-critical, so it can be called every frame...
	InitTextureMapping();
}
