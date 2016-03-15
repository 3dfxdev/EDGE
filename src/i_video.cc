//----------------------------------------------------------------------------
//  EDGE2 SDL Video Code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2016 Isotope SoftWorks.
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

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "i_defs.h"
#include "i_sdlinc.h"
#include "SDL_opengl.h" /// <--- replacing GLEW with SDL_opengl! 
///#include "i_defs_gl.h" <--- GLEW specific stuff which we are avoiding now.

#include "e_event.h"
#include "m_argv.h"
#include "m_misc.h"
#include "r_modes.h"
#include "i_video.h"

#ifdef _WIN32
///include "i_xinput.h" <--- Not Yet. . . . . . . . . . 
#endif

static bool grab_state;

//SDL_Surface *my_vis;
SDL_Window      *window;
SDL_GLContext   glContext;

int graphics_shutdown = 0;

/// extern cvar_c r_width, r_height, r_depth, r_fullscreen;
cvar_c in_grab;
cvar_c v_msensitivityx;//, 5);
cvar_c v_msensitivityy;//, 5);
cvar_c v_macceleration;//, 0);
cvar_c v_mlook;//, 0);
cvar_c v_mlookinvert;//, 0);
cvar_c v_yaxismove;//, 0);
extern cvar_c r_width;//, 640);
extern cvar_c r_height;//, 480);
cvar_c v_windowed;//, 1);
cvar_c r_vsync;//, 1);
extern cvar_c r_depth;///v_depthsize;//, 24);
cvar_c v_buffersize;//, 32);

cvar_c m_menumouse;

//================================================================================
// Video
//================================================================================

SDL_Surface *screen;
int video_width;
int video_height;
float video_ratio;
/* bool window_focused;
bool window_mouse;

int mouse_x = 0;
int mouse_y = 0; */

//
// I_InitScreen
//


///Con_cvarsetvalue = CON_SetVar 
///src/i_video.cc:88:39: error: 'class cvar_c' has no member named 'value'
///     						  InWindow        = (int)v_windowed.value;
/// .value should be .d

/// v_width, v_height, v_depth
/// void I_StartupGraphics(void)
void I_InitScreen(void)
{
    int     newwidth;
    int     newheight;
    int     p;
    uint32_t  flags = 0;
    char    title[256];

    InWindow        = (int)v_windowed.d;
    video_width     = (int)r_width.d;
    video_height    = (int)r_height.d;
    video_ratio     = (float)video_width / (float)video_height;

    if(M_CheckParm("-window")) {
        InWindow = true;
    }
    if(M_CheckParm("-fullscreen")) {
        InWindow = false;
    }

    newwidth = newheight = 0;

/*     p = M_CheckParm("-width");
    if(p && p < myargc - 1) {
        newwidth = datoi(myargv[p+1]);
    }

    p = M_CheckParm("-height");
    if(p && p < myargc - 1) {
        newheight = datoi(myargv[p+1]);
    } */

    if(newwidth && newheight) {
        video_width = newwidth;
        video_height = newheight;
		///float fov = CLAMP(5, r_fov.f, 175);
		video_width = r_width.f;
        //CON_SetVar(&r_width.f, (float)video_width);
		video_height = r_height.f;
        //CON_SetVar(r_height.f, (float)video_height);
    }

    if( r_depth.d != 8 &&
        r_depth.d != 16 &&
        r_depth.d != 24) 
		{
            r_depth.f = 24;
		}

    if( v_buffersize.d != 8 &&
        v_buffersize.d != 16 &&
        v_buffersize.d != 24 &&
        v_buffersize.d != 32) {
            v_buffersize.f = 32;
    }

    ///usingGL = false;

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_RED_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_GREEN_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_BLUE_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_ALPHA_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, (int)v_buffersize.d);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, (int)r_depth.d);
    SDL_GL_SetSwapInterval((int)r_vsync.d); /// v_vsync replaced with r_vsync! 

    flags |= SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_MOUSE_FOCUS;

    if(!InWindow) {
        flags |= SDL_WINDOW_FULLSCREEN;
    }

    sprintf(title, "3DGE");
    window = SDL_CreateWindow(title,
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              video_width,
                              video_height,
                              flags);

    if(window == NULL) {
        I_Error("I_InitScreen: Failed to create window");
        return;
    }

    if((glContext = SDL_GL_CreateContext(window)) == NULL) {
        // re-adjust depth size if video can't run it
        if(r_depth.d >= 24) 
		{
            r_depth.f = 16;
        }
        else if(r_depth.d >= 16) 
		{
            r_depth = 8;
        }

        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, (int)r_depth.d);

        if((glContext = SDL_GL_CreateContext(window)) == NULL) {
            // fall back to lower buffer setting
            r_depth = 16;
            SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, (int)r_depth.d);

            if((glContext = SDL_GL_CreateContext(window)) == NULL) {
                // give up
                I_Error("I_InitScreen: Failed to create OpenGL context");
                return;
            }
        }
    }
}

//
// I_ShutdownWait
//

int I_ShutdownWait(void) 
{
    static SDL_Event event;

    while(SDL_PollEvent(&event)) 
	{
        if(event.type == SDL_QUIT ||
            (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) 
		{
            I_ShutdownVideo();
            return 1;
        }
    }

    return 0;
}

//
// I_ShutdownVideo
//
/// void I_ShutdownGraphics(void)
void I_ShutdownVideo(void) 
{
    if(glContext) 
	{
        SDL_GL_DeleteContext(glContext);
        glContext = NULL;
    }

    if(window) 
	{
        SDL_DestroyWindow(window);
        window = NULL;
    }

    SDL_Quit();
}

//
// I_InitVideo
//

void I_InitVideo(void) 
{
    uint32_t f = SDL_INIT_VIDEO;

#ifdef _DEBUG
    f |= SDL_INIT_NOPARACHUTE;
#endif

    if(SDL_Init(f) < 0) {
        I_Error("ERROR - Failed to initialize SDL");
        return;
    }

    SDL_ShowCursor(0);
    ///I_InitInputs();
	I_ControlGetEvents();
    I_InitScreen();
}

//
// I_StartTic
//
	/// This is called in E_Main! 
/* void I_StartTic(void) 
{
    SDL_Event event;

    while(SDL_PollEvent(&Event)) 
	{
       E_PostEvent(&event);
    }

#ifdef _USE_XINPUT
    I_XInputPollEvent();
#endif

    I_ReadMouse();
} */

void I_UpdateGrab(void) 
{
	bool enable;
	grab_state = enable;

	if (grab_state && in_grab.d)
	{
		SDL_SetRelativeMouseMode(SDL_TRUE);
	}
	else
	{
		SDL_SetRelativeMouseMode(SDL_FALSE);
	}
}

//
// I_FinishUpdate
//

void I_FinishUpdate(void) {
    I_UpdateGrab();
    SDL_GL_SwapWindow(window);

    ///BusyDisk = false;
}

void I_GetDesktopSize(int *width, int *height)
{
	*width  = video_width;
	*height = video_height;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
