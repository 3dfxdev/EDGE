//------------------------------------------------------------------------
//  Information Bar (bottom of window)
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2007-2009 Andrew Apted
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

#ifndef __UI_PIC_H__
#define __UI_PIC_H__


class Img;


class UI_Pic : public Fl_Box
{
private:
	Fl_RGB_Image *rgb;

	bool unknown;

public:
	UI_Pic(int X, int Y, int W, int H);
	virtual ~UI_Pic();

public:
	void Nil();

	void GetFlat(const wad_flat_name_t& fname);
	void GetTex (const wad_tex_name_t& tname);
	void GetSprite(const wad_ttype_t& type);

	// FLTK virtual method for drawing.
	int handle(int event);

private:
	// FLTK virtual method for drawing.
	void draw();

	void UploadRGB(const byte *buf, int depth);

	void TiledImg(Img *img);
};


#endif // __UI_PIC_H__

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
