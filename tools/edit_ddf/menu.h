//------------------------------------------------------------------------
//  MENU : Menu handling
//------------------------------------------------------------------------
//
//  Editor for DDF   (C) 2005  The EDGE Team
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//------------------------------------------------------------------------

#ifndef __EDITDDF_MENU_H__
#define __EDITDDF_MENU_H__

#define MANUAL_WINDOW_MIN_W  500
#define MANUAL_WINDOW_MIN_H  200

#ifdef MACOSX
Fl_Sys_Menu_Bar
#else
Fl_Menu_Bar
#endif
* MenuCreate(int x, int y, int w, int h);

void GUI_PrintMsg(const char *str, ...);

#endif /* __EDITDDF_MENU_H__ */
