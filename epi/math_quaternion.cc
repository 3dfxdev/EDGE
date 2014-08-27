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

#include "epi.h"
#include "math_quaternion.h"

namespace epi
{

//*
mat4_c quat_c::ToMat4() const
{
	mat4_c mat;
	
	float xx = x*x, xy = x*y, xz = x*z, xw = x*w;
	float yy = y*y, yz = y*z, yw = y*w;
	float zz = z*z, zw = z*w;
	
	/*mat.m[0]  = 1 - 2 * (yy + zz);   mat.m[1]  =     2 * (xy - zw);   mat.m[2]  =     2 * (xz + yw);
	mat.m[4]  =     2 * ( xy + zw ); mat.m[5]  = 1 - 2 * ( xx + zz ); mat.m[6]  =     2 * ( yz - xw );
	mat.m[8]  =     2 * ( xz - yw ); mat.m[9]  =     2 * ( yz + xw ); mat.m[10] = 1 - 2 * ( xx + yy );
	mat.Transpose(); */
	
	mat.m[0] = 1 - 2 * (yy + zz); mat.m[4] =     2 * (xy - zw); mat.m[8]  =     2 * (xz + yw);
	mat.m[1] =     2 * (xy + zw); mat.m[5] = 1 - 2 * (xx + zz); mat.m[9]  =     2 * (yz - xw);
	mat.m[2] =     2 * (xz - yw); mat.m[6] =     2 * (yz + xw); mat.m[10] = 1 - 2 * (xx + yy);
	
	return mat;
} //*/
/*
mat4_c quat_c::ToMat4() const
{
	mat4_c mat;
	float d = x*x+y*y+z*z+w*w;
	
	if (d == 0)
		return mat;
	
	float s = 2.0 / d;
	float xs = x * s,   ys = y * s,   zs = z * s;
	float wx = w * xs,  wy = w * ys,  wz = w * zs;
	float xx = x * xs,  xy = x * ys,  xz = x * zs;
	float yy = y * ys,  yz = y * zs,  zz = z * zs;
	
	#define mat_elem(col,row) mat.m[row * 4 + col]
	mat_elem(0,0) = 1.0 - (yy + zz);
	mat_elem(0,1) =        xy - wz;
	mat_elem(0,2) =        xz + wy;
	mat_elem(1,0) =        xy + wz;
	mat_elem(1,1) = 1.0 - (xx + zz);
	mat_elem(1,2) =        yz - wx;
	mat_elem(2,0) =        xz - wy;
	mat_elem(2,1) =        yz + wx;
	mat_elem(2,2) = 1.0 - (xx + yy);
	#undef mat_elem
	
	return mat;
}//*/

/*
	from Slerping Clock Cycles
	http://fabiensanglard.net/doom3_documentation/37725-293747_293747.pdf
*/
static float ReciprocalSqrt( float x ) {
	long i;
	float y, r;
	y = x * 0.5f;
	i = *(long *)( &x );
	i = 0x5f3759df - ( i >> 1 );
	r = *(float *)( &i );
	r = r * ( 1.5f - r * r * y );
	return r;
}

static float SinZeroHalfPI( float a ) {
	// valid input range is [0, PI/2]
	float s, t;
	if (0) {
		s = a*a;
		t = ((((((((((((-2.39e-08f * s) + 2.7526e-06f) * s) - 1.98409e-04f) * s) + 8.3333315e-03f) * s) + 8.3333315e-03f) * s) - 1.666666664e-01f) * s) + 1) * a;
		//(-2.39e-08f + 2.7526e-06f) * (s + 2.7526e-06f)
	}
	s = a * a;
	t = -2.39e-08f;
	t *= s;
	t += 2.7526e-06f;
	t *= s;
	t += -1.98409e-04f;
	t *= s;
	t += 8.3333315e-03f;
	t *= s;
	t += -1.666666664e-01f;
	t *= s;
	t += 1.0f;
	t *= a;
	t = ((((((((((((-2.39e-08f * s) + 2.7526e-06f) * s) - 1.98409e-04f) * s) + 8.3333315e-03f) * s) + 8.3333315e-03f) * s) - 1.666666664e-01f) * s) + 1) * a;
	return t;
}

static float ATanPositive( float y, float x ) {
	// both inputs must be positive
	float a, d, s, t;
	if ( y > x ) {
		a = -x / y;
		d = M_PI / 2;
	} else {
		a = y / x;
		d = 0.0f;
	}
	s = a * a;
	t = 0.0028662257f;
	t *= s;
	t += -0.0161657367f;
	t *= s;
	t += 0.0429096138f;
	t *= s;
	t += -0.0752896400f;
	t *= s;
	t += 0.1065626393f;
	t *= s;
	t += -0.1420889944f;
	t *= s;
	t += 0.1999355085f;
	t *= s;
	t += -0.3333314528f;
	t *= s;
	t += 1.0f;
	t *= a;
	t += d;
	return t;
}

//void SlerpOptimized( const Quaternion &from, const Quaternion &to, float t, Quaternion &result ) {
quat_c quat_c::Slerp(const quat_c& to, float t) const
{
	float cosom, absCosom, sinom, sinSqr, omega, scale0, scale1;
	cosom = x*to.x + y*to.y + z*to.z + w*to.w;
	absCosom = fabs( cosom );
	if ( ( 1.0f - absCosom ) > 1e-6f ) {
		sinSqr = 1.0f - absCosom * absCosom;
		sinom = ReciprocalSqrt( sinSqr );
		omega = ATanPositive( sinSqr * sinom, absCosom );
		scale0 = SinZeroHalfPI( ( 1.0f - t ) * omega ) * sinom;
		scale1 = SinZeroHalfPI( t * omega ) * sinom;
	} else {
		scale0 = 1.0f - t;
		scale1 = t;
	}
	scale1 = ( cosom >= 0.0f ) ? scale1 : -scale1;
	
	quat_c result;
	result.x = scale0 * x + scale1 * to.x;
	result.y = scale0 * y + scale1 * to.y;
	result.z = scale0 * z + scale1 * to.z;
	result.w = scale0 * w + scale1 * to.w;
	return result.MakeUnit();
}

quat_c quat_c::Invert() const {
	return quat_c(-x,-y,-z,w);
}

quat_c quat_c::operator* (const quat_c& rhs) const {
	return quat_c(	w * rhs.x + x * rhs.w + y * rhs.z - z * rhs.y,
			w * rhs.y + y * rhs.w + z * rhs.x - x * rhs.z,
			w * rhs.z + z * rhs.w + x * rhs.y - y * rhs.x,
			w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z);
}

quat_c quat_c::operator* (const vec3_c& rhs) const {
	return quat_c(	w * rhs.x + y * rhs.z - z * rhs.y,
			w * rhs.y + z * rhs.x - x * rhs.z,
			w * rhs.z + x * rhs.y - y * rhs.x,
			-x * rhs.x - y * rhs.y - z * rhs.z);
}

vec3_c quat_c::Rotate (const vec3_c& v) const {
	//quat_c vq = quat_c(v.x, v.y, v.z, 0);
	
	//quat_c nq = (*this * vq) * Invert();
	//quat_c nq = (*this * vq) * Invert();
	quat_c nq = (*this * v) * Invert();
	
	return vec3_c(nq.x,nq.y,nq.z);
}

}  // namespace epi

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
