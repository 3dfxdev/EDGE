/*
 *  game.h
 *  Header for game.cc
 *  AYM 19980129
 */


#include "im_rgb.h"


extern const char ygd_file_magic[];


/*
 *  Data structures for game definition data
 */

// ldt <number> <ldtgroup> <shortdesc> <longdesc>
typedef struct
   {
   int number;
   char ldtgroup;
   const char *shortdesc;
   const char *longdesc;
   } ldtdef_t;

// ldtgroup <ldtgroup> <description>
typedef struct
   {
   char ldtgroup;
   const char *desc;
   } ldtgroup_t;

// st <number> <shortdesc> <longdesc>
typedef struct
   {
   int number;
   const char *shortdesc;
   const char *longdesc;
   } stdef_t;

// thing <number> <thinggroup> <flags> <radius> <description> [<sprite>]
typedef struct
   {
   int number;    // Thing number
   char thinggroup; // Thing group
   char flags;    // Flags
   int radius;    // Radius of thing
   const char *desc;  // Short description of thing
   const char *sprite;  // Root of name of sprite for thing
   } thingdef_t;
/* (1)  This is only here for speed, to avoid having to lookup
        thinggroup for each thing when drawing things */
const char THINGDEF_SPECTRAL = 0x01;

// thinggroup <thinggroup> <colour> <description>
typedef struct
   {
   char thinggroup; // Thing group
   rgb_c rgb;   // RGB colour
   acolour_t acn; // Application colour#
   const char *desc;  // Description of thing group
   } thinggroup_t;


/*
 *  Global variables that contain game definition data
 *  Those variables are defined in yadex.cc
 */

typedef enum { YGLF__, YGLF_ALPHA, YGLF_DOOM, YGLF_HEXEN } yglf_t;
typedef enum { YGLN__, YGLN_E1M10, YGLN_E1M1, YGLN_MAP01 } ygln_t;
// ygpf_t and ygtf_t are defined in yadex.h
extern yglf_t yg_level_format;
extern ygln_t yg_level_name;
extern ygpf_t yg_picture_format;
extern ygtf_t yg_texture_format;
extern ygtl_t yg_texture_lumps;

extern std::vector<ldtdef_t *> ldtdef;
extern std::vector<ldtgroup_t *> ldtgroup;
extern std::vector<stdef_t *> stdef;
extern std::vector<thingdef_t *> thingdef;
extern std::vector<thinggroup_t *> thinggroup;


// Shorthands to make the code more readable
#define CUR_LDTDEF     ((ldtdef_t     *)al_lptr (ldtdef    ))
#define CUR_LDTGROUP   ((ldtgroup_t   *)al_lptr (ldtgroup  ))
#define CUR_STDEF      ((stdef_t      *)al_lptr (stdef     ))
#define CUR_THINGDEF   ((thingdef_t   *)al_lptr (thingdef  ))
#define CUR_THINGGROUP ((thinggroup_t *)al_lptr (thinggroup))

#define LDT_FREE    '\0'  /* KLUDGE: bogus ldt group   (see game.c) */
#define ST_FREE     '\0'  /* KLUDGE: bogus sector type (see game.c) */
#define THING_FREE  '\0'  /* KLUDGE: bogus thing group (see game.c) */


