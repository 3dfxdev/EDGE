//----------------------------------------------------------------------------
//  EDGE New SaveGame Handling (Main defs)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2000  The EDGE Team.
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
  int num_size;

  // name of structure for SFKIND_Struct
  const char *struct_name;

  // used in loaded structures to refer to the loaded SDEF
  struct savestruct_t *loaded_ptr;
}
savefieldtype_t;

#define SVT_INVALID        { SFKIND_Invalid, 0, NULL, NULL }

#define SVT_INT            { SFKIND_Numeric, 4, NULL, NULL }
#define SVT_SHORT          { SFKIND_Numeric, 2, NULL, NULL }
#define SVT_BYTE           { SFKIND_Numeric, 1, NULL, NULL }
#define SVT_FLOAT          { SFKIND_Numeric, 4, NULL, NULL }
#define SVT_ANGLE          SVT_INT
#define SVT_FIXED          SVT_INT
#define SVT_BOOLEAN        SVT_INT
#define SVT_ENUM           SVT_INT
#define SVT_INDEX          { SFKIND_Index,   4, NULL, NULL }
#define SVT_STRING         { SFKIND_String,  0, NULL, NULL }
#define SVT_STRUCT(name)   { SFKIND_Struct,  0, name, NULL }


// This describes a single field
typedef struct savefieldtype_s
{
  // offset of field into structure
  int offset;

  // name of field in savegame system
  const char *field_name;

  // field type information
  savefieldtype_t type;

  // get & put routines
  boolean_t (* field_get)(void *storage, const savefieldtype_t *ftype);
  void (* field_put)(void *storage, const savefieldtype_t *ftype);
}
savefield_t;

///---  #define SV_FIELD_OFFSET(struct,field)  \
///---      ((char *)& sv_dummy_##struct . field - (char *)& sv_dummy_##struct )
 
// Note well: requires SV_F_BASE to be properly defined
#define SVFIELD(field,fname,ftype,getter,putter)  \
    { ((char *)& (SV_F_BASE . field)) - ((char *)& SV_F_BASE),  \
      fname, ftype, getter, putter }

#define SVFIELD_END  { 0, NULL, SVT_INVALID, NULL, NULL }


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

  // this only used for loaded info.  Either the known structure with
  // the same name, or NULL.
  struct savestruct_s *known;
}
savestruct_t;


// This describes a single array
typedef struct savearray_s
{
  // link in list of array definitions
  struct savearray_s *next;

  // array name (for ADEF and STOR chunks)
  const char *array_name;

  // array type
  savestruct_t *sdef;

  // array routines.  Not used for loaded info.
  int (* count_elems)(void);
  void * (* get_elem)(int index);
  void (* create_elems)(int num_elems);
  void (* finalise_elems)(void);

  // this only used for loaded info.  Either the known array with
  // the same name, or NULL.
  struct savearray_s *known;
}
savearray_t;


//
//  COMMON GET ROUTINES
//
//  Note the `SR_' prefix.
//
boolean_t SR_GetByte(void *storage, const savefieldtype_t *ftype);
boolean_t SR_GetShort(void *storage, const savefieldtype_t *ftype);
boolean_t SR_GetInt(void *storage, const savefieldtype_t *ftype);

boolean_t SR_GetFixed(void *storage, const savefieldtype_t *ftype);
boolean_t SR_GetAngle(void *storage, const savefieldtype_t *ftype);
boolean_t SR_GetFloat(void *storage, const savefieldtype_t *ftype);

boolean_t SR_GetStruct(void *storage, const savefieldtype_t *ftype);

#define SR_GetBoolean  SR_GetInt
#define SR_GetEnum     SR_GetInt

//
//  COMMON PUT ROUTINES
//
//  Note the `SR_' prefix.
//
void SR_PutByte(void *storage, const savefieldtype_t *ftype);
void SR_PutShort(void *storage, const savefieldtype_t *ftype);
void SR_PutInt(void *storage, const savefieldtype_t *ftype);

void SR_PutFixed(void *storage, const savefieldtype_t *ftype);
void SR_PutAngle(void *storage, const savefieldtype_t *ftype);
void SR_PutFloat(void *storage, const savefieldtype_t *ftype);

void SR_PutStruct(void *storage, const savefieldtype_t *ftype);

#define SR_PutBoolean  SR_PutInt
#define SR_PutEnum     SR_PutInt

//
//  ADMININISTRATION
//

boolean_t SV_MainInit(void);

const savestruct_t *SV_MainLookupStruct(const char *name);
const savearray_t  *SV_MainLookupArray(const char *name);

void SV_MainBeginLoad(void);
void SV_MainFinishLoad(void);

boolean_t SV_MainLoadSTRU(void);
boolean_t SV_MainLoadARRY(void);
boolean_t SV_MainLoadSTOR(void);

void SV_MainSaveSTRU(void);
void SV_MainSaveARRY(void);
void SV_MainSaveDATA(void);


//
//  EXTERNAL DEFS
//

extern savestruct_t sv_struct_mobj;
extern savestruct_t sv_struct_player;
//....

extern savearray_t sv_array_mobj;
extern savearray_t sv_array_player;
//....

boolean_t SR_MobjGetMobj(void *storage, const savefieldtype_t *ftype);
void SR_MobjPutMobj(void *storage, const savefieldtype_t *ftype);

int SV_MobjFindElem(mobj_t *elem);
int SV_PlayerFindElem(player_t *elem);

void * SV_MobjGetElem(int index);
void * SV_PlayerGetElem(int index);


#endif  // __SV_MAIN_
