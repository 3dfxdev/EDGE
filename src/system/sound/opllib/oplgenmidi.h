//
// Copyright (C) 2016 Alexey Khokholov (Nuke.YKT)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//
//  MUS/MIDI player.
//
// version: 1.0
//
#ifndef OPL_OPLGENMIDI_H
#define OPL_OPLGENMIDI_H
#include "opldef.h"

typedef struct _opl_genmidi_operator {
    Bit8u mult;
    Bit8u attack;
    Bit8u sustain;
    Bit8u wave;
    Bit8u ksl;
    Bit8u level;
    Bit8u feedback;
} opl_genmidi_operator;

typedef struct _opl_genmidi_voice {
    opl_genmidi_operator mod;
    opl_genmidi_operator car;
    Bit16s offset;
} opl_genmidi_voice;

typedef struct _opl_genmidi_patch {
    Bit16u flags;
    Bit8u finetune;
    Bit8u note;
    opl_genmidi_voice voice[2];
} opl_genmidi_patch;

typedef struct _opl_genmidi {
    char header[8];
    opl_genmidi_patch patch[175];
} opl_genmidi;

#define OPL_GENMIDI_FIXED   0x01
#define OPL_GENMIDI_DUAL    0x04

#endif
