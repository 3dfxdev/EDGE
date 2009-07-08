/*
 *  gcolour1.h
 *  Allocate and free the game colours.
 *
 *  By "game colours", I mean the colours used to draw
 *  the game graphics (flats, textures, sprites), as
 *  opposed to the "application colours" which don't
 *  depend on the the game and are used to draw the
 *  windows, the menus and the map.
 *
 *  The application colours are handled in acolours.cc.
 *
 *  AYM 1998-11-29
 */

#ifndef __GCOLOUR1_H__
#define __GCOLOUR1_H__

#include "im_color.h"


pcolour_t *alloc_game_colours (int playpalnum);
void free_game_colours (pcolour_t *game_colours);


extern pcolour_t *game_colour;  // Pixel values for the DOOM_COLOURS game clrs.
extern int colour0;   // Game colour to which g. colour 0 is remapped
extern int sky_colour;    // Game colour for a medium blue sky


/*
 *  Game_colour_24 - convert game colours to pixel values
 *  
 *  This class is used to speed up displaying of images on
 *  visuals where bits_per_pixel is 24.
 *
 *  Game_colour_24::lut() returns a pointer to a const table
 *  that is similar to game_colour[] except that it is
 *  optimized toward the needs of display_img() :
 *  each member can be readily copied into the XImage buffer
 *  (this is not true of game_colour[], at least not if the
 *  client is big-endian or does not have the same
 *  endianness as the server).
 *
 *  There is exactly 1 instance of this class (global). It's
 *  refreshed in InitGfx() and used in display_img().
 *  To avoid wasting memory when it's not needed (when the
 *  depth is != 24), the array is allocated upon refresh and
 *  refresh() is only called when the depth is == 24.
 */


typedef byte pv24_t[3];  // A 24-bit pixel value


class Game_colour_24
{
  public :
    Game_colour_24 () { pv_table = 0; }
    ~Game_colour_24 () { if (pv_table) delete[] pv_table; }
    // Create/refresh the table
    void refresh (const pcolour_t *game_colour, const bool big_endian);
    // Return a pointer on an array of pv24_t[DOOM_COLOURS]
    const pv24_t *lut () { return pv_table; }
  private :
    pv24_t *pv_table;
};


extern Game_colour_24 game_colour_24;

#endif
