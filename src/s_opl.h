#ifndef __S_OPL_H__
#define __S_OPL_H__

#include "system/i_defs.h"

bool S_StartupOPL(void);
void S_ShutdownOPL(void);

abstract_music_c * S_PlayOPL(byte *data, int length, float volume, bool loop);

#endif
