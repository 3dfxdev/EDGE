//----------------------------------------------------------------------------
//  EDGE New SaveGame Handling (Main)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2001  The EDGE Team.
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

#include "i_defs.h"
#include "sv_chunk.h"

#include "dm_state.h"
#include "e_main.h"
#include "g_game.h"
#include "m_inline.h"
#include "m_math.h"
#include "m_random.h"
#include "p_local.h"
#include "p_spec.h"
#include "r_state.h"
#include "sv_chunk.h"
#include "sv_main.h"
#include "w_wad.h"
#include "wi_stuff.h"
#include "z_zone.h"


savestruct_t *sv_known_structs;
savearray_t  *sv_known_arrays;

// the current element of an array being read/written
void *sv_current_elem;


// sv_mobj.c
extern savestruct_t sv_struct_mobj;
extern savestruct_t sv_struct_spawnpoint;
extern savestruct_t sv_struct_iteminque;
extern savearray_t sv_array_mobj;
extern savearray_t sv_array_iteminque;

// sv_play.c
extern savestruct_t sv_struct_player;
extern savestruct_t sv_struct_playerweapon;
extern savestruct_t sv_struct_playerammo;
extern savestruct_t sv_struct_psprite;
extern savearray_t sv_array_player;

// sv_level.c
extern savestruct_t sv_struct_surface;
extern savestruct_t sv_struct_side;
extern savestruct_t sv_struct_line;
extern savestruct_t sv_struct_regprops;
extern savestruct_t sv_struct_exfloor;
extern savestruct_t sv_struct_sector;
extern savearray_t sv_array_side;
extern savearray_t sv_array_line;
extern savearray_t sv_array_exfloor;
extern savearray_t sv_array_sector;

// sv_misc.c
extern savestruct_t sv_struct_button;
extern savestruct_t sv_struct_light;
extern savestruct_t sv_struct_trigger;
extern savestruct_t sv_struct_drawtip;
extern savestruct_t sv_struct_plane_move;
extern savearray_t sv_array_button;
extern savearray_t sv_array_light;
extern savearray_t sv_array_trigger;
extern savearray_t sv_array_drawtip;
extern savearray_t sv_array_plane_move;


//----------------------------------------------------------------------------
//
//  GET ROUTINES
//

//
// SR_GetByte
//
boolean_t SR_GetByte(void *storage, int index, void *extra)
{
  ((unsigned char *)storage)[index] = SV_GetByte();
  return true;
}

//
// SR_GetShort
//
boolean_t SR_GetShort(void *storage, int index, void *extra)
{
  ((unsigned short *)storage)[index] = SV_GetShort();
  return true;
}

//
// SR_GetInt
//
boolean_t SR_GetInt(void *storage, int index, void *extra)
{
  ((unsigned int *)storage)[index] = SV_GetInt();
  return true;
}

//
// SR_GetFixed
//
boolean_t SR_GetFixed(void *storage, int index, void *extra)
{
  ((fixed_t *)storage)[0] = SV_GetFixed();
  return true;
}

//
// SR_GetAngle
//
boolean_t SR_GetAngle(void *storage, int index, void *extra)
{
  ((angle_t *)storage)[index] = SV_GetAngle();
  return true;
}

//
// SR_GetFloat
//
boolean_t SR_GetFloat(void *storage, int index, void *extra)
{
  ((flo_t *)storage)[index] = SV_GetFloat();
  return true;
}

//
// SR_GetVec2
//
boolean_t SR_GetVec2(void *storage, int index, void *extra)
{
  ((vec2_t *)storage)[index].x = SV_GetFloat();
  ((vec2_t *)storage)[index].y = SV_GetFloat();
  return true;
}

//
// SR_GetVec3
//
boolean_t SR_GetVec3(void *storage, int index, void *extra)
{
  ((vec3_t *)storage)[index].x = SV_GetFloat();
  ((vec3_t *)storage)[index].y = SV_GetFloat();
  ((vec3_t *)storage)[index].z = SV_GetFloat();
  return true;
}

//
// SR_GetIntAsFloat
//
boolean_t SR_GetIntAsFloat(void *storage, int index, void *extra)
{
  ((flo_t *)storage)[index] = (flo_t)SV_GetInt();
  return true;
}


//----------------------------------------------------------------------------
//
//  COMMON PUT ROUTINES
//

//
// SR_PutByte
//
void SR_PutByte(void *storage, int index, void *extra)
{
  SV_PutByte(((unsigned char *)storage)[index]);
}

//
// SR_PutShort
//
void SR_PutShort(void *storage, int index, void *extra)
{
  SV_PutShort(((unsigned short *)storage)[index]);
}

//
// SR_PutInt
//
void SR_PutInt(void *storage, int index, void *extra)
{
  SV_PutInt(((unsigned int *)storage)[index]);
}

//
// SR_PutFixed
//
void SR_PutFixed(void *storage, int index, void *extra)
{
  SV_PutFixed(((fixed_t *)storage)[index]);
}

//
// SR_PutAngle
//
void SR_PutAngle(void *storage, int index, void *extra)
{
  SV_PutAngle(((angle_t *)storage)[index]);
}

//
// SR_PutFloat
//
void SR_PutFloat(void *storage, int index, void *extra)
{
  SV_PutFloat(((flo_t *)storage)[index]);
}

//
// SR_PutVec2
//
void SR_PutVec2(void *storage, int index, void *extra)
{
  SV_PutFloat(((vec2_t *)storage)[index].x);
  SV_PutFloat(((vec2_t *)storage)[index].y);
}

//
// SR_PutVec3
//
void SR_PutVec3(void *storage, int index, void *extra)
{
  SV_PutFloat(((vec3_t *)storage)[index].x);
  SV_PutFloat(((vec3_t *)storage)[index].y);
  SV_PutFloat(((vec3_t *)storage)[index].z);
}


//----------------------------------------------------------------------------
//
//  ADMININISTRATION
//

static void AddKnownStruct(savestruct_t *S)
{
  S->next = sv_known_structs;
  sv_known_structs = S;
}

static void AddKnownArray(savearray_t *A)
{
  A->next = sv_known_arrays;
  sv_known_arrays = A;
}


//
// SV_MainInit
//
// One-time initialisation.  Sets up lists of known structures and
// arrays.
//
boolean_t SV_MainInit(void)
{
  // sv_mobj.c
  AddKnownStruct(&sv_struct_mobj);
  AddKnownStruct(&sv_struct_spawnpoint);
  AddKnownStruct(&sv_struct_iteminque);
  
  AddKnownArray(&sv_array_mobj);
  AddKnownArray(&sv_array_iteminque);

  // sv_play.c
  AddKnownStruct(&sv_struct_player);
  AddKnownStruct(&sv_struct_playerweapon);
  AddKnownStruct(&sv_struct_playerammo);
  AddKnownStruct(&sv_struct_psprite);
  
  AddKnownArray(&sv_array_player);

  // sv_level.c
  AddKnownStruct(&sv_struct_surface);
  AddKnownStruct(&sv_struct_side);
  AddKnownStruct(&sv_struct_line);
  AddKnownStruct(&sv_struct_regprops);
  AddKnownStruct(&sv_struct_exfloor);
  AddKnownStruct(&sv_struct_sector);

  AddKnownArray(&sv_array_side);
  AddKnownArray(&sv_array_line);
  AddKnownArray(&sv_array_exfloor);
  AddKnownArray(&sv_array_sector);

  // sv_misc.c
  AddKnownStruct(&sv_struct_button);
  AddKnownStruct(&sv_struct_light);
  AddKnownStruct(&sv_struct_trigger);
  AddKnownStruct(&sv_struct_drawtip);
  AddKnownStruct(&sv_struct_plane_move);

  AddKnownArray(&sv_array_button);
  AddKnownArray(&sv_array_light);
  AddKnownArray(&sv_array_trigger);
  AddKnownArray(&sv_array_drawtip);
  AddKnownArray(&sv_array_plane_move);

  return true;
}

//
// SV_MainLookupStruct
//
savestruct_t *SV_MainLookupStruct(const char *name)
{
  savestruct_t *cur;

  for (cur=sv_known_structs; cur; cur=cur->next)
    if (strcmp(cur->struct_name, name) == 0)
      return cur;
    
  // not found
  return NULL;
}

//
// SV_MainLookupArray
//
savearray_t *SV_MainLookupArray(const char *name)
{
  savearray_t *cur;

  for (cur=sv_known_arrays; cur; cur=cur->next)
    if (strcmp(cur->array_name, name) == 0)
      return cur;

  // not found
  return NULL;
}

//----------------------------------------------------------------------------
//
//  TEST CODE
//

#if 0
void SV_MainTestPrimitives(void)
{
  int i;
  int version;

  if (! SV_OpenWriteFile("savegame/prim.tst", 0x7654))
    I_Error("SV_MainTestPrimitives: couldn't create output\n");
  
  SV_PutByte(0x00);
  SV_PutByte(0x55);
  SV_PutByte(0xAA);
  SV_PutByte(0xFF);

  SV_PutShort(0x0000);
  SV_PutShort(0x4567);
  SV_PutShort(0xCDEF);
  SV_PutShort(0xFFFF);

  SV_PutInt(0x00000000);
  SV_PutInt(0x11223344);
  SV_PutInt(0xbbccddee);
  SV_PutInt(0xffffffff);

  SV_PutFixed(0);
  SV_PutFixed(FRACUNIT);
  SV_PutFixed(M_FloatToFixed(123.456));
  SV_PutFixed(-FRACUNIT);
  SV_PutFixed(-M_FloatToFixed(345.789));

  SV_PutAngle(ANG1);
  SV_PutAngle(ANG45);
  SV_PutAngle(ANG135);
  SV_PutAngle(ANG180);
  SV_PutAngle(ANG270);
  SV_PutAngle(ANG315);
  SV_PutAngle(0 - ANG1);

  SV_PutFloat(0.0);
  SV_PutFloat(0.001);   SV_PutFloat(-0.001);
  SV_PutFloat(0.1);     SV_PutFloat(-0.1);
  SV_PutFloat(0.25);    SV_PutFloat(-0.25);
  SV_PutFloat(1.0);     SV_PutFloat(-1.0);
  SV_PutFloat(2.0);     SV_PutFloat(-2.0);
  SV_PutFloat(3.1416);  SV_PutFloat(-3.1416);
  SV_PutFloat(1000.0);  SV_PutFloat(-1000.0);
  SV_PutFloat(1234567890.0);  
  SV_PutFloat(-1234567890.0);
  
  SV_PutString(NULL);
  SV_PutString("");
  SV_PutString("A");
  SV_PutString("123");
  SV_PutString("HELLO world !");

  SV_PutMarker("ABCD");
  SV_PutMarker("xyz3");

  SV_CloseWriteFile();

  // ------------------------------------------------------------ //

  if (! SV_OpenReadFile("savegame/prim.tst"))
    I_Error("SV_MainTestPrimitives: couldn't open input\n");
  
  if (! SV_VerifyHeader(&version))
    I_Error("SV_MainTestPrimitives: couldn't open input\n");
  
  L_WriteDebug("TEST HEADER: version=0x%04x\n", version);

  for (i=0; i < 4; i++)
  {
    unsigned int val = SV_GetByte();
    L_WriteDebug("TEST BYTE: 0x%02x %d\n", val, (char) val);
  }

  for (i=0; i < 4; i++)
  {
    unsigned int val = SV_GetShort();
    L_WriteDebug("TEST SHORT: 0x%02x %d\n", val, (short) val);
  }
  
  for (i=0; i < 4; i++)
  {
    unsigned int val = SV_GetInt();
    L_WriteDebug("TEST INT: 0x%02x %d\n", val, (int) val);
  }
  
  for (i=0; i < 5; i++)
  {
    fixed_t val = SV_GetFixed();
    L_WriteDebug("TEST FIXED: 0x%08x = %1.6f\n", (unsigned int) val,
        M_FixedToFloat(val));
  }
  
  for (i=0; i < 7; i++)
  {
    angle_t val = SV_GetAngle();
    L_WriteDebug("TEST ANGLE: 0x%08x = %1.6f\n", (unsigned int) val,
        ANG_2_FLOAT(val));
  }
  
  for (i=0; i < 17; i++)
  {
    flo_t val = SV_GetFloat();
    L_WriteDebug("TEST FLOAT: %1.6f\n", val);
  }
  
  for (i=0; i < 5; i++)
  {
    const char *val = SV_GetString();
    L_WriteDebug("TEST STRING: [%s]\n", val ? val : "--NULL--");
    Z_Free((char *)val);
  }
  
  for (i=0; i < 2; i++)
  {
    char val[6];
    SV_GetMarker(val);
    L_WriteDebug("TEST MARKER: [%s]\n", val);
  }
  
  SV_CloseReadFile();
}

#endif  // TEST CODE
