/*
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Half-Life Model Renderer (Experimental) Copyright (C) 2001 James 'Ender' Brown [ender@quakesrc.org] This program is
    free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.
    This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
    warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
    details. You should have received a copy of the GNU General Public License along with this program; if not, write
    to the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. fromquake.h -
    
	model_hl.h - halflife model structure
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */

#include "../epi/file.h"

#include "r_defs.h"
#include "p_mobj.h"

// Opaque handle
class mdl_model_c;
mdl_model_c *MDL_LoadModel(epi::file_c *f);

/* HL mathlib prototypes: */
void	QuaternionGLAngle(const vec3_t angles, vec4_t quaternion);
void	QuaternionGLMatrix(float x, float y, float z, float w, vec4_t *GLM);
//void	UploadTexture(hlmdl_tex_t *ptexture, qbyte *data, qbyte *pal);

/* HL drawing */
//qboolean Mod_LoadHLModel (model_t *mod, void *buffer);
//void	R_DrawHLModel(entity_t	*curent);

/* physics stuff */
void *Mod_GetHalfLifeModelData(model_t *mod);
