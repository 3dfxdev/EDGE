//------------------------------------------------------------------------
//  INCLUDES
//------------------------------------------------------------------------
//
//  Edge MultiPlayer Server (C) 2005  Andrew Apted
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

#ifndef __MP_INCLUDES_H__
#define __MP_INCLUDES_H__

/* C library */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>

/* STL stuffage */

#include <string>
#include <vector>
#include <list>
#include <map>
#include <bitset>
#include <algorithm>
#include <functional>

/* FLTK - Widget Library */

#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Counter.H>
// #include <FL/Fl_Double_Window.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Image.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Int_Input.H>
#include <FL/Fl_Light_Button.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Multiline_Output.H>
#include <FL/Fl_Multi_Browser.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Pixmap.H>
#include <FL/Fl_Progress.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Round_Button.H>
#include <FL/Fl_Scrollbar.H>
#include <FL/Fl_Slider.H>
#ifdef MACOSX
#include <FL/Fl_Sys_Menu_Bar.H>
#endif
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Value_Input.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Window.H>

#include <FL/fl_ask.H>
#include <FL/fl_draw.H>
#include <FL/fl_file_chooser.H>

#include "FL/Fl_Table.H"
#include "FL/Fl_Table_Row.H"

/* HawkNL - Network Library */

#include "hawknl1.68/include/nl.h"

/* Our own system defs */

#include "sys_type.h"
#include "sys_macro.h"
#include "sys_assert.h"
#include "sys_debug.h"
#include "sys_endian.h"

#endif /* __MP_INCLUDES_H__ */
