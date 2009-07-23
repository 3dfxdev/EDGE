//----------------------------------------------------------------------------
//  EDGE New SaveGame Handling (Miscellaneous)
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
//
// See the file "docs/save_sys.txt" for a complete description of the
// new savegame system.
//
// This file handles
//    light_t         [LITE]
//    button_t        [BUTN]
//    rad_trigger_t   [TRIG]
//    drawtip_t       [DTIP]
//
//    plane_move_t    [PMOV]
//    slider_move_t   [SMOV]
//
// TODO HERE:
//   +  Fix donuts.
//   -  Button off_sound field.
//

#include "i_defs.h"

#include "epi/str_format.h"

#include "rad_trig.h"
#include "sv_chunk.h"
#include "sv_main.h"
#include "z_zone.h"


#undef SF
#define SF  SVFIELD


// forward decls.
int SV_ButtonCountElems(void);
int SV_ButtonFindElem(button_t *elem);
void * SV_ButtonGetElem(int index);
void SV_ButtonCreateElems(int num_elems);
void SV_ButtonFinaliseElems(void);

int SV_LightCountElems(void);
int SV_LightFindElem(light_t *elem);
void * SV_LightGetElem(int index);
void SV_LightCreateElems(int num_elems);
void SV_LightFinaliseElems(void);

int SV_TriggerCountElems(void);
int SV_TriggerFindElem(rad_trigger_t *elem);
void * SV_TriggerGetElem(int index);
void SV_TriggerCreateElems(int num_elems);
void SV_TriggerFinaliseElems(void);

int SV_TipCountElems(void);
int SV_TipFindElem(drawtip_t *elem);
void * SV_TipGetElem(int index);
void SV_TipCreateElems(int num_elems);
void SV_TipFinaliseElems(void);

int SV_PlaneMoveCountElems(void);
int SV_PlaneMoveFindElem(plane_move_t *elem);
void * SV_PlaneMoveGetElem(int index);
void SV_PlaneMoveCreateElems(int num_elems);
void SV_PlaneMoveFinaliseElems(void);

int SV_SliderMoveCountElems(void);
int SV_SliderMoveFindElem(plane_move_t *elem);
void * SV_SliderMoveGetElem(int index);
void SV_SliderMoveCreateElems(int num_elems);
void SV_SliderMoveFinaliseElems(void);


bool SR_LightGetType(void *storage, int index, void *extra);
void SR_LightPutType(void *storage, int index, void *extra);

bool SR_TriggerGetScript(void *storage, int index, void *extra);
void SR_TriggerPutScript(void *storage, int index, void *extra);

bool SR_TriggerGetState(void *storage, int index, void *extra);
void SR_TriggerPutState(void *storage, int index, void *extra);

bool SR_TipGetString(void *storage, int index, void *extra);
void SR_TipPutString(void *storage, int index, void *extra);

bool SR_PlaneMoveGetType(void *storage, int index, void *extra);
void SR_PlaneMovePutType(void *storage, int index, void *extra);

bool SR_SliderGetInfo(void *storage, int index, void *extra);
void SR_SliderPutInfo(void *storage, int index, void *extra);


//----------------------------------------------------------------------------
//
//  BUTTON STRUCTURE
//
static button_t sv_dummy_button;

#define SV_F_BASE  sv_dummy_button

static savefield_t sv_fields_button[] =
{
	SF(line, "line", 1, SVT_INDEX("lines"), 
	SR_LineGetLine, SR_LinePutLine),
	SF(where, "where", 1, SVT_ENUM, SR_GetEnum, SR_PutEnum),
	SF(bimage, "bimage", 1, SVT_STRING, SR_LevelGetImage, SR_LevelPutImage),
	SF(btimer, "btimer", 1, SVT_INT, SR_GetInt, SR_PutInt),

	// FIXME: off_sound

	SVFIELD_END
};

savestruct_t sv_struct_button =
{
	NULL,              // link in list
	"button_t",        // structure name
	"butn",            // start marker
	sv_fields_button,  // field descriptions
  	SVDUMMY,           // dummy base
	true,              // define_me
	NULL               // pointer to known struct
};

#undef SV_F_BASE

savearray_t sv_array_button =
{
	NULL,               // link in list
	"buttonlist",       // array name
	&sv_struct_button,  // array type
	true,               // define_me
	true,               // allow_hub

	SV_ButtonCountElems,     // count routine
	SV_ButtonGetElem,        // index routine
	SV_ButtonCreateElems,    // creation routine
	SV_ButtonFinaliseElems,  // finalisation routine

	NULL,     // pointer to known array
	0         // loaded size
};


//----------------------------------------------------------------------------
//
//  LIGHT STRUCTURE
//
static light_t sv_dummy_light;

#define SV_F_BASE  sv_dummy_light

static savefield_t sv_fields_light[] =
{
	SF(type, "type", 1, SVT_STRING, SR_LightGetType, SR_LightPutType),
	SF(sector, "sector", 1, SVT_INDEX("sectors"), 
	SR_SectorGetSector, SR_SectorPutSector),
	SF(count, "count", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(minlight, "minlight", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(maxlight, "maxlight", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(direction, "direction", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(fade_count, "fade_count", 1, SVT_INT, SR_GetInt, SR_PutInt),

	// NOT HERE:
	//   - prev & next: automatically regenerated

	SVFIELD_END
};

savestruct_t sv_struct_light =
{
	NULL,              // link in list
	"light_t",         // structure name
	"lite",            // start marker
	sv_fields_light,   // field descriptions
  	SVDUMMY,           // dummy base
	true,              // define_me
	NULL               // pointer to known struct
};

#undef SV_F_BASE

savearray_t sv_array_light =
{
	NULL,              // link in list
	"lights",          // array name
	&sv_struct_light,  // array type
	true,              // define_me
	true,              // allow_hub

	SV_LightCountElems,     // count routine
	SV_LightGetElem,        // index routine
	SV_LightCreateElems,    // creation routine
	SV_LightFinaliseElems,  // finalisation routine

	NULL,     // pointer to known array
	0         // loaded size
};


//----------------------------------------------------------------------------
//
//  TRIGGER STRUCTURE
//
static rad_trigger_t sv_dummy_trigger;

#define SV_F_BASE  sv_dummy_trigger

static savefield_t sv_fields_trigger[] =
{
	SF(info, "info", 1, SVT_STRING,
	SR_TriggerGetScript, SR_TriggerPutScript),

	SF(disabled, "disabled", 1, SVT_BOOLEAN, SR_GetBoolean, SR_PutBoolean),
	SF(activated, "activated", 1, SVT_BOOLEAN, SR_GetBoolean, SR_PutBoolean),
	SF(acti_players, "acti_players", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(repeats_left, "repeats_left", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(repeat_delay, "repeat_delay", 1, SVT_INT, SR_GetInt, SR_PutInt),

	SF(state, "state", 1, SVT_INT, SR_TriggerGetState, SR_TriggerPutState),
	SF(wait_tics, "wait_tics", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(tip_slot, "tip_slot", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(menu_style_name, "menu_style_name", 1, SVT_STRING,
		SR_TipGetString, SR_TipPutString),
	SF(menu_result, "menu_result", 1, SVT_INT, SR_GetInt, SR_PutInt),

	// NOT HERE
	//   - next & prev: can be regenerated.
	//   - tag_next & tag_prev: ditto
	//   - sound: can be recomputed.
	//   - last_con_message: doesn't matter.

	SVFIELD_END
};

savestruct_t sv_struct_trigger =
{
	NULL,               // link in list
	"rad_trigger_t",    // structure name
	"trig",             // start marker
	sv_fields_trigger,  // field descriptions
  	SVDUMMY,            // dummy base
	true,               // define_me
	NULL                // pointer to known struct
};

#undef SV_F_BASE

savearray_t sv_array_trigger =
{
	NULL,               // link in list
	"r_triggers",       // array name
	&sv_struct_trigger, // array type
	true,               // define_me
	true,               // allow_hub

	SV_TriggerCountElems,     // count routine
	SV_TriggerGetElem,        // index routine
	SV_TriggerCreateElems,    // creation routine
	SV_TriggerFinaliseElems,  // finalisation routine

	NULL,     // pointer to known array
	0         // loaded size
};


//----------------------------------------------------------------------------
//
//  DRAWTIP STRUCTURE
//
static drawtip_t sv_dummy_drawtip;

#define SV_F_BASE  sv_dummy_drawtip

static savefield_t sv_fields_drawtip[] =
{
	// treating the `p' sub-struct here as if the fields were directly
	// in drawtip_t.

	SF(p.x_pos, "x_pos", 1, SVT_PERCENT, SR_GetPercent, SR_PutPercent),
	SF(p.y_pos, "y_pos", 1, SVT_PERCENT, SR_GetPercent, SR_PutPercent),
	SF(p.left_just, "left_just", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(p.translucency, "translucency", 1, SVT_PERCENT, SR_GetPercent, SR_PutPercent),

	SF(delay, "delay", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(tip_text, "tip_text", 1, SVT_STRING, SR_TipGetString, SR_TipPutString),
	SF(tip_graphic, "tip_graphic", 1, SVT_STRING, SR_LevelGetImage, SR_LevelPutImage),
	SF(playsound, "playsound", 1, SVT_BOOLEAN, SR_GetBoolean, SR_PutBoolean),
	SF(fade_time, "fade_time", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(fade_target, "fade_target", 1, SVT_FLOAT, SR_GetFloat, SR_PutFloat),
	SF(color, "color", 1, SVT_RGBCOL, SR_GetRGB, SR_PutRGB),

	// NOT HERE:
	//    p.slot_num, p.time: not used withing drawtip_t
	//    dirty: this is set in the finalizer
	//    hu_*: these are regenerated on next display
	//    p.colourmap_name: deprecated field
	//    p.colmap: deprecated field

	SVFIELD_END
};

savestruct_t sv_struct_drawtip =
{
	NULL,              // link in list
	"drawtip_t",       // structure name
	"dtip",            // start marker
	sv_fields_drawtip, // field descriptions
  	SVDUMMY,           // dummy base
	true,              // define_me
	NULL               // pointer to known struct
};

#undef SV_F_BASE

savearray_t sv_array_drawtip =
{
	NULL,               // link in list
	"tip_slots",        // array name
	&sv_struct_drawtip, // array type
	true,               // define_me
	true,               // allow_hub

	SV_TipCountElems,     // count routine
	SV_TipGetElem,        // index routine
	SV_TipCreateElems,    // creation routine
	SV_TipFinaliseElems,  // finalisation routine

	NULL,     // pointer to known array
	0         // loaded size
};


//----------------------------------------------------------------------------
//
//  PLANEMOVE STRUCTURE
//
static plane_move_t sv_dummy_plane_move;

#define SV_F_BASE  sv_dummy_plane_move

static savefield_t sv_fields_plane_move[] =
{
	SF(type, "type", 1, SVT_STRING, SR_PlaneMoveGetType, SR_PlaneMovePutType),
	SF(sector, "sector", 1, SVT_INDEX("sectors"), 
		SR_SectorGetSector, SR_SectorPutSector),

	SF(is_ceiling, "is_ceiling", 1, SVT_BOOLEAN, SR_GetBoolean, SR_PutBoolean),
	SF(is_elevator, "is_elevator", 1, SVT_BOOLEAN, SR_GetBoolean, SR_PutBoolean),
	SF(startheight, "startheight", 1, SVT_FLOAT, SR_GetFloat, SR_PutFloat),
	SF(destheight, "destheight", 1, SVT_FLOAT, SR_GetFloat, SR_PutFloat),
	SF(elev_height, "elevheight", 1, SVT_FLOAT, SR_GetFloat, SR_PutFloat),
	SF(speed, "speed", 1, SVT_FLOAT, SR_GetFloat, SR_PutFloat),
	SF(crush, "crush", 1, SVT_BOOLEAN, SR_GetBoolean, SR_PutBoolean),

	SF(direction, "direction", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(olddirection, "olddirection", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(tag, "tag", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(waited, "waited", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(sfxstarted, "sfxstarted", 1, SVT_BOOLEAN, SR_GetBoolean, SR_PutBoolean),

	SF(newspecial, "newspecial", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(new_image, "new_image", 1, SVT_STRING, SR_LevelGetImage, SR_LevelPutImage),

	SVFIELD_END
};

savestruct_t sv_struct_plane_move =
{
	NULL,                  // link in list
	"plane_move_t",        // structure name
	"pmov",                // start marker
	sv_fields_plane_move,  // field descriptions
	SVDUMMY,               // dummy base
	true,                  // define_me
	NULL                   // pointer to known struct
};

#undef SV_F_BASE

savearray_t sv_array_plane_move =
{
	NULL,                   // link in list
	"plane_movers",         // array name (virtual list)
	&sv_struct_plane_move,  // array type
	true,                   // define_me
	true,                   // allow_hub

	SV_PlaneMoveCountElems,     // count routine
	SV_PlaneMoveGetElem,        // index routine
	SV_PlaneMoveCreateElems,    // creation routine
	SV_PlaneMoveFinaliseElems,  // finalisation routine

	NULL,     // pointer to known array
	0         // loaded size
};


//----------------------------------------------------------------------------
//
//  SLIDERMOVE STRUCTURE
//
static slider_move_t sv_dummy_slider_move;

#define SV_F_BASE  sv_dummy_slider_move

static savefield_t sv_fields_slider_move[] =
{
	SF(info, "info", 1, SVT_STRING, SR_SliderGetInfo, SR_SliderPutInfo),
	SF(line, "line", 1, SVT_INDEX("lines"), SR_LineGetLine, SR_LinePutLine),

	SF(opening, "opening", 1, SVT_FLOAT, SR_GetFloat, SR_PutFloat),
	SF(target, "target", 1, SVT_FLOAT, SR_GetFloat, SR_PutFloat),

	SF(direction, "direction", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(waited, "waited", 1, SVT_INT, SR_GetInt, SR_PutInt),

	SF(sfxstarted, "sfxstarted", 1, SVT_BOOLEAN, SR_GetBoolean, SR_PutBoolean),
	SF(final_open, "final_open", 1, SVT_BOOLEAN, SR_GetBoolean, SR_PutBoolean),

	// NOT HERE:
	//   - line_len (can recreate)

	SVFIELD_END
};

savestruct_t sv_struct_slider_move =
{
	NULL,                  // link in list
	"slider_move_t",       // structure name
	"pmov",                // start marker
	sv_fields_slider_move, // field descriptions
	SVDUMMY,               // dummy base
	true,                  // define_me
	NULL                   // pointer to known struct
};

#undef SV_F_BASE

savearray_t sv_array_slider_move =
{
	NULL,                   // link in list
	"active_sliders",       // array name (virtual list)
	&sv_struct_slider_move, // array type
	true,                   // define_me
	true,                   // allow_hub

	SV_SliderMoveCountElems,     // count routine
	SV_SliderMoveGetElem,        // index routine
	SV_SliderMoveCreateElems,    // creation routine
	SV_SliderMoveFinaliseElems,  // finalisation routine

	NULL,     // pointer to known array
	0         // loaded size
};


//----------------------------------------------------------------------------

extern std::vector<button_t *> active_buttons;

int SV_ButtonCountElems(void)
{
	// Note: also saves the unused button_ts (btimer == 0)
	return (int)active_buttons.size();
}

void *SV_ButtonGetElem(int index)
{
	if (index < 0 || index >= (int)active_buttons.size())
	{
		I_Warning("LOADGAME: Invalid Button: %d\n", index);
		index = 0;
	}	

	return active_buttons[index];
}

int SV_ButtonFindElem(button_t *elem)
{
	int index = 0;

	std::vector<button_t *>::iterator LI;

	for (LI=active_buttons.begin(); LI != active_buttons.end() && (*LI) != elem; LI++)
		index++;

	if (LI == active_buttons.end())
		I_Error("LOADGAME: No such LightPtr: %p\n", elem);

	return index;
}

void SV_ButtonCreateElems(int num_elems)
{
	P_ClearButtons();
	
	for (; num_elems > 0; num_elems--)
	{
		button_t *b = new button_t;

		Z_Clear(b, button_t, 1);

		active_buttons.push_back(b);
	}
}

void SV_ButtonFinaliseElems(void)
{
	// nothing to do
}


//----------------------------------------------------------------------------

extern std::vector<light_t *> active_lights;

int SV_LightCountElems(void)
{
	return (int)active_lights.size();
}

void *SV_LightGetElem(int index)
{
	if (index < 0 || index >= (int)active_lights.size())
		I_Error("LOADGAME: Invalid Light: %d\n", index);

	return active_lights[index];
}

int SV_LightFindElem(light_t *elem)
{
	int index = 0;

	std::vector<light_t *>::iterator LI;

	for (LI=active_lights.begin(); LI != active_lights.end() && (*LI) != elem; LI++)
		index++;

	if (LI == active_lights.end())
		I_Error("LOADGAME: No such LightPtr: %p\n", elem);

	return index;
}

void SV_LightCreateElems(int num_elems)
{
	P_DestroyAllLights();

	for (; num_elems > 0; num_elems--)
	{
		light_t *cur = P_NewLight();

		// initialise defaults
		cur->type = &sectortypes[0]->l;
		cur->sector = sectors + 0;
	}
}

void SV_LightFinaliseElems(void)
{
	// nothing to do
}


//----------------------------------------------------------------------------

int SV_TriggerCountElems(void)
{
	rad_trigger_t *cur;
	int count;

	for (cur=active_triggers, count=0; cur; cur=cur->next, count++)
	{ /* nothing here */ }

	return count;
}

void *SV_TriggerGetElem(int index)
{
	rad_trigger_t *cur;

	for (cur=active_triggers; cur && index > 0; cur=cur->next)
		index--;

	if (! cur)
		I_Error("LOADGAME: Invalid Trigger: %d\n", index);

	SYS_ASSERT(index == 0);
	return cur;
}

int SV_TriggerFindElem(rad_trigger_t *elem)
{
	rad_trigger_t *cur;
	int index;

	for (cur=active_triggers, index=0; cur && cur != elem; cur=cur->next)
		index++;

	if (! cur)
		I_Error("LOADGAME: No such TriggerPtr: %p\n", elem);

	return index;
}

void SV_TriggerCreateElems(int num_elems)
{
	RAD_ClearTriggers();

	for (; num_elems > 0; num_elems--)
	{
		rad_trigger_t *cur = Z_New(rad_trigger_t, 1);

		Z_Clear(cur, rad_trigger_t, 1);

		// link it in
		cur->next = active_triggers;
		cur->prev = NULL;

		if (active_triggers)
			active_triggers->prev = cur;

		active_triggers = cur;

		// initialise defaults
		cur->info = r_scripts;
		cur->state = r_scripts ? r_scripts->first_state : NULL;
		cur->disabled = true;
	}
}

void SV_TriggerFinaliseElems(void)
{
	rad_trigger_t *cur;

	for (cur=active_triggers; cur; cur=cur->next)
	{
		RAD_GroupTriggerTags(cur);
	}
}


//----------------------------------------------------------------------------

int SV_TipCountElems(void)
{
	return MAXTIPSLOT;
}

void *SV_TipGetElem(int index)
{
	if (index < 0 || index >= MAXTIPSLOT)
	{
		I_Warning("LOADGAME: Invalid Tip: %d\n", index);
		index = MAXTIPSLOT-1;
	}

	return tip_slots + index;
}

int SV_TipFindElem(drawtip_t *elem)
{
	SYS_ASSERT(tip_slots <= elem && elem < (tip_slots + MAXTIPSLOT));

	return elem - tip_slots;
}

void SV_TipCreateElems(int num_elems)
{
	RAD_ResetTips();
}

void SV_TipFinaliseElems(void)
{
	int i;

	// mark all active tip slots as dirty
	for (i=0; i < MAXTIPSLOT; i++)
	{
		if (tip_slots[i].delay > 0)
			tip_slots[i].dirty = true;
	}
}


//----------------------------------------------------------------------------

extern std::vector<plane_move_t *> active_planes;

int SV_PlaneMoveCountElems(void)
{
	return (int)active_planes.size();
}

void *SV_PlaneMoveGetElem(int index)
{
	// Note: the index value starts at 0.

	if (index < 0 || index >= (int)active_planes.size())
		I_Error("LOADGAME: Invalid PlaneMove: %d\n", index);

	return active_planes[index];
}

int SV_PlaneMoveFindElem(plane_move_t *elem)
{
	// returns the index value (starts at 0).

	int index = 0;

	std::vector<plane_move_t *>::iterator PMI;

	for (PMI = active_planes.begin();
		 PMI != active_planes.end() && (*PMI) != elem;
		 PMI++)
	{
		index++;
	}

	if (PMI == active_planes.end())
		I_Error("LOADGAME: No such PlaneMove: %p\n", elem);

	return index;
}

void SV_PlaneMoveCreateElems(int num_elems)
{
	P_DestroyAllPlanes();

	for (; num_elems > 0; num_elems--)
	{
		plane_move_t *pmov = new plane_move_t;

		Z_Clear(pmov, plane_move_t, 1);

		// link it in
		P_AddActivePlane(pmov);
	}
}

void SV_PlaneMoveFinaliseElems(void)
{
	// nothing to do
}


//----------------------------------------------------------------------------

extern std::vector<slider_move_t *> active_sliders;

int SV_SliderMoveCountElems(void)
{
	return (int)active_sliders.size();
}

void *SV_SliderMoveGetElem(int index)
{
	// Note: the index value starts at 0.

	if (index < 0 || index >= (int)active_sliders.size())
		I_Error("LOADGAME: Invalid SliderMove: %d\n", index);

	return active_sliders[index];
}

int SV_SliderMoveFindElem(slider_move_t *elem)
{
	// returns the index value (starts at 0).

	int index = 0;

	std::vector<slider_move_t *>::iterator SMI;

	for (SMI = active_sliders.begin();
		 SMI != active_sliders.end() && (*SMI) != elem;
		 SMI++)
	{
		index++;
	}

	if (SMI == active_sliders.end())
		I_Error("LOADGAME: No such SliderMove: %p\n", elem);

	return index;
}

void SV_SliderMoveCreateElems(int num_elems)
{
	P_DestroyAllSliders();

	for (; num_elems > 0; num_elems--)
	{
		slider_move_t *smov = new slider_move_t;

		Z_Clear(smov, slider_move_t, 1);

		// link it in
		P_AddActiveSlider(smov);
	}
}

void SV_SliderMoveFinaliseElems(void)
{
	std::vector<slider_move_t *>::iterator SMI;

	for (SMI = active_sliders.begin();
		 SMI != active_sliders.end();
		 SMI++)
	{
		slider_move_t *smov = *SMI;

		if (smov->line)
			smov->line_len = R_PointToDist(0, 0, smov->line->dx, smov->line->dy);
	}
}


//----------------------------------------------------------------------------

bool SR_LightGetType(void *storage, int index, void *extra)
{
	const lightdef_c ** dest = (const lightdef_c **)storage + index;

	int number;
	const char *str;

	str = SV_GetString();

	if (! str)
	{
		(*dest) = NULL;
		return true;
	}

	if (str[1] != ':')
		I_Error("SR_LightGetType: invalid lighttype `%s'\n", str);

	number = strtol(str + 2, NULL, 0);

	if (str[0] == 'S')
	{
		const sectortype_c *special = P_LookupSectorType(number);
		(*dest) = &special->l;
	}
	else if (str[0] == 'L')
	{
		const linetype_c *special = P_LookupLineType(number);
		(*dest) = &special->l;
	}
	else
		I_Error("SR_LightGetType: invalid lighttype `%s'\n", str);

	SV_FreeString(str);
	return true;
}

//
// SR_LightPutType
//
// Format of the string:
//
//   <source char>  `:'  <source ref>
//
// The source char determines where the lighttype_t is found: `S' in a
// sector type or `L' in a linedef type.  The source ref is the
// numeric ID of the sector/line type in DDF.
//
void SR_LightPutType(void *storage, int index, void *extra)
{
	const lightdef_c *src = ((const lightdef_c **)storage)[index];
	epi::array_iterator_c it;
	linetype_c *ln;
	sectortype_c *sec;

	if (! src)
	{
		SV_PutString(NULL);
		return;
	}

	// look for it in the line types
	for (it=linetypes.GetBaseIterator(); it.IsValid(); it++)
	{
		ln = ITERATOR_TO_TYPE(it, linetype_c*);
		if (src == &ln->l)
		{
			std::string s = epi::STR_Format("L:%s", ln->name.c_str());
			SV_PutString(s.c_str());
			return;
		}
	}

	// look for it in the sector types
	for (it=sectortypes.GetBaseIterator(); it.IsValid(); it++)
	{
		sec = ITERATOR_TO_TYPE(it, sectortype_c*);
		if (src == &sec->l)
		{
			std::string s = epi::STR_Format("S:%s", sec->name.c_str());
			SV_PutString(s.c_str());
			return;
		}
	}

	// not found !

	I_Warning("SAVEGAME: could not find lightdef_c %p !\n", src);
	SV_PutString("S:1");
}


bool SR_TriggerGetState(void *storage, int index, void *extra)
{
	const rts_state_t ** dest = (const rts_state_t **)storage + index;
	const rts_state_t *temp;

	int value;
	const rad_trigger_t *trig = (rad_trigger_t *) sv_current_elem;

	value = SV_GetInt();

	if (value == 0)
	{
		(*dest) = NULL;
		return true;
	}

	for (temp=trig->info->first_state; temp;
		temp=temp->next, value--)
	{
		if (value == 1)
			break;
	}

	if (! temp)
	{
		I_Warning("LOADGAME: invalid RTS state !\n");
		temp = trig->info->last_state;
	}

	(*dest) = temp;
	return true;
}

void SR_TriggerPutState(void *storage, int index, void *extra)
{
	const rts_state_t *src = ((const rts_state_t **)storage)[index];
	const rts_state_t *temp;

	int value;
	const rad_trigger_t *trig = (rad_trigger_t *) sv_current_elem;

	if (! src)
	{
		SV_PutInt(0);
		return;
	}

	// determine index value
	for (temp=trig->info->first_state, value=1; temp; 
		temp=temp->next, value++)
	{
		if (temp == src)
			break;
	}

	if (! temp)
		I_Error("INTERNAL ERROR: no such RTS state %p !\n", src);

	SV_PutInt(value);
}


bool SR_TriggerGetScript(void *storage, int index, void *extra)
{
	const rad_script_t ** dest = (const rad_script_t **)storage + index;
	const rad_script_t *temp;

	const char *swizzle;
	char buffer[256];
	char *base_p, *use_p;
	char *map_name;

	int idx_val;
	u32_t crc;

	swizzle = SV_GetString();

	if (! swizzle)
	{
		(*dest) = NULL;
		return true;
	}

	Z_StrNCpy(buffer, swizzle, 256-1);
	SV_FreeString(swizzle);

	if (buffer[0] != 'B' || buffer[1] != ':')
		I_Error("Corrupt savegame: bad script ref 1/4: `%s'\n", buffer);

	// get map name

	map_name = buffer + 2;
	base_p = strchr(map_name, ':');

	if (base_p == NULL || base_p == map_name || base_p[0] == 0)
		I_Error("Corrupt savegame: bad script ref 2/4: `%s'\n", map_name);

	// terminate the map name
	*base_p++ = 0;

	// get index value

	use_p = base_p;
	base_p = strchr(use_p, ':');

	if (base_p == NULL || base_p == use_p || base_p[0] == 0)
		I_Error("Corrupt savegame: bad script ref 3/4: `%s'\n", use_p);

	*base_p++ = 0;

	idx_val = strtol(use_p, NULL, 0);
	SYS_ASSERT(idx_val >= 1);

	// get CRC value

	crc = (u32_t) strtoul(base_p, NULL, 16);

	// now find the bugger !
	// FIXME: move into RTS code

	for (temp=r_scripts; temp; temp=temp->next)
	{
		if (DDF_CompareName(temp->mapid, map_name) != 0)
			continue;

		if (temp->crc.crc != crc)
			continue;

		if (idx_val == 1)
			break;

		idx_val--;
	}

	if (! temp)
	{
		I_Warning("LOADGAME: No such RTS script !!\n");
		temp = r_scripts;
	}

	(*dest) = temp;
	return true;
}

//
// SR_TriggerPutScript
//
// Format of the string:
//
//   `B'  `:'  <map>  `:'  <index>  `:'  <crc>
//
// The `B' is a format descriptor -- future changes should use other
// letters.  The CRC is used to find the radius script.  There may be
// several in the same map with the same CRC, and the `index' part is
// used to differentiate them.  Index values begin at 1.  The CRC
// value is in hexadecimal.
//
void SR_TriggerPutScript(void *storage, int index, void *extra)
{
	const rad_script_t *src = ((const rad_script_t **)storage)[index];
	const rad_script_t *temp;

	int idx_val;
	char buffer[256];

	if (! src)
	{
		SV_PutString(NULL);
		return;
	}

	// determine index idx_val
	// FIXME: move into RTS code
	for (temp=r_scripts, idx_val=1; temp; temp=temp->next)
	{
		if (DDF_CompareName(src->mapid, temp->mapid) != 0)
			continue;

		if (temp == src)
			break;

		if (temp->crc.crc == src->crc.crc)
			idx_val++;
	}

	if (! temp)
		I_Error("SR_TriggerPutScript: invalid ScriptPtr %p\n", src);

	sprintf(buffer, "B:%s:%d:%X", src->mapid, idx_val, src->crc.crc);

	SV_PutString(buffer);
}



//----------------------------------------------------------------------------

bool SR_TipGetString(void *storage, int index, void *extra)
{
	const char ** dest = (const char **)storage + index;

	SV_FreeString(*dest);

	(*dest) = SV_GetString();
	return true;
}

void SR_TipPutString(void *storage, int index, void *extra)
{
	const char *src = ((const char **)storage)[index];

	SV_PutString(src);
}


bool SR_PlaneMoveGetType(void *storage, int index, void *extra)
{
	const movplanedef_c ** dest = (const movplanedef_c **)storage + index;

	int number;
	bool is_ceil;
	const char *str;

	str = SV_GetString();

	if (! str)
	{
		(*dest) = NULL;
		return true;
	}

	if (str[1] != ':' || str[3] != ':')
		I_Error("SR_PlaneMoveGetType: invalid movestr `%s'\n", str);

	is_ceil = false;

	if (str[2] == 'F')
		;
	else if (str[2] == 'C')
		is_ceil = true;
	else
		I_Error("SR_PlaneMoveGetType: invalid floortype `%s'\n", str);

	number = strtol(str + 4, NULL, 0);

	if (str[0] == 'S')
	{
		const sectortype_c *special = P_LookupSectorType(number);
		(*dest) = is_ceil ? &special->c : &special->f;
	}
	else if (str[0] == 'L')
	{
		const linetype_c *special = P_LookupLineType(number);
		(*dest) = is_ceil ? &special->c : &special->f;
	}
	else if (str[0] == 'D')
	{
		// FIXME: this ain't gonna work, freddy
		(*dest) = is_ceil ? &donut[number].c : &donut[number].f;
	}
	else
		I_Error("SR_PlaneMoveGetType: invalid srctype `%s'\n", str);

	SV_FreeString(str);
	return true;
}

//
// Format of the string:
//
//   <line/sec>  `:'  <floor/ceil>  `:'  <ddf num>
//
// The first field contains `L' if the movplanedef_c is within a
// linetype_c, `S' for a sectortype_c, or `D' for the donut (which
// prolly won't work yet).  The second field is `F' for the floor
// field in the line/sectortype, or `C' for the ceiling field.  The
// last value is the line/sector DDF number.
//
void SR_PlaneMovePutType(void *storage, int index, void *extra)
{
	const movplanedef_c *src = ((const movplanedef_c **)storage)[index];

	epi::array_iterator_c it;
	linetype_c *ln;
	sectortype_c *sec;

	if (! src)
	{
		SV_PutString(NULL);
		return;
	}

	// check for donut
	{
		int i;
		for (i=0; i < 2; i++)
		{
			if (src == &donut[i].f)
			{
				std::string s = epi::STR_Format("D:F:%d", i);
				SV_PutString(s.c_str());
				return;
			}
			else if (src == &donut[i].c)
			{
				std::string s = epi::STR_Format("D:C:%d", i);
				SV_PutString(s.c_str());
				return;
			}
		}
	}
	
	// check all the line types
	for (it=linetypes.GetBaseIterator(); it.IsValid(); it++)
	{
		ln = ITERATOR_TO_TYPE(it, linetype_c*);
		
		if (src == &ln->f)
		{
			std::string s = epi::STR_Format("L:F:%s", ln->name.c_str());
			SV_PutString(s.c_str());
			return;
		}
		
		if (src == &ln->c)
		{
			std::string s = epi::STR_Format("L:C:%s", ln->name.c_str());
			SV_PutString(s.c_str());
			return;
		}
	}

	// check all the sector types
	for (it=sectortypes.GetBaseIterator(); it.IsValid(); it++)
	{
		sec = ITERATOR_TO_TYPE(it, sectortype_c*);

		if (src == &sec->f)
		{
			std::string s = epi::STR_Format("S:F:%s", sec->name.c_str());
			SV_PutString(s.c_str());
			return;
		}
		
		if (src == &sec->c)
		{
			std::string s = epi::STR_Format("S:C:%s", sec->name.c_str());
			SV_PutString(s.c_str());
			return;
		}
	}

	// not found !

	I_Warning("SAVEGAME: could not find moving_plane %p !\n", src);
	SV_PutString("L:C:1");
}


bool SR_SliderGetInfo(void *storage, int index, void *extra)
{
	const sliding_door_c ** dest = (const sliding_door_c **)storage + index;
	const char *str;

	str = SV_GetString();

	if (! str)
	{
		(*dest) = NULL;
		return true;
	}

	if (str[0] != ':')
		I_Error("SR_SliderGetInfo: invalid special `%s'\n", str);

	const linetype_c *ld_type = P_LookupLineType(strtol(str+1, NULL, 0));

	(*dest) = &ld_type->s;

	SV_FreeString(str);
	return true;
}

//
// Format of the string will usually be a colon followed by the
// linedef number (e.g. ":123").
//
void SR_SliderPutInfo(void *storage, int index, void *extra)
{
	const sliding_door_c *src = ((const sliding_door_c **)storage)[index];

	if (! src)
	{
		SV_PutString(NULL);
		return;
	}

	// check all the line types
	epi::array_iterator_c it;

	for (it = linetypes.GetBaseIterator(); it.IsValid(); it++)
	{
		linetype_c *ld_type = ITERATOR_TO_TYPE(it, linetype_c*);
		
		if (src == &ld_type->s)
		{
			std::string s = epi::STR_Format(":%s", ld_type->name.c_str());
			SV_PutString(s.c_str());
			return;
		}
	}

	// not found !

	I_Warning("SAVEGAME: could not find sliding door %p !\n", src);
	SV_PutString(":1");
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
