/*
Copyright (C) 2002 John R. Hall

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
//***************************************************************************
//
//    byteordr.c - Byte order conversion routines.
//
//***************************************************************************

//#include "stdafx.h"
#include "../src/system/i_defs.h"
#include "../epi/endianess.h"
#include "rt_byteordr.h"

#define DEFINE_CONVERTER(type)            \
	void Cvt_##type(void *lmp, int num) { }

//#define DEFINE_CONVERTER(type)            \
//void Cvt_##type(void *lmp, int num)       \
//{                                         \
//    int i;                                \
//    type *recs = (type *)lmp;             \
//    for (i = 0; i < num; i++, recs++) {   \
//        CONVERT_ENDIAN_##type(recs);      \
//    }                                     \
//}

DEFINE_CONVERTER(pic_t);
DEFINE_CONVERTER(lpic_t);
DEFINE_CONVERTER(font_t);
DEFINE_CONVERTER(lbm_t);
DEFINE_CONVERTER(rottpatch_t);
DEFINE_CONVERTER(transpatch_t);
DEFINE_CONVERTER(cfont_t);


void CvtNull(void *lmp, int num)
{
    I_Debugf("No-op endian converter on %p.\n", lmp);
}

void CvtFixme(void *lmp, int num)
{
	I_Debugf("FIXME endian converter on %p.\n", lmp);
}
