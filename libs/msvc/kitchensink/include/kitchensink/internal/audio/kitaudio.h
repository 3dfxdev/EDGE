#ifndef KITAUDIO_H
#define KITAUDIO_H

#include "kitchensink/kitconfig.h"
#include "kitchensink/kitformats.h"
#include "kitchensink/kitplayer.h"
#include "kitchensink/internal/kitdecoder.h"

KIT_LOCAL Kit_Decoder* Kit_CreateAudioDecoder(const Kit_Source *src, Kit_AudioFormat *format);
KIT_LOCAL int Kit_GetAudioDecoderData(Kit_Decoder *dec, unsigned char *buf, int len);

#endif // KITAUDIO_H
