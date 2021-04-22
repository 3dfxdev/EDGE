 //----------------------------------------------------------------------------
//  EDGE SDL Video Code
//----------------------------------------------------------------------------
//
//  Copyright (c) 2016  Isotope SoftWorks.
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
#include "i_defs_gl.h"

#include "i_sdlinc.h"

#include <signal.h>

#include "../hu_draw.h"
#include "../m_argv.h"
#include "../m_misc.h"
#include "../r_modes.h"
#include "../r_renderbuffers.h"

SDL_version compiled;
SDL_version linked;

extern int r_vsync, r_anisotropy;

//The window we'll be rendering to
SDL_Window *my_vis;
//Renderer for window - used by cinematic
SDL_Renderer *my_rndrr;

int graphics_shutdown = 0;

DEF_CVAR(in_grab, int, "c", 1);

DEF_CVAR(r_swapinterval, int, "", 1);

static bool grab_state;

static int display_W, display_H;

SDL_GLContext   glContext;

float gamma_settings = 0.0f;
float fade_gamma;
float fade_gdelta;
extern int fade_starttic;
extern bool fade_active;

// Possible Windowed Modes
static struct { int w, h; } possible_modes[] =
{
	{  320, 200, },
	{  320, 240, },
	{  400, 300, },
	{  512, 384, },
	{  640, 400, },
	{  640, 480, },
	{  800, 600, },
	{ 1024, 640, },
	{ 1024, 768, },
	{ 1280, 720, },
	{ 1280, 960, },
	{ 1440, 900, },
	{ 1440,1080, },
	{ 1600,1000, },
	{ 1600,1200, },
	{ 1680,1050, },
	{ 1920,1080, },
	{ 1920,1200, },

	{  -1,  -1, }
};


void I_GrabCursor(bool enable)
{
	if (! my_vis || graphics_shutdown)
		return;

	grab_state = enable;

	if (grab_state && in_grab)
	{
		SDL_ShowCursor(SDL_FALSE);
		SDL_SetRelativeMouseMode(SDL_TRUE);
		SDL_SetWindowGrab(my_vis, SDL_TRUE);
	}
	else
	{
		SDL_SetRelativeMouseMode(SDL_FALSE);
		SDL_SetWindowGrab(my_vis, SDL_FALSE);
		SDL_ShowCursor(SDL_FALSE);
	}
}


void I_StartupGraphics(void)
{

	uint32_t  flags = 0;
    char    title[256];

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

	if (stricmp(driver, "default") != 0)
	{
		char nameBuffer[200];
		char valueBuffer[200];
		bool overWrite = true;
		snprintf(nameBuffer, sizeof(nameBuffer), "SDL_VIDEODRIVER");
		snprintf(valueBuffer, sizeof(valueBuffer), "%s", driver);
		SDL_setenv(nameBuffer, valueBuffer, overWrite);
	}

	//I_Printf("SDL_Video_Driver: %s\n", driver);


	if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0)
		I_Error("Couldn't init SDL!\n%s\n", SDL_GetError());

	if (M_CheckParm("-nograb"))
		in_grab = 0;

#if 0
	// anti-aliasing
	if (r_anisotropy > 1)
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 4);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, r_anisotropy);
	}
	else
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
	}

#endif // 0

	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 1 );
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE,     8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,   8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,    8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE,    8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,   16);
	// ~CA 5.7.2016:

	flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_INPUT_FOCUS;

	display_W = SCREENWIDTH;
	display_H = SCREENHEIGHT;



	sprintf(title, "EDGE");
    my_vis = SDL_CreateWindow(title,
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              display_W,
                              display_H,
                              flags);

	my_rndrr = SDL_CreateRenderer(my_vis, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	glContext = SDL_GL_CreateContext( my_vis );

    SDL_GL_MakeCurrent( my_vis, glContext );
    if(my_vis == NULL)
	{
        I_Error("I_InitScreen: Failed to create window");
        return;
    }

	if (r_vsync == 1 && r_swapinterval == 0)
		SDL_GL_SetSwapInterval(1);		// SDL-based standard
	else if (r_vsync == 1 && r_swapinterval == 1)
		SDL_GL_SetSwapInterval(-1);


	static bool first = true;

	if (first)
	{
		if (ogl_LoadFunctions() == ogl_LOAD_FAILED)
		{
			I_Error("Failed to load OpenGL functions.");
			return;
		}
	}


	// add fullscreen modes
	int nummodes = SDL_GetNumDisplayModes(0); // for now just assume display #0

	for (int i=0; i<nummodes; i++)
	{
		SDL_DisplayMode mode;

		memset(&mode, 0, sizeof(mode));

		if (SDL_GetDisplayMode(0, i, &mode) == 0)
		{
			scrmode_c scr_mode;
			scr_mode.width = mode.w;
			scr_mode.height = mode.h;
			scr_mode.full = true;

			if ((scr_mode.width & 15) != 0)
				continue;

            R_AddResolution(&scr_mode);
		}
	}

	// add windowed modes
	SDL_DisplayMode mode;
	if (SDL_GetDesktopDisplayMode(0, &mode) == 0)
	{
		for (int i=0; possible_modes[i].w != -1; i++)
		{
			scrmode_c scr_mode;
			scr_mode.width = possible_modes[i].w;
			scr_mode.height = possible_modes[i].h;
			scr_mode.full = false;

			if (scr_mode.width <= mode.w && scr_mode.height <= mode.h)
				R_AddResolution(&scr_mode);
		}
	}

	I_Printf("I_StartupGraphics: initialisation OK\n");

	I_Printf("Desktop resolution: %dx%d\n", mode.w ? mode.w : display_W, mode.h ? mode.h : display_H);

	SDL_VERSION(&compiled);
	SDL_GetVersion(&linked);
	I_Printf("==============================================================================\n");
	I_Printf("Getting SDL2 Version Information...\n");
	I_Printf("EDGE compiled against SDL version %d.%d.%d ...\n",
		compiled.major, compiled.minor, compiled.patch);
	I_Printf("But EDGE is linking against SDL version %d.%d.%d.\n",
		linked.major, linked.minor, linked.patch);
	I_Printf("==============================================================================\n");
}


bool I_SetScreenSize(scrmode_c *mode)
{
 	I_Printf("I_SetScreenSize: trying %dx%d (%s)\n",
 			 mode->width, mode->height,
 			 mode->full ? "fullscreen" : "windowed");

 	// -AJA- turn off cursor -- BIG performance increase.
 	//       Plus, the combination of no-cursor + grab gives
 	//       continuous relative mouse motion.

 	// ~CA~  TODO:  Eventually we will want to turn on the cursor
 	//				when we get Doom64-style mouse control for
 	//				the options drawer.
 	I_GrabCursor(false);

// 	    // reset gamma to default
//         I_SetGamma(1.0f);

	SDL_DisplayMode dm;
	memset(&dm, 0, sizeof(dm));
	dm.format = SDL_PIXELFORMAT_RGBA8888; // TODO: set proper pixel format
	dm.w = mode->width;
	dm.h = mode->height;

	if(SDL_SetWindowDisplayMode(my_vis, &dm) != 0) {
        I_Printf("I_SetScreenSize: failed to set video mode: %s\n", SDL_GetError());
        return false;
	}
	SDL_SetWindowFullscreen(my_vis, mode->full ? SDL_WINDOW_FULLSCREEN : 0);
    if(!mode->full) {
        SDL_SetWindowSize(my_vis, mode->width, mode->height);
        SDL_SetWindowPosition(my_vis, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    }

    if (r_vsync == 1 && r_swapinterval == 0)
        SDL_GL_SetSwapInterval(1);		// SDL-based standard
    else if (r_vsync == 1 && r_swapinterval == 1)
        SDL_GL_SetSwapInterval(-1);

	I_GrabCursor(false);

	//HUD_Reset();
	return true;
}


void I_StartFrame(void)
{
	// CA 11/17/19:
	// This wasn't needed here except for the letterboxing (this is mostly for image "borders"), so we need a better method. For now disabling this helps rendering overall.
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
}


void I_FinishFrame(void)
{

#ifdef GL_BRIGHTNESS
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBegin(GL_QUADS);
	glColor4f(fade_active ? 0.0f : 1.0f,
		fade_active ? 0.0f : 1.0f,
		fade_active ? 0.0f : 1.0f,
		fade_active ? 1.0f - fade_gamma : gamma_settings);
	glVertex3i(0, 0, 0);
	glVertex3i(SCREENWIDTH, 0, 0);
	glVertex3i(SCREENWIDTH, SCREENHEIGHT, 0);
	glVertex3i(0, SCREENHEIGHT, 0);
	glEnd();
	glColor4f(1, 1, 1, 1);
#endif


	if (r_vsync > 0)
		glFinish();

	/* 	Some systems allow specifying -1 for the interval,
	to enable late swap tearing. Late swap tearing works
	the same as vsync, but if you've already missed the
	vertical retrace for a given frame, it swaps buffers
	immediately, which might be less jarring for the user
	during occasional framerate drops. If application
	requests late swap tearing and the system does not support
	it, this function will fail and return -1. In such a case,
	you should probably retry the call with 1 for the interval. */


	SDL_GL_SwapWindow(my_vis);

	if (in_grab_cv_.CheckModified())
		I_GrabCursor(grab_state);
}

void I_PutTitle(const char *title)
{
	SDL_SetWindowTitle(my_vis, title);
}

void I_SetGamma(float gamma)
{
	#ifdef GL_BRIGHTNESS
	gamma_settings = (gamma - 1.0f) * 0.08f;
	#else
	if (SDL_SetWindowBrightness(my_vis, gamma) < 0)
		I_Printf("Failed to change gamma.\n");
	#endif
}


void I_ShutdownGraphics(void)
{
	if (graphics_shutdown)
		return;

	graphics_shutdown = 1;

	if (SDL_WasInit(SDL_INIT_EVERYTHING))
	{
        // reset gamma to default
        I_SetGamma(1.0f);

		if (glContext)
			SDL_GL_DeleteContext(glContext);
		if (my_rndrr)
			SDL_DestroyRenderer(my_rndrr);
		if (my_vis)
			SDL_DestroyWindow(my_vis);

		SDL_Quit ();
	}
}


void I_GetDesktopSize(int *width, int *height)
{
	SDL_DisplayMode mode;
	if (SDL_GetDesktopDisplayMode(0, &mode) != 0)
	{
		// error - just return the current width/height
		*width  = display_W;
		*height = display_H;
	}

	*width = mode.w; //TODO: V519 https://www.viva64.com/en/w/v519/ The '* width' variable is assigned values twice successively. Perhaps this is a mistake. Check lines: 447, 451.
	*height = mode.h; //TODO: V519 https://www.viva64.com/en/w/v519/ The '* height' variable is assigned values twice successively. Perhaps this is a mistake. Check lines: 448, 452.

}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
