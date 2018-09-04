#include <assert.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <libavformat/avformat.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
#include <SDL2/SDL.h>

#include "kitchensink/kiterror.h"
#include "kitchensink/internal/utils/kithelpers.h"
#include "kitchensink/internal/utils/kitbuffer.h"
#include "kitchensink/internal/audio/kitaudio.h"
#include "kitchensink/internal/utils/kitringbuffer.h"
#include "kitchensink/internal/utils/kitlog.h"

#define KIT_AUDIO_OUT_SIZE 64
#define AUDIO_SYNC_THRESHOLD 0.05

typedef struct Kit_AudioDecoder {
    Kit_AudioFormat *format;
    SwrContext *swr;
    AVFrame *scratch_frame;
} Kit_AudioDecoder;

typedef struct Kit_AudioPacket {
    double pts;
    size_t original_size;
    Kit_RingBuffer *rb;
} Kit_AudioPacket;


Kit_AudioPacket* _CreateAudioPacket(const char* data, size_t len, double pts) {
    Kit_AudioPacket *p = calloc(1, sizeof(Kit_AudioPacket));
    p->rb = Kit_CreateRingBuffer(len);
    Kit_WriteRingBuffer(p->rb, data, len);
    p->pts = pts;
    return p;
}

enum AVSampleFormat _FindAVSampleFormat(int format) {
    switch(format) {
        case AUDIO_U8: return AV_SAMPLE_FMT_U8;
        case AUDIO_S16SYS: return AV_SAMPLE_FMT_S16;
        case AUDIO_S32SYS: return AV_SAMPLE_FMT_S32;
        default: return AV_SAMPLE_FMT_NONE;
    }
}

int64_t _FindAVChannelLayout(int channels) {
    switch(channels) {
        case 1: return AV_CH_LAYOUT_MONO;
        case 2: return AV_CH_LAYOUT_STEREO;
        case 4: return AV_CH_LAYOUT_QUAD;
        case 6: return AV_CH_LAYOUT_5POINT1;
        default: return AV_CH_LAYOUT_STEREO_DOWNMIX;
    }
}

int _FindChannelLayout(uint64_t channel_layout) {
    switch(channel_layout) {
        case AV_CH_LAYOUT_MONO: return 1;
        case AV_CH_LAYOUT_STEREO: return 2;
        case AV_CH_LAYOUT_QUAD: return 4;
        case AV_CH_LAYOUT_5POINT1: return 6;
        default: return 2;
    }
}

void _FindAudioFormat(enum AVSampleFormat fmt, int *bytes, bool *is_signed, unsigned int *format) {
    switch(fmt) {
        case AV_SAMPLE_FMT_U8:
            *bytes = 1;
            *is_signed = false;
            *format = AUDIO_U8;
            break;
        case AV_SAMPLE_FMT_S32:
            *bytes = 4;
            *is_signed = true;
            *format = AUDIO_S32SYS;
            break;
        case AV_SAMPLE_FMT_S16:
        default:
            *bytes = 2;
            *is_signed = true;
            *format = AUDIO_S16SYS;
            break;
    }
}

static void free_out_audio_packet_cb(void *packet) {
    Kit_AudioPacket *p = packet;
    Kit_DestroyRingBuffer(p->rb);
    free(p);
}

static void dec_decode_audio_cb(Kit_Decoder *dec, AVPacket *in_packet) {
    assert(dec != NULL);
    assert(in_packet != NULL);

    Kit_AudioDecoder *audio_dec = dec->userdata;
    int frame_finished;
    int len, len2;
    int dst_linesize;
    int dst_nb_samples, dst_bufsize;
    unsigned char **dst_data;

    // Decode as long as there is data
    while(in_packet->size > 0) {
        len = avcodec_decode_audio4(dec->codec_ctx, audio_dec->scratch_frame, &frame_finished, in_packet);
        if(len < 0) {
            return;
        }

        if(frame_finished) {
            dst_nb_samples = av_rescale_rnd(
                audio_dec->scratch_frame->nb_samples,
                audio_dec->format->samplerate,  // Target samplerate
                dec->codec_ctx->sample_rate,  // Source samplerate
                AV_ROUND_UP);

            av_samples_alloc_array_and_samples(
                &dst_data,
                &dst_linesize,
                audio_dec->format->channels,
                dst_nb_samples,
                _FindAVSampleFormat(audio_dec->format->format),
                0);

            len2 = swr_convert(
                audio_dec->swr,
                dst_data,
                audio_dec->scratch_frame->nb_samples,
                (const unsigned char **)audio_dec->scratch_frame->extended_data,
                audio_dec->scratch_frame->nb_samples);

            dst_bufsize = av_samples_get_buffer_size(
                &dst_linesize,
                audio_dec->format->channels,
                len2,
                _FindAVSampleFormat(audio_dec->format->format), 1);

            // Get presentation timestamp
            double pts = av_frame_get_best_effort_timestamp(audio_dec->scratch_frame);
            pts *= av_q2d(dec->format_ctx->streams[dec->stream_index]->time_base);

            // Lock, write to audio buffer, unlock
            Kit_AudioPacket *out_packet = _CreateAudioPacket(
                (char*)dst_data[0], (size_t)dst_bufsize, pts);
            Kit_WriteDecoderOutput(dec, out_packet);

            // Free temps
            av_freep(&dst_data[0]);
            av_freep(&dst_data);
        }

        in_packet->size -= len;
        in_packet->data += len;
    }
}

static void dec_close_audio_cb(Kit_Decoder *dec) {
    if(dec == NULL) return;

    Kit_AudioDecoder *audio_dec = dec->userdata;
    if(audio_dec->scratch_frame != NULL) {
        av_frame_free(&audio_dec->scratch_frame);
    }
    if(audio_dec->swr != NULL) {
        swr_free(&audio_dec->swr);
    }
    free(audio_dec);
}

Kit_Decoder* Kit_CreateAudioDecoder(const Kit_Source *src, Kit_AudioFormat *format) {
    assert(src != NULL);
    assert(format != NULL);
    if(src->audio_stream_index < 0) {
        return NULL;
    }

    // First the generic decoder component ...
    Kit_Decoder *dec = Kit_CreateDecoder(
        src, src->audio_stream_index,
        KIT_AUDIO_OUT_SIZE,
        free_out_audio_packet_cb);
    if(dec == NULL) {
        goto exit_0;
    }

    // Find formats
    format->samplerate = dec->codec_ctx->sample_rate;
    format->is_enabled = true;
    format->stream_index = src->audio_stream_index;
    format->channels = _FindChannelLayout(dec->codec_ctx->channel_layout);
    _FindAudioFormat(dec->codec_ctx->sample_fmt, &format->bytes, &format->is_signed, &format->format);

    // ... then allocate the audio decoder
    Kit_AudioDecoder *audio_dec = calloc(1, sizeof(Kit_AudioDecoder));
    if(audio_dec == NULL) {
        goto exit_1;
    }

    // Create temporary audio frame
    audio_dec->scratch_frame = av_frame_alloc();
    if(audio_dec->scratch_frame == NULL) {
        Kit_SetError("Unable to initialize temporary audio frame");
        goto exit_2;
    }

    // Create resampler
    audio_dec->swr = swr_alloc_set_opts(
        NULL,
        _FindAVChannelLayout(format->channels), // Target channel layout
        _FindAVSampleFormat(format->format), // Target fmt
        format->samplerate, // Target samplerate
        dec->codec_ctx->channel_layout, // Source channel layout
        dec->codec_ctx->sample_fmt, // Source fmt
        dec->codec_ctx->sample_rate, // Source samplerate
        0, NULL);

    if(swr_init(audio_dec->swr) != 0) {
        Kit_SetError("Unable to initialize audio resampler context");
        goto exit_3;
    }

    // Set callbacks and userdata, and we're go
    audio_dec->format = format;
    dec->dec_decode = dec_decode_audio_cb;
    dec->dec_close = dec_close_audio_cb;
    dec->userdata = audio_dec;
    return dec;

exit_3:
    av_frame_free(&audio_dec->scratch_frame);
exit_2:
    free(audio_dec);
exit_1:
    Kit_CloseDecoder(dec);
exit_0:
    return NULL;
}

int Kit_GetAudioDecoderData(Kit_Decoder *dec, unsigned char *buf, int len) {
    assert(dec != NULL);

    Kit_AudioPacket *packet = Kit_PeekDecoderOutput(dec);
    if(packet == NULL) {
        return 0;
    }

    int ret = 0;
    Kit_AudioDecoder *audio_dec = dec->userdata;
    int bytes_per_sample = audio_dec->format->bytes * audio_dec->format->channels;
    double bytes_per_second = bytes_per_sample * audio_dec->format->samplerate;
    double sync_ts = _GetSystemTime() - dec->clock_sync;

    if(packet->pts > sync_ts + AUDIO_SYNC_THRESHOLD) {
        return 0;
    } else if(packet->pts < sync_ts - AUDIO_SYNC_THRESHOLD) {
        // Audio is lagging, skip until good pts is found
        while(1) {
            Kit_AdvanceDecoderOutput(dec);
            free_out_audio_packet_cb(packet);
            packet = Kit_PeekDecoderOutput(dec);
            if(packet == NULL) {
                break;
            } else {
                dec->clock_pos = packet->pts;
            }
            if(packet->pts > sync_ts - AUDIO_SYNC_THRESHOLD) {
                break;
            }
        }
    }

    // If we have no viable packet, just skip
    if(packet == NULL) {
        return 0;
    }

    // Read data from packet ringbuffer
    if(len > 0) {
        ret = Kit_ReadRingBuffer(packet->rb, (char*)buf, len);
    }

    // If ringbuffer is cleared, kill packet and advance buffer.
    // Otherwise forward the pts value for the current packet.
    if(Kit_GetRingBufferLength(packet->rb) == 0) {
        Kit_AdvanceDecoderOutput(dec);
        dec->clock_pos = packet->pts;
        free_out_audio_packet_cb(packet);
    } else {
        packet->pts += ((double)ret) / bytes_per_second;
        dec->clock_pos = packet->pts;
    }

    return ret;
}
