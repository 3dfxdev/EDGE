///Interpolation header for includes. . .
//
//-----------------------------------------------------------------------------
//
// DESCRIPTION:
//    Generic Interpolation header.
//
//-----------------------------------------------------------------------------

#ifndef R_LERP_H__
#define R_LERP_H__

#include "i_defs.h"
#include "n_network.h"


struct prevpos_t
{
   float x;
   float y;
   float z;
   angle_t angle;
};

/* inline fixed_t lerpCoord(fixed_t lerp, fixed_t oldpos, fixed_t newpos)
{
   return oldpos + FixedMul(lerp, newpos - oldpos);
} */
extern float N_GetInterpolater(void);
// Uses epi::vec3_c for epi:: class table.
inline float lerpCoord(float interp, float lastpos, float newpos)
{
	epi::vec3_c lastpos(lastticrender.x,lastticrender.y,lastticrender.z);
	epi::vec3_c curpos(x,y,z);
	///float interp = N_GetInterpolater();
	return lastpos.Lerp(curpos, interp);
};
#endif