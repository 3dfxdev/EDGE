#ifndef KITERROR_H
#define KITERROR_H

#include "kitchensink/kitconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

KIT_API const char* Kit_GetError();
KIT_API void Kit_SetError(const char* fmt, ...);
KIT_API void Kit_ClearError();

#ifdef __cplusplus
}
#endif

#endif // KITERROR_H
