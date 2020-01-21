//----------------------------------------------------------------------------
//  EDGE SDL Timer
//----------------------------------------------------------------------------
//
//  Copyright (c) 1999-2019  The EDGE Team.
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
//
// CA: Moved out of i_ctrl.cc

#include "i_defs.h"
#include "i_sdlinc.h"
#include "../../ddf/main.h"

static Uint32 basetime = 0;

void I_ResetBaseTime(void)
{
    basetime = 0;
}


int I_GetTime(void)
{
    Uint32 t = SDL_GetTicks();

    // more complex than "t*35/1000" to give more accuracy
    //return (t / 1000) * 35 + (t % 1000) * 35 / 1000;

    return t*(35.0f/1000.0f);
}

int  I_GetTime2(void)
{
    Uint32 ticks;

    ticks = SDL_GetTicks();

    if (basetime == 0)
        basetime = ticks;

    ticks -= basetime;

    return (ticks * TICRATE) / 1000;
}

int I_GetTimeCON(void)
{
    Uint32 t = SDL_GetTicks();
    // more complex than "t*35/1000" to give more accuracy
    return (t / 1000) * 35 + (t % 1000) * 35 / 1000;
}


//
// Same as I_GetTime, but returns time in milliseconds
//
int I_GetMillies(void)
{
    return SDL_GetTicks();
}

//
// Same as I_GetTime, but returns time in milliseconds (more precision)
//

int I_GetTimeMS(void)
{
    Uint32 ticks;

    ticks = SDL_GetTicks();

    if (basetime == 0)
        basetime = ticks;

    return ticks - basetime;
}

// Sleep for a specified number of ms

//void I_Sleep(int ms)
//{
//    SDL_Delay(ms);
//}

//void I_WaitVBL(int count)
//{
//    I_Sleep((count * 1000) / 70);
//}

static unsigned int start_displaytime;
static unsigned int displaytime;

static unsigned int rendertic_start;
unsigned int        rendertic_step;
static unsigned int rendertic_next;
float               rendertic_msec;

const float realtic_clock_rate = 100.0f;

//
// I_setMSec
//
// Module private; set the milliseconds per render frame.
//
static void I_setMSec(void)
{
    rendertic_msec = realtic_clock_rate * TICRATE / 100000.0f;
}

#define eclamp(a, min, max) (a < min ? min : (a > max ? max : a))

//
// I_TimerGetFrac
//
// Calculate the fractional multiplier for interpolating the current frame.
//
// TODO: Recompute properly from Frac -> Float
//u32_t I_TimerGetFrac(void)
//{
//    ;;
//}

//
// I_TimerStartDisplay
//
// Calculate the starting display time.
//
void I_TimerStartDisplay(void)
{
    start_displaytime = SDL_GetTicks();
}

//
// I_TimerEndDisplay
//
// Calculate the ending display time.
//
void I_TimerEndDisplay(void)
{
    displaytime = SDL_GetTicks() - start_displaytime;
}

//
// I_TimerSaveMS
//
// Update interpolation state variables at the end of gamesim logic.
//
void I_TimerSaveMS(void)
{
    rendertic_start = SDL_GetTicks();
    rendertic_next = (unsigned int)((rendertic_start * rendertic_msec + 1.0f) / rendertic_msec);
    rendertic_step = rendertic_next - rendertic_start;
}