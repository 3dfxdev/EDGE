 /*==============================================================================

 CINEMATICS

 ==============================================================================
*/

#ifndef __I_FFMPEG__
#define __I_FFMPEG__

#include <kitchensink\kitchensink.h>
#include <epi\file.h>

#include "i_sdlinc.h"

#define MAX_CINEMATICS      16
#define PLAYER_NAME_MAX_LEN 256
#define AUDIOBUFFER_SIZE    (1024 * 64)
#define ATLAS_WIDTH         4096
#define ATLAS_HEIGHT        4096
#define ATLAS_MAX           1024

#define BIT(num)            (1 << (num))


typedef struct
{
    bool                playing;

    char                name[PLAYER_NAME_MAX_LEN];
    int                 flags;

    epi::file_c         *file;
    int                 size;
    int                 offset;

    Kit_Source          *src;
    Kit_SourceStreamInfo      sinfo;
    Kit_Player          *player;
    Kit_PlayerInfo      pinfo;

    SDL_AudioSpec       wanted_spec;
    unsigned char       *audiobuf;

    SDL_Texture         *video_tex;
    SDL_Texture         *render_tex;
    SDL_Texture         *flip_tex;
    GLuint              vtex[1];

    SDL_Texture         *subtitle_tex;
    GLuint              stex[1];

    int                 frameWidth;
    int                 frameHeight;
    int                 frameCount;

    SDL_Rect            sources[ATLAS_MAX];
    SDL_Rect            targets[ATLAS_MAX];
    int                 got;

} cinematic_t;




#endif // I__FFMPEG__