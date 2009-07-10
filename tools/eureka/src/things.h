/*
 *  things.h
 *  Header for things.cc
 *  BW & RQ sometime in 1993 or 1994.
 */


#ifndef YH_THINGS  /* Prevent multiple inclusion */
#define YH_THINGS  /* Prevent multiple inclusion */


#include "w_structs.h"


/* starting areas */
#define THING_PLAYER1         1
#define THING_PLAYER2         2
#define THING_PLAYER3         3
#define THING_PLAYER4         4
#define THING_DEATHMATCH      11


extern
#ifndef YC_THINGS
const
#endif
int _max_radius;


void        create_things_table ();
void        delete_things_table ();
bool        is_thing_type    (wad_ttype_t type);
acolour_t   get_thing_colour (wad_ttype_t type);
const char *get_thing_name   (wad_ttype_t type);
const char *get_thing_sprite (wad_ttype_t type);
char        get_thing_flags  (wad_ttype_t type);
int         get_thing_radius (wad_ttype_t type);
inline int  get_max_thing_radius () { return _max_radius; }
const char *GetAngleName (int);
const char *GetWhenName (int);


/*
 *  angle_to_direction - convert angle to direction (0-7)
 *
 *  Return a value that is guaranteed to be within [0-7].
 */
inline int angle_to_direction (wad_tangle_t angle)
{
  return ((unsigned) angle / 45) % 8;
}


void spin_things (SelPtr obj, int degrees);

void centre_of_things (SelPtr list, int *x, int *y);

void frob_things_flags (SelPtr list, int op, int operand);


#endif  /* Prevent multiple inclusion */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
