//----------------------------------------------------------------------------
//  EDGE UNIX UI System Code (Error messages etc...)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2009  The EDGE Team.
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
//----------------------------------------------------------------------------
#include "i_defs.h"
#include "i_local.h"

#ifdef USE_FLTK

// remove some problematic #defines
#undef VISIBLE
#undef INVISIBLE

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/fl_ask.H>
#endif

//
// I_MessageBox
//
// Generate a message box displaying a notice and 'OK' button.
//
void I_MessageBox(const char *_msg, const char *_title)
{
#ifdef USE_FLTK
	Fl::scheme(NULL);
	fl_message_font(FL_HELVETICA /*_BOLD*/, 18);	
	fl_message("%s", _msg);
#else // USE_FLTK
	fprintf(stderr, "\n%s\n", _msg);
#endif // USE_FLTK
}
