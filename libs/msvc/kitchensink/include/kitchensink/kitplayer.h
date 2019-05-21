#ifndef KITPLAYER_H
#define KITPLAYER_H

#include "kitchensink/kitsource.h"
#include "kitchensink/kitconfig.h"
#include "kitchensink/kitformats.h"

#include <SDL2/SDL_render.h>
#include <SDL2/SDL_thread.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_mutex.h>


#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define KIT_CODECMAX 16
#define KIT_CODECNAMEMAX 128

typedef enum Kit_PlayerState {
    KIT_STOPPED = 0, ///< Playback stopped or has not started yet.
    KIT_PLAYING,     ///< Playback started & player is actively decoding.
    KIT_PAUSED,      ///< Playback paused; player is actively decoding but no new data is given out.
    KIT_CLOSED,      ///< Playback is stopped and player is closing.
} Kit_PlayerState;

typedef struct Kit_Player {
    Kit_PlayerState state;      ///< Playback state
    Kit_VideoFormat vformat;    ///< Video format information
    Kit_AudioFormat aformat;    ///< Audio format information
    Kit_SubtitleFormat sformat; ///< Subtitle format information
    void *audio_dec;            ///< Audio decoder context (or NULL)
    void *video_dec;            ///< Video decoder context (or NULL)
    void *subtitle_dec;         ///< Subtitle decoder context (or NULL)
    SDL_Thread *dec_thread;     ///< Decoder thread
    SDL_mutex *dec_lock;        ///< Decoder lock
    const Kit_Source *src;      ///< Reference to Audio/Video source
    double pause_started;       ///< Temporary flag for handling pauses
} Kit_Player;

typedef struct Kit_PlayerInfo {
    char acodec[KIT_CODECMAX]; ///< Audio codec short name, eg "ogg", "mp3"
    char acodec_name[KIT_CODECNAMEMAX]; ///< Audio codec long, more descriptive name
    char vcodec[KIT_CODECMAX]; ///< Video codec short name, eg. "x264"
    char vcodec_name[KIT_CODECNAMEMAX]; ///< Video codec long, more descriptive name
    char scodec[KIT_CODECMAX]; ///< Subtitle codec short name, eg. "ass"
    char scodec_name[KIT_CODECNAMEMAX]; ///< Subtitle codec long, more descriptive name
    Kit_VideoFormat video; ///< Video format information
    Kit_AudioFormat audio; ///< Audio format information
    Kit_SubtitleFormat subtitle; ///< Subtitle format information
} Kit_PlayerInfo;

KIT_API Kit_Player* Kit_CreatePlayer(const Kit_Source *src);
KIT_API void Kit_ClosePlayer(Kit_Player *player);

KIT_API int Kit_UpdatePlayer(Kit_Player *player);
KIT_API int Kit_GetVideoData(Kit_Player *player, SDL_Texture *texture);
KIT_API int Kit_GetSubtitleData(Kit_Player *player, SDL_Texture *texture, SDL_Rect *sources, SDL_Rect *targets, int limit);
KIT_API int Kit_GetAudioData(Kit_Player *player, unsigned char *buffer, int length);
KIT_API void Kit_GetPlayerInfo(const Kit_Player *player, Kit_PlayerInfo *info);

KIT_API Kit_PlayerState Kit_GetPlayerState(const Kit_Player *player);
KIT_API void Kit_PlayerPlay(Kit_Player *player);
KIT_API void Kit_PlayerStop(Kit_Player *player);
KIT_API void Kit_PlayerPause(Kit_Player *player);

KIT_API int Kit_PlayerSeek(Kit_Player *player, double time);
KIT_API double Kit_GetPlayerDuration(const Kit_Player *player);
KIT_API double Kit_GetPlayerPosition(const Kit_Player *player);

#ifdef __cplusplus
}
#endif

#endif // KITPLAYER_H
