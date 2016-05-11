//---------------------------------------------------------------------------
//  3DGE OpenGL Shadows (Simple)
// 
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
//
// DESCRIPTION: Simple Shadows for 3DGE
//    Based on Prboom+:
//    using the Risen3D implementation which in turn is based on Doomsday v1.7.8.
//    Uses SDL!
//---------------------------------------------------------------------------

#include "r_shadows.h"

 // i_video:
 #include "i_defs_gl.h"
#include "i_defs.h"
#include "i_sdlinc.h"

#include "p_local.h"  // P_ApproxDistance
#include "r_gldefs.h"
#include "m_argv.h"
#include "m_misc.h"
#include "r_modes.h"
#include "r_units.h"
#include "w_wad.h" //Lump Lookups

/// NOTES
/// g l d_* in this file has been replaced with R_* in accordance to 
/// 3DGE coding standards.

/* #include "gl_opengl.h"
#include "gl_intern.h"
#include "doomstat.h"
#include "p_maputl.h"
#include "w_wad.h"
#include "r_bsp.h"
#include "r_sky.h"
#include "lprintf.h" */


//shadows


int gl_shadows_maxdist;
int gl_shadows_factor;

simple_shadow_params_t simple_shadows =
{
  0, 0,
  -1, 0, 0,
  80, 1000, 0.5f, 0.0044f
};

//===========================================================================
// RGL_InitShadows:
//	The dynamic light map is a 64x64 grayscale 8-bit image.
//===========================================================================
void RGL_InitShadows(void)
{
  //int lump;

  
  simple_shadows.loaded = false;

  simple_shadows.tex_id = -1;
  simple_shadows.width = 0;
  simple_shadows.height = 0;

  simple_shadows.max_radius = 80;
  simple_shadows.max_dist = gl_shadows_maxdist;
  simple_shadows.factor = (float)gl_shadows_factor / 256.0f;
  simple_shadows.bias = 0.0044f;
  
  int lump = W_CheckNumForName("GLSHADOW"/**/);
  
  const void *data = W_CacheLumpNum(lump);
  if (lump != -1)
  {
    SDL_PixelFormat fmt;
    SDL_Surface *surf = NULL;
    SDL_Surface *surf_raw;
    surf_raw = SDL_LoadBMP_RW(SDL_RWFromConstMem(W_CacheLumpNum(lump), W_LumpLength(lump)), 1); //rafactor W_
	
    W_DoneWithLump(data); //

    fmt = *surf_raw->format;
    fmt.BitsPerPixel = 24;
    fmt.BytesPerPixel = 3;

    surf = SDL_ConvertSurface(surf_raw, &fmt, surf_raw->flags);
    SDL_FreeSurface(surf_raw);
    if (surf)
    {
      glGenTextures(1, &simple_shadows.tex_id);
      glBindTexture(GL_TEXTURE_2D, simple_shadows.tex_id);

      glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, surf->w, surf->h, 0, GL_RGB, GL_UNSIGNED_BYTE, surf->pixels);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
/*       if (gl_ext_texture_filter_anisotropic)
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, (GLfloat)(1<<gl_texture_filter_anisotropic)); */

      simple_shadows.loaded = true;
      simple_shadows.width = surf->w;
      simple_shadows.height = surf->h;

      SDL_FreeSurface(surf);
    }
  }

  if (simple_shadows.enable && !simple_shadows.loaded)
  {
    I_Printf("RGL_InitShadows: failed to initialise shadow texture!");
  }
}

static void RGL_DrawShadow(GLShadow *shadow)
{
  glColor3f(shadow->light, shadow->light, shadow->light);

  glBegin(GL_TRIANGLE_FAN);

  glTexCoord2f(1.0f, 0.0f);
  glVertex3f(shadow->x + shadow->radius, shadow->z, shadow->y - shadow->radius);
  glTexCoord2f(0.0f, 0.0f);
  glVertex3f(shadow->x - shadow->radius, shadow->z, shadow->y - shadow->radius);
  glTexCoord2f(0.0f, 1.0f);
  glVertex3f(shadow->x - shadow->radius, shadow->z, shadow->y + shadow->radius);
  glTexCoord2f(1.0f, 1.0f);
  glVertex3f(shadow->x + shadow->radius, shadow->z, shadow->y + shadow->radius);
  
/*   glVertex3f(seg->v1->x, seg->v1->y, h); */

  glEnd();
}

//===========================================================================
// Rend_RenderShadows
//===========================================================================
void R_RenderShadows(void)
{
  int i;

/*   if (!simple_shadows.enable || !simple_shadows.loaded || players[displayplayer].fixedcolormap)
    return;

  if (R_drawinfo.num_items[GLDIT_SHADOW] <= 0)
    return;

  if (!gl_ztrick)
  {
    glDepthRange(simple_shadows.bias, 1);
  } */

  //gl_EnableFog(false);

  glDepthMask(GL_FALSE);
  glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);

  // Apply a modelview shift.
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();

  // Scale towards the viewpoint to avoid Z-fighting.

  glTranslatef(viewx, viewy, viewz);
  glScalef(0.99f, 0.99f, 0.99f);
  glTranslatef(-viewx, -viewy, -viewz);//glTranslatef(-xCamera, -zCamera, -yCamera);

  glBindTexture(GL_TEXTURE_2D, simple_shadows.tex_id);
  ///??? R_ResetLastTexture();
  
  RGL_DrawShadow(GLShadow *shadow);

///  if (!gl_ztrick)
///  {
///    glDepthRange(0, 1);
///  }

  glPopMatrix();
  glDepthMask(GL_TRUE);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
