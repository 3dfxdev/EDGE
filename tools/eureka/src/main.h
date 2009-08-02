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
#include <math.h>

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
void FatalError(const char *fmt, ...);
void BugError(const char *fmt, ...);

#include "sys_type.h"
#include "sys_macro.h"
#include "sys_endian.h"
#include "sys_debug.h"


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


/*
 *  Even more stuff ("the macros and constants")
 */

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



// l_unlink.cc
void unlink_sidedef (SelPtr linedefs, int side1, int side2);


int entryname_cmp (const char *entry1, const char *entry2);


void Beep (void);


int vertex_radius (double scale);


#define verbmsg  LogPrintf
#define warn     LogPrintf
#define err      LogPrintf


#endif  /* DO NOT ADD ANYTHING AFTER THIS LINE */
//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
