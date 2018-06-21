#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>

#include "kitchensink/kitsource.h"
#include "kitchensink/kiterror.h"
#include "kitchensink/internal/utils/kitlog.h"

#define AVIO_BUF_SIZE 32768

Kit_Source* Kit_CreateSourceFromUrl(const char *url) {
    assert(url != NULL);

    Kit_Source *src = calloc(1, sizeof(Kit_Source));
    if(src == NULL) {
        Kit_SetError("Unable to allocate source");
        return NULL;
    }

    // Attempt to open source
    if(avformat_open_input((AVFormatContext **)&src->format_ctx, url, NULL, NULL) < 0) {
        Kit_SetError("Unable to open source Url");
        goto exit_0;
    }

    av_opt_set_int(src->format_ctx, "probesize", INT_MAX, 0);
    av_opt_set_int(src->format_ctx, "analyzeduration", INT_MAX, 0);

    // Fetch stream information. This may potentially take a while.
    if(avformat_find_stream_info((AVFormatContext *)src->format_ctx, NULL) < 0) {
        Kit_SetError("Unable to fetch source information");
        goto exit_1;
    }

    // Find best streams for defaults
    src->audio_stream_index = Kit_GetBestSourceStream(src, KIT_STREAMTYPE_AUDIO);
    src->video_stream_index = Kit_GetBestSourceStream(src, KIT_STREAMTYPE_VIDEO);
    src->subtitle_stream_index = Kit_GetBestSourceStream(src, KIT_STREAMTYPE_SUBTITLE);
    return src;

exit_1:
    avformat_close_input((AVFormatContext **)&src->format_ctx);
exit_0:
    free(src);
    return NULL;
}

Kit_Source* Kit_CreateSourceFromCustom(Kit_ReadCallback read_cb, Kit_SeekCallback seek_cb, void *userdata) {
    assert(read_cb != NULL);

    Kit_Source *src = calloc(1, sizeof(Kit_Source));
    if(src == NULL) {
        Kit_SetError("Unable to allocate source");
        return NULL;
    }

    uint8_t *avio_buf = av_malloc(AVIO_BUF_SIZE);
    if(avio_buf == NULL) {
        Kit_SetError("Unable to allocate avio buffer");
        goto exit_0;
    }

    AVFormatContext *format_ctx = avformat_alloc_context();
    if(format_ctx == NULL) {
        Kit_SetError("Unable to allocate format context");
        goto exit_1;
    }

    AVIOContext *avio_ctx = avio_alloc_context(
        avio_buf, AVIO_BUF_SIZE, 0, userdata, read_cb, 0, seek_cb);
    if(avio_ctx == NULL) {
        Kit_SetError("Unable to allocate avio context");
        goto exit_2;
    }

    // Set the format as AVIO format
    format_ctx->pb = avio_ctx;

    // Attempt to open source
    if(avformat_open_input(&format_ctx, "", NULL, NULL) < 0) {
        Kit_SetError("Unable to open custom source");
        goto exit_3;
    }

    // Set probe opts for input scanning
    av_opt_set_int(src->format_ctx, "probesize", INT_MAX, 0);
    av_opt_set_int(src->format_ctx, "analyzeduration", INT_MAX, 0);

    // Fetch stream information. This may potentially take a while.
    if(avformat_find_stream_info(format_ctx, NULL) < 0) {
        Kit_SetError("Unable to fetch source information");
        goto exit_4;
    }

    // Set internals
    src->format_ctx = format_ctx;
    src->avio_ctx = avio_ctx;
    src->avio_buf = avio_buf;

    // Find best streams for defaults
    src->audio_stream_index = Kit_GetBestSourceStream(src, KIT_STREAMTYPE_AUDIO);
    src->video_stream_index = Kit_GetBestSourceStream(src, KIT_STREAMTYPE_VIDEO);
    src->subtitle_stream_index = Kit_GetBestSourceStream(src, KIT_STREAMTYPE_SUBTITLE);
    return src;

exit_4:
    avformat_close_input(&format_ctx);
    free(src);
    return NULL;

exit_3:
    av_free(avio_ctx);
exit_2:
    avformat_free_context(format_ctx);
exit_1:
    av_free(avio_buf);
exit_0:
    free(src);
    return NULL;
}

void Kit_CloseSource(Kit_Source *src) {
    assert(src != NULL);
    avformat_close_input((AVFormatContext **)&src->format_ctx);
    free(src);
}

int Kit_GetSourceStreamInfo(const Kit_Source *src, Kit_StreamInfo *info, int index) {
    assert(src != NULL);
    assert(info != NULL);

    AVFormatContext *format_ctx = (AVFormatContext *)src->format_ctx;
    if(index < 0 || index >= format_ctx->nb_streams) {
        Kit_SetError("Invalid stream index");
        return 1;
    }

    AVStream *stream = format_ctx->streams[index];
    switch(stream->codec->codec_type) {
        case AVMEDIA_TYPE_UNKNOWN: info->type = KIT_STREAMTYPE_UNKNOWN; break;
        case AVMEDIA_TYPE_DATA: info->type = KIT_STREAMTYPE_DATA; break;
        case AVMEDIA_TYPE_VIDEO: info->type = KIT_STREAMTYPE_VIDEO; break;
        case AVMEDIA_TYPE_AUDIO: info->type = KIT_STREAMTYPE_AUDIO; break;
        case AVMEDIA_TYPE_SUBTITLE: info->type = KIT_STREAMTYPE_SUBTITLE; break;
        case AVMEDIA_TYPE_ATTACHMENT: info->type = KIT_STREAMTYPE_ATTACHMENT; break;
        default:
            Kit_SetError("Unknown native stream type");
            return 1;
    }

    info->index = index;
    return 0;
}

int Kit_GetBestSourceStream(const Kit_Source *src, const Kit_StreamType type) {
    assert(src != NULL);
    int avmedia_type = 0;
    switch(type) {
        case KIT_STREAMTYPE_VIDEO: avmedia_type = AVMEDIA_TYPE_VIDEO; break;
        case KIT_STREAMTYPE_AUDIO: avmedia_type = AVMEDIA_TYPE_AUDIO; break;
        case KIT_STREAMTYPE_SUBTITLE: avmedia_type = AVMEDIA_TYPE_SUBTITLE; break;
        default: return -1;
    }
    int ret = av_find_best_stream((AVFormatContext *)src->format_ctx, avmedia_type, -1, -1, NULL, 0);
    if(ret == AVERROR_STREAM_NOT_FOUND) {
        return -1;
    }
    if(ret == AVERROR_DECODER_NOT_FOUND) {
        Kit_SetError("Unable to find a decoder for the stream");
        return 1;
    }
    return ret;
}

int Kit_SetSourceStream(Kit_Source *src, const Kit_StreamType type, int index) {
    assert(src != NULL);
    switch(type) {
        case KIT_STREAMTYPE_AUDIO: src->audio_stream_index = index; break;
        case KIT_STREAMTYPE_VIDEO: src->video_stream_index = index; break;
        case KIT_STREAMTYPE_SUBTITLE: src->subtitle_stream_index = index; break;
        default:
            Kit_SetError("Invalid stream type");
            return 1;
    }
    return 0;
}

int Kit_GetSourceStream(const Kit_Source *src, const Kit_StreamType type) {
    assert(src != NULL);
    switch(type) {
        case KIT_STREAMTYPE_AUDIO: return src->audio_stream_index;
        case KIT_STREAMTYPE_VIDEO: return src->video_stream_index;
        case KIT_STREAMTYPE_SUBTITLE: return src->subtitle_stream_index;
        default:
            break;
    }
    return -1;
}

int Kit_GetSourceStreamCount(const Kit_Source *src) {
    assert(src != NULL);
    return ((AVFormatContext *)src->format_ctx)->nb_streams;
}
