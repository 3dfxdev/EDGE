/*
 *  acolours.h
 *  Allocate and free the application colours.
 *
 *  By "application colours", I mean the colours used to draw
 *  the windows, the menus and the map, as opposed to the
 *  "game colours" which depend on the game (they're in the
 *  PLAYPAL lump) and are used to draw the game graphics
 *  (flats, textures, sprites...).
 *
 *  The game colours are handled in gcolour1.cc and gcolour2.cc.
 *
 *  AYM 1998-11-29
 */


#include "im_color.h"


#define BLACK           FL_BLACK
#define BLUE            FL_BLUE
#define GREEN           FL_GREEN
#define CYAN            FL_CYAN
#define RED             FL_RED
#define MAGENTA         FL_MAGENTA
#define BROWN           FL_DARK_RED
#define YELLOW          fl_rgb_color(255,255,0)
#define WHITE           FL_WHITE

#define LIGHTGREY       fl_rgb_color(128,128,128)
#define DARKGREY        fl_rgb_color(64,64,64)

#define LIGHTBLUE       fl_rgb_color(128,128,255)
#define LIGHTGREEN      fl_rgb_color(128,255,128)
#define LIGHTCYAN       fl_rgb_color(128,255,255)
#define LIGHTRED        fl_rgb_color(255,128,128)
#define LIGHTMAGENTA    fl_rgb_color(255,128,255)

#define WINBG           fl_rgb_color(0x2a, 0x24, 0x18)
#define WINBG_LIGHT     fl_rgb_color(0x48, 0x42, 0x3c)
#define WINBG_DARK      fl_rgb_color(0x20, 0x1b, 0x12)
#define WINBG_HL        fl_rgb_color(0x58, 0x50, 0x48)

#define WINFG           fl_rgb_color(0xa0, 0xa0, 0xa0)
#define WINFG_HL        fl_rgb_color(0xd0, 0xd0, 0xd0)
#define WINFG_DIM       fl_rgb_color(0x48, 0x48, 0x48)
#define WINFG_DIM_HL    fl_rgb_color(0x70, 0x70, 0x70)

#define WINLABEL        fl_rgb_color(0x78, 0x78, 0x78)
#define WINLABEL_HL     fl_rgb_color(0xa0, 0xa0, 0xa0)
#define WINLABEL_DIM    fl_rgb_color(0x38, 0x38, 0x38)
#define WINLABEL_DIM_HL fl_rgb_color(0x50, 0x50, 0x50)

#define GRID_POINT      fl_rgb_color(0, 0, 0xFF)
#define GRID_DARK       fl_rgb_color(0, 0, 0x99)
#define GRID_MEDIUM     fl_rgb_color(0, 0, 0xBB)
#define GRID_BRIGHT     fl_rgb_color(0, 0, 0xEE)

#define LINEDEF_NO      fl_rgb_color(0x40, 0xd0, 0xf0)
#define SECTOR_NO       fl_rgb_color(0x40, 0xd0, 0xf0)
#define THING_NO        fl_rgb_color(0x40, 0xd0, 0xf0)
#define VERTEX_NO       fl_rgb_color(0x40, 0xd0, 0xf0)
#define CLR_ERROR       fl_rgb_color(0xff, 0,    0)
#define THING_REM       fl_rgb_color(0x40, 0x40, 0x40)

#define SECTOR_TAG      fl_rgb_color(0x00, 0xff, 0x00)
#define SECTOR_TAGTYPE  fl_rgb_color(0x00, 0xe0, 0xe0)
#define SECTOR_TYPE     fl_rgb_color(0x00, 0x80, 0xff)

#define WINTITLE        fl_rgb_color(0xff, 0xff, 0x00)

