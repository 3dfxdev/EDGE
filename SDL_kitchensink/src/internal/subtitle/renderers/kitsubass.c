#include <assert.h>
#include <stdlib.h>

#include <SDL2/SDL_surface.h>

#include "kitchensink/kiterror.h"
#include "kitchensink/internal/utils/kitlog.h"
#include "kitchensink/internal/kitlibstate.h"
#include "kitchensink/internal/subtitle/kitsubtitlepacket.h"
#include "kitchensink/internal/subtitle/kitatlas.h"
#include "kitchensink/internal/utils/kithelpers.h"
#include "kitchensink/internal/subtitle/renderers/kitsubass.h"

typedef struct Kit_ASSSubtitleRenderer {
    ASS_Renderer *renderer;
    ASS_Track *track;
} Kit_ASSSubtitleRenderer;

static void Kit_ProcessAssImage(SDL_Surface *surface, const ASS_Image *img) {
    unsigned char r = ((img->color) >> 24) & 0xFF;
    unsigned char g = ((img->color) >> 16) & 0xFF;
    unsigned char b = ((img->color) >>  8) & 0xFF;
    unsigned char a = (img->color) & 0xFF;
    unsigned char *src = img->bitmap;
    unsigned char *dst = surface->pixels;
    unsigned int x, y;

    for(y = 0; y < img->h; y++) {
        for(x = 0; x < img->w; x++) {
            dst[x * 4 + 0] = r;
            dst[x * 4 + 1] = g;
            dst[x * 4 + 2] = b;
            dst[x * 4 + 3] = ((255 - a) * src[x]) >> 8;
        }
        src += img->stride;
        dst += surface->pitch;
    }
}

static void ren_render_ass_cb(Kit_SubtitleRenderer *ren, void *src, double start_pts, double end_pts) {
    assert(ren != NULL);
    assert(src != NULL);

    Kit_ASSSubtitleRenderer *ass_ren = ren->userdata;
    AVSubtitle *sub = src;

    // Read incoming subtitle packets to libASS
    if(Kit_LockDecoderOutput(ren->dec) == 0) {
        for(int r = 0; r < sub->num_rects; r++) {
            if(sub->rects[r]->ass == NULL)
                continue;
            ass_process_data(ass_ren->track, sub->rects[r]->ass, strlen(sub->rects[r]->ass));
        }
        Kit_UnlockDecoderOutput(ren->dec);
    }
}

static void ren_close_ass_cb(Kit_SubtitleRenderer *ren) {
    if(ren == NULL) return;

    Kit_ASSSubtitleRenderer *ass_ren = ren->userdata;
    ass_free_track(ass_ren->track);
    ass_renderer_done(ass_ren->renderer);
    free(ass_ren);
}

static int ren_get_data_cb(Kit_SubtitleRenderer *ren, Kit_TextureAtlas *atlas, double current_pts) {
    Kit_ASSSubtitleRenderer *ass_ren = ren->userdata;
    SDL_Surface *dst = NULL;
    ASS_Image *src = NULL;
    int change = 0;
    unsigned int now = current_pts * 1000;

    // First, we tell ASS to render images for us
    if(Kit_LockDecoderOutput(ren->dec) == 0) {
        src = ass_render_frame(ass_ren->renderer, ass_ren->track, now, &change);
        Kit_UnlockDecoderOutput(ren->dec);
    }

    // If there was no change, stop here
    if(change == 0) {
        return 0;
    }

    // There was some change, process images and add them to atlas
    Kit_ClearAtlasContent(atlas);
    for(; src; src = src->next) {
        if(src->w == 0 || src->h == 0)
            continue;
        dst = SDL_CreateRGBSurfaceWithFormat(0, src->w, src->h, 32, SDL_PIXELFORMAT_RGBA32);
        Kit_ProcessAssImage(dst, src);
        SDL_Rect target;
        target.x = src->dst_x;
        target.y = src->dst_y;
        target.w = src->w;
        target.h = src->h;
        Kit_AddAtlasItem(atlas, dst, &target);
        SDL_FreeSurface(dst);
    }

    ren->dec->clock_pos = current_pts;
    return 0;
}

Kit_SubtitleRenderer* Kit_CreateASSSubtitleRenderer(Kit_Decoder *dec, int w, int h) {
    assert(dec != NULL);
    assert(w >= 0);
    assert(h >= 0);

    // Make sure that libass library has been initialized + get handle
    Kit_LibraryState *state = Kit_GetLibraryState();
    if(state->libass_handle == NULL) {
        Kit_SetError("Libass library has not been initialized");
        return NULL;
    }

    // First allocate the generic decoder component
    Kit_SubtitleRenderer *ren = Kit_CreateSubtitleRenderer(dec);
    if(ren == NULL) {
        goto exit_0;
    }

    // Next, allocate ASS subtitle renderer context.
    Kit_ASSSubtitleRenderer *ass_ren = calloc(1, sizeof(Kit_ASSSubtitleRenderer));
    if(ass_ren == NULL) {
        goto exit_1;
    }

    // Initialize libass renderer
    ASS_Renderer *ass_renderer = ass_renderer_init(state->libass_handle);
    if(ass_renderer == NULL) {
        Kit_SetError("Unable to initialize libass renderer");
        goto exit_2;
    }

    // Read fonts from attachment streams and give them to libass
    for(int j = 0; j < dec->format_ctx->nb_streams; j++) {
        AVStream *st = dec->format_ctx->streams[j];
        if(st->codec->codec_type == AVMEDIA_TYPE_ATTACHMENT && attachment_is_font(st)) {
            const AVDictionaryEntry *tag = av_dict_get(
                st->metadata,
                "filename",
                NULL,
                AV_DICT_MATCH_CASE);
            if(tag) {
                ass_add_font(
                    state->libass_handle,
                    tag->value, 
                    (char*)st->codec->extradata,
                    st->codec->extradata_size);
            }
        }
    }

    // Init libass fonts and window frame size
    ass_set_fonts(
        ass_renderer,
        NULL, "sans-serif",
        ASS_FONTPROVIDER_AUTODETECT,
        NULL, 1);
    ass_set_frame_size(ass_renderer, w, h);
    ass_set_hinting(ass_renderer, ASS_HINTING_NONE);

    // Initialize libass track
    ASS_Track *ass_track = ass_new_track(state->libass_handle);
    if(ass_track == NULL) {
        Kit_SetError("Unable to initialize libass track");
        goto exit_3;
    }

    // Set up libass track headers (ffmpeg provides these)
    if(dec->codec_ctx->subtitle_header) {
        ass_process_codec_private(
            ass_track,
            (char*)dec->codec_ctx->subtitle_header,
            dec->codec_ctx->subtitle_header_size);
    }

    // Set callbacks and userdata, and we're go
    ass_ren->renderer = ass_renderer;
    ass_ren->track = ass_track;
    ren->ren_render = ren_render_ass_cb;
    ren->ren_close = ren_close_ass_cb;
    ren->ren_get_data = ren_get_data_cb;
    ren->userdata = ass_ren;
    return ren;

exit_3:
    ass_renderer_done(ass_renderer);
exit_2:
    free(ass_ren);
exit_1:
    Kit_CloseSubtitleRenderer(ren);
exit_0:
    return NULL;
}
