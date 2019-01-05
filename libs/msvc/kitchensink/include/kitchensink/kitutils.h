#ifndef KITUTILS_H
#define KITUTILS_H

#include "kitchensink/kitconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

KIT_API const char* Kit_GetSDLAudioFormatString(unsigned int type);
KIT_API const char* Kit_GetSDLPixelFormatString(unsigned int type);
KIT_API const char* Kit_GetKitStreamTypeString(unsigned int type);

#ifdef __cplusplus
}
#endif

#endif // KITUTILS_H
