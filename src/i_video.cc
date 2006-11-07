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

// Possible Windowed Screen Modes
i_scrmode_t posswinmode[] =
{
	// windowed modes
	{ 320, 200, 16, true},
	{ 320, 240, 16, true},
	{ 400, 300, 16, true},
	{ 512, 384, 16, true},
	{ 640, 400, 16, true},
	{ 640, 480, 16, true},
	{ 800, 600, 16, true},
	{1024, 768, 16, true},
	{1280,1024, 16, true},
	{1600,1200, 16, true},

	{ 320, 200, 32, true},  // TRUECOLOR
	{ 320, 240, 32, true},
	{ 400, 300, 32, true},
	{ 512, 384, 32, true},
	{ 640, 400, 32, true},
	{ 640, 480, 32, true},
	{ 800, 600, 32, true},
	{1024, 768, 32, true},
	{1280,1024, 32, true},
	{1600,1200, 32, true},

	{  -1,  -1, -1, false}
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

//
// I_StartupGraphics
//
void I_StartupGraphics(void)
{
	const char *driver = M_GetParm("-videodriver");

	if (! driver)
		driver = SDL_getenv("SDL_VIDEODRIVER");

	// FIXME:
	// if (! driver)
	//   driver = CFG variable

#ifdef WIN32
	if (! driver)
		driver = "directx";
#endif

	if (! driver)
		driver = "default";

	if (strcasecmp(driver, "default") != 0)
	{
		char buffer[200];
		snprintf(buffer, sizeof(buffer), "SDL_VIDEODRIVER=%s", driver);
		SDL_putenv(buffer);
	}

	I_Printf("SDL_Video_Driver: %s\n", driver);

	// FIXME: CFG variable = driver

	Uint32 flags = SDL_INIT_VIDEO;

	if (M_CheckParm("-core"))
		flags |= SDL_INIT_NOPARACHUTE;

	if (SDL_Init(flags) < 0)
		I_Error("Couldn't init SDL\n");

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
					  SDL_OPENGL | SDL_DOUBLEBUF |
					  SDL_FULLSCREEN);

	if (modes && modes != (SDL_Rect **)-1)
	{
		for (; *modes; ++modes)
		{
			i_scrmode_t test_mode = {
			  (*modes)->w, (*modes)->h,
			  info->vfmt->BitsPerPixel, false
			};

			V_AddAvailableResolution(&test_mode);
		}
	}

	// -ACB- 2000/03/16 Test for possible windowed resolutions
	for (int i = 0; posswinmode[i].width != -1; i++)
	{
		i_scrmode_t *mode = posswinmode + i;

		int got_depth = SDL_VideoModeOK(mode->width, mode->height,
				mode->depth, SDL_OPENGL | SDL_DOUBLEBUF |
				(mode->windowed ? 0 : SDL_FULLSCREEN));

		int token = (got_depth * 100 + mode->depth);

		if (got_depth == mode->depth ||
			token == 1516 || token == 1615 ||
			token == 2432 || token == 3224)
		{
			V_AddAvailableResolution(mode);
		}
	}
}

//
// I_SetScreenSize
//
bool I_SetScreenSize(i_scrmode_t *mode)
{
	if (mode->depth < 15)
		return false;

	my_vis = SDL_SetVideoMode(mode->width, mode->height, mode->depth, 
					SDL_OPENGL | SDL_DOUBLEBUF |
					(mode->windowed ? 0 : SDL_FULLSCREEN));

	if (my_vis == NULL)
		return false;

	if (my_vis->format->BytesPerPixel <= 1)
		return false;

	VideoModeCommonStuff();
	SDL_GL_SwapBuffers();
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
	SDL_GL_SwapBuffers();
}

//
// I_PutTitle
//
void I_PutTitle(const char *title)
{
	SDL_WM_SetCaption(title, title);
}
//
// I_RemoveGrab()
//
void I_RemoveGrab(void)
{
	if (my_vis && ! graphics_shutdown)
	{
		SDL_ShowCursor(1);
		SDL_WM_GrabInput(SDL_GRAB_OFF);
	}
}

//
// I_ShutdownGraphics
//
void I_ShutdownGraphics(void)
{
	if (graphics_shutdown)
		return;

	graphics_shutdown = 1;

	if (SDL_WasInit(SDL_INIT_EVERYTHING))
		SDL_Quit ();
}
