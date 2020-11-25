//----------------------------------------------------------------------------
//  EDGE Cinematic Playback Engine (ROQ)
//----------------------------------------------------------------------------
//
//  Copyright (c) 2018-2020 The EDGE Team
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

#ifdef USE_FFMPEG
#include <epi/epi.h>
#include <epi/bytearray.h>
#include <epi/file.h>
#include <epi/filesystem.h>
#include <epi/math_oddity.h>
#include <epi/endianess.h>

#include "i_defs.h"
#include "i_defs_gl.h"
#include "i_sdlinc.h"
#include "i_system.h"
#include "i_ffmpeg.h"
#include "z_zone.h"

#include "s_blit.h"
#include "s_sound.h"

#include "m_misc.h"

#include <signal.h>

#include "con_main.h"
#include "e_input.h"
#include "m_argv.h"
#include "m_menu.h"
#include "m_misc.h"
#include "r_modes.h"

#include "dm_state.h"
#include "w_wad.h"
#include "e_main.h"
#include "r_gldefs.h"
#include "r_image.h"


//#define DEBUG_MOVIE_PLAYER
#define HSS(u) ((float)(u) / (float)(ATLAS_WIDTH))
#define VSS(v) ((float)(v) / (float)(ATLAS_HEIGHT))
#define HTS(x) ((x) * (SCREENWIDTH) / (ATLAS_WIDTH))
#define VTS(y) ((y) * (SCREENHEIGHT) / (ATLAS_HEIGHT))


static cinematic_t cin_cinematics[MAX_CINEMATICS];

static bool cin_init = false;

bool playing_movie = false;

extern SDL_Window *my_vis;
extern SDL_Renderer *my_rndrr;
extern SDL_GLContext glContext;
extern int r_vsync;

typedef int cinHandle_t;
cinHandle_t cinematicHandle;

#define DEBUG_MOVIE_PLAYER 1

typedef enum
{
	CIN_SYSTEM = BIT(0),
	CIN_LOOPING = BIT(1),
	CIN_SILENT = BIT(2)
} cinFlags_t;

// Plays a cinematic
cinHandle_t     CIN_PlayCinematic(const char *name, int flags);

// Runs a cinematic frame
void            CIN_UpdateCinematic(cinHandle_t handle);

// Update audio stream from cinematic buffer
int             CIN_UpdateAudio(cinHandle_t handle, int need, SDL_AudioDeviceID audio_dev);

// Resets a cinematic
void            CIN_ResetCinematic(cinHandle_t handle);

// Stops a cinematic
void            CIN_StopCinematic(cinHandle_t handle);

// Checks if cinematic playing - handles looping
bool            CIN_CheckCinematic(cinHandle_t handle);

// Initializes the cinematic module
void            CIN_Init(void);

// Shuts down the cinematic module
void            CIN_Shutdown(void);

void            E_PlayMovie(const char *name, int flags);



// ============================================================================

extern "C" int ReadFunc(void *ptr, unsigned char *buf, int len);

int ReadFunc(void *ptr, unsigned char *buf, int len)
{
    cinematic_t *cin = reinterpret_cast<cinematic_t*>(ptr);
    epi::file_c *F = cin->file;

    if (F->GetPosition() == F->GetLength())
        return -1;

    return F->Read(buf, len);
}

extern "C" int64_t SeekFunc(void* ptr, int64_t pos, int whence);

int64_t SeekFunc(void* ptr, int64_t pos, int whence)
{
    cinematic_t *cin = reinterpret_cast<cinematic_t*>(ptr);
    epi::file_c *F = cin->file;

    if (F->Seek(pos, whence))
        return -1;

    return F->GetPosition();
}

// ============================================================================

/*
 ==================
 CIN_HandleForCinematic

 Finds a free cinHandle_t
 ==================
*/
static cinematic_t *CIN_HandleForCinematic (cinHandle_t *handle)
{

    cinematic_t *cin;
    int         i;

    for (i = 0, cin = cin_cinematics; i < MAX_CINEMATICS; i++, cin++)
    {
        if (!cin->playing)
            break;
    }

    if (i == MAX_CINEMATICS)
        I_Error("CIN_HandleForCinematic: none free");

    *handle = i + 1;

    memset(cin, 0, sizeof(cinematic_t));
    return cin;
}

/*
 ==================
 CIN_GetCinematicByHandle

 Returns a cinematic_t for the given cinHandle_t
 ==================
*/
static cinematic_t *CIN_GetCinematicByHandle (cinHandle_t handle)
{

    cinematic_t *cin;

    if (handle <= 0 || handle > MAX_CINEMATICS)
        I_Error("CIN_GetCinematicByHandle: handle out of range");

    cin = &cin_cinematics[handle - 1];

    if (!cin->playing)
        I_Error("CIN_GetCinematicByHandle: invalid handle");

    return cin;
}

/*
 ==================
 CIN_PlayCinematic

 Setup kitchensink player for a given video
 Returns a cinHandle_t
 ==================
*/
cinHandle_t CIN_PlayCinematic (const char *name, int flags)
{
    cinematic_t        *cin;
    cinHandle_t        handle;
    int                i;
    epi::file_c        *F;

    if (name)
    {
        // See if already playing
        for (i = 0, cin = cin_cinematics; i < MAX_CINEMATICS; i++, cin++)
        {
            if (!cin->playing)
                continue;

            if (!strncmp(cin->name, name, PLAYER_NAME_MAX_LEN))
            {
                if (cin->flags != flags)
                    continue;
                // Yes, return existing handle
                return i + 1;
            }
        }
        // No, get a new handle
        cin = CIN_HandleForCinematic(&handle);
        if (!cin)
        {
            I_Printf("CIN_PlayCinematic: Couldn't get a cinematic handle!\n");
            return -1;
        }

        std::string fn = M_ComposeFileName(game_dir.c_str(), name);
        F = epi::FS_Open(fn.c_str(), epi::file_c::ACCESS_READ |
            epi::file_c::ACCESS_BINARY);

        if (F)
        {
            I_Printf("CIN_PlayCinematic: Opened movie file '%s'\n", fn.c_str());
        }
        else
        {
            // not a file, try a lump
            int lump = W_FindLumpFromPath(name);
            if (lump < 0)
            {
                M_WarnError("CIN_PlayCinematic: Missing file or lump: %s\n", name);
                return -1;
            }

            F = W_OpenLump(lump);
            if (!F)
            {
                M_WarnError("CIN_PlayCinematic: Can't open lump: %s\n", name);
                return -1;
            }
            I_Printf("CIN_PlayCinematic: Opened movie lump '%s'\n", name);
        }
        cin->file = F;

        // open the source file
        cin->src = Kit_CreateSourceFromCustom(ReadFunc, SeekFunc, (void*)cin);
        if (cin->src)
        {
            I_Printf("CIN_PlayCinematic: Created movie source\n");
        }
        else
        {
            M_WarnError("CIN_PlayCinematic: Cannot create movie source for %s\n", name);
            delete F;
            return -1;
        }

        // print stream types
        I_Printf("  Source streams:\n");
        for(i = 0; i < Kit_GetSourceStreamCount(cin->src); i++)
            if (!Kit_GetSourceStreamInfo(cin->src, &cin->sinfo, i))
                I_Printf("   * Stream #%d: %s\n", i, Kit_GetKitStreamTypeString(cin->sinfo.type));

        // create the player
        cin->player = Kit_CreatePlayer(
            cin->src,
            Kit_GetBestSourceStream(cin->src, KIT_STREAMTYPE_VIDEO),
            Kit_GetBestSourceStream(cin->src, KIT_STREAMTYPE_AUDIO),
            Kit_GetBestSourceStream(cin->src, KIT_STREAMTYPE_SUBTITLE),
            SCREENWIDTH, SCREENHEIGHT); //~CA: before 1280x720...? Probably needs to be changed for screen vars?

        if (cin->player == NULL)
        {
            M_WarnError("CIN_PlayCinematic: Couldn't create kitchensink player!\n");
            Kit_CloseSource(cin->src);
            delete F;
            return -1;
        }

        // get player info
        Kit_GetPlayerInfo(cin->player, &cin->pinfo);
        //if (!cin->pinfo.video.is_enabled)
        if (Kit_GetPlayerVideoStream(cin->player) == -1)
        {
            M_WarnError("CIN_PlayCinematic: File contains no video!\n");
            Kit_ClosePlayer(cin->player);
            Kit_CloseSource(cin->src);
            delete F;
            return -1;
        }
        I_Printf("  Media information:\n");
        if (Kit_GetPlayerVideoStream(cin->player) >= 0)
        {
            I_Printf("  *Video: % s(% s),thd= %d,res=%dx%d\n",
                cin->pinfo.video.codec.name,
                cin->pinfo.video.codec.description,
                cin->pinfo.video.codec.threads,
                cin->pinfo.video.output.width,
                cin->pinfo.video.output.height);
        }
        //if (cin->pinfo.audio.is_enabled)
        if (Kit_GetPlayerAudioStream(cin->player) >= 0)
        {
            I_Printf("  *Audio: %s (%s), thd=%d, %dHz, %dch, %db, %s\n",
                cin->pinfo.audio.codec.name,
                cin->pinfo.audio.codec.description,
                cin->pinfo.video.codec.threads,
                cin->pinfo.audio.output.samplerate,
                cin->pinfo.audio.output.channels,
                cin->pinfo.audio.output.bytes,
                cin->pinfo.audio.output.is_signed ? "signed" : "unsigned");
        }
        else I_Printf("GetPlayerAudioStream: No Audio detected!\n");
       // if (cin->pinfo.subtitle.is_enabled)
        if (Kit_GetPlayerSubtitleStream(cin->player) >= 0)
        {
            I_Printf("   * Subtitle: %s (%s)\n",
                cin->pinfo.subtitle.codec,
                cin->pinfo.subtitle.codec.name);
        }
        I_Printf("  Duration: %f seconds\n", Kit_GetPlayerDuration(cin->player));


        // set audio specs wanted
        if (Kit_GetPlayerAudioStream(cin->player) >= 0)
        {
            memset(&cin->wanted_spec, 0, sizeof(cin->wanted_spec));
            cin->wanted_spec.freq = cin->pinfo.audio.output.samplerate;
            cin->wanted_spec.format = cin->pinfo.audio.output.format;
            cin->wanted_spec.channels = cin->pinfo.audio.output.channels;
            cin->audiobuf = (unsigned char*)Z_Malloc(AUDIOBUFFER_SIZE);
        }

        //if (cin->pinfo.video.is_enabled)
        if (Kit_GetPlayerVideoStream(cin->player) >= 0)
        {
            I_Printf("  Texture type: %s\n", Kit_GetSDLPixelFormatString(cin->pinfo.video.output.format));
        }
        else 
            Kit_GetBestSourceStream(cin->src, KIT_STREAMTYPE_VIDEO);// , -1);

        //if (cin->pinfo.audio.is_enabled)
        if (Kit_GetPlayerAudioStream(cin->player) >= 0) 
        {
            I_Printf("  Audio format: %s\n", Kit_GetSDLAudioFormatString(cin->pinfo.audio.output.format));
        }
        else
            Kit_GetBestSourceStream(cin->src, KIT_STREAMTYPE_AUDIO);// , -1);

        //if (cin->pinfo.subtitle.is_enabled)
        if (Kit_GetPlayerSubtitleStream(cin->player) >= 0)
        {
            I_Printf("  Subtitle format: %s\n", Kit_GetSDLPixelFormatString(cin->pinfo.subtitle.output.format));
        }
        else
            Kit_GetBestSourceStream(cin->src, KIT_STREAMTYPE_SUBTITLE);//, -1);

        if (Kit_GetPlayerVideoStream(cin->player) >= 0)
        {
            // create video texture; this will be YV12 most of the time.
            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

            I_Printf(" CIN_PlayCinematic: Setting up video_tex\n");
            cin->video_tex = SDL_CreateTexture(
                my_rndrr,
                cin->pinfo.video.output.format,
                SDL_TEXTUREACCESS_STATIC,
                cin->pinfo.video.output.width,
                cin->pinfo.video.output.height);

            if (cin->video_tex == NULL)
            {
                M_WarnError("CIN_PlayCinematic: Couldn't create a video texture\n");
                Kit_ClosePlayer(cin->player);
                Kit_CloseSource(cin->src);
                delete F;
                return -1;
            }

            ///!! setup OGL texture buffer
            glGenTextures(1, &cin->vtex[0]);

            I_Printf(" ffmpeg: Setting up render_tex variable\n");

            cin->render_tex = SDL_CreateTexture(
                my_rndrr,
                SDL_PIXELFORMAT_RGB24,
                SDL_TEXTUREACCESS_TARGET,
                cin->pinfo.video.output.width,
                cin->pinfo.video.output.height);

            if (cin->render_tex == NULL)
            {
                M_WarnError("CIN_PlayCinematic: Couldn't create a render target texture\n");
                SDL_DestroyTexture(cin->video_tex);
                if (cin->vtex[0])
                    glDeleteTextures(1, &cin->vtex[0]);
                Kit_ClosePlayer(cin->player);
                Kit_CloseSource(cin->src);
                delete F;
                return -1;
            }

            cin->flip_tex = SDL_CreateTexture(
                my_rndrr,
                SDL_PIXELFORMAT_RGB24,
                SDL_TEXTUREACCESS_TARGET,
                cin->pinfo.video.output.width,
                cin->pinfo.video.output.height);

            if (cin->flip_tex == NULL)
            {
                M_WarnError("CIN_PlayCinematic: Couldn't create a render flip texture\n");

                SDL_DestroyTexture(cin->video_tex);

                if (cin->vtex[0])
                    glDeleteTextures(1, &cin->vtex[0]);

                SDL_DestroyTexture(cin->render_tex);
                Kit_ClosePlayer(cin->player);
                Kit_CloseSource(cin->src);
                delete F;
                return -1;
            }

            // The subtitle texture atlas contains all the subtitle image fragments.
            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest"); // Always nearest for atlas operations
            cin->subtitle_tex = SDL_CreateTexture(
                my_rndrr,
                cin->pinfo.subtitle.output.format,
                SDL_TEXTUREACCESS_STATIC,
                ATLAS_WIDTH, ATLAS_HEIGHT);

            if (cin->subtitle_tex == NULL)
            {
                M_WarnError("CIN_PlayCinematic: Couldn't create a subtitle texture atlas\n");

                SDL_DestroyTexture(cin->video_tex);

                if (cin->vtex[0])
                    glDeleteTextures(1, &cin->vtex[0]);

                SDL_DestroyTexture(cin->render_tex);
                SDL_DestroyTexture(cin->flip_tex);
                Kit_ClosePlayer(cin->player);
                Kit_CloseSource(cin->src);
                delete F;
                return 1;
            }
            // setup OGL texture buffer
            glGenTextures(1, &cin->stex[0]);

            // set subtitle texture blendmode
            SDL_SetTextureBlendMode(cin->subtitle_tex, SDL_BLENDMODE_BLEND);

        }


        // Fill in the rest
        cin->playing = true;
        cin->file = F;
        strncpy(cin->name, name, PLAYER_NAME_MAX_LEN);
        cin->name[PLAYER_NAME_MAX_LEN-1] = 0;
        cin->flags = flags;
        cin->frameWidth = cin->pinfo.video.output.width;
        cin->frameHeight = cin->pinfo.video.output.height;
        cin->frameCount = 0;

        I_Printf("CIN: Start playback\n");
        Kit_PlayerPlay(cin->player);


        return handle;
    }

    return -1; // didn't pass a name
}

//EDGE Function to play movie
void E_PlayMovie(const char* name, int flags)
{
    cinHandle_t midx;
    cinematic_t *cin;
    SDL_AudioDeviceID audio_dev;
    SDL_AudioSpec audio_spec;
    int afree;

    I_Printf("E_PlayMovie: Attemptimp to play %s\n",name);
    if (flags & CIN_SYSTEM)
        I_Printf("  SYSTEM flag\n");
    if (flags & CIN_LOOPING)
        I_Printf("  LOOPING flag\n");
    if (flags & CIN_SILENT)
        I_Printf("  SILENT flag\n");

    playing_movie = false;

    CIN_Init();

    // try to setup movie using kitchensink
    midx = CIN_PlayCinematic(name, flags);

    if (midx < 0)
    {
        CIN_Shutdown();
        M_WarnError("E_PlayMovie: Could not play movie %s\n", name);
        return;
    }
    cin = CIN_GetCinematicByHandle(midx);

    if (flags & CIN_SYSTEM)
    {
        I_Printf("E_PlayMovie: Playing cinematic %s\n", name);

        // Force console and GUI off
        //Con_Close();
        //CON_SetVisible(vs_notvisible);
        //GUI_Close();
        M_ClearMenus();

        // Make sure sounds aren't playing
        //S_FreeChannels();
    }

    // close game audio and setup movie audio
    SDL_CloseAudio();
    audio_dev = SDL_OpenAudioDevice(NULL, 0, &cin->wanted_spec, &audio_spec, 0);
    SDL_PauseAudioDevice(audio_dev, 0);

    I_Printf("E_PlayMovie: Opened audio device at %d Hz / %d chan\n", audio_spec.freq, audio_spec.channels);

    // sync to vblank
    SDL_GL_SetSwapInterval(1);		// SDL-based standard

    // setup to draw 2D screen
    RGL_SetupMatrices2D();

    // set global flag that a movie is playing as opposed to just animated textures
    playing_movie = true;

    while (playing_movie)
    {
        // update audio
        SDL_LockAudio();
        afree = AUDIOBUFFER_SIZE - SDL_GetQueuedAudioSize(audio_dev); // buffer space to fill
        CIN_UpdateAudio(midx, afree, audio_dev);
        SDL_UnlockAudio();
        SDL_PauseAudioDevice(audio_dev, 0);

        // update video
        CIN_UpdateCinematic(midx);

        // draw a movie frame
        I_StartFrame();

        //!!! upload decoded video into opengl texture
		//TODO: 
		//Results in a white screen (???), BindTexture/my_rndrr problem...?
        glBindTexture(GL_TEXTURE_2D, cin->vtex[0]);
        SDL_GL_BindTexture(cin->render_tex, NULL, NULL);

        // draw to screen
        glEnable(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glDisable(GL_ALPHA_TEST);

        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

        glBegin(GL_QUADS);

        glTexCoord2f(0.0f, 1.0f);
        glVertex2i(0, 0);

        glTexCoord2f(1.0f, 1.0f);
        glVertex2i(SCREENWIDTH, 0);

        glTexCoord2f(1.0f, 0.0f);
        glVertex2i(SCREENWIDTH, SCREENHEIGHT);

        glTexCoord2f(0.0f, 0.0f);
        glVertex2i(0, SCREENHEIGHT);

        SDL_GL_UnbindTexture(cin->render_tex);

        if (Kit_GetPlayerSubtitleStream(cin->player) >= 0)
        {
            // upload decoded subtitle into opengl texture
            glBindTexture(GL_TEXTURE_2D, cin->stex[0]);
            SDL_GL_BindTexture(cin->subtitle_tex, NULL, NULL);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            // draw all subtitle fragments
            for(int i = 0; i < cin->got; i++)
            {
                glTexCoord2f(HSS(cin->sources[i].x), VSS(cin->sources[i].y + cin->sources[i].h)); //0.0f, 1.0f
                glVertex2i(HTS(cin->targets[i].x), VTS(cin->targets[i].y)); // 0, 0

                glTexCoord2f(HSS(cin->sources[i].x + cin->sources[i].w), VSS(cin->sources[i].y + cin->sources[i].h)); // 1.0f, 1.0f
                glVertex2i(HTS(cin->targets[i].x + cin->targets[i].w), VTS(cin->targets[i].y)); // sw, 0

                glTexCoord2f(HSS(cin->sources[i].x + cin->sources[i].w), VSS(cin->sources[i].y)); // 1.0f, 0.0f
                glVertex2i(HTS(cin->targets[i].x + cin->targets[i].w), VTS(cin->targets[i].y + cin->targets[i].h)); // sw, sh

                glTexCoord2f(HSS(cin->sources[i].x), VSS(cin->sources[i].y)); // 0.0f, 0.0f
                glVertex2i(HTS(cin->targets[i].x), VTS(cin->targets[i].y + cin->targets[i].h)); // 0, sh
            }

            SDL_GL_UnbindTexture(cin->subtitle_tex);
        }

        glEnd();

        glDisable(GL_BLEND);
        glDisable(GL_TEXTURE_2D);

        I_FinishFrame();

        playing_movie = CIN_CheckCinematic(midx);

        // check if press key/button
        SDL_PumpEvents();
        SDL_Event sdl_ev;

        while (SDL_PollEvent(&sdl_ev))
        {
            switch(sdl_ev.type)
            {
                case SDL_KEYUP:
                case SDL_MOUSEBUTTONUP:
                case SDL_JOYBUTTONUP:
                    playing_movie = false;
                    I_Printf("Someone Set Us Up The Bomb!");
                    break;

                default:
                    break; // Don't care
            }
        }
    }

    // stop cinematic (just in case the video was interrupted or hit an error)
    CIN_StopCinematic(midx);
    I_Printf("E_PlayMovie: Cinematic stopped\n");

    CIN_Shutdown();

    // close movie audio and restart game audio
    SDL_CloseAudioDevice(audio_dev);
    I_StartupSound();
    SDL_PauseAudio(0);

    // restore OGL context
    SDL_GL_MakeCurrent( my_vis, glContext );

    // restore OGL swap interval
    if (r_vsync != 1)
        SDL_GL_SetSwapInterval(-1);

    I_Printf("E_PlayMovie exiting\n");
    return;
}

/*
 ==================
 CIN_CheckCinematic
 ==================
*/
bool CIN_CheckCinematic (cinHandle_t handle)
{
    cinematic_t *cin = CIN_GetCinematicByHandle(handle);

    if (Kit_GetPlayerState(cin->player) == KIT_STOPPED)
    {
#ifdef DEBUG_MOVIE_PLAYER
        const char *estr = Kit_GetError();
        I_Printf("  Cinematic stopped - %s\n", estr ? estr : "done");
#endif
        if (cin->flags & CIN_LOOPING)
        {
#ifdef DEBUG_MOVIE_PLAYER
            I_Printf("  Cinematic looping!\n");
#endif
            CIN_ResetCinematic(handle);
        }
        else
        {
            // done or error
            return false;
        }
    }
    return true;
}

/*
 ==================
 CIN_UpdateCinematic
 ==================
*/
void CIN_UpdateCinematic (cinHandle_t handle)
{
#ifdef DEBUG_MOVIE_PLAYER
   // I_Printf("CIN_UpdateCinematic!\n");
#endif
    cinematic_t *cin = CIN_GetCinematicByHandle(handle);
    cin->got = 0;
	
	// TODO: Needed??
    // Get movie area size
    //SDL_RenderSetLogicalSize(my_rndrr, cin->pinfo.video.output.width, cin->pinfo.video.output.height);

   // CA: SDL_CreateRenderer went back to being handled in "system/i_video.cc"
   // my_rndrr = SDL_CreateRenderer(my_vis, -1, 0);
   
    // update video texture
    if (Kit_GetPlayerVideoStream(cin->player) >= 0)
    {
        Kit_GetPlayerVideoData(cin->player, cin->video_tex);
        // convert video texture to flip target
        SDL_SetRenderTarget(my_rndrr, cin->flip_tex);
        // Clear screen with black
        SDL_SetRenderDrawColor(my_rndrr, 0, 0, 0, 255);
        SDL_RenderClear(my_rndrr);

        SDL_RenderCopy(my_rndrr, cin->video_tex, NULL, NULL);
        SDL_SetRenderTarget(my_rndrr, cin->render_tex);
        SDL_RenderCopyEx(my_rndrr, cin->flip_tex, NULL, NULL, 0, NULL, SDL_FLIP_VERTICAL);
        SDL_SetRenderTarget(my_rndrr, NULL);
    }

    // update subtitle texture
    if (Kit_GetPlayerSubtitleStream(cin->player) >= 0)
        cin->got = Kit_GetPlayerSubtitleData(cin->player, cin->subtitle_tex, &cin->sources[0], &cin->targets[0], ATLAS_MAX);

    SDL_RenderPresent(my_rndrr);
    cin->frameCount++;
    return;
}

/*
 =================
 CIN_UpdateAudio

 Returns number of bytes of audio fetched
 =================
*/
int CIN_UpdateAudio(cinHandle_t handle, int need, SDL_AudioDeviceID audio_dev)
{
#ifdef DEBUG_MOVIE_PLAYER
    //I_Printf("CIN_UpdateAudio!\n");
#endif
    cinematic_t *cin = CIN_GetCinematicByHandle(handle);
    int bytes = 0;

    if (!cin->audiobuf)
        return 0; // no audiobuf => no audio for this cinematic

    while (need > 0)
    {
        int ret = Kit_GetPlayerAudioData(
            cin->player,
            cin->audiobuf,
            AUDIOBUFFER_SIZE);
        need -= ret;
        bytes += ret;
        if ((ret > 0) && audio_dev)
            SDL_QueueAudio(audio_dev, cin->audiobuf, ret);
        else
            break; // error or no audio device (only do one fetch and return)
    }
    return bytes;
}

/*
 ==================
 CIN_ResetCinematic
 ==================
*/
void CIN_ResetCinematic (cinHandle_t handle)
{
    cinematic_t *cin = CIN_GetCinematicByHandle(handle);

    // Reset the cinematic
    Kit_PlayerSeek(cin->player, 0.0);
    Kit_PlayerPlay(cin->player);

    cin->frameCount = 0;
}

/*
 ==================
 CIN_StopCinematic
 ==================
*/
void CIN_StopCinematic (cinHandle_t handle)
{
    cinematic_t *cin = CIN_GetCinematicByHandle(handle);
    epi::file_c *F;

    // Stop the cinematic
    if (cin->flags & CIN_SYSTEM)
    {
        I_Debugf("Stopped cinematic %s\n", cin->name);

        // Make sure sounds aren't playing
        // S_FreeChannels();
    }

    // free OGL textures
    if (cin->vtex[0])
        glDeleteTextures(1, &cin->vtex[0]);
    if (cin->stex[0])
        glDeleteTextures(1, &cin->stex[0]);

    // free SDL textures
    if (cin->video_tex)
        SDL_DestroyTexture(cin->video_tex);
    if (cin->render_tex)
        SDL_DestroyTexture(cin->render_tex);
    if (cin->flip_tex)
        SDL_DestroyTexture(cin->flip_tex);
    if (cin->subtitle_tex)
        SDL_DestroyTexture(cin->subtitle_tex);

    // free audio buffer
    if (cin->audiobuf)
        Z_Free(cin->audiobuf);

    // close player
    if (cin->player)
        Kit_ClosePlayer(cin->player);

    // close source
    if (cin->src)
        Kit_CloseSource(cin->src);

    F = cin->file;
    delete F;

    memset(cin, 0, sizeof(cinematic_t));
}


// ============================================================================

#if 0

/*
 ==================
 CIN_PlayCinematic_f (play through COAL/RTS?)
 ==================
*/
static void CIN_PlayCinematic_f(void)
{

    char    name[MAX_PATH];

    if (Cmd_Argc() != 2)
    {
        I_Printf("Usage: playCinematic <name>\n");
        return;
    }

    Str_Copy(name, Cmd_Argv(1), sizeof(name));
    Str_DefaultFilePath(name, "videos", sizeof(name));
    Str_DefaultFileExtension(name, ".RoQ", sizeof(name));

    // If running a local server, kill it
    SV_Shutdown("Server quit");

    // If connected to a server, disconnect
    CL_Disconnect(true);

    // Play the cinematic
    cls.cinematicHandle = CIN_PlayCinematic(name, CIN_SYSTEM);
}
#endif // 0


/*
 ==================
 CIN_ListCinematics_f (EDGE Console Command?)
 ==================
*/
#if 0
static void CIN_ListCinematics_f(void)
{

    cinematic_t *cin;
    int         count = 0, bytes = 0;
    int         i;

    I_Printf("\n");
    I_Printf("    -w-- -h-- -size- fps -name-----------\n");

    for (i = 0, cin = cin_cinematics; i < MAX_CINEMATICS; i++, cin++) {
        if (!cin->playing)
            continue;

        count++;
        bytes += cin->frameWidth * cin->frameHeight * 8;

        I_Printf("%2i: ", i);

        I_Printf("%4i %4i ", cin->frameWidth, cin->frameHeight);

        I_Printf("%5ik ", BYTES_TO_KB(cin->frameWidth * cin->frameHeight * 8));

        I_Printf("%3i ", cin->frameRate);

        I_Printf("%s\n", cin->name);
    }

    I_Printf("-----------------------------------------\n");
    I_Printf("%i total cinematics\n", count);
    I_Printf("%.2f MB of cinematic data\n", BYTES_TO_FLOAT_MB(bytes));
    I_Printf("\n");
}
#endif // 0


/*
 ==================
 CIN_Init
 ==================
*/
void CIN_Init (void)
{
    if (cin_init)
        return;

    I_Printf("CIN_Init: Starting up...\n");

    // Clear global array
    memset(cin_cinematics, 0, sizeof(cin_cinematics));

   // my_rndrr = SDL_CreateRenderer(my_vis, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (my_rndrr == NULL) 
    {
        I_Error("FUCKMYBUTTHOLE: Unable to create a renderer!\n");
    }

    // initialize SDL kitchensink
    Kit_Init(KIT_INIT_ASS);

    // Allow Kit to use more threads
    //Kit_SetHint(KIT_HINT_THREAD_COUNT, SDL_GetCPUCount() <= 4 ? SDL_GetCPUCount() : 4);

    // Lots of buffers for smooth playback (will eat up more memory, too).
      Kit_SetHint(KIT_HINT_VIDEO_BUFFER_FRAMES, 5);
   // Kit_SetHint(KIT_HINT_AUDIO_BUFFER_FRAMES, 192);

    // Add commands
    //Cmd_AddCommand("playCinematic", CIN_PlayCinematic_f, "Plays a cinematic", Cmd_ArgCompletion_VideoName);
    //Cmd_AddCommand("listCinematics", CIN_ListCinematics_f, "Lists playing cinematics", NULL);

    cin_init = true;
}

/*
 ==================
 CIN_Shutdown
 ==================
*/
void CIN_Shutdown (void)
{
    cinematic_t *cin;
    int         i;

    if (!cin_init)
        return; // not initialized

    // check all cinematics
    for (i = 0, cin = cin_cinematics; i < MAX_CINEMATICS; i++, cin++)
        if (cin->playing)
            return; // cinematics still in use!

    I_Printf("CIN_Shutdown: Shutting down...\n");

    // Remove commands
    //Cmd_RemoveCommand("playCinematic");
    //Cmd_RemoveCommand("listCinematics");

    // close SDL kitchensink
    Kit_Quit();

    cin_init = false;
}
#endif