//------------------------------------------------------------------------
//  GRID STUFF
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

#ifndef __GRID_2_H__
#define __GRID_2_H__


class Grid_State_c
{
public:
	bool shown;

	// if true, objects forced to be on the grid
	bool snap;

	// if true, grid step does not automatically change when zoom changes
	bool locked;

	// minimum grid step in pixels when not locked
	int pixels_min;

	// map coordinates for centre of canvas
	int orig_x;
	int orig_y;

	// scale for drawing map
	float Scale;

	int step;

public:
	Grid_State_c();
	virtual ~Grid_State_c();

public:
	inline bool isShown() const
	{
		return shown;
	}

	// change the view so that the map coordinates (x, y)
	// appear at the centre of the window
	void CenterMapAt(int x, int y);

	// return X/Y coordinate snapped to grid
	// (or unchanged is snap_to_grid is off)
	int SnapX(int mapx) const;
	int SnapY(int mapy) const;

	// interface with UI_Infobar widget...
	static const char *scale_options();
	static const char *grid_options();

	void ScaleFromWidget(int i);
	void  StepFromWidget(int i);

	// compute new grid step from current scale
	void StepFromScale();

	// increase or decrease the grid size.  The 'delta' parameter
	// is positive to increase it, negative to decrease it.
	void AdjustStep(int delta);
	void AdjustScale(int delta);

	// choose the scale nearest to (and less than) the wanted one
	void NearestScale(double want_scale);

	void ScaleFromDigit(int digit);

	// move the origin so that the focus point of the last zoom
	// operation (scale change) is map_x/y.
	void RefocusZoom(int map_x, int map_y, float before_Scale);

private:
	static const double scale_values[];
	static const int digit_scales[];
	static const int grid_values[];
};

extern Grid_State_c grid;


#endif // __GRID_2_H__

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
