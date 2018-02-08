//----------------------------------------------------------------------------
//  EDGE2 System Specific Header
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2009  The EDGE2 Team.
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

#ifndef __SYSTEM_SPECIFIC_DEFS__
#define __SYSTEM_SPECIFIC_DEFS__

#include "../../epi/epi.h"

#include "../con_var.h"
#include "i_system.h"

#ifdef HAVE_PHYSFS
#include <physfs.h>
#endif

/// `CA- 6.5.2016: quick hacks to change these in Visual Studio (less warnings). 
#ifdef _MSC_VER
#define strdup _strdup
#define stricmp _stricmp
#define strnicmp _strnicmp
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#define uint unsigned int
#define ALIGN_8(x)							__declspec(align(8)) x
#define ALIGN_16(x)							__declspec(align(16)) x
#define ALIGN_32(x)							__declspec(align(32)) x
#pragma warning( disable : 4099)
#elif defined MACOSX
typedef unsigned int uint;
#endif
// Template for LINUX like MSVC 
#elif defined LINUX 
#define ALIGN_STRUCT(x)    __attribute__((aligned(x)))
#endif

// Uncomment the line below to enable SIMD_X64 intricies for ROQ playback via i_cinematic.cc (disabled by default!)
//#define SIMD_X64
#ifdef SIMD_X64

#include <xmmintrin.h>
#include <emmintrin.h>

#define _mm_muladd_ss(A, B, C)				_mm_add_ss(_mm_mul_ss((A), (B)), (C))
#define _mm_muladd_ps(A, B, C)				_mm_add_ps(_mm_mul_ps((A), (B)), (C))

#define _mm_mulsub_ss(A, B, C)				_mm_sub_ss((C), _mm_mul_ss((A), (B)))
#define _mm_mulsub_ps(A, B, C)				_mm_sub_ps((C), _mm_mul_ps((A), (B)))

#define _mm_clamp_ss(A, B, C)				_mm_min_ss(_mm_max_ss((A), (B)), (C))
#define _mm_clamp_ps(A, B, C)				_mm_min_ps(_mm_max_ps((A), (B)), (C))

#define _mm_splat_ps(A, I)					_mm_shuffle_ps((A), (A), _MM_SHUFFLE(I, I, I, I))
#define _mm_splat_epi32(A, I)				_mm_shuffle_epi32((A), _MM_SHUFFLE(I, I, I, I))


#endif /*__SYSTEM_SPECIFIC_DEFS__*/

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
