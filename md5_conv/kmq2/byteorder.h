//from quake 2

#ifndef _BYTEORDER_H_
#define _BYTEORDER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "q2limits.h"

typedef long long qint64;

short   BigShort(short l);
short   LittleShort(short l);
int     BigLong (int l);
int     LittleLong (int l);
qint64  BigLong64 (qint64 l);
qint64  LittleLong64 (qint64 l);
float   BigFloat (float l);
float   LittleFloat (float l);

void    Swap_Init (void);

#ifdef __cplusplus
}
#endif

#endif
 