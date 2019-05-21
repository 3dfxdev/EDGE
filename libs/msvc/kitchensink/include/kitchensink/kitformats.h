#ifndef KITFORMATS_H
#define KITFORMATS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

typedef struct Kit_AudioFormat {
    int stream_index; ///< Stream index
    bool is_enabled; ///< Is stream enabled
    unsigned int format; ///< SDL Audio Format
    bool is_signed; ///< Signedness
    int bytes; ///< Bytes per sample per channel
    int samplerate; ///< Sampling rate
    int channels; ///< Channels
} Kit_AudioFormat;

typedef struct Kit_VideoFormat {
    int stream_index; ///< Stream index
    bool is_enabled; ///< Is stream enabled
    unsigned int format; ///< SDL Pixel Format
    int width; ///< Width in pixels
    int height; ///< Height in pixels
} Kit_VideoFormat;

typedef struct Kit_SubtitleFormat {
    int stream_index; ///< Stream index
    bool is_enabled; ///< Is stream enabled
    unsigned int format; ///< SDL Pixel Format
} Kit_SubtitleFormat;

#ifdef __cplusplus
}
#endif

#endif // KITFORMATS_H
