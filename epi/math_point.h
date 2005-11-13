//------------------------------------------------------------------------
//  EPI Point types
//------------------------------------------------------------------------
//
//  Copyright (c) 2004-2005  The EDGE Team.
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

#ifndef __EPI_MATH_POINT__
#define __EPI_MATH_POINT__

#include "macros.h"
#include "math_vector.h"

namespace epi
{

class ipos_c
{
	/* sealed class, value semantics */

public:
	int x, y;

	ipos_c() : x(0), y(0) { }
	ipos_c(int nx, int ny) : x(nx), y(ny) { }
	ipos_c(const ipos_c& rhs) : x(rhs.x), y(rhs.y) { }
	ipos_c(const ivec_c& rhs) : x(rhs.x), y(rhs.y) { }

	/* ---- read-only operations ---- */

	bool operator==(const ipos_c& rhs) const;
	bool operator!=(const ipos_c& rhs) const;

	ipos_c operator+ (const ivec_c& rhs) const;
	ipos_c operator- (const ivec_c& rhs) const;

	/* ---- modifying operations ---- */

	ipos_c& operator= (const ipos_c& rhs);

	ipos_c& operator+= (const ivec_c& rhs);
	ipos_c& operator-= (const ivec_c& rhs);
};

class pos2_c
{
	/* sealed class, value semantics */

public:
	float x, y;

	pos2_c() : x(0), y(0) { }
	pos2_c(float nx, float ny) : x(nx), y(ny) { }
	pos2_c(const pos2_c& rhs)  : x(rhs.x), y(rhs.y) { }
	pos2_c(const ipos_c& rhs)  : x(rhs.x), y(rhs.y) { }
	pos2_c(const vec2_c& rhs)  : x(rhs.x), y(rhs.y) { }

	/* ---- read-only operations ---- */

	bool Match(const pos2_c& rhs, float precision = 0.001f);
	// no equality operators since we're using floating point.

	pos2_c operator+ (const vec2_c& rhs) const;
	pos2_c operator- (const vec2_c& rhs) const;

	/* ---- modifying operations ---- */

	pos2_c& operator= (const pos2_c& rhs);

	pos2_c& operator+= (const vec2_c& rhs);
	pos2_c& operator-= (const vec2_c& rhs);
};

class pos3_c
{
	/* sealed class, value semantics */

public:
	float x, y, z;

	pos3_c() : x(0), y(0), z(0) { }
	pos3_c(float nx, float ny, float nz) : x(nx), y(ny), z(nz) { }
	pos3_c(const pos3_c& rhs) : x(rhs.x), y(rhs.y), z(rhs.z) { }
	pos3_c(const vec3_c& rhs) : x(rhs.x), y(rhs.y), z(rhs.z) { }
	pos3_c(const pos2_c& horiz, float nz) : x(horiz.x), y(horiz.y), z(nz) { }

	/* ---- read-only operations ---- */

	bool Match(const pos3_c& rhs, float precision = 0.001f);
	// no equality operators since we're using floating point.

	pos2_c Get2D() const;
	// return the 2D position (dropping the z coord).

	pos3_c operator+ (const vec3_c& rhs) const;
	pos3_c operator- (const vec3_c& rhs) const;

	/* ---- modifying operations ---- */

	pos3_c& operator= (const pos3_c& rhs);

	pos3_c& operator+= (const vec3_c& rhs);
	pos3_c& operator-= (const vec3_c& rhs);
};

//------------------------------------------------------------------------
//  IMPLEMENTATION
//------------------------------------------------------------------------

inline bool ipos_c::operator==(const ipos_c& rhs) const
{
	return (rhs.x == x) && (rhs.y == y);
}

inline bool ipos_c::operator!=(const ipos_c& rhs) const
{
	return (rhs.x != x) || (rhs.y != y);
}

inline ipos_c ipos_c::operator+ (const ivec_c& rhs) const
{
	return ipos_c(x + rhs.x, y + rhs.y);
}

inline ipos_c ipos_c::operator- (const ivec_c& rhs) const
{
	return ipos_c(x - rhs.x, y - rhs.y);
}

inline ipos_c& ipos_c::operator= (const ipos_c& rhs)
{
	x = rhs.x;  y = rhs.y;
	return *this;
}

inline ipos_c& ipos_c::operator+= (const ivec_c& rhs)
{
	x += rhs.x;  y += rhs.y;
	return *this;
}

inline ipos_c& ipos_c::operator-= (const ivec_c& rhs)
{
	x -= rhs.x;  y -= rhs.y;
	return *this;
}


//------------------------------------------------------------------------

inline bool pos2_c::Match(const pos2_c& rhs, float precision)
{
	return fabs(x - rhs.x) < precision && fabs(y - rhs.y) < precision;
}

inline pos2_c pos2_c::operator+ (const vec2_c& rhs) const
{
	return pos2_c(x + rhs.x, y + rhs.y);
}

inline pos2_c pos2_c::operator- (const vec2_c& rhs) const
{
	return pos2_c(x - rhs.x, y - rhs.y);
}

inline pos2_c& pos2_c::operator= (const pos2_c& rhs)
{
	x = rhs.x;  y = rhs.y;
	return *this;
}

inline pos2_c& pos2_c::operator+= (const vec2_c& rhs)
{
	x += rhs.x;  y += rhs.y;
	return *this;
}

inline pos2_c& pos2_c::operator-= (const vec2_c& rhs)
{
	x -= rhs.x;  y -= rhs.y;
	return *this;
}


//------------------------------------------------------------------------

inline bool pos3_c::Match(const pos3_c& rhs, float precision)
{
	return fabs(x - rhs.x) < precision &&
		   fabs(y - rhs.y) < precision &&
		   fabs(z - rhs.z) < precision;
}

inline pos2_c pos3_c::Get2D() const
{
	return pos2_c(x, y);
}

inline pos3_c pos3_c::operator+ (const vec3_c& rhs) const
{
	return pos3_c(x + rhs.x, y + rhs.y, z + rhs.z);
}

inline pos3_c pos3_c::operator- (const vec3_c& rhs) const
{
	return pos3_c(x - rhs.x, y - rhs.y, z - rhs.z);
}

inline pos3_c& pos3_c::operator= (const pos3_c& rhs)
{
	x = rhs.x;  y = rhs.y;  z = rhs.z;
	return *this;
}

inline pos3_c& pos3_c::operator+= (const vec3_c& rhs)
{
	x += rhs.x;  y += rhs.y;  z += rhs.z;
	return *this;
}

inline pos3_c& pos3_c::operator-= (const vec3_c& rhs)
{
	x -= rhs.x;  y -= rhs.y;  z -= rhs.z;
	return *this;
}

}  // namespace epi

#endif  /* __EPI_MATH_POINT__ */
