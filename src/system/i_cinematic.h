 /*==============================================================================

 CINEMATICS

 ==============================================================================
*/

#include <kitchensink/kitchensink.h>
#include <SDL2/SDL.h>

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
    Kit_StreamInfo      sinfo;
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

    int					frameWidth;
    int					frameHeight;
    int                 frameCount;

    SDL_Rect            sources[ATLAS_MAX];
    SDL_Rect            targets[ATLAS_MAX];
    int                 got;

} cinematic_t;

typedef int                 cinHandle_t;

cinHandle_t cinematicHandle;

typedef enum
{
    CIN_SYSTEM              = BIT(0),
    CIN_LOOPING             = BIT(1),
    CIN_SILENT              = BIT(2)
} cinFlags_t;

// Plays a cinematic
cinHandle_t     CIN_PlayCinematic (const char *name, int flags);

// Runs a cinematic frame
void            CIN_UpdateCinematic (cinHandle_t handle);

// Update audio stream from cinematic buffer
int             CIN_UpdateAudio (cinHandle_t handle, int need, SDL_AudioDeviceID audio_dev);

// Resets a cinematic
void            CIN_ResetCinematic (cinHandle_t handle);

// Stops a cinematic
void            CIN_StopCinematic (cinHandle_t handle);

// Checks if cinematic playing - handles looping
bool            CIN_CheckCinematic (cinHandle_t handle);

// Initializes the cinematic module
void            CIN_Init (void);

// Shuts down the cinematic module
void            CIN_Shutdown (void);

void            E_PlayMovie(const char *name, int flags);
