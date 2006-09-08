//----------------------------------------------------------------------------
//  EDGE Specials Lines, Elevator & Floor Code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2005  The EDGE Team.
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
// -KM-  1998/09/01 Lines.ddf
// -ACB- 1998/09/13 Cleaned Up.
// -ACB- 2001/01/14 Added Elevator Types
//

#ifndef __P_SPEC__
#define __P_SPEC__

#include "e_player.h"
#include "ddf_main.h"
#include "r_defs.h"
#include "w_image.h"

#define MENU_GRAV_NORMAL  8

typedef enum
{
	MDT_INVALID  = 0,
	MDT_ELEVATOR = 1,
	MDT_PLANE    = 2,
	MDT_SLIDER   = 3,
	ENDOFMDTTYPES
}
movedat_e;

typedef struct light_s
{
	// type of light effect
	const lightdef_c *type;

	sector_t *sector;

	// countdown value to next change, or 0 if disabled
	int count;

	// dark and bright levels
	int minlight;
	int maxlight;

	// current direction for GLOW type, -1 down, +1 up
	int direction;

	// countdown value for FADE type
	int fade_count;

	struct light_s *prev, *next;
}
light_t;

typedef enum
{
	BWH_None,
	BWH_Top,
	BWH_Middle,
	BWH_Bottom
}
bwhere_e;

typedef struct button_s
{
	line_t *line;
	bwhere_e where;
	const image_t *bimage;
	int btimer;
	sfx_t *off_sound;
}
button_t;

// --> Button list class
class buttonlist_c : public epi::array_c
{
public:
	buttonlist_c() : epi::array_c(sizeof(button_t)) {}
	~buttonlist_c() { Clear(); }

private:
	void CleanupObject(void *obj) { /* ... */ }

public:
	int Find(button_t *b);
	button_t* GetNew();
	int GetSize() {	return array_entries; } 
	bool IsPressed(line_t* line);
	void SetSize(int count);
	
	button_t* operator[](int idx) { return (button_t*)FetchObject(idx); } 
};

typedef struct gen_move_s
{
	movedat_e whatiam;
	struct gen_move_s *next, *prev;
}
gen_move_t;

typedef enum
{
    DIRECTION_UP     = +1,
    DIRECTION_WAIT   =  0,
    DIRECTION_DOWN   = -1,
    DIRECTION_STASIS = -2
}
plane_dir_e;

typedef struct elev_move_s
{
	movedat_e whatiam;
	struct elev_move_s *next, *prev;

	const elevatordef_c *type;
	sector_t *sector;

	float startheight;
	float destheight;
	float speed;

	// 1 = up, 0 = waiting at top, -1 = down
	int direction;
	int olddirection;

	int tag;

	// tics to wait when fully open
	int waited;

	bool sfxstarted;

	int newspecial;

	const image_t *new_ceiling_image;
	const image_t *new_floor_image;
}
elev_move_t;

typedef struct plane_move_s
{
	movedat_e whatiam;
	struct plane_move_s *next, *prev;

	const movplanedef_c *type;
	sector_t *sector;

	bool is_ceiling;

	float startheight;
	float destheight;
	float speed;
	bool crush;

	// 1 = up, 0 = waiting at top, -1 = down
	int direction;
	int olddirection;

	int tag;

	// tics to wait when fully open
	int waited;

	bool sfxstarted;

	int newspecial;
	const image_t *new_image;
}
plane_move_t;

typedef struct slider_move_s
{
	movedat_e whatiam;
	struct slider_move_s *next, *prev;

	const sliding_door_c *info;
	line_t *line;

	// current distance it has opened
	float opening;

	// target distance
	float target;

	// length of line
	float line_len;

	// 1 = opening, 0 = waiting, -1 = closing
	int direction;

	// tics to wait at the top
	int waited;

	bool sfxstarted;
	bool final_open;
}
slider_move_t;

// End-level timer (-TIMER option)
extern bool levelTimer;
extern int levelTimeCount;

extern buttonlist_c buttonlist;

extern light_t *lights;
extern gen_move_t *active_movparts;

extern linetype_c donut[2];

// at map load
void P_SpawnSpecials(int autotag);

// at map exit
void P_StopAmbientSectorSfx(void);

// every tic
void P_UpdateSpecials(void);

// when needed
bool P_UseSpecialLine(mobj_t * thing, line_t * line, int side,
    float open_bottom, float open_top);
bool P_CrossSpecialLine(line_t *ld, int side, mobj_t * thing);
void P_ShootSpecialLine(line_t *ld, int side, mobj_t * thing);
void P_RemoteActivation(mobj_t * thing, int typenum, int tag, 
    int side, trigger_e method);
void P_PlayerInSpecialSector(player_t * player, sector_t *sec);

// Utilities...
int P_TwoSided(int sector, int line);
side_t *P_GetSide(int currentSector, int line, int side);
sector_t *P_GetSector(int currentSector, int line, int side);
sector_t *P_GetNextSector(const line_t * line, const sector_t * sec);

// Info Needs....
float P_FindSurroundingHeight(const heightref_e ref, const sector_t *sec);
float P_FindRaiseToTexture(sector_t * sec);  // -KM- 1998/09/01 New func, old inline
sector_t *P_FindSectorFromTag(int tag);
int P_FindMinSurroundingLight(sector_t * sector, int max);

// start an action...
bool EV_Lights(sector_t * sec, const lightdef_c * type);

void P_RunActiveSectors(void);

void P_RemoveAllActiveParts(void);
void P_AddActivePart(gen_move_t *movpart);

extern line_t * line_speciallist;
extern sector_t * sect_speciallist;
void P_AddSpecialLine(line_t *ld);
void P_AddSpecialSector(sector_t *sec);

void P_RunLights(void);
light_t *P_NewLight(void);
void P_DestroyAllLights(void);
void P_RunSectorSFX(void);
void P_DestroyAllSectorSFX(void);

void EV_LightTurnOn(int tag, int bright);
bool EV_DoDonut(sector_t * s1, sfx_t * sfx[4]);
bool EV_Teleport(line_t * line, int tag, mobj_t *thing, const teleportdef_c *def);
bool EV_ManualPlane(line_t * line, mobj_t * thing, const movplanedef_c * type);
bool EV_ManualElevator(line_t * line, mobj_t * thing, const elevatordef_c * type);

void EV_DoSlider(line_t * line, mobj_t * thing, const sliding_door_c * s);
bool EV_DoPlane(sector_t * sec, const movplanedef_c * type, sector_t * model);
bool EV_DoElevator(sector_t * sec, const elevatordef_c * type, sector_t * model);

void P_AddPointForce(sector_t *sec, float length);
void P_AddSectorForce(sector_t *sec, bool is_wind, float x_mag, float y_mag);

//
//  P_SWITCH
//
void P_InitSwitchList(void);
void P_ChangeSwitchTexture(line_t * line, bool useAgain, line_special_e specials, bool noSound);

#endif
