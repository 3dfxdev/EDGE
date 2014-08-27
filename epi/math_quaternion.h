//------------------------------------------------------------------------
//  EPI Quaternion type
//------------------------------------------------------------------------
//
//  Copyright (c) 2004-2008  The EDGE Team.
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

#ifndef __EPI_MATH_QUATERNION__
#define __EPI_MATH_QUATERNION__

#include <math.h>
#include "macros.h"
#include "math_angle.h"
#include "math_matrix.h"
#include "math_vector.h"

namespace epi
{

class quat_c : public vec4_c
{
public:
	
	quat_c() : vec4_c(0,0,0,1) { }
	quat_c(float nx, float ny, float nz, float nw) : vec4_c(nx,ny,nz,nw) { }
	
	quat_c(float nx, float ny, float nz) : vec4_c(nx,ny,nz,0) {
		w = 1.0f - (nx * nx) - (ny * ny) - (nz * nz);
		w = (w < 0) ? 0 : -sqrtf(w);
	}
	
	quat_c(const vec3_c &rhs) : vec4_c(rhs, 0) {
		w = 1.0f - (rhs.x * rhs.x) - (rhs.y * rhs.y) - (rhs.z * rhs.z);
		w = (w < 0) ? 0 : -sqrtf(w);
	}
	//create quaternion from eular rotation
	quat_c(angle_c yaw, angle_c pitch, angle_c roll) {
		float halfYaw = yaw.Radians() * 0.5;
		float halfPitch = pitch.Radians() * 0.5;
		float halfRoll = roll.Radians() * 0.5;
		double cosYaw, sinYaw;
		double cosPitch, sinPitch;
		double cosRoll, sinRoll;
		#ifdef USE_SINCOS
			sincos(halfYaw,&sinYaw,&cosYaw);
			sincos(halfPitch,&sinPitch&cosPitch);
			sincos(halfRoll,&sinRoll,&cosRoll);
		#else
			sinYaw = sin(halfYaw);
			cosYaw = cos(halfYaw);
			sinPitch = sin(halfPitch);
			cosPitch = cos(halfPitch);
			sinRoll = sin(halfRoll);
			cosRoll = cos(halfRoll);
		#endif
		if (1) {
			x = cosRoll * sinPitch * cosYaw + sinRoll * cosPitch * sinYaw;
			y = cosRoll * cosPitch * sinYaw - sinRoll * sinPitch * cosYaw;
			z = sinRoll * cosPitch * cosYaw - cosRoll * sinPitch * sinYaw;
			w = cosRoll * cosPitch * cosYaw + sinRoll * sinPitch * sinYaw;
		} else {
			x = sinRoll * sinPitch * cosYaw + cosRoll * cosPitch * sinYaw;
			y = sinRoll * cosPitch * cosYaw + cosRoll * sinPitch * sinYaw;
			z = sinRoll * cosPitch * sinYaw + cosRoll * sinPitch * cosYaw;
			w = cosRoll * cosPitch * cosYaw - sinRoll * sinPitch * sinYaw;
		}
	}
	
	quat_c(const mat3_c& rhs);
	quat_c(const mat4_c& rhs);
	inline quat_c& MakeUnit();
	
	quat_c Slerp(const quat_c& target, float weight) const;
	mat4_c ToMat4() const;
	
	quat_c Invert() const;
	//rotate quaternion with another quaternion
	quat_c operator* (const quat_c& rhs) const;
	quat_c operator* (const vec3_c& rhs) const;
	quat_c operator*= (const quat_c& rhs) const;
	//rotate vector with quaternion
	vec3_c Rotate (const vec3_c& rhs) const;
};

inline quat_c& quat_c::MakeUnit()
{
	*this /= Length();
	return *this;
}

}  // namespace epi

#endif  /* __EPI_MATH_QUATERNION__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
