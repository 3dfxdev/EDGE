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
#ifndef OPL_OPLCONFIG_H
#define OPL_OPLCONFIG_H

typedef struct _opl_config {
    int             samplerate;
    bool            opl3mode;
    bool            stereo;
} opl_config;

#endif
