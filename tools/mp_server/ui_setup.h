//------------------------------------------------------------------------
//  Setup Screen
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

#ifndef __UI_SETUP_H__
#define __UI_SETUP_H__

class UI_Setup : public Fl_Group
{
public:
	UI_Setup(int x, int y, int w, int h);
	virtual ~UI_Setup();

private:
	Fl_Output *address;
	Fl_Output *port;


public:
};

#endif /* __UI_SETUP_H__ */
