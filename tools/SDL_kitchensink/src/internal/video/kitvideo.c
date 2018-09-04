#include <assert.h>

#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>

#include "kitchensink/kiterror.h"
#include "kitchensink/internal/kitdecoder.h"
#include "kitchensink/internal/utils/kithelpers.h"
#include "kitchensink/internal/utils/kitbuffer.h"
#include "kitchensink/internal/video/kitvideo.h"
#include "kitchensink/internal/utils/kitlog.h"

#define KIT_VIDEO_OUT_SIZE 2
#define KIT_VIDEO_SYNC_THRESHOLD 0.01

typedef struct Kit_VideoDecoder {
    Kit_VideoFormat *format;
    struct SwsContext *sws;
    AVFrame *scratch_frame;
} Kit_VideoDecoder;

typedef struct Kit_VideoPacket {
    double pts;
    AVFrame *frame;
} Kit_VideoPacket;


static Kit_VideoPacket* _CreateVideoPacket(AVFrame *frame, double pts) {
    Kit_VideoPacket *p = calloc(1, sizeof(Kit_VideoPacket));
    p->frame = frame;
    p->pts = pts;
    return p;
}

static unsigned int _FindPixelFormat(enum AVPixelFormat fmt) {
    switch(fmt) {
        case AV_PIX_FMT_YUV420P9:
        case AV_PIX_FMT_YUV420P10:
        case AV_PIX_FMT_YUV420P12:
        case AV_PIX_FMT_YUV420P14:
        case AV_PIX_FMT_YUV420P16:
        case AV_PIX_FMT_YUV420P:
            return SDL_PIXELFORMAT_YV12;
        case AV_PIX_FMT_YUYV422:
            return SDL_PIXELFORMAT_YUY2;
        case AV_PIX_FMT_UYVY422:
            return SDL_PIXELFORMAT_UYVY;
        default:
            return SDL_PIXELFORMAT_RGBA32;
    }
}

static enum AVPixelFormat _FindAVPixelFormat(unsigned int fmt) {
    switch(fmt) {
        case SDL_PIXELFORMAT_IYUV: return AV_PIX_FMT_YUV420P;
        case SDL_PIXELFORMAT_YV12: return AV_PIX_FMT_YUV420P;
        case SDL_PIXELFORMAT_YUY2: return AV_PIX_FMT_YUYV422;
        case SDL_PIXELFORMAT_UYVY: return AV_PIX_FMT_UYVY422;
        case SDL_PIXELFORMAT_ARGB32: return AV_PIX_FMT_BGRA;
        case SDL_PIXELFORMAT_RGBA32: return AV_PIX_FMT_RGBA;
        default:
            return AV_PIX_FMT_NONE;
    }
}

static void free_out_video_packet_cb(void *packet) {
    Kit_VideoPacket *p = packet;
    av_freep(&p->frame->data[0]);
    av_frame_free(&p->frame);
    free(p);
}

static void dec_decode_video_cb(Kit_Decoder *dec, AVPacket *in_packet) {
    assert(dec != NULL);
    assert(in_packet != NULL);
    
    Kit_VideoDecoder *video_dec = dec->userdata;
    int frame_finished;
    

    while(in_packet->size > 0) {
        int len = avcodec_decode_video2(dec->codec_ctx, video_dec->scratch_frame, &frame_finished, in_packet);
        if(len < 0) {
            return;
        }

        if(frame_finished) {
            // Target frame
            AVFrame *out_frame = av_frame_alloc();
            av_image_alloc(
                    out_frame->data,
                    out_frame->linesize,
                    dec->codec_ctx->width,
                    dec->codec_ctx->height,
                    _FindAVPixelFormat(video_dec->format->format),
                    1);

            // Scale from source format to target format, don't touch the size
            sws_scale(
                video_dec->sws,
                (const unsigned char * const *)video_dec->scratch_frame->data,
                video_dec->scratch_frame->linesize,
                0,
                dec->codec_ctx->height,
                out_frame->data,
                out_frame->linesize);

            // Get presentation timestamp
            double pts = av_frame_get_best_effort_timestamp(video_dec->scratch_frame);
            pts *= av_q2d(dec->format_ctx->streams[dec->stream_index]->time_base);

            // Lock, write to audio buffer, unlock
            Kit_VideoPacket *out_packet = _CreateVideoPacket(out_frame, pts);
            Kit_WriteDecoderOutput(dec, out_packet);
        }
        in_packet->size -= len;
        in_packet->data += len;
    }
}

static void dec_close_video_cb(Kit_Decoder *dec) {
    if(dec == NULL) return;

    Kit_VideoDecoder *video_dec = dec->userdata;
    if(video_dec->scratch_frame != NULL) {
        av_frame_free(&video_dec->scratch_frame);
    }
    if(video_dec->sws != NULL) {
        sws_freeContext(video_dec->sws);
    }
    free(video_dec);
}

Kit_Decoder* Kit_CreateVideoDecoder(const Kit_Source *src, Kit_VideoFormat *format) {
    assert(src != NULL);
    assert(format != NULL);
    if(src->video_stream_index < 0) {
        return NULL;
    }

    // First the generic decoder component ...
    Kit_Decoder *dec = Kit_CreateDecoder(
        src, src->video_stream_index,
        KIT_VIDEO_OUT_SIZE,
        free_out_video_packet_cb);
    if(dec == NULL) {
        goto exit_0;
    }

    // Find formats
    format->is_enabled = true;
    format->width = dec->codec_ctx->width;
    format->height = dec->codec_ctx->height;
    format->stream_index = src->video_stream_index;
    format->format = _FindPixelFormat(dec->codec_ctx->pix_fmt);

    // ... then allocate the video decoder
    Kit_VideoDecoder *video_dec = calloc(1, sizeof(Kit_VideoDecoder));
    if(video_dec == NULL) {
        goto exit_1;
    }

    // Create temporary video frame
    video_dec->scratch_frame = av_frame_alloc();
    if(video_dec->scratch_frame == NULL) {
        Kit_SetError("Unable to initialize temporary video frame");
        goto exit_2;
    }

    // Create scaler for handling format changes
    video_dec->sws = sws_getContext(
        dec->codec_ctx->width, // Source w
        dec->codec_ctx->height, // Source h
        dec->codec_ctx->pix_fmt, // Source fmt
        dec->codec_ctx->width, // Target w
        dec->codec_ctx->height, // Target h
        _FindAVPixelFormat(format->format), // Target fmt
        SWS_BILINEAR,
        NULL, NULL, NULL);
    if(video_dec->sws == NULL) {
        Kit_SetError("Unable to initialize video converter context");
        goto exit_3;
    }

    // Set callbacks and userdata, and we're go
    video_dec->format = format;
    dec->dec_decode = dec_decode_video_cb;
    dec->dec_close = dec_close_video_cb;
    dec->userdata = video_dec;
    return dec;

exit_3:
    av_frame_free(&video_dec->scratch_frame);
exit_2:
    free(video_dec);
exit_1:
    Kit_CloseDecoder(dec);
exit_0:
    return NULL;
}

int Kit_GetVideoDecoderData(Kit_Decoder *dec, SDL_Texture *texture) {
    assert(dec != NULL);
    assert(texture != NULL);

    Kit_VideoPacket *packet = Kit_PeekDecoderOutput(dec);
    if(packet == NULL) {
        return 0;
    }

    Kit_VideoDecoder *video_dec = dec->userdata;
    double sync_ts = _GetSystemTime() - dec->clock_sync;

    // Check if we want the packet
    if(packet->pts > sync_ts + KIT_VIDEO_SYNC_THRESHOLD) {
        // Video is ahead, don't show yet.
        return 0;
    } else if(packet->pts < sync_ts - KIT_VIDEO_SYNC_THRESHOLD) {
        // Video is lagging, skip until we find a good PTS to continue from.
        while(packet != NULL) {
            Kit_AdvanceDecoderOutput(dec);
            free_out_video_packet_cb(packet);
            packet = Kit_PeekDecoderOutput(dec);
            if(packet == NULL) {
                break;
            } else {
                dec->clock_pos = packet->pts;
            }
            if(packet->pts > sync_ts - KIT_VIDEO_SYNC_THRESHOLD) {
                break;
            }
        }
    }

    // If we have no viable packet, just skip
    if(packet == NULL) {
        return 0;
    }

    // Update output texture with current video data.
    // Take formats into account.
    switch(video_dec->format->format) {
        case SDL_PIXELFORMAT_YV12:
        case SDL_PIXELFORMAT_IYUV:
            SDL_UpdateYUVTexture(
                texture, NULL, 
                packet->frame->data[0], packet->frame->linesize[0],
                packet->frame->data[1], packet->frame->linesize[1],
                packet->frame->data[2], packet->frame->linesize[2]);
            break;
        default:
            SDL_UpdateTexture(
                texture, NULL,
                packet->frame->data[0],
                packet->frame->linesize[0]);
            break;
    }

    // Advance buffer, and free the decoded frame.
    Kit_AdvanceDecoderOutput(dec);
    dec->clock_pos = packet->pts;
    free_out_video_packet_cb(packet);

    return 0;
}
