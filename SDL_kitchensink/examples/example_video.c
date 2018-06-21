#include <kitchensink/kitchensink.h>
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdbool.h>

/*
* Note! This example does not do proper error handling etc.
* It is for example use only!
*/

#define AUDIOBUFFER_SIZE (1024 * 64)
#define ATLAS_WIDTH 4096
#define ATLAS_HEIGHT 4096
#define ATLAS_MAX 1024


void render_gui(SDL_Renderer *renderer, double percent) {
    // Get window size
    int size_w, size_h;
    SDL_RenderGetLogicalSize(renderer, &size_w, &size_h);

    // Render progress bar
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_Rect progress_border;
    progress_border.x = 28;
    progress_border.y = size_h - 61;
    progress_border.w = size_w - 57;
    progress_border.h = 22;
    SDL_RenderFillRect(renderer, &progress_border);

    SDL_SetRenderDrawColor(renderer, 155, 155, 155, 255);
    SDL_Rect progress_bottom;
    progress_bottom.x = 30;
    progress_bottom.y = size_h - 60;
    progress_bottom.w = size_w - 60;
    progress_bottom.h = 20;
    SDL_RenderFillRect(renderer, &progress_bottom);

    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_Rect progress_top;
    progress_top.x = 30;
    progress_top.y = size_h - 60;
    progress_top.w = (size_w - 60) * percent;
    progress_top.h = 20;
    SDL_RenderFillRect(renderer, &progress_top);
}

int main(int argc, char *argv[]) {
    int err = 0, ret = 0;
    const char* filename = NULL;

    // Video
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;

    // Events
    SDL_Event event;
    bool run = true;
    
    // Kitchensink
    Kit_Source *src = NULL;
    Kit_Player *player = NULL;

    // Audio playback
    SDL_AudioSpec wanted_spec, audio_spec;
    SDL_AudioDeviceID audio_dev;
    char audiobuf[AUDIOBUFFER_SIZE];

    // Get filename to open
    if(argc != 2) {
        fprintf(stderr, "Usage: exampleplay <filename>\n");
        return 0;
    }
    filename = argv[1];

    // Init SDL
    err = SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO);
    if(err != 0) {
        fprintf(stderr, "Unable to initialize SDL2!\n");
        return 1;
    }

    // Create a resizable window.
    window = SDL_CreateWindow("Example Player", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720, SDL_WINDOW_RESIZABLE);
    if(window == NULL) {
        fprintf(stderr, "Unable to create a new window!\n");
        return 1;
    }

    // Create an accelerated renderer. Enable vsync, so we don't need to play around with SDL_Delay.
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
    if(window == NULL) {
        fprintf(stderr, "Unable to create a renderer!\n");
        return 1;
    }

    // Initialize Kitchensink with network and libass support.
    err = Kit_Init(KIT_INIT_NETWORK|KIT_INIT_ASS);
    if(err != 0) {
        fprintf(stderr, "Unable to initialize Kitchensink: %s", Kit_GetError());
        return 1;
    }

    // Open up the sourcefile.
    // This can be a local file, network url, ...
    src = Kit_CreateSourceFromUrl(filename);
    if(src == NULL) {
        fprintf(stderr, "Unable to load file '%s': %s\n", filename, Kit_GetError());
        return 1;
    }

    // Print stream information
    Kit_StreamInfo sinfo;
    fprintf(stderr, "Source streams:\n");
    for(int i = 0; i < Kit_GetSourceStreamCount(src); i++) {
        err = Kit_GetSourceStreamInfo(src, &sinfo, i);
        if(err) {
            fprintf(stderr, "Unable to fetch stream #%d information: %s.\n", i, Kit_GetError());
            return 1;
        }
        fprintf(stderr, " * Stream #%d: %s\n", i, Kit_GetKitStreamTypeString(sinfo.type));
    }

    // Create the player
    player = Kit_CreatePlayer(src);
    if(player == NULL) {
        fprintf(stderr, "Unable to create player: %s\n", Kit_GetError());
        return 1;
    }

    // Print some information
    Kit_PlayerInfo pinfo;
    Kit_GetPlayerInfo(player, &pinfo);

    if(!pinfo.video.is_enabled) {
        fprintf(stderr, "File contains no video!\n");
        return 1;
    }

    fprintf(stderr, "Media information:\n");
    if(pinfo.audio.is_enabled) {
        fprintf(stderr, " * Audio: %s (%s), %dHz, %dch, %db, %s\n",
            pinfo.acodec,
            pinfo.acodec_name,
            pinfo.audio.samplerate,
            pinfo.audio.channels,
            pinfo.audio.bytes,
            pinfo.audio.is_signed ? "signed" : "unsigned");
    }
    fprintf(stderr, " * Video: %s (%s), %dx%d\n",
        pinfo.vcodec,
        pinfo.vcodec_name,
        pinfo.video.width,
        pinfo.video.height);
    if(pinfo.subtitle.is_enabled) {
        fprintf(stderr, " * Subtitle: %s (%s)\n",
            pinfo.scodec,
            pinfo.scodec_name);
    }
    fprintf(stderr, "Duration: %f seconds\n", Kit_GetPlayerDuration(player));

    // Init audio
    SDL_memset(&wanted_spec, 0, sizeof(wanted_spec));
    wanted_spec.freq = pinfo.audio.samplerate;
    wanted_spec.format = pinfo.audio.format;
    wanted_spec.channels = pinfo.audio.channels;
    audio_dev = SDL_OpenAudioDevice(NULL, 0, &wanted_spec, &audio_spec, 0);
    SDL_PauseAudioDevice(audio_dev, 0);

    // Print some format info
    fprintf(stderr, "Texture type: %s\n", Kit_GetSDLPixelFormatString(pinfo.video.format));
    fprintf(stderr, "Audio format: %s\n", Kit_GetSDLAudioFormatString(pinfo.audio.format));
    fprintf(stderr, "Subtitle format: %s\n", Kit_GetSDLPixelFormatString(pinfo.subtitle.format));
    fflush(stderr);

    // Initialize video texture. This will probably end up as YV12 most of the time.
    SDL_Texture *video_tex = SDL_CreateTexture(
        renderer,
        pinfo.video.format,
        SDL_TEXTUREACCESS_STATIC,
        pinfo.video.width,
        pinfo.video.height);
    if(video_tex == NULL) {
        fprintf(stderr, "Error while attempting to create a video texture\n");
        return 1;
    }

    // This is the subtitle texture atlas. This contains all the subtitle image fragments.
    SDL_Texture *subtitle_tex = SDL_CreateTexture(
        renderer,
        pinfo.subtitle.format,
        SDL_TEXTUREACCESS_STATIC,
        ATLAS_WIDTH, ATLAS_HEIGHT);
    if(subtitle_tex == NULL) {
        fprintf(stderr, "Error while attempting to create a subtitle texture atlas\n");
        return 1;
    }

    // Make sure subtitle texture is in correct blendmode
    SDL_SetTextureBlendMode(subtitle_tex, SDL_BLENDMODE_BLEND);

    // Set logical size for the renderer. This way when we scale, we keep aspect ratio.
    SDL_RenderSetLogicalSize(renderer, pinfo.video.width, pinfo.video.height); 

    // Start playback
    Kit_PlayerPlay(player);

    // Get movie area size
    int mouse_x = 0, mouse_y = 0;
    int size_w = 0, size_h = 0;
    SDL_RenderGetLogicalSize(renderer, &size_w, &size_h);
    bool gui_enabled = false;
    bool fullscreen = false;
    while(run) {
        if(Kit_GetPlayerState(player) == KIT_STOPPED) {
            run = false;
            continue;
        }

        // Check for events
        const Uint8 *state;
        while(SDL_PollEvent(&event)) {
            switch(event.type) {
                case SDL_KEYUP:
                    if(event.key.keysym.sym == SDLK_ESCAPE) {
                        run = false;
                    }
                    break;

                case SDL_KEYDOWN:
                    // Find alt+enter
                    state = SDL_GetKeyboardState(NULL);
                    if(state[SDL_SCANCODE_RETURN] && state[SDL_SCANCODE_LALT]) {
                        if(!fullscreen) {
                            SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
                        } else {
                            SDL_SetWindowFullscreen(window, 0);
                        }
                        fullscreen = !fullscreen;
                    }
                    break;

                case SDL_MOUSEMOTION:
                    mouse_x = event.motion.x;
                    mouse_y = event.motion.y;
                    break;

                case SDL_MOUSEBUTTONUP:
                    // Handle user clicking the progress bar
                    if(mouse_x >= 30 && mouse_x <= size_w-30 && mouse_y >= size_h - 60 && mouse_y <= size_h - 40) {
                        double pos = ((double)mouse_x - 30) / ((double)size_w - 60);
                        double m_time = Kit_GetPlayerDuration(player) * pos;
                        if(Kit_PlayerSeek(player, m_time) != 0) {
                            fprintf(stderr, "%s\n", Kit_GetError());
                        }
                        SDL_ClearQueuedAudio(audio_dev);
                    } else {
                        // Handle pause
                        if(Kit_GetPlayerState(player) == KIT_PAUSED) {
                            Kit_PlayerPlay(player);
                        } else {
                            Kit_PlayerPause(player);
                        }
                    }
                    break;

                case SDL_QUIT:
                    run = false;
                    break;
            }
        }

        // Enable GUI if mouse is hovering over the bottom third of the screen
        int limit = (pinfo.video.height / 3) * 2; 
        gui_enabled = (mouse_y >= limit);

        // Refresh audio
        int queued = SDL_GetQueuedAudioSize(audio_dev);
        if(queued < AUDIOBUFFER_SIZE) {
            int need = AUDIOBUFFER_SIZE - queued;

            while(need > 0) {
                ret = Kit_GetAudioData(
                    player,
                    (unsigned char*)audiobuf,
                    AUDIOBUFFER_SIZE);
                need -= ret;
                if(ret > 0) {
                    SDL_QueueAudio(audio_dev, audiobuf, ret);
                } else {
                    break;
                }
            }
            // If we now have data, start playback (again)
            if(SDL_GetQueuedAudioSize(audio_dev) > 0) {
                SDL_PauseAudioDevice(audio_dev, 0);
            }
        }

        // Clear screen with black
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Refresh videotexture and render it
        Kit_GetVideoData(player, video_tex);
        SDL_RenderCopy(renderer, video_tex, NULL, NULL);

        // Refresh subtitle texture atlas and render subtitle frames from it
        SDL_Rect sources[ATLAS_MAX];
        SDL_Rect targets[ATLAS_MAX];
        int got = Kit_GetSubtitleData(player, subtitle_tex, sources, targets, ATLAS_MAX);
        for(int i = 0; i < got; i++) {
            SDL_RenderCopy(renderer, subtitle_tex, &sources[i], &targets[i]);
        }

        // Render GUI
        if(gui_enabled) {
            double percent = Kit_GetPlayerPosition(player) / Kit_GetPlayerDuration(player);
            render_gui(renderer, percent);
        }

        // Render to screen + wait for vsync
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(subtitle_tex);
    SDL_DestroyTexture(video_tex);
    SDL_CloseAudioDevice(audio_dev);

    Kit_ClosePlayer(player);
    Kit_CloseSource(src);

    Kit_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
