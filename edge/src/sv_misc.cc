//----------------------------------------------------------------------------
//  EDGE New SaveGame Handling (Miscellaneous)
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
//    elev_move_t     [EMOV]
//
// TODO HERE:
//   +  Fix donuts.
//   +  Implement slider_move array.
//   -  Implement elev_move array (when elevators are done).
//   -  Button off_sound field.
//

#include "i_defs.h"

#include "dm_state.h"
#include "e_player.h"
#include "p_local.h"
#include "p_spec.h"
#include "sv_chunk.h"
#include "sv_main.h"
#include "r_state.h"
#include "rad_main.h"
#include "rad_trig.h"
#include "z_zone.h"

#include "./epi/strings.h"

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
	//   - soundorg: can be recomputed.
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
	SF(p.colourmap_name, "colourmap_name", 1, SVT_STRING,
		SR_TipGetString, SR_TipPutString),
	SF(p.translucency, "translucency", 1, SVT_PERCENT, 
		SR_GetPercent, SR_PutPercent),

	SF(delay, "delay", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(tip_text, "tip_text", 1, SVT_STRING, SR_TipGetString, SR_TipPutString),
	SF(tip_graphic, "tip_graphic", 1, SVT_STRING,
		SR_LevelGetImage, SR_LevelPutImage),
	SF(playsound, "playsound", 1, SVT_BOOLEAN, SR_GetBoolean, SR_PutBoolean),
	SF(fade_time, "fade_time", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(fade_target, "fade_target", 1, SVT_FLOAT, SR_GetFloat, SR_PutFloat),

	// NOT HERE:
	//    p.slot_num, p.time: not used withing drawtip_t
	//    dirty: this is set in the finalizer
	//    colmap, hu_*: these are regenerated on next display

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
	SF(startheight, "startheight", 1, SVT_FLOAT, SR_GetFloat, SR_PutFloat),
	SF(destheight, "destheight", 1, SVT_FLOAT, SR_GetFloat, SR_PutFloat),
	SF(speed, "speed", 1, SVT_FLOAT, SR_GetFloat, SR_PutFloat),
	SF(crush, "crush", 1, SVT_BOOLEAN, SR_GetBoolean, SR_PutBoolean),

	SF(direction, "direction", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(olddirection, "olddirection", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(tag, "tag", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(waited, "waited", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(sfxstarted, "sfxstarted", 1, SVT_BOOLEAN, SR_GetBoolean, SR_PutBoolean),

	SF(newspecial, "newspecial", 1, SVT_INT, SR_GetInt, SR_PutInt),
	SF(new_image, "new_image", 1, SVT_STRING, 
	SR_LevelGetImage, SR_LevelPutImage),

	// NOT HERE:
	//   - whatiam: will always be MDT_PLANE
	//   - next, prev: regenerated automatically.

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

	SV_PlaneMoveCountElems,     // count routine
	SV_PlaneMoveGetElem,        // index routine
	SV_PlaneMoveCreateElems,    // creation routine
	SV_PlaneMoveFinaliseElems,  // finalisation routine

	NULL,     // pointer to known array
	0         // loaded size
};


//----------------------------------------------------------------------------

//
// SV_ButtonCountElems
//
int SV_ButtonCountElems(void)
{
	return buttonlist.GetSize();
}

//
// SV_ButtonGetElem
//
void *SV_ButtonGetElem(int index)
{
	if (index < 0 || index >= buttonlist.GetSize())
	{
		I_Warning("LOADGAME: Invalid Button: %d\n", index);
		index = 0;
	}	

	return buttonlist[index];
}

//
// SV_ButtonFindElem
//
int SV_ButtonFindElem(button_t *elem)
{
	int idx = buttonlist.Find(elem);
	DEV_ASSERT2(idx >= 0);
	return idx;
}

//
// SV_ButtonCreateElems
//
void SV_ButtonCreateElems(int num_elems)
{
	buttonlist.SetSize(num_elems);
}

//
// SV_ButtonFinaliseElems
//
void SV_ButtonFinaliseElems(void)
{
	// nothing to do
}


//----------------------------------------------------------------------------

//
// SV_LightCountElems
//
int SV_LightCountElems(void)
{
	light_t *cur;
	int count;

	for (cur=lights, count=0; cur; cur=cur->next, count++)
	{ /* nothing here */ }

	return count;
}

//
// SV_LightGetElem
//
void *SV_LightGetElem(int index)
{
	light_t *cur;

	for (cur=lights; cur && index > 0; cur=cur->next)
		index--;

	if (! cur)
		I_Error("LOADGAME: Invalid Light: %d\n", index);

	DEV_ASSERT2(index == 0);
	return cur;
}

//
// SV_LightFindElem
//
int SV_LightFindElem(light_t *elem)
{
	light_t *cur;
	int index;

	for (cur=lights, index=0; cur && cur != elem; cur=cur->next)
		index++;

	if (! cur)
		I_Error("LOADGAME: No such LightPtr: %p\n", elem);

	return index;
}

//
// SV_LightCreateElems
//
void SV_LightCreateElems(int num_elems)
{
	P_DestroyAllLights();

	for (; num_elems > 0; num_elems--)
	{
		light_t *cur = Z_ClearNew(light_t, 1);

		// link it in
		cur->next = lights;
		cur->prev = NULL;

		if (lights)
			lights->prev = cur;

		lights = cur;

		// initialise defaults
		cur->type = &sectortypes[0]->l;
		cur->sector = sectors + 0;
	}
}

//
// SV_LightFinaliseElems
//
void SV_LightFinaliseElems(void)
{
	// nothing to do
}


//----------------------------------------------------------------------------

//
// SV_TriggerCountElems
//
int SV_TriggerCountElems(void)
{
	rad_trigger_t *cur;
	int count;

	for (cur=r_triggers, count=0; cur; cur=cur->next, count++)
	{ /* nothing here */ }

	return count;
}

//
// SV_TriggerGetElem
//
void *SV_TriggerGetElem(int index)
{
	rad_trigger_t *cur;

	for (cur=r_triggers; cur && index > 0; cur=cur->next)
		index--;

	if (! cur)
		I_Error("LOADGAME: Invalid Trigger: %d\n", index);

	DEV_ASSERT2(index == 0);
	return cur;
}

//
// SV_TriggerFindElem
//
int SV_TriggerFindElem(rad_trigger_t *elem)
{
	rad_trigger_t *cur;
	int index;

	for (cur=r_triggers, index=0; cur && cur != elem; cur=cur->next)
		index++;

	if (! cur)
		I_Error("LOADGAME: No such TriggerPtr: %p\n", elem);

	return index;
}

//
// SV_TriggerCreateElems
//
void SV_TriggerCreateElems(int num_elems)
{
	RAD_ClearTriggers();

	for (; num_elems > 0; num_elems--)
	{
		rad_trigger_t *cur = Z_ClearNew(rad_trigger_t, 1);

		// link it in
		cur->next = r_triggers;
		cur->prev = NULL;

		if (r_triggers)
			r_triggers->prev = cur;

		r_triggers = cur;

		// initialise defaults
		cur->info = r_scripts;
		cur->state = r_scripts ? r_scripts->first_state : NULL;
		cur->disabled = true;
	}
}

//
// SV_TriggerFinaliseElems
//
void SV_TriggerFinaliseElems(void)
{
	rad_trigger_t *cur;

	for (cur=r_triggers; cur; cur=cur->next)
	{
		RAD_GroupTriggerTags(cur);
	}
}


//----------------------------------------------------------------------------

//
// SV_TipCountElems
//
int SV_TipCountElems(void)
{
	return MAXTIPSLOT;
}

//
// SV_TipGetElem
//
void *SV_TipGetElem(int index)
{
	if (index < 0 || index >= MAXTIPSLOT)
	{
		I_Warning("LOADGAME: Invalid Tip: %d\n", index);
		index = MAXTIPSLOT-1;
	}

	return tip_slots + index;
}

//
// SV_TipFindElem
//
int SV_TipFindElem(drawtip_t *elem)
{
	DEV_ASSERT2(tip_slots <= elem && elem < (tip_slots + MAXTIPSLOT));

	return elem - tip_slots;
}

//
// SV_TipCreateElems
//
void SV_TipCreateElems(int num_elems)
{
	RAD_ResetTips();
}

//
// SV_TipFinaliseElems
//
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

//
// SV_PlaneMoveCountElems
//
int SV_PlaneMoveCountElems(void)
{
	gen_move_t *cur;
	int count;

	for (cur=active_movparts, count=0; cur; cur=cur->next)
	{
		if (cur->whatiam == MDT_PLANE)
			count++;
	}

	return count;
}

//
// SV_PlaneMoveGetElem
//
// The index value starts at 0.
//
void *SV_PlaneMoveGetElem(int index)
{
	gen_move_t *cur;

	for (cur=active_movparts; cur; cur=cur->next)
	{
		if (cur->whatiam != MDT_PLANE)
			continue;

		if (index == 0)
			break;

		index--;
	}

	if (! cur)
		I_Error("LOADGAME: Invalid PlaneMove: %d\n", index);

	DEV_ASSERT2(index == 0);
	return cur;
}

//
// SV_PlaneMoveFindElem
//
// Returns the index value (starts at 0).
//
int SV_PlaneMoveFindElem(plane_move_t *elem)
{
	gen_move_t *cur;
	int index;

	for (cur=active_movparts, index=0; cur; cur=cur->next)
	{
		if (cur->whatiam != MDT_PLANE)
			continue;

		if ((plane_move_t *)cur == elem)
			break;

		index++;
	}

	if (! cur)
		I_Error("LOADGAME: No such PlaneMovePtr: %p\n", elem);

	return index;
}

//
// SV_PlaneMoveCreateElems
//
void SV_PlaneMoveCreateElems(int num_elems)
{
	// NOTE: this removes all the other movers too.  Hence plane movers
	//       should be loaded before the other ones.
	//
	P_RemoveAllActiveParts();

	for (; num_elems > 0; num_elems--)
	{
		plane_move_t *cur = Z_ClearNew(plane_move_t, 1);

		// initialise defaults
		cur->whatiam = MDT_PLANE;

		// link it in
		P_AddActivePart((gen_move_t *)cur);
	}
}

//
// SV_PlaneMoveFinaliseElems
//
void SV_PlaneMoveFinaliseElems(void)
{
	// nothing to do
}


//----------------------------------------------------------------------------

//
// SR_LightGetType
//
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
		const sectortype_c *special = playsim::LookupSectorType(number);
		(*dest) = &special->l;
	}
	else if (str[0] == 'L')
	{
		const linetype_c *special = playsim::LookupLineType(number);
		(*dest) = &special->l;
	}
	else
		I_Error("SR_LightGetType: invalid lighttype `%s'\n", str);

	Z_Free((char *)str);
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
			epi::string_c s;
			s.Format("L:%d", ln->ddf.number);
			SV_PutString(s.GetString());
			return;
		}
	}

	// look for it in the sector types
	for (it=sectortypes.GetBaseIterator(); it.IsValid(); it++)
	{
		sec = ITERATOR_TO_TYPE(it, sectortype_c*);
		if (src == &sec->l)
		{
			epi::string_c s;
			s.Format("S:%d", sec->ddf.number);
			SV_PutString(s.GetString());
			return;
		}
	}

	// not found !

	I_Warning("LOADGAME: could not find lightdef_c %p !\n", src);
	SV_PutString("S:1");
}


//
// SR_TriggerGetState
//
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

//
// SR_TriggerPutState
//
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


//
// SR_TriggerGetScript
//
bool SR_TriggerGetScript(void *storage, int index, void *extra)
{
	const rad_script_t ** dest = (const rad_script_t **)storage + index;
	const rad_script_t *temp;

	const char *swizzle;
	char buffer[256];
	char *base_p, *use_p;
	char *map_name;

	int idx_val;
	unsigned long crc;

	swizzle = SV_GetString();

	if (! swizzle)
	{
		(*dest) = NULL;
		return true;
	}

	Z_StrNCpy(buffer, swizzle, 256-1);
	Z_Free((char *)swizzle);

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
	DEV_ASSERT2(idx_val >= 1);

	// get CRC value

	crc = strtoul(base_p, NULL, 16);

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

//
// SR_TipGetString
//
bool SR_TipGetString(void *storage, int index, void *extra)
{
	const char ** dest = (const char **)storage + index;

	if (*dest)
		Z_Free((char *)(*dest));

	(*dest) = SV_GetString();
	return true;
}

//
// SR_TipPutString
//
void SR_TipPutString(void *storage, int index, void *extra)
{
	const char *src = ((const char **)storage)[index];

	SV_PutString(src);
}


//
// SR_PlaneMoveGetType
//
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
		const sectortype_c *special = playsim::LookupSectorType(number);
		(*dest) = is_ceil ? &special->c : &special->f;
	}
	else if (str[0] == 'L')
	{
		const linetype_c *special = playsim::LookupLineType(number);
		(*dest) = is_ceil ? &special->c : &special->f;
	}
	else if (str[0] == 'D')
	{
		// FIXME: this ain't gonna work, freddy
		(*dest) = is_ceil ? &donut[number].c : &donut[number].f;
	}
	else
		I_Error("SR_PlaneMoveGetType: invalid srctype `%s'\n", str);

	Z_Free((char *)str);
	return true;
}

//
// SR_PlaneMovePutType
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
				epi::string_c s;
				s.Format("D:F:%d", i);
				SV_PutString(s.GetString());
				return;
			}
			else if (src == &donut[i].c)
			{
				epi::string_c s;
				s.Format("D:C:%d", i);
				SV_PutString(s.GetString());
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
			epi::string_c s;
			s.Format("L:F:%d", ln->ddf.number);
			SV_PutString(s.GetString());
			return;
		}
		
		if (src == &ln->c)
		{
			epi::string_c s;
			s.Format("L:C:%d", ln->ddf.number);
			SV_PutString(s.GetString());
			return;
		}
	}

	// check all the sector types
	for (it=sectortypes.GetBaseIterator(); it.IsValid(); it++)
	{
		sec = ITERATOR_TO_TYPE(it, sectortype_c*);

		if (src == &sec->f)
		{
			epi::string_c s;
			s.Format("S:F:%d", sec->ddf.number);
			SV_PutString(s.GetString());
			return;
		}
		
		if (src == &sec->c)
		{
			epi::string_c s;
			s.Format("S:C:%d", sec->ddf.number);
			SV_PutString(s.GetString());
			return;
		}
	}

	// not found !

	I_Warning("LOADGAME: could not find moving_plane %p !\n", src);
	SV_PutString("L:C:1");
}

