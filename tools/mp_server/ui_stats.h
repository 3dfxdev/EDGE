//------------------------------------------------------------------------
//  Statistics
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

#ifndef __UI_STATS_H__
#define __UI_STATS_H__

class UI_Stats : public Fl_Group
{
public:
	UI_Stats(int x, int y, int w, int h);
	virtual ~UI_Stats();

private:
	Fl_Output *clients;
	Fl_Output *queued;
	Fl_Output *played;

	Fl_Output *in_pks;
	Fl_Output *out_pks;
	Fl_Output *buf_pks;

	Fl_Output *in_bytes;
	Fl_Output *out_bytes;
	Fl_Output *buf_bytes;

public:
	void Update();
};

#endif /* __UI_STATS_H__ */
