#ifndef KITLIB_H
#define KITLIB_H

#include "kitchensink/kiterror.h"
#include "kitchensink/kitsource.h"
#include "kitchensink/kitplayer.h"
#include "kitchensink/kitutils.h"
#include "kitchensink/kitconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Kit_Version {
    unsigned char major;
    unsigned char minor;
    unsigned char patch;
} Kit_Version;

enum {
    KIT_INIT_NETWORK = 0x1,
    KIT_INIT_ASS = 0x2
};

KIT_API int Kit_Init(unsigned int flags);
KIT_API void Kit_Quit();
KIT_API void Kit_GetVersion(Kit_Version *version);

#ifdef __cplusplus
}
#endif

#endif // KITLIB_H
