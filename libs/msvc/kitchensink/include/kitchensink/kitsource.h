#ifndef KITSOURCE_H
#define KITSOURCE_H

#include <inttypes.h>
#include "kitchensink/kitconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

#define KIT_CODECNAMESIZE 32
#define KIT_CODECLONGNAMESIZE 128

typedef enum Kit_StreamType {
    KIT_STREAMTYPE_UNKNOWN, ///< Unknown stream type
    KIT_STREAMTYPE_VIDEO, ///< Video stream
    KIT_STREAMTYPE_AUDIO, ///< Audio stream
    KIT_STREAMTYPE_DATA, ///< Data stream
    KIT_STREAMTYPE_SUBTITLE, ///< Subtitle streawm
    KIT_STREAMTYPE_ATTACHMENT ///< Attachment stream (images, etc)
} Kit_StreamType;

typedef struct Kit_Source {
    int audio_stream_index;    ///< Audio stream index
    int video_stream_index;    ///< Video stream index
    int subtitle_stream_index; ///< Subtitle stream index
    void *format_ctx;          ///< FFmpeg: Videostream format context
    void *avio_ctx;            ///< FFmpeg: AVIO context
    uint8_t *avio_buf;   ///< Buffer for custom AVIO source
} Kit_Source;

typedef struct Kit_StreamInfo {
    int index; ///< Stream index
    Kit_StreamType type; ///< Stream type
} Kit_StreamInfo;

typedef int (*Kit_ReadCallback)(void*, uint8_t*, int);
typedef int64_t (*Kit_SeekCallback)(void*, int64_t, int);

KIT_API Kit_Source* Kit_CreateSourceFromUrl(const char *path);
KIT_API Kit_Source* Kit_CreateSourceFromCustom(Kit_ReadCallback read_cb, Kit_SeekCallback seek_cb, void *userdata);
KIT_API void Kit_CloseSource(Kit_Source *src);

KIT_API int Kit_GetSourceStreamInfo(const Kit_Source *src, Kit_StreamInfo *info, int index);
KIT_API int Kit_GetSourceStreamCount(const Kit_Source *src);
KIT_API int Kit_GetBestSourceStream(const Kit_Source *src, const Kit_StreamType type);
KIT_API int Kit_SetSourceStream(Kit_Source *src, const Kit_StreamType type, int index);
KIT_API int Kit_GetSourceStream(const Kit_Source *src, const Kit_StreamType type);

#ifdef __cplusplus
}
#endif

#endif // KITSOURCE_H
