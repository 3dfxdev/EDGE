//----------------------------------------------------------------------------
//  EDGE Linux SDL Video Code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2003  The EDGE Team.
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

#ifdef MACOSX
#include <SDL.h>
#else
#include <SDL/SDL.h>
#endif

#include "i_defs.h"
#include "./i_trans.h"

#include "version.h"
#include "dm_state.h"
#include "dm_defs.h"
#include "dm_type.h"
#include "m_argv.h"
#include "st_stuff.h"
#include "v_video1.h"
#include "v_video2.h"
#include "v_res.h"
#include "v_colour.h"
#include "v_screen.h"

#include "e_main.h"
#include "e_event.h"
#include "z_zone.h"
#include "w_wad.h"

#include "s_sound.h"


#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <stdarg.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <signal.h>


short hicolortransmask1, hicolortransmask2;

// Dummy screen... 
// mainscreen is a full-size subscreen of this one.
static screen_t dummy_screen;
static SDL_Surface *my_vis;

// This needs to be global to retain fullscreen options
#ifdef USE_GL
static Uint32 my_flags = (SDL_OPENGL | SDL_DOUBLEBUF);
#else
static Uint32 my_flags = (SDL_SWSURFACE | SDL_HWPALETTE);
#endif

unsigned char *thepalette;

static int graphics_shutdown = 0;

#if defined(MACOSX) || defined(BeOS)
bool use_warp_mouse = true;
#else
bool use_warp_mouse = false;
#endif

static SDL_Color my_pal[256];

colourshift_t redshift, greenshift, blueshift;

// Possible Screen Modes
static screenmode_t possresmode[] =
{
  // fullscreen modes
  { 320, 200,  8, false},
  { 320, 240,  8, false},
  { 400, 300,  8, false},
  { 512, 384,  8, false},
  { 640, 400,  8, false},
  { 640, 480,  8, false},
  { 800, 600,  8, false},
  {1024, 768,  8, false},
  { 320, 200, 16, false},
  { 320, 240, 16, false},
  { 400, 300, 16, false},
  { 512, 384, 16, false},
  { 640, 400, 16, false},
  { 640, 480, 16, false},
  { 800, 600, 16, false},
  {1024, 768, 16, false},

  // windowed modes
  { 320, 200,  8, true},
  { 320, 240,  8, true},
  { 400, 300,  8, true},
  { 512, 384,  8, true},
  { 640, 400,  8, true},
  { 640, 480,  8, true},
  { 800, 600,  8, true},
  {1024, 768,  8, true},
  { 320, 200, 16, true},
  { 320, 240, 16, true},
  { 400, 300, 16, true},
  { 512, 384, 16, true},
  { 640, 400, 16, true},
  { 640, 480, 16, true},
  { 800, 600, 16, true},
  {1024, 768, 16, true},

  {  -1,  -1, -1}
};


// ====================== INTERNALS ======================

//
// MakeCol
//
static inline unsigned long MakeCol(long r, long g, long b)
{
  r = (r >> (8 - redshift.bits))   << redshift.shift;
  g = (g >> (8 - greenshift.bits)) << greenshift.shift;
  b = (b >> (8 - blueshift.bits))  << blueshift.shift;

  return r|g|b;
}

//
// CountLeadingZeros
//
static const inline unsigned long CountLeadingZeros(unsigned long x)
{
  unsigned long r = 0;

  if (x == 0)
    return 0;

  while (!(x & 1))
  {
    x >>= 1;
    r++;
  }

  return r;
}

//
// CountBitsInMask
//
static const inline unsigned long CountBitsInMask(unsigned long x)
{
  unsigned long r = 0;

  while (!(x & 1))
    x >>= 1;

  while (x & 1)
  {
    x >>= 1;
    r++;
  }

  if (x)
    I_Error ("X Version: Mask is not contiguous.\n");

  return r;
}

//
// SetColourShift
//
// Constructs the shifts necessary to set one element of an RGB colour value
//
void SetColourShift(unsigned long mask, colourshift_t * ps)
{
  ps->mask  = mask;
  ps->shift = CountLeadingZeros(mask);
  ps->bits  = CountBitsInMask(mask);
}

//
// BlitToScreen
//
static inline void BlitToScreen(void)
{
  register int y;
  register unsigned char *s1, *d1;

  s1 = (unsigned char *) dummy_screen.data;
  d1 = (unsigned char *) my_vis->pixels;

  for (y=0; y < SCREENHEIGHT; y++)
  {
    memcpy(d1, s1, SCREENWIDTH * BPP);

    s1 += SCREENPITCH;
    d1 += SCREENWIDTH * BPP;
  }
}

static void VideoModeCommonStuff(void)
{
  // -AJA- turn off cursor -- BIG performance increase.
  //       Plus, the combination of no-cursor + grab gives 
  //       continuous relative mouse motion.
  SDL_ShowCursor(0);
  SDL_WM_GrabInput(SDL_GRAB_ON);

  // -DEL- 2001/01/29 SDL_GrabInput doesn't work on beos so try to
  // stop the mouse leaving the window every frame.
  if (use_warp_mouse)
    SDL_WarpMouse(SCREENWIDTH/2, SCREENHEIGHT/2);

  SDL_WM_SetCaption("Enhanced Doom Gaming Engine", "EDGE");

#ifdef DEVELOPERS
  // override SDL signal handlers (the so-called "parachute").
  signal(SIGFPE,SIG_DFL);
  signal(SIGSEGV,SIG_DFL);
#endif
}

// =================== END OF INTERNALS ===================

//
// I_StartupGraphics
//
void I_StartupGraphics(void)
{
  int i;
  int got_depth;

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) < 0)
    I_Error("Couldn't init SDL\n");

  M_CheckBooleanParm("warpmouse", &use_warp_mouse, false);
      
#ifdef USE_GL
  SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   5);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  5);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
#endif
  
  // -ACB- 2000/03/16 Detect Possible Resolutions
  for (i = 0; possresmode[i].width != -1; i++)
  {
    if (possresmode[i].windowed)
    {
      V_AddAvailableResolution(possresmode + i);
      continue;
    }
      
    got_depth = SDL_VideoModeOK(possresmode[i].width, possresmode[i].width,
        possresmode[i].depth, my_flags | SDL_FULLSCREEN);

    if (got_depth == possresmode[i].depth ||
        (got_depth == 15 && possresmode[i].depth == 16))
    {
      V_AddAvailableResolution(possresmode + i);
    }
  }
}

//
// I_GetTruecolInfo
//
void I_GetTruecolInfo(truecol_info_t * info)
{
  info->red_bits = redshift.bits;
  info->red_shift = redshift.shift;
  info->red_mask = redshift.mask;

  info->green_bits = greenshift.bits;
  info->green_shift = greenshift.shift;
  info->green_mask = greenshift.mask;

  info->blue_bits = blueshift.bits;
  info->blue_shift = blueshift.shift;
  info->blue_mask = blueshift.mask;

  info->grey_mask = 0x7FFFFFFF;

  // check for RGB 5:6:5 mode (should be more general !)
  if (info->red_bits == 5 && info->green_bits == 6 &&
      info->blue_bits == 5)
  {
    info->grey_mask = 0xFFDF;
  }
}

//
// I_SetScreenSize
//
bool I_SetScreenSize(screenmode_t *mode)
{
#ifdef USE_GL
  if (mode->depth != 16)
    return false;
#endif

  my_vis = SDL_SetVideoMode(mode->width, mode->height, mode->depth, 
      my_flags | (mode->windowed ? 0 : SDL_FULLSCREEN));
   
  if (my_vis == NULL)
    return false;

///  if (my_vis->format->BytesPerPixel != mode->depth / 8)
///    return false;
 
  SCREENWIDTH  = my_vis->w;
  SCREENHEIGHT = my_vis->h;
  SCREENBITS   = my_vis->format->BytesPerPixel * 8;

#ifdef USE_GL
    // -AJA- 2003/10/21: For OPENGL, SDL does not create a shadow surface
	//       (so you ask for a 16 bit mode, but get a 32 bit one).  Hence
	//       the value of SCREENBITS becomes a value that the main EDGE
	//       code can't handle. [The "BPP 4 invalid" error].  After 1.28,
	//       this hack can go away (use SDL's mode query function).
	SCREENBITS = mode->depth;
#endif

  SCREENPITCH = V_GetPitch(mode->width, mode->depth / 8);

  if (BPP != 1)
  {
    SetColourShift(my_vis->format->Rmask,   &redshift);
    SetColourShift(my_vis->format->Gmask, &greenshift);
    SetColourShift(my_vis->format->Bmask,  &blueshift);
  }

  if (dummy_screen.data)
    Z_Free(dummy_screen.data);

  dummy_screen.width  = SCREENWIDTH;
  dummy_screen.height = SCREENHEIGHT;
  dummy_screen.pitch  = SCREENPITCH;
  dummy_screen.bytepp = BPP;
  dummy_screen.parent = NULL;
  dummy_screen.data   = Z_New(byte, SCREENPITCH * SCREENHEIGHT);

  main_scr = V_CreateSubScreen (&dummy_screen, 0, 0, SCREENWIDTH, SCREENHEIGHT);

  VideoModeCommonStuff();

#ifdef USE_GL
  SDL_GL_SwapBuffers();
#endif

  return true;
}

//
// I_StartFrame
//
void I_StartFrame(void)
{
}

//
// I_FinishFrame
//
void I_FinishFrame(void)
{
#ifdef USE_GL
  SDL_GL_SwapBuffers();
#else
  BlitToScreen();

  SDL_UpdateRect(my_vis, 0, 0, SCREENWIDTH, SCREENHEIGHT);
#endif

  if (use_warp_mouse)
    SDL_WarpMouse(SCREENWIDTH/2, SCREENHEIGHT/2);
}

//
// I_SetPalette
//
void I_SetPalette(byte palette[256][3])
{
  int i;

  const byte *gtable = gammatable[usegamma];

  if (BPP != 1)
    return;

  for (i = 0; i < 256; i++)
  {
    int r = gtable[palette[i][0]];
    int g = gtable[palette[i][1]];
    int b = gtable[palette[i][2]];

    my_pal[i].r = (r << 8) + r;
    my_pal[i].g = (g << 8) + g;
    my_pal[i].b = (b << 8) + b;
  }

  SDL_SetColors(my_vis, my_pal, 0, 256);
}

//
// I_Colour2Pixel
//
long I_Colour2Pixel(byte palette[256][3], int col)
{
  if (BPP == 1)
    return col;

  return MakeCol(palette[col][0], palette[col][1], palette[col][2]);
}

//
// I_ShutdownGraphics
//
void I_ShutdownGraphics(void)
{
  if (graphics_shutdown)
    return;

  graphics_shutdown = 1;

  fprintf(stderr, "Shutting down graphics...\n");
  SDL_Quit ();
}
