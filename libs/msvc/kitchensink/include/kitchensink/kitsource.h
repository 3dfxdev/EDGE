#ifndef KITSOURCE_H
#define KITSOURCE_H

#include "kitchensink/kitconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

#define KIT_CODECNAMESIZE 32
#define KIT_CODECLONGNAMESIZE 128

typedef enum Kit_StreamType {
    KIT_STREAMTYPE_UNKNOWN,   ///< Unknown stream type
    KIT_STREAMTYPE_VIDEO,     ///< Video stream
    KIT_STREAMTYPE_AUDIO,     ///< Audio stream
    KIT_STREAMTYPE_DATA,      ///< Data stream
    KIT_STREAMTYPE_SUBTITLE,  ///< Subtitle streawm
    KIT_STREAMTYPE_ATTACHMENT ///< Attachment stream (images, etc)
} Kit_StreamType;

typedef struct Kit_Source {
    int audio_stream_index;     ///< Audio stream index
    int video_stream_index;     ///< Video stream index
    int subtitle_stream_index;  ///< Subtitle stream index
    void *format_ctx;           ///< FFmpeg: Videostream format context
    unsigned char *read_buffer; ///< custom IO read buffer
} Kit_Source;

typedef struct Kit_StreamInfo {
    int index;           ///< Stream index
    Kit_StreamType type; ///< Stream type
} Kit_StreamInfo;

KIT_API Kit_Source* Kit_CreateSourceFromUrl(const char *path);
KIT_API Kit_Source* Kit_CreateSourceFromCio(int (*ReadFunc)(void*, unsigned char*, int), long long (*SeekFunc)(void*, long long, int), void *UserPtr, int BufferSize);
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
