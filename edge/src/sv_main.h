//----------------------------------------------------------------------------
//  EDGE New SaveGame Handling (Main defs)
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
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------
//
// TERMINOLOGY:
//
//   - "known" here means an array/structure that is currently built
//     into EDGE.
//
//   - "loaded" here means an array/structure definition that has been
//     loaded from the savegame file.
//

#ifndef __SV_MAIN_
#define __SV_MAIN_

#include "i_defs.h"
#include "p_local.h"

//
// STRUCTURE TABLE STUFF
//

typedef enum
{
  SFKIND_Invalid = 0,  // invalid values can be helpful
  SFKIND_Numeric,
  SFKIND_Index,
  SFKIND_String,
  SFKIND_Struct
}
savefieldkind_e;

typedef struct
{
  // basic kind of field (for SDEF chunk)
  savefieldkind_e kind;
  
  // number of bytes for SFKIND_Numeric (1, 2, 4 or 8)
  int size;

  // name of structure for SFKIND_Struct, or name of array
  // for SFKIND_Index.
  const char *name;
}
savefieldtype_t;

#define SVT_INVALID        { SFKIND_Invalid, 0, NULL }

#define SVT_INT            { SFKIND_Numeric, 4, NULL }
#define SVT_SHORT          { SFKIND_Numeric, 2, NULL }
#define SVT_BYTE           { SFKIND_Numeric, 1, NULL }
#define SVT_FLOAT          { SFKIND_Numeric, 4, NULL }
#define SVT_INDEX(name)    { SFKIND_Index,   4, name }
#define SVT_STRING         { SFKIND_String,  0, NULL }
#define SVT_STRUCT(name)   { SFKIND_Struct,  0, name }

#define SVT_ANGLE          SVT_INT
#define SVT_FIXED          SVT_INT
#define SVT_BOOLEAN        SVT_INT
#define SVT_ENUM           SVT_INT


// This describes a single field
typedef struct savefieldtype_s
{
  // offset of field into structure
  int offset;

  // name of field in savegame system
  const char *field_name;

  // number of sequential elements
  int count;
 
  // field type information
  savefieldtype_t type;

  // get & put routines.  The extra parameter depends on the type, for
  // SFKIND_Struct it is the name of the structure, for SFKIND_Index
  // it is the name of the array.  When `field_put' is NULL, then this
  // field is not saved into the output SDEF chunk.
  boolean_t (* field_get)(void *storage, int index, void *extra);
  void (* field_put)(void *storage, int index, void *extra);

  // for loaded info, this points to the known version of the field,
  // otherwise NULL if the loaded field is unknown.
  struct savefieldtype_s *known_field;
}
savefield_t;

// NOTE: requires SV_F_BASE to be defined as the dummy struct

#define SVFIELD(field,fname,fnum,ftype,getter,putter)  \
    { ((char *)& (SV_F_BASE . field)) - ((char *)& SV_F_BASE),  \
      fname, fnum, ftype, getter, putter, NULL }

#define SVFIELD_END  { 0, NULL, 0, SVT_INVALID, NULL, NULL, NULL }


// This describes a single structure
typedef struct savestruct_s
{
  // link in list of structure definitions
  struct savestruct_s *next;

  // structure name (for SDEF/ADEF chunks)
  const char *struct_name;

  // four letter marker
  const char *marker;

  // array of field definitions
  savefield_t *fields;

  // this must be true to put the definition into the savegame file.
  // Allows compatibility structures that are read-only.
  boolean_t define_me;

  // only used when loading.  For loaded info, this refers to the
  // known struct of the same name (or NULL if none).  For known info,
  // this points to the loaded info (or NULL if absent).
  struct savestruct_s *counterpart;
}
savestruct_t;


// This describes a single array
typedef struct savearray_s
{
  // link in list of array definitions
  struct savearray_s *next;

  // array name (for ADEF and STOR chunks)
  const char *array_name;

  // array type.  For loaded info, this points to the loaded
  // structure.  Never NULL.
  savestruct_t *sdef;

  // this must be true to put the definition into the savegame file.
  // Allows compatibility arrays that are read-only.
  boolean_t define_me;

  // array routines.  Not used for loaded info.
  int (* count_elems)(void);
  void * (* get_elem)(int index);
  void (* create_elems)(int num_elems);
  void (* finalise_elems)(void);

  // only used when loading.  For loaded info, this refers to the
  // known array (or NULL if none).  For known info, this points to
  // the loaded info (or NULL if absent).
  struct savearray_s *counterpart;

  // number of elements to be loaded.
  int loaded_size;
}
savearray_t;


//
//  COMMON GET ROUTINES
//
//  Note the `SR_' prefix.
//
boolean_t SR_GetByte(void *storage, int index, void *extra);
boolean_t SR_GetShort(void *storage, int index, void *extra);
boolean_t SR_GetInt(void *storage, int index, void *extra);

boolean_t SR_GetFixed(void *storage, int index, void *extra);
boolean_t SR_GetAngle(void *storage, int index, void *extra);
boolean_t SR_GetFloat(void *storage, int index, void *extra);

#define SR_GetBoolean  SR_GetInt
#define SR_GetEnum     SR_GetInt

//
//  COMMON PUT ROUTINES
//
//  Note the `SR_' prefix.
//
void SR_PutByte(void *storage, int index, void *extra);
void SR_PutShort(void *storage, int index, void *extra);
void SR_PutInt(void *storage, int index, void *extra);

void SR_PutFixed(void *storage, int index, void *extra);
void SR_PutAngle(void *storage, int index, void *extra);
void SR_PutFloat(void *storage, int index, void *extra);

#define SR_PutBoolean  SR_PutInt
#define SR_PutEnum     SR_PutInt


//
//  GLOBAL STUFF
//

typedef struct
{
  // number of items
  int count;

  // CRC computed over all the items
  unsigned long crc;
}
crc_check_t;

// this structure contains everything for the top-level [GLOB] chunk.
// Strings are copies and need to be freed.
typedef struct
{
  // [IVAR] stuff:
  
  const char *game;
  const char *level;
  gameflags_t flags;
  int gravity;
  
  int level_time;
  int p_random;
  int total_kills;
  int total_items;
  int total_secrets;
 
  int console_player;
  int skill;
  int netgame;

  const char *description;
  const char *desc_date;

  crc_check_t mapsector;
  crc_check_t mapline;
  crc_check_t mapthing;

  crc_check_t rscript;
  crc_check_t ddfatk;
  crc_check_t ddfgame;
  crc_check_t ddflevl;
  crc_check_t ddfline;
  crc_check_t ddfsect;
  crc_check_t ddfmobj;
  crc_check_t ddfweap;

  // [VIEW] info.  Unused if view_pixels is NULL.
  unsigned short *view_pixels;
  int view_width;
  int view_height;

  // [WADS] info
  int wad_num;
  const char ** wad_names;
}
saveglobals_t;

saveglobals_t *SV_NewGLOB(void);
saveglobals_t *SV_LoadGLOB(void);
void SV_SaveGLOB(saveglobals_t *globs);
void SV_FreeGLOB(saveglobals_t *globs);


//
//  ADMININISTRATION
//

boolean_t SV_MainInit(void);

const savestruct_t *SV_MainLookupStruct(const char *name);
const savearray_t  *SV_MainLookupArray(const char *name);

void SV_BeginLoad(void);
void SV_FinishLoad(void);

boolean_t SV_LoadStruct(void *base, savestruct_t *info);
boolean_t SV_LoadEverything(void);

void SV_BeginSave(void);
void SV_FinishSave(void);

void SV_SaveStruct(void *base, savestruct_t *info);
void SV_SaveEverything(void);


//
//  DEBUGGING
//

void SV_DumpSaveGame(int slot);


//
//  EXTERNAL DEFS
//

extern void *sv_current_elem;

extern savestruct_t *sv_known_structs;
extern savearray_t  *sv_known_arrays;

extern savestruct_t sv_struct_mobj;
extern savestruct_t sv_struct_spawnpoint;
extern savestruct_t sv_struct_player;
extern savestruct_t sv_struct_playerweapon;
extern savestruct_t sv_struct_playerammo;
extern savestruct_t sv_struct_psprite;
extern savestruct_t sv_struct_sidepart;
extern savestruct_t sv_struct_side;
extern savestruct_t sv_struct_line;

extern savearray_t sv_array_mobj;
extern savearray_t sv_array_player;
extern savearray_t sv_array_side;
extern savearray_t sv_array_line;

boolean_t SR_MobjGetMobj(void *storage, int index, void *extra);
void SR_MobjPutMobj(void *storage, int index, void *extra);

int SV_MobjFindElem(mobj_t *elem);
int SV_PlayerFindElem(player_t *elem);

void * SV_MobjGetElem(int index);
void * SV_PlayerGetElem(int index);


#endif  // __SV_MAIN_
