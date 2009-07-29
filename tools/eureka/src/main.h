//------------------------------------------------------------------------
//  MAIN DEFINITIONS
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2009 Andrew Apted
//  Copyright (C) 1997-2003 André Majorel et al
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
//------------------------------------------------------------------------
//
//  Based on Yadex which incorporated code from DEU 5.21 that was put
//  in the public domain in 1994 by Raphaël Quinet and Brendon Wyber.
//
//------------------------------------------------------------------------


#ifndef YH_YADEX  /* DO NOT INSERT ANYTHING BEFORE THIS LINE */
#define YH_YADEX


#define Y_UNIX
#define Y_GETTIMEOFDAY
#define Y_NANOSLEEP
#define Y_SNPRINTF
#define Y_USLEEP


/*
 *  Standard headers
 */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>

#if defined Y_UNIX
#include <unistd.h>
#endif

#include <vector>


/*
 *  Additional libraries
 */

#include "hdr_fltk.h"


/*
 *  Platform-independant types and formats.
 */
#include "sys_type.h"
#include "sys_macro.h"
#include "sys_endian.h"
#include "sys_assert.h"


typedef int  SelPtr;   // TEMPORARY FIXME


typedef enum
{
	BOP_ADD = 0,   // Add to selection
	BOP_REMOVE,    // Remove from selection
	BOP_TOGGLE     // If not in selection, add; else, remove
}
sel_op_e;


#include "objid.h"
#include "m_bitvec.h"  /* bv_set, bv_clear, bv_toggle */
#include "m_select.h"

#include "yutil.h"
#include "ymemory.h"



/*
 *  Platform definitions
 */
#if defined Y_UNIX
const int Y_PATH      = 255;
const int Y_FILE_NAME = 255;
#endif
typedef char y_file_name_t[Y_FILE_NAME + 1];


/*
 *  Constants of the universe.
 */
const double HALFPI = 1.5707963;
const double ONEPI  = 3.1415926;
const double TWOPI  = 6.2831852;
const double ANSWER = 42;


/*
 *  Syntactic sugar
 */
const char *const Y_NULL = 0;  // NULL (const char *)


/*
 *  To avoid including certain headers.
 */
class rgb_c;


#include "e_basis.h"

#include "w_structs.h"


/*
 *  Doom definitions
 *  Things about the Doom engine
 *  FIXME should move as much of this as possible to the ygd file...
 *  FIXME Hexen has a different value for MIN_DEATHMATH_STARTS
 */
const int DOOM_PLAYER_HEIGHT         = 56;
const int DOOM_FLAT_WIDTH            = 64;
const int DOOM_FLAT_HEIGHT           = 64;
const size_t DOOM_MIN_DEATHMATCH_STARTS = 4;
const size_t DOOM_MAX_DEATHMATCH_STARTS = 10;
const size_t DOOM_COLOURS               = 256;
typedef enum { YGPF_NORMAL, YGPF_ALPHA, YGPF_PR } ygpf_t;
typedef enum { YGTF_NORMAL, YGTF_NAMELESS, YGTF_STRIFE11 } ygtf_t;
typedef enum { YGTL_NORMAL, YGTL_TEXTURES, YGTL_NONE } ygtl_t;





/*
 *  More stuff
 */


// Operations on the selection :

#include "objects.h"


// Confirmation options are stored internally this way :
typedef enum
{
   YC_YES      = 'y',
   YC_NO       = 'n',
   YC_ASK      = 'a',
   YC_ASK_ONCE = 'o'
}
confirm_t;

// Bit bashing operations
const int YO_AND    = 'a';  // Argument = mask
const int YO_CLEAR  = 'c';  // Argument = bit#
const int YO_COPY   = 'd';  // Argument = source_bit# dest_bit#
const int YO_OR     = 'o';  // Argument = mask
const int YO_SET    = 's';  // Argument = bit#
const int YO_TOGGLE = 't';  // Argument = bit#
const int YO_XOR    = 'x';  // Argument = mask


/*
 *  Even more stuff ("the macros and constants")
 */

extern const char *const log_file;
extern const char *const msg_unexpected;  // "unexpected error"
extern const char *const msg_nomem;       // "Not enough memory"

// AYM 19980213: InputIntegerValue() uses this to mean that Esc was pressed
#define IIV_CANCEL  INT_MIN


/*
 *  Not real variables -- just a way for functions
 *  that return pointers to report errors in a better
 *  fashion than by just returning NULL and setting
 *  a global variable.
 */
extern char error_non_unique[1];  // Found more than one
extern char error_none[1];        // Found none
extern char error_invalid[1];     // Invalid parameter


/*
 *  Interfile global variables
 */

extern FILE *logfile;     // Filepointer to the error log
extern bool  Registered;    // Registered or shareware iwad ?
extern int remind_to_build_nodes; // Remind the user to build nodes

// Set from command line and/or config file
extern bool  autoscroll;  // Autoscrolling enabled.
extern unsigned long autoscroll_amp;// Amplitude in percents of screen/window size.
extern unsigned long autoscroll_edge; // Max. dist. in pixels to edge.
extern const char *config_file; // Name of the configuration file
extern int   copy_linedef_reuse_sidedefs;
extern bool  Debug;     // Are we debugging?
extern int   default_ceiling_height;      // For new sectors
extern char  default_ceiling_texture[8 + 1];// For new sectors
extern int   default_floor_height;      // For new sectors
extern char  default_floor_texture[8 + 1];  // For new sectors
extern int   default_light_level;     // For new sectors
extern char  default_lower_texture[8 + 1]; // For new linedefs
extern char  default_middle_texture[8 + 1];  // For new linedefs
extern int   default_thing;       // For new THINGS
extern char  default_upper_texture[8 + 1]; // For new linedefs
extern int   double_click_timeout;// Max ms between clicks of double click.
extern bool  Expert;    // Don't ask for confirmation for some ops.
extern const char *Game;  // Name of game "doom", "doom2", "heretic", ...
extern const char *Warp;  // Map name to edit

extern int   idle_sleep_ms; // Time to sleep after empty XPending()
extern int   zoom_default;  // Initial zoom factor for map
extern int   zoom_step;   // Step between zoom factors in percent
extern int   digit_zoom_base; // Zoom factor of `1' key, in percent
extern int   digit_zoom_step; // Step between digit keys, in percent 
extern confirm_t insert_vertex_merge_vertices;
extern confirm_t insert_vertex_split_linedef;
extern bool  blindly_swap_sidedefs;
extern const char *Iwad1; // Name of the Doom iwad
extern const char *Iwad2; // Name of the Doom II iwad
extern const char *Iwad3; // Name of the Heretic iwad
extern const char *Iwad4; // Name of the Hexen iwad
extern const char *Iwad5; // Name of the Strife iwad
extern const char *Iwad6; // Name of the Doom alpha 0.2 iwad
extern const char *Iwad7; // Name of the Doom alpha 0.4 iwad
extern const char *Iwad8; // Name of the Doom alpha 0.5 iwad
extern const char *Iwad9; // Name of the Doom press release iwad
extern const char *Iwad10;  // Name of the Strife 1.0 iwad
extern const char *MainWad; // Name of the main wad file
#ifdef AYM_MOUSE_HACKS
extern int   MouseMickeysH; 
extern int   MouseMickeysV; 
#endif
extern char **PatchWads;  // List of pwad files
extern bool  Quiet;   // Don't beep when an object is selected
extern bool  Quieter;   // Don't beep, even on error
extern unsigned long scroll_less;// %s of screenful to scroll by
extern unsigned long scroll_more;// %s of screenful to scroll by
extern int   show_help;   // Print usage message and exit.
extern int   sprite_scale;  // Relative scale used to display sprites
extern bool  SwapButtons; // Swap right and middle mouse buttons
extern int   verbose;   // Verbose mode
extern int   welcome_message; // Print the welcome message on startup.
extern const char *bench; // Benchmark to run


typedef int acolour_t;



// Defined in edit.cc
extern bool InfoShown;          // Is the bottom line displayed?


extern int KF;  // widget scale Factor
extern int KF_fonth;  // default font size


/*
 *  Prototypes
 *  AYM 1998-10-16: DEU used to have _all_ prototypes here. Thus
 *  I had to recompile _all_ the modules every time I changed
 *  a single prototype. To avoid this, my theory is to put all
 *  the prototypes I can in individual headers. There is still
 *  room for improvement on that matter...
 */

// checks.cc (previously in editobj.cc)
void CheckLevel (int, int, int pulldown);
bool CheckStartingPos (void);


// editobj.cc
int InputObjectNumber (int, int, int, int);
int InputObjectXRef (int, int, int, bool, int);
bool Input2VertexNumbers (int, int, const char *, int *, int *);
void EditObjectsInfo (int, int, int, SelPtr);
void InsertStandardObject (int, int, int choice);
void MiscOperations (int, SelPtr *, int choice);


// geom.cc
unsigned ComputeAngle (int, int);
unsigned ComputeDist (int, int);

// l_align.cc (previously in objects.cc)
void AlignTexturesY (SelPtr *);
void AlignTexturesX (SelPtr *);

// l_misc.cc (previously in objects.cc)
void FlipLineDefs (SelPtr, bool);
void SplitLineDefs (SelPtr);
void MakeRectangularNook (SelPtr obj, int width, int depth, int convex);
void SetLinedefLength (SelPtr obj, int length, int move_2nd_vertex);

// l_prop.cc (previously in editobj.cc)
void TransferLinedefProperties (int src_linedef, SelPtr linedefs);

// l_unlink.cc
void unlink_sidedef (SelPtr linedefs, int side1, int side2);

// levels.cc
int ReadLevelData (const char *);
void ForgetLevelData (void);
int SaveLevelData (const char *, const char *level_name);
void ReadWTextureNames (void);
void ForgetFTextureNames (void);
int is_flat_name_in_list (const char *name);
void ReadFTextureNames (void);
void ForgetWTextureNames (void);

// names.cc
const char *GetObjectTypeName (int);
const char *GetEditModeName (int);
const char *GetLineDefTypeName (int);
const char *GetLineDefTypeLongName (int);
const char *GetLineDefFlagsName (int);
const char *GetLineDefFlagsLongName (int);
const char *GetSectorTypeName (int);
const char *GetSectorTypeLongName (int);

// s_door.cc (previously in objects.cc)
void MakeDoorFromSector (int);

// s_lift.cc (previously in objects.cc)
void MakeLiftFromSector (int);

// s_merge.cc (previously in objects.cc)
void MergeSectors (SelPtr *);
void DeleteLineDefsJoinSectors (SelPtr *);

// s_misc.cc (previously in objects.cc)
void DistributeSectorFloors (SelPtr);
void DistributeSectorCeilings (SelPtr);
void RaiseOrLowerSectors (SelPtr obj);
void BrightenOrDarkenSectors (SelPtr obj);
void SuperSectorSelector (int map_x, int map_y, int new_sec);

// s_prop.cc (previously in editobj.cc)
void TransferSectorProperties (int src_sector, SelPtr sectors);

// s_split.cc (previously in objects.cc)
void SplitSector (int, int);
void SplitLineDefsAndSector (int, int);
void MultiSplitLineDefsAndSector (int, int);

// selrect.cc
// t_prop.c (previously in editobj.c)
void TransferThingProperties (int src_thing, SelPtr things);

// v_merge.cc
void DeleteVerticesJoinLineDefs (SelPtr );
void MergeVertices (SelPtr *);
bool AutoMergeVertices (SelPtr *, int obj_type, char operation);

// v_polyg.cc
void InsertPolygonVertices (int, int, int, int);

// verbmsg.cc
void verbmsg (const char *fmt, ...);

// version.cc
extern const char *const yadex_version;
extern const char *const yadex_source_date;

// prefix.cc
extern const char *const yadex_etc_version;
extern const char *const yadex_etc_common;
extern const char *const yadex_ygd_version;
extern const char *const yadex_ygd_common;

int entryname_cmp (const char *entry1, const char *entry2);

// warning.cc
void warn (const char *fmt, ...);

// yadex.cc
void Beep (void);
void LogMessage (const char *, ...);

// OTHER

int vertex_radius (double scale);


void FatalError (const char *fmt, ...);
void err (const char *fmt, ...);
void nf_bug (const char *fmt, ...);


#endif  /* DO NOT ADD ANYTHING AFTER THIS LINE */
//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
