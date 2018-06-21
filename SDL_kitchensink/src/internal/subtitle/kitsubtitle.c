#include <assert.h>

#include <SDL2/SDL.h>
#include <libavformat/avformat.h>

#include "kitchensink/internal/utils/kitlog.h"

#include "kitchensink/kiterror.h"
#include "kitchensink/kitlib.h"
#include "kitchensink/internal/utils/kitlog.h"
#include "kitchensink/internal/kitlibstate.h"
#include "kitchensink/internal/subtitle/kitsubtitlepacket.h"
#include "kitchensink/internal/subtitle/kitsubtitle.h"
#include "kitchensink/internal/subtitle/kitatlas.h"
#include "kitchensink/internal/subtitle/renderers/kitsubimage.h"
#include "kitchensink/internal/subtitle/renderers/kitsubass.h"
#include "kitchensink/internal/subtitle/renderers/kitsubrenderer.h"
#include "kitchensink/internal/utils/kithelpers.h"


#define KIT_SUBTITLE_OUT_SIZE 64

typedef struct Kit_SubtitleDecoder {
    Kit_SubtitleFormat *format;
    Kit_SubtitleRenderer *renderer;
    AVSubtitle scratch_frame;
    int w;
    int h;
    Kit_TextureAtlas *atlas;
} Kit_SubtitleDecoder;


static void free_out_subtitle_packet_cb(void *packet) {
    Kit_FreeSubtitlePacket((Kit_SubtitlePacket*)packet);
}

static void dec_decode_subtitle_cb(Kit_Decoder *dec, AVPacket *in_packet) {
    assert(dec != NULL);
    assert(in_packet != NULL);

    Kit_SubtitleDecoder *subtitle_dec = dec->userdata;
    double pts, start, end;
    int frame_finished;
    int len;

    if(in_packet->size > 0) {
        len = avcodec_decode_subtitle2(dec->codec_ctx, &subtitle_dec->scratch_frame, &frame_finished, in_packet);
        if(len < 0) {
            return;
        }

        if(frame_finished) {
            // Start and end presentation timestamps for subtitle frame
            pts = 0;
            if(in_packet->pts != AV_NOPTS_VALUE) {
                pts = in_packet->pts;
                pts *= av_q2d(dec->format_ctx->streams[dec->stream_index]->time_base);
            }

            // If subtitle has no ending time, we set some safety value.
            if(subtitle_dec->scratch_frame.end_display_time == UINT_MAX) {
                subtitle_dec->scratch_frame.end_display_time = 30000;
            }

            start = pts + subtitle_dec->scratch_frame.start_display_time / 1000.0f;
            end = pts + subtitle_dec->scratch_frame.end_display_time / 1000.0f;

            // Create a packet. This should be filled by renderer.
            Kit_RunSubtitleRenderer(
                subtitle_dec->renderer, &subtitle_dec->scratch_frame, start, end);

            // Free subtitle since it has now been handled
            avsubtitle_free(&subtitle_dec->scratch_frame);
        }
    }
}

static void dec_close_subtitle_cb(Kit_Decoder *dec) {
    if(dec == NULL) return;
    Kit_SubtitleDecoder *subtitle_dec = dec->userdata;
    Kit_FreeAtlas(subtitle_dec->atlas);
    Kit_CloseSubtitleRenderer(subtitle_dec->renderer);
    free(subtitle_dec);
}

Kit_Decoder* Kit_CreateSubtitleDecoder(const Kit_Source *src, Kit_SubtitleFormat *format, int w, int h) {
    assert(src != NULL);
    assert(format != NULL);
    if(src->subtitle_stream_index < 0) {
        return NULL;
    }

    Kit_LibraryState *library_state = Kit_GetLibraryState();

    // First the generic decoder component
    Kit_Decoder *dec = Kit_CreateDecoder(
        src, src->subtitle_stream_index,
        KIT_SUBTITLE_OUT_SIZE,
        free_out_subtitle_packet_cb);
    if(dec == NULL) {
        goto exit_0;
    }

    // Set format. Note that is_enabled may be changed below ...
    format->is_enabled = true;
    format->stream_index = src->subtitle_stream_index;
    format->format = SDL_PIXELFORMAT_RGBA32; // Always this

    // ... then allocate the subtitle decoder
    Kit_SubtitleDecoder *subtitle_dec = calloc(1, sizeof(Kit_SubtitleDecoder));
    if(subtitle_dec == NULL) {
        goto exit_1;
    }

    // For subtitles, we need a renderer for the stream. Pick one based on codec ID.
    Kit_SubtitleRenderer *ren = NULL;
    switch(dec->codec_ctx->codec_id) {
        case AV_CODEC_ID_TEXT:
        case AV_CODEC_ID_HDMV_TEXT_SUBTITLE:
        case AV_CODEC_ID_SRT:
        case AV_CODEC_ID_SUBRIP:
        case AV_CODEC_ID_SSA:
        case AV_CODEC_ID_ASS:
            if(library_state->init_flags & KIT_INIT_ASS) {
                ren = Kit_CreateASSSubtitleRenderer(dec, w, h);
            } else {
                format->is_enabled = false;
            }
            break;
        case AV_CODEC_ID_DVD_SUBTITLE:
        case AV_CODEC_ID_DVB_SUBTITLE:
        case AV_CODEC_ID_HDMV_PGS_SUBTITLE:
        case AV_CODEC_ID_XSUB:
            ren = Kit_CreateImageSubtitleRenderer(dec, w, h);
            break;
        default:
            format->is_enabled = false;
            break;
    }
    if(ren == NULL) {
        Kit_SetError("Unrecognized subtitle format");
        goto exit_2;
    }

    // Allocate texture atlas for subtitle rectangles
    Kit_TextureAtlas *atlas = Kit_CreateAtlas(1024, 1024);
    if(atlas == NULL) {
        Kit_SetError("Unable to allocate subtitle texture atlas");
        goto exit_3;
    }

    // Set callbacks and userdata, and we're go
    subtitle_dec->format = format;
    subtitle_dec->renderer = ren;
    subtitle_dec->w = w;
    subtitle_dec->h = h;
    subtitle_dec->atlas = atlas;
    dec->dec_decode = dec_decode_subtitle_cb;
    dec->dec_close = dec_close_subtitle_cb;
    dec->userdata = subtitle_dec;
    return dec;

exit_3:
    Kit_CloseSubtitleRenderer(ren);
exit_2:
    free(subtitle_dec);
exit_1:
    Kit_CloseDecoder(dec);
exit_0:
    return NULL;
}

int Kit_GetSubtitleDecoderData(Kit_Decoder *dec, SDL_Texture *texture, SDL_Rect *sources, SDL_Rect *targets, int limit) {
    assert(dec != NULL);
    assert(texture != NULL);

    Kit_SubtitleDecoder *subtitle_dec = dec->userdata;
    double sync_ts = _GetSystemTime() - dec->clock_sync;

    // Tell the renderer to render content to atlas
    Kit_GetSubtitleRendererData(subtitle_dec->renderer, subtitle_dec->atlas, sync_ts);

    // Next, update the actual atlas texture contents. This may force the atlas to do partial or complete reordering
    // OR it may simply be a texture update from cache.
    if(Kit_UpdateAtlasTexture(subtitle_dec->atlas, texture) != 0) {
        return -1;
    }

    // Next, get targets and sources of visible atlas items
    int item_count = Kit_GetAtlasItems(subtitle_dec->atlas, sources, targets, limit);

    // All done
    return item_count;
}
