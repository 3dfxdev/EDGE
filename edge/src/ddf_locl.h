//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (Local Header)
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

#ifndef __DDF_LOCAL__
#define __DDF_LOCAL__

#include "p_weapon.h"
#include "dm_defs.h"

#define DDFVERSION 120

// defines for parser stuff.
#define BACKSLASH   '\\'
#define COMMANDREAD '='
#define DEFSTART    '['
#define DEFSTOP     ']'
#define TAGSTART    '<'
#define TAGSTOP     '>'
#define DIVIDE      ':'
#define REDIRECTOR  '#'
#define REMARKSTART '{'
#define REMARKSTOP  '}'
#define GROUPSTART  '('
#define GROUPSTOP   ')'
#define SEPARATOR   ','
#define SPACE       ' '
#define STRINGSTART '\"'
#define STRINGSTOP  '\"'
#define SUBSPACE    '_'
#define TERMINATOR  ';'

#define BUFFERSIZE 1024
#define NUMSPLIT 10  // Max Number of sections a state is split info

// enum thats gives the parser's current status
typedef enum
{
  waiting_tag,
  reading_tag,
  waiting_newdef,
  reading_newdef,
  reading_command,
  reading_data,
  reading_remark,
  reading_string
}
readstatus_t;

// enum thats describes the return value from DDF_MainProcessChar
typedef enum
{
  nothing,
  command_read,
  property_read,
  def_start,
  def_stop,
  remark_start,
  remark_stop,
  separator,
  string_start,
  string_stop,
  group_start,
  group_stop,
  tag_start,
  tag_stop,
  terminator,
  ok_char
}
readchar_t;

//
// This structure forms the basis for the command checking, it hands back a code
// pointer and sometimes a pointer to a (int) numeric value. (used for info that gets
// its value directly from the file).
//
typedef struct commandlist_s
{
  // command name
  const char *name;

  // parse function.  `storage' is where the data should go (for
  // routines that don't modify the buffer_xxxx structure directly).
  //
  void (* parse_command)(const char *info, void *storage);

  void *storage;  // FIXME: should be `int offset'

  const struct commandlist_s *sub_comms;
}
commandlist_t;

// -ACB- 2000/08/15 - FIELD_OFFSET is defined in Win32 headers
#define FIELD_OFF(dummy,field)  \
    ((char *) & ((dummy).field) - (char *) & (dummy))

#define FIELD_OFF2P(dummy,field)  \
    ((char *) FIELD_OFF(dummy,field))

#define FIELD_P2OFF(ptr)  ((int) ptr)

// NOTE: requires DDF_CMD_BASE to be defined as the dummy struct

#define DDF_CMD(name,field,parser)  \
    { /* ((char *)& (DDF_CMD_BASE . field)) - ((char *)& DDF_CMD_BASE), */  \
      name, parser, &DDF_CMD_BASE.field, NULL }

#define DDF_CMD_SUB(name,field,parser)  \
    { name, parser, FIELD_OFF2P(DDF_CMD_BASE,field), NULL }
 
#define DDF_SUB_LIST(name,field,subcomms)  \
    { "*" name, NULL, &DDF_CMD_BASE.field, subcomms }

#define DDF_CMD_END  { NULL, NULL, NULL, NULL }


//
// This structure passes the information needed to DDF_MainReadFile, so that
// the reader uses the correct procedures when reading a file.
//
typedef struct readinfo_s
{
  char *message;   // message displayed
  char *filename;  // filename (when memfile == NULL)
  char *lumpname;  // lumpnume (when memfile != NULL)

  char *tag;    // the file has to start with <tag>

  char *memfile;
  size_t memsize;

  // number of entries per displayed `.'
  int entries_per_dot;
  
  // NOTES
  // -----
  // There are two entries: the "dynamic" entry and the "static"
  // entry.  The dynamic entry will be the final entry within the
  // associated list.  We keep a pointer to it.  The static entry has
  // the name "buffer_xxxxx" in the associated ddf_xxx.c file, and
  // holds the parsed info until the entry is complete, at which time
  // the static part is copied into the dynamic part.

  //
  //  FUNCTIONS
  //

  // create a new dynamic entry with the given name.  For number-only
  // ddf files (lines, sectors and playlist), it is a number.  For
  // things.ddf, it is a name with an optional ":####" number
  // appended.  For everything else it is just a normal name.
  //
  // this also instantiates the static entry's information (excluding
  // name and/or number) using the built-in defaults.
  //
  // if an entry with the given name/number already exists, re-use
  // that entry for the dynamic part, otherwise create a new dynamic
  // entry and add it to the list.  Note that only the name and/or
  // number need to be kept valid in the dynamic entry.  Returns true
  // if the name already existed, otherwise false.
  //
  // Note: for things.ddf, only the name is significant when checking
  // if the entry already exists.
  //
  boolean_t (* start_entry)(const char *name);

  // parse a single field for the entry.  Usually it will just call
  // the ddf_main routine to handle the command list.  For
  // comma-separated fields (specials, states, etc), this routine will
  // be called multiple times, once for each element, and `index' is
  // used to indicate which element (starting at 0).
  //
  void (* parse_field)(const char *field, const char *contents,
      int index, boolean_t is_last);

  // when the entry has finished, this routine can perform any
  // necessary operations here (such as updating a number -> entry
  // lookup table).  In particular, it should copy the static buffer
  // part into the dynamic part.  It also should compute the CRC.
  //
  void (* finish_entry)(void);

  // this function is called when the #CLEARALL directive is used.
  // The entries should be deleted if it is safe (i.e. there are no
  // pointers to them), otherwise they should be marked `disabled' and
  // ignored in subsequent searches.  Note: The parser ensures that
  // this is never called in the middle of an entry.
  //
  void (* clear_all)(void);
}
readinfo_t;

//
// This structure forms the basis for referencing specials.
//
typedef struct
{
  // name of special
  char *name;

  // flag(s) or value of special
  int flags;

  // this is true if the DDF name (e.g. "GRAVITY") is opposite to the
  // code's flag name (e.g. MF_NoGravity).
  boolean_t negative; 
}
specflags_t;

typedef enum
{
  // special flag is unknown
  CHKF_Unknown,

  // the flag should be set (i.e. forced on)
  CHKF_Positive,

  // the flag should be cleared (i.e. forced off)
  CHKF_Negative,

  // the flag should be made user-definable
  CHKF_User
}
checkflag_result_e;

//
// This is a reference table, that determines what code pointer is placed in the
// states table entry.
//
typedef struct
{
  char *actionname;
  void (* action)(mobj_t * object);

  // -AJA- 1999/08/09: This function handles the argument when brackets
  // are present (e.g. "WEAPON_SHOOT(FIREBALL)").  NULL if unused.
  void (*handle_arg)(const char *arg, state_t * curstate);
}
actioncode_t;

// This structure is used for parsing states
typedef struct
{
  // state label
  const char *label;

  // redirection label for last state
  const char *last_redir;

  // pointer to state_num storage
  int *state_num;
}
state_starter_t;

// DDF_MAIN Code (Reading all files, main init & generic functions).
void DDF_MainReadFile(readinfo_t * readinfo);

void DDF_Error(const char *err, ...);
void DDF_Warning(const char *err, ...);
void DDF_WarnError(const char *err, ...);
void DDF_ErrorSetEntryName(const char *err, ...);
void DDF_ErrorClearEntryName(void);

void DDF_MainGetPercent(const char *info, void *storage);
void DDF_MainGetPercentAny(const char *info, void *storage);
void DDF_MainGetBoolean(const char *info, void *storage);
void DDF_MainGetFloat(const char *info, void *storage);
void DDF_MainGetAngle(const char *info, void *storage);
void DDF_MainGetSlope(const char *info, void *storage);
void DDF_MainGetInlineStr10(const char *info, void *storage);
void DDF_MainGetInlineStr32(const char *info, void *storage);
void DDF_MainGetNumeric(const char *info, void *storage);
void DDF_MainGetString(const char *info, void *storage);
void DDF_MainGetTime(const char *info, void *storage);
void DDF_MainGetColourmap(const char *info, void *storage);
void DDF_MainGetRGB(const char *info, void *storage);
void DDF_MainGetWhenAppear(const char *info, void *storage);
void DDF_MainGetBitSet(const char *info, void *storage);

boolean_t DDF_MainParseField(const commandlist_t *commands,
    const char *field, const char *contents);
void DDF_MainLookupSound(const char *info, void *storage);
void DDF_MainRefAttack(const char *info, void *storage);

void DDF_DummyFunction(const char *info, void *storage);

checkflag_result_e DDF_MainCheckSpecialFlag(const char *name,
  const specflags_t *flag_set, int *flag_value, 
  boolean_t allow_prefixes, boolean_t allow_user);

char *DDF_MainCreateUniqueName(const char *prefix, int num);
int DDF_MainLookupDirector(const mobjinfo_t *obj, const char *info);

// DDF_ANIM Code
void DDF_AnimInit(void);
void DDF_AnimCleanUp(void);

// DDF_ATK Code
void DDF_AttackInit(void);
void DDF_AttackCleanUp(void);
attacktype_t *DDF_AttackLookup(const char *name);

// DDF_GAME Code
void DDF_GameInit(void);
void DDF_GameCleanUp(void);

// DDF_LANG Code
void DDF_LanguageInit(void);
void DDF_LanguageCleanUp(void);

// DDF_LEVL Code
void DDF_LevelInit(void);
void DDF_LevelCleanUp(void);

// DDF_LINE Code
void DDF_LinedefInit(void);
void DDF_LinedefCleanUp(void);

#define EMPTY_COLMAP_NAME  "_NONE_"
#define EMPTY_COLMAP_NUM   -777

// DDF_MOBJ Code  (Moving Objects)
void DDF_MobjInit(void);
void DDF_MobjCleanUp(void);
void DDF_MobjGetSpecial(const char *info, void *storage);
void DDF_MobjGetExtra(const char *info, void *storage);
void DDF_MobjGetItemType(const char *info, void *storage);
void DDF_MobjGetBpAmmo(const char *info, void *storage);
void DDF_MobjGetBpAmmoLimit(const char *info, void *storage);
void DDF_MobjGetBpArmour(const char *info, void *storage);
void DDF_MobjGetBpKeys(const char *info, void *storage);
void DDF_MobjGetBpWeapon(const char *info, void *storage);
void DDF_MobjGetPlayer(const char *info, void *storage);
mobjinfo_t *DDF_MobjMakeAttackObj(mobjinfo_t *info, const char *atk_name);

void ThingParseField(const char *field, const char *contents,
    int index, boolean_t is_last);

// DDF_MUS Code
void DDF_MusicPlaylistInit(void);
void DDF_MusicPlaylistCleanUp(void);

// DDF_STAT Code
void DDF_StateInit(void);
void DDF_StateGetAttack(const char *arg, state_t * cur_state);
void DDF_StateGetMobj(const char *arg, state_t * cur_state);
void DDF_StateGetSound(const char *arg, state_t * cur_state);
void DDF_StateGetInteger(const char *arg, state_t * cur_state);
void DDF_StateGetIntPair(const char *arg, state_t * cur_state);
void DDF_StateGetFloat(const char *arg, state_t * cur_state);
void DDF_StateGetPercent(const char *arg, state_t * cur_state);
void DDF_StateGetJump(const char *arg, state_t * cur_state);
void DDF_StateGetAngle(const char *arg, state_t * cur_state);
void DDF_StateGetSlope(const char *arg, state_t * cur_state);

void DDF_StateReadState(const char *info, const char *label,
    int *first, int *last, int *state_num, int index, 
    const char *redir, const actioncode_t *action_list);
void DDF_StateFinishStates(int first, int last);
void DDF_StateFixupStates(int first, int last);
void DDF_StateCleanUp(void);
int StateFindLabel(int first, int last, const char *label);

// DDF_SECT Code
void DDF_SectorInit(void);
void DDF_SectGetDestRef(const char *info, void *storage);
void DDF_SectGetExit(const char *info, void *storage);
void DDF_SectGetLighttype(const char *info, void *storage);
void DDF_SectGetMType(const char *info, void *storage);
void DDF_SectorCleanUp(void);

// DDF_SFX Code
void DDF_SFXInit(void);
void DDF_SFXCleanUp(void);

// DDF_SWTH Code
// -KM- 1998/07/31 Switch and Anim ddfs.
void DDF_SWInit(void);
void DDF_SWCleanUp(void);

// DDF_WEAP Code
void DDF_WeaponInit(void);
void DDF_WeaponCleanUp(void);
extern const specflags_t ammo_types[];

// DDF_COLM Code -AJA- 1999/07/09.
void DDF_ColmapInit(void);
void DDF_ColmapCleanUp(void);

// DDF_BOOM Code
void DDF_BoomMakeGenSector(specialsector_t *sec, int number);
void DDF_BoomMakeGenLine(linedeftype_t *line, int number);

// Miscellaneous stuff needed here & there
extern mobjinfo_t buffer_mobj;
extern const mobjinfo_t template_mobj;
extern const commandlist_t elevator_commands[];
extern const commandlist_t floor_commands[];
extern const commandlist_t thing_commands[];
extern const commandlist_t damage_commands[];

#endif // __DDF_LOCAL__
