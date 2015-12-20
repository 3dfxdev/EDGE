//----------------------------------------------------------------------------
//  3DGE OPL Class Header
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2015 Isotope SoftWorks
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
//

#ifndef OPL_H
#define OPL_H

// Abstract base class for OPL emulators

class OPLEmul
{
public:
	OPLEmul() {}
	virtual ~OPLEmul() {}

	virtual void Reset() = 0;
	virtual void WriteReg(int reg, int v) = 0;
	virtual void Update(float *buffer, int length) = 0;
	virtual void SetPanning(int c, float left, float right) = 0;
};

OPLEmul *NukedOPL3Create(bool stereo);

#define OPL_SAMPLE_RATE			49716.0
#define CENTER_PANNING_POWER	0.70710678118


#endif
