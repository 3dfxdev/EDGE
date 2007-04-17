//----------------------------------------------------------------------------
//  EDGE Linux SDL Video Code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2005  The EDGE Team.
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

#include "i_defs.h"
#include "i_sdlinc.h"

#include "m_argv.h"
#include "m_misc.h"
#include "v_res.h"

#include <signal.h>

#include <string.h>

// workaround for old SDL version (< 1.2.10)
#if (SDL_PATCHLEVEL < 10)
#include <stdlib.h>
#define SDL_getenv  getenv
#define SDL_putenv  putenv
#endif


SDL_Surface *my_vis;

int graphics_shutdown = 0;


// Possible Screen Modes
static struct { int w, h; } possible_modes[] =
{
	{  320, 200, },
	{  320, 240, },
	{  400, 300, },
	{  512, 384, },
	{  640, 400, },
	{  640, 480, },
	{  800, 600, },
	{ 1024, 768, },
	{ 1280,1024, },
	{ 1600,1200, },

	{  -1,  -1, }
};


// ====================== INTERNALS ======================

void VideoModeCommonStuff(void)
{
	// -AJA- turn off cursor -- BIG performance increase.
	//       Plus, the combination of no-cursor + grab gives 
	//       continuous relative mouse motion.
	if (use_grab)
	{
		SDL_ShowCursor(0);
		SDL_WM_GrabInput(SDL_GRAB_ON);
	}

#ifdef DEVELOPERS
	// override SDL signal handlers (the so-called "parachute").
	signal(SIGFPE,SIG_DFL);
	signal(SIGSEGV,SIG_DFL);
#endif
}

// =================== END OF INTERNALS ===================


void I_StartupGraphics(void)
{
	if (M_CheckParm("-directx"))
		force_directx = true;

	if (M_CheckParm("-gdi") || M_CheckParm("-nodirectx"))
		force_directx = false;

	const char *driver = M_GetParm("-videodriver");

	if (! driver)
		driver = SDL_getenv("SDL_VIDEODRIVER");

	if (! driver)
	{
		driver = "default";

#ifdef WIN32
		if (force_directx)
			driver = "directx";
#endif
	}

	if (strcasecmp(driver, "default") != 0)
	{
		char buffer[200];
		snprintf(buffer, sizeof(buffer), "SDL_VIDEODRIVER=%s", driver);
		SDL_putenv(buffer);
	}

	I_Printf("SDL_Video_Driver: %s\n", driver);


	Uint32 flags = SDL_INIT_VIDEO;

	if (M_CheckParm("-core"))
		flags |= SDL_INIT_NOPARACHUTE;

	if (SDL_Init(flags) < 0)
		I_Error("Couldn't init SDL\n%s\n", SDL_GetError());

	M_CheckBooleanParm("warpmouse", &use_warp_mouse, false);
	M_CheckBooleanParm("grab", &use_grab, false);

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE,     5);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,   5);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,    5);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,   16);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);

	
    // -DS- 2005/06/27 Detect SDL Resolutions
	const SDL_VideoInfo *info = SDL_GetVideoInfo ();

	SDL_Rect **modes = SDL_ListModes (info->vfmt,
					  SDL_OPENGL | SDL_DOUBLEBUF | SDL_FULLSCREEN);

	if (modes && modes != (SDL_Rect **)-1)
	{
		for (; *modes; modes++)
		{
			scrmode_c test_mode;

			test_mode.width  = (*modes)->w;
			test_mode.height = (*modes)->h;
			test_mode.depth  = info->vfmt->BitsPerPixel;  // HMMMM ???
			test_mode.full   = true;

			if ((test_mode.width & 15) != 0)
				continue;

			if (test_mode.depth == 15 || test_mode.depth == 16 ||
			    test_mode.depth == 24 || test_mode.depth == 32)
			{
				R_AddResolution(&test_mode);
			}
		}
	}

	// -ACB- 2000/03/16 Test for possible windowed resolutions
	for (int full = 0; full <= 1; full++)
	{
		for (int depth = 16; depth <= 32; depth = depth+16)
		{
			for (int i = 0; possible_modes[i].w != -1; i++)
			{
				scrmode_c mode;

				mode.width  = possible_modes[i].w;
				mode.height = possible_modes[i].h;
				mode.depth  = depth;
				mode.full   = full;

				int got_depth = SDL_VideoModeOK(mode.width, mode.height,
						mode.depth, SDL_OPENGL | SDL_DOUBLEBUF |
						(mode.full ? SDL_FULLSCREEN : 0));

				if (R_DepthIsEquivalent(got_depth, mode.depth))
				{
					R_AddResolution(&mode);
				}
			}
		}
	}

	I_Printf("I_StartupGraphics: initialisation OK\n");
}


bool I_SetScreenSize(scrmode_c *mode)
{
	I_Printf("I_SetScreenSize: trying %dx%d %dbpp (%s)\n",
			 mode->width, mode->height, mode->depth,
			 mode->full ? "fullscreen" : "windowed");

	my_vis = SDL_SetVideoMode(mode->width, mode->height, mode->depth, 
					SDL_OPENGL | SDL_DOUBLEBUF |
					(mode->full ? SDL_FULLSCREEN : 0));

	if (my_vis == NULL)
	{
		I_Printf("I_SetScreenSize: (mode not possible)\n");
		return false;
	}

	if (my_vis->format->BytesPerPixel <= 1)
	{
		I_Printf("I_SetScreenSize: 8-bit mode set (not suitable)\n");
		return false;
	}

	I_Printf("I_SetScreenSize: mode now %dx%d %dbpp flags:0x%x\n",
			 my_vis->w, my_vis->h,
			 my_vis->format->BitsPerPixel, my_vis->flags);

	VideoModeCommonStuff();

	SDL_GL_SwapBuffers();

	return true;
}


void I_StartFrame(void)
{
}


void I_FinishFrame(void)
{
	SDL_GL_SwapBuffers();
}


void I_PutTitle(const char *title)
{
	SDL_WM_SetCaption(title, title);
}


void I_RemoveGrab(void)
{
	if (my_vis && ! graphics_shutdown)
	{
		SDL_ShowCursor(1);
		SDL_WM_GrabInput(SDL_GRAB_OFF);
	}
}


void I_ShutdownGraphics(void)
{
	if (graphics_shutdown)
		return;

	graphics_shutdown = 1;

	if (SDL_WasInit(SDL_INIT_EVERYTHING))
		SDL_Quit ();
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
