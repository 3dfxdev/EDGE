 //----------------------------------------------------------------------------
//  EDGE2 SDL Video Code
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
#include "i_sdlinc.h"
#include "i_defs_gl.h"

#ifdef WIN32
#include <GL/wglew.h>
/// If you don't hardlink under Win32, replace with ---> ^___________________^
#else
#include <GL/glew.h>
#endif


#include <SDL2/SDL_opengl.h>



#include <signal.h>

#include "m_argv.h"
#include "m_misc.h"
#include "r_modes.h"

extern cvar_c r_width, r_height, r_depth, r_fullscreen, r_vsync;

//The window we'll be rendering to
SDL_Window *my_vis;

int graphics_shutdown = 0;

cvar_c in_grab;

static bool grab_state;

static int display_W, display_H;

SDL_GLContext   glContext;

#ifdef LINUX
static float gamma_settings = 0.0f;
#endif

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
	{ 1920,1080, },
	{ 1920,1200, },

	{  -1,  -1, }
};


void I_GrabCursor(bool enable)
{
	if (! my_vis || graphics_shutdown)
		return;

	grab_state = enable;

	if (grab_state && in_grab.d)
	{
		SDL_ShowCursor(0);
//		SDL_WM_GrabInput(SDL_GRAB_ON);
		SDL_SetWindowGrab(my_vis, SDL_TRUE); //TODO: grab which window??
	}
	else
	{
		SDL_ShowCursor(1);
//		SDL_WM_GrabInput(SDL_GRAB_OFF);
		SDL_SetWindowGrab(my_vis, SDL_FALSE); //TODO: grab which window??
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

	I_Printf("SDL_Video_Driver: %s\n", driver);


	if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0)
		I_Error("Couldn't init SDL VIDEO!\n%s\n", SDL_GetError());

	if (M_CheckParm("-nograb"))
		in_grab = 0;

#ifdef OPENGL_2
SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 2 );
SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 1 );
#endif
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE,     8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,   8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,    8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE,    8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,   16);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	// ~CA 5.7.2016:

	flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_MOUSE_FOCUS;

	display_W = SCREENWIDTH;
	display_H = SCREENHEIGHT;

	sprintf(title, "3DGE");
    my_vis = SDL_CreateWindow(title,
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              display_W,
                              display_H,
                              flags);

	glContext = SDL_GL_CreateContext( my_vis );
    SDL_GL_MakeCurrent( my_vis, glContext );
    if(my_vis == NULL)
	{
        I_Error("I_InitScreen: Failed to create window");
        return;
    }

/* 	Some systems allow specifying -1 for the interval,
	to enable late swap tearing. Late swap tearing works
	the same as vsync, but if you've already missed the
	vertical retrace for a given frame, it swaps buffers
	immediately, which might be less jarring for the user
	during occasional framerate drops. If application
	requests late swap tearing and the system does not support
	it, this function will fail and return -1. In such a case,
	you should probably retry the call with 1 for the interval. */

	if (r_vsync.d == 1)
		SDL_GL_SetSwapInterval(1);
	else
		SDL_GL_SetSwapInterval(-1);

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
			scr_mode.depth = SDL_BITSPERPIXEL(mode.format);
			scr_mode.full = true;

			if ((scr_mode.width & 15) != 0)
				continue;

			if (scr_mode.depth == 15 || scr_mode.depth == 16 ||
				scr_mode.depth == 24 || scr_mode.depth == 32)
			{
				R_AddResolution(&scr_mode);
			}
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
			scr_mode.depth = SDL_BITSPERPIXEL(mode.format);
			scr_mode.full = false;

			if (scr_mode.width <= mode.w && scr_mode.height <= mode.h)
				R_AddResolution(&scr_mode);
		}
	}

	I_Printf("I_StartupGraphics: initialisation OK\n");

	I_Printf("Desktop resolution: %dx%d\n", mode.w ? mode.w : display_W, mode.h ? mode.h : display_H);
}


bool I_SetScreenSize(scrmode_c *mode)
{
	///I_Printf("I_SetScreenSize = reached");
	I_GrabCursor(false);

	SDL_GL_DeleteContext(glContext);
	SDL_DestroyWindow(my_vis);

	I_Printf("I_SetScreenSize: trying %dx%d %dbpp (%s)\n",
			 mode->width, mode->height, mode->depth,
			 mode->full ? "fullscreen" : "windowed");

	my_vis = SDL_CreateWindow("Hyper3DGE",
					SDL_WINDOWPOS_UNDEFINED,
                    SDL_WINDOWPOS_UNDEFINED,
                    mode->width, mode->height,
                    SDL_WINDOW_OPENGL | //SDL2 is double-buffered by default
                    (mode->full ? SDL_WINDOW_FULLSCREEN :0));
	glContext = SDL_GL_CreateContext( my_vis );
    SDL_GL_MakeCurrent( my_vis, glContext );
	if (my_vis == NULL)
	{
		I_Printf("I_SetScreenSize: (mode not possible)\n");
		return false;
	}

	if (r_vsync.d == 1)
		SDL_GL_SetSwapInterval(1);
	else
		SDL_GL_SetSwapInterval(-1);

	// -AJA- turn off cursor -- BIG performance increase.
	//       Plus, the combination of no-cursor + grab gives
	//       continuous relative mouse motion.

	// ~CA~  TODO:  Eventually we will want to turn on the cursor
	//				when we get Doom64-style mouse control for
	//				the options drawer.
	I_GrabCursor(true);

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);

#ifdef MACOSX
	//TODO: On Mac OS X make sure to bind 0 to the draw framebuffer before swapping the window, otherwise nothing will happen.
#endif
	SDL_GL_SwapWindow(my_vis);

	return true;
}


void I_StartFrame(void)
{
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
}


void I_FinishFrame(void)
{
	extern cvar_c r_vsync;

	//FIXME: WIN32 relies on WGLEW, so when we go to GLAD, make sure to generate a WGL_GLAD header to compensate.
	//       I wonder if SDL_GL_SwapWindow will work under Win32 without WGLEW extensions. hmmm.
	#ifdef WIN32
	if (WGLEW_EXT_swap_control)
	{
		if (r_vsync.d > 0)
			glFinish();
		wglSwapIntervalEXT(r_vsync.d != 0);
	}
	#endif
	
	#ifdef LINUX
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBegin(GL_QUADS);
	glColor4f(255.0f,
			  255.0f, 
			  255.0f,
			 gamma_settings);
	glVertex3i(0,0,0);
	glVertex3i(display_W,0,0);
	glVertex3i(display_W,display_H,0);
	glVertex3i(0,display_H,0);
	glEnd();
	glColor4f(1, 1, 1, 1);
	#endif

	SDL_GL_SwapWindow(my_vis);
	if (r_vsync.d > 0)
			glFinish();
	if (in_grab.CheckModified())
		I_GrabCursor(grab_state);
}


void I_PutTitle(const char *title)
{
	SDL_SetWindowTitle(my_vis, title);
}

void I_SetGamma(float gamma)
{
	#ifdef LINUX
		gamma_settings = 0.05f * gamma;
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

	*width = mode.w;
	*height = mode.h;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
