//----------------------------------------------------------------------------
//  EDGE OpenGL Rendering (Things)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2007  The EDGE Team.
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
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------

#include "i_defs.h"
#include "i_defs_gl.h"

#include <math.h>

#include "epi/image_data.h"
#include "epi/image_jpeg.h"

#include "dm_data.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "p_local.h"
#include "r_colormap.h"
#include "r_defs.h"
#include "r_draw.h"
#include "r_gldefs.h"
#include "r_image.h"
#include "r_md2.h"
#include "r_misc.h"
#include "r_modes.h"
#include "r_shader.h"
#include "r_texgl.h"
#include "r_units.h"
#include "st_stuff.h"
#include "w_model.h"
#include "w_sprite.h"

#include "w_wad.h"  // fuzz test
#include "m_misc.h"  // !!!! model test


#define DEBUG  0


float sprite_skew;

angle_t fuzz_ang_tl;
angle_t fuzz_ang_tr;
angle_t fuzz_ang_bl;
angle_t fuzz_ang_br;

float fuzz_yoffset;

GLuint fuzz_tex;

// colour of the player's weapon
int rgl_weapon_r;
int rgl_weapon_g;
int rgl_weapon_b;


// The minimum distance between player and a visible sprite.
#define MINZ        (4.0f)


static GLuint MakeFuzzTexture(void)
{
	// FIXME !!!!!  verify lump size
	const byte *fuzz = (const byte*)W_CacheLumpName("FUZZMAP7");

	epi::image_data_c img(256, 256, 4);

	img.Clear();

	for (int y = 0; y < 256; y++)
	for (int x = 0; x < 256; x++)
	{
		byte *dest = img.PixelAt(x, y);

		dest[3] = fuzz[y*256 + x];
	}

	W_DoneWithLump(fuzz);
	
	return R_UploadTexture(&img, NULL, UPL_NONE);
}

void RGL_UpdateTheFuzz(void)
{
	// fuzzy warping effect

	fuzz_ang_tl += FLOAT_2_ANG(90.0f / 17.0f);
	fuzz_ang_tr += FLOAT_2_ANG(90.0f / 11.0f);
	fuzz_ang_bl += FLOAT_2_ANG(90.0f /  8.0f);
	fuzz_ang_br += FLOAT_2_ANG(90.0f / 21.0f);

	fuzz_yoffset = ((framecount * 3) & 255) / 256.0;

	if (! fuzz_tex)
		fuzz_tex = MakeFuzzTexture();
}

static float GetHoverDZ(mobj_t *mo)
{
	// compute a different phase for different objects
	angle_t phase = (angle_t)(long)mo;
	phase ^= (angle_t)(phase << 19);
	phase += (angle_t)(leveltime << (ANGLEBITS-6));

	return M_Sin(phase) * 4.0f;
}

typedef struct
{
///---	float R, G, B;

	vec3_t vert[4];
	vec2_t texc[4];
	vec3_t lit_pos;

	multi_color_c col[4];
}
psprite_coord_data_t;


static void PSpriteCoordFunc(void *d, int v_idx,
		vec3_t *pos, float *rgb, vec2_t *texc,
		vec3_t *normal, vec3_t *lit_pos)
{
	const psprite_coord_data_t *data = (psprite_coord_data_t *)d;

	*pos     = data->vert[v_idx];
	*texc    = data->texc[v_idx];
	*lit_pos = data->lit_pos;

///---	rgb[0] = data->R;
///---	rgb[1] = data->G;
///---	rgb[2] = data->B;

	normal->Set(0, 0, 1);
}

static void DLIT_PSprite(mobj_t *mo, void *dataptr)
{
	psprite_coord_data_t *data = (psprite_coord_data_t *)dataptr;

	SYS_ASSERT(mo->dlight.shader);

	mo->dlight.shader->Sample(data->col + 0,
			data->lit_pos.x, data->lit_pos.y, data->lit_pos.z);
}

static int GetMulticolMaxRGB(multi_color_c *cols, int num, bool additive)
{
	int result = 0;

	for (; num > 0; num--, cols++)
	{
		int mx = additive ? cols->add_MAX() : cols->mod_MAX();

		result = MAX(result, mx);
	}
	
	return result;
}


static void RGL_DrawPSprite(pspdef_t * psp, int which,
							player_t * player, region_properties_t *props,
							const state_t *state)
{
	if (state->flags & SFF_Model)
		return;

	// determine sprite patch
	bool flip;
	const image_c *image = R2_GetOtherSprite(state->sprite, state->frame, &flip);

	if (!image)
		return;

	GLuint tex_id = W_ImageCache(image, false);

	float w = IM_WIDTH(image);
	float h = IM_HEIGHT(image);
	float right = IM_RIGHT(image);
	float top = IM_TOP(image);

	int fuzzy = (player->mo->flags & MF_FUZZY);

	float trans = fuzzy ? FUZZY_TRANS : player->mo->visibility;

	if (which == ps_crosshair)
	{
		fuzzy = false;
		trans = 1.0f;
	}

	trans *= psp->visibility;

	if (trans < 0.04f)
		return;

	// psprites are never totally solid and opaque
#if 0
	glEnable(GL_ALPHA_TEST);
	if (trans <= 0.99 || use_smoothing)
		glEnable(GL_BLEND);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tex_id);
#endif

	float tex_top_h = top;  ///## 1.00f; // 0.98;
	float tex_bot_h = 0.0f; ///## 1.00f - top;  // 1.02 - bottom;

	float tex_x1 = 0.01f;
	float tex_x2 = right - 0.01f;

	if (flip)
	{
		tex_x1 = right - tex_x1;
		tex_x2 = right - tex_x2;
	}

	float tx1 = (161.0f - IM_WIDTH(image) / 2.0) + psp->sx - IM_OFFSETX(image);
	float tx2 = tx1 + w;

	float ty1 = - psp->sy + IM_OFFSETY(image);
	float ty2 = ty1 + h;


	float x1b, y1b, x1t, y1t, x2b, y2b, x2t, y2t;  // screen coords

	x1b = x1t = viewwindowwidth  * tx1 / 320.0f;
	x2b = x2t = viewwindowwidth  * tx2 / 320.0f;

	y1b = y2b = viewwindowheight * ty1 / 200.0f;
	y1t = y2t = viewwindowheight * ty2 / 200.0f;

	if (false) //!!!! fuzzy)
	{
		float range_x = (float)fabs(x2b - x1b) / 12.0f;
		float range_y = (float)fabs(y1t - y1b) / 12.0f;

		float tl_x = M_Sin(fuzz_ang_tl);
		float tr_x = M_Sin(fuzz_ang_tr);
		float bl_x = M_Sin(fuzz_ang_bl);
		float br_x = M_Sin(fuzz_ang_br);

		float tl_y = M_Cos(fuzz_ang_tl);
		float tr_y = M_Cos(fuzz_ang_tr);
    
		// don't adjust the bottom Y positions
    
		x1b += bl_x * range_x;
		x1t += tl_x * range_x; y1t += tl_y * range_y;
		x2t += tr_x * range_x; y2t += tr_y * range_y;
		x2b += br_x * range_x;

	}

	// clip psprite to view window
	glEnable(GL_SCISSOR_TEST);

	glScissor(viewwindowx, SCREENHEIGHT-viewwindowheight-viewwindowy,
			  viewwindowwidth, viewwindowheight);

	x1b = (float)viewwindowx + x1b;
	x1t = (float)viewwindowx + x1t;
	x2t = (float)viewwindowx + x2t;
	x2b = (float)viewwindowx + x2b;

	y1b = (float)(SCREENHEIGHT - viewwindowy - viewwindowheight) + y1b - 1;
	y1t = (float)(SCREENHEIGHT - viewwindowy - viewwindowheight) + y1t - 1;
	y2t = (float)(SCREENHEIGHT - viewwindowy - viewwindowheight) + y2t - 1;
	y2b = (float)(SCREENHEIGHT - viewwindowy - viewwindowheight) + y2b - 1;

	int blending = BL_Masked; //!!!!!


	psprite_coord_data_t data;

///---	data.R = fuzzy ? 0.2 : 1;
///---	data.G = fuzzy ? 0.2 : 1;
///---	data.B = fuzzy ? 0.2 : 1;

	data.vert[0].Set(x1b, y1b, 0);
	data.vert[1].Set(x1t, y1t, 0);
	data.vert[2].Set(x2t, y1t, 0);
	data.vert[3].Set(x2b, y2b, 0);
		
	data.texc[0].Set(tex_x1, tex_bot_h);
	data.texc[1].Set(tex_x1, tex_top_h);
	data.texc[2].Set(tex_x2, tex_top_h);
	data.texc[3].Set(tex_x2, tex_bot_h);

	float away = 120.0;

	data.lit_pos.x = player->mo->x + viewcos * away;
	data.lit_pos.y = player->mo->y + viewsin * away;
	data.lit_pos.z = player->mo->z + player->mo->height *
		PERCENT_2_FLOAT(player->mo->info->shotheight);


	RGL_StartUnits(false);


	int v_idx;

	data.col[0].Clear();

	if (! fuzzy)
	{
		abstract_shader_c *shader = R_GetColormapShader(props, state->bright);

		shader->Sample(data.col + 0, data.lit_pos.x, data.lit_pos.y, data.lit_pos.z);

		if (use_dlights)
		{
			data.lit_pos.x = player->mo->x + viewcos * 24;
			data.lit_pos.y = player->mo->y + viewsin * 24;

			float r = 96;

			P_DynamicLightIterator(
				data.lit_pos.x - r, data.lit_pos.y - r, player->mo->z,
				data.lit_pos.x + r, data.lit_pos.y + r, player->mo->z + player->mo->height,
				DLIT_PSprite, &data);

			P_SectorGlowIterator(player->mo->subsector->sector,
				data.lit_pos.x - r, data.lit_pos.y - r, player->mo->z,
				data.lit_pos.x + r, data.lit_pos.y + r, player->mo->z + player->mo->height,
				DLIT_PSprite, &data);
		}
	}

	// FIXME: sample at least TWO points (left and right edges)
	data.col[1] = data.col[0];
	data.col[2] = data.col[0];
	data.col[3] = data.col[0];

	for (int pass = 0; pass < 1; pass++) //!!!!!! pass < 4
	{
		if (pass > 0 && pass < 3 && GetMulticolMaxRGB(data.col, 4, false) <= 0)
			continue;

		if (pass == 3 && GetMulticolMaxRGB(data.col, 4, true) <= 0)
			continue;

		if (pass >= 1)
		{
			blending &= ~BL_Alpha;
			blending |=  BL_Add;
		}

		bool is_additive = (pass == 3);

#if 0
		local_gl_vert_t * glvert = RGL_BeginUnit(GL_POLYGON, 4,
				 is_additive ? ENV_SKIP_RGB : GL_MODULATE, tex_id,
				 ENV_NONE, 0, pass, blending);
#endif

		//!!!! FUZZ TEST
		blending |= BL_Alpha;

		local_gl_vert_t * glvert = RGL_BeginUnit(GL_POLYGON, 4,
				 GL_MODULATE, tex_id, GL_MODULATE, fuzz_tex,
				 pass, blending);

		for (v_idx=0; v_idx < 4; v_idx++)
		{
			local_gl_vert_t *dest = glvert + v_idx;

			vec3_t lit_pos;

			PSpriteCoordFunc(&data, v_idx, &dest->pos, dest->rgba,
					&dest->texc[0], &dest->normal, &lit_pos);

			if (! is_additive)
			{
				dest->rgba[0] = data.col[v_idx].mod_R / 255.0;
				dest->rgba[1] = data.col[v_idx].mod_G / 255.0;
				dest->rgba[2] = data.col[v_idx].mod_B / 255.0;

				data.col[v_idx].mod_R -= 256;
				data.col[v_idx].mod_G -= 256;
				data.col[v_idx].mod_B -= 256;
// FUZZ TEST
dest->texc[1].x = (dest->pos.x / 256.0) / SCREENWIDTH  * 320.0;
dest->texc[1].y = (dest->pos.y / 256.0) / SCREENHEIGHT * 200.0;
dest->texc[1].x += fmod(data.lit_pos.x / 900.0, 1.0);
dest->texc[1].y += fmod(data.lit_pos.y / 900.0, 1.0) + fuzz_yoffset;
trans=1.00;
			}
			else
			{
				dest->rgba[0] = data.col[v_idx].add_R / 255.0;
				dest->rgba[1] = data.col[v_idx].add_G / 255.0;
				dest->rgba[2] = data.col[v_idx].add_B / 255.0;
			}
			dest->rgba[3] = trans;
		}

		RGL_EndUnit(4);
	}

	RGL_FinishUnits();

#if 0  // OLD WAY (KEEP FOR CROSSHAIRS !!!)
	if (fuzzy)
		L_r = L_g = L_b = 50;
	glColor4f(LT_RED(L_r), LT_GRN(L_g), LT_BLU(L_b), trans);

	glBegin(GL_QUADS);
  
	glTexCoord2f(tex_x1, tex_bot_h);
	glVertex2f(x1b, y1b);
  
	glTexCoord2f(tex_x1, tex_top_h);
	glVertex2f(x1t, y1t);
  
	glTexCoord2f(tex_x2, tex_top_h);
	glVertex2f(x2t, y1t);
  
	glTexCoord2f(tex_x2, tex_bot_h);
	glVertex2f(x2b, y2b);
  
	glEnd();

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);
#endif
	glDisable(GL_SCISSOR_TEST);
}

static void DrawStdCrossHair(int sbarheight)
{
	static int crhcount = 0;
	static int crhdir = 1;   // -ACB- 1999/09/19 change from ch * to crh *. chdir is a function.

	// -jc- Pulsating
	if (crhcount == 31)
		crhdir = -1;
	else if (crhcount == 0)
		crhdir = 1;

	crhcount += crhdir;

	int col = RED + crhcount / 4;  /* FIXME: configurable colour ? */
	int mul = 1 + (SCREENWIDTH / 300);

	int x = SCREENWIDTH / 2;
	int y = (SCREENHEIGHT - sbarheight) / 2;

	y += sbarheight;

	switch (crosshair)
	{
		case 1:
			RGL_SolidLine(x - 3*mul, y, x - 2*mul, y, col);
			RGL_SolidLine(x + 2*mul, y, x + 3*mul, y, col);
			RGL_SolidLine(x, y - 3*mul, x, y - 2*mul, col);
			RGL_SolidLine(x, y + 2*mul, x, y + 3*mul, col);
			break;
	
		case 2:
			RGL_SolidLine(x, y, x + 1, y, col);
			break;
	
		case 3:
			RGL_SolidLine(x, y, x + 2*mul, y, col);
			RGL_SolidLine(x, y + 1, x, y + 2*mul, col);
			break;
	
		default:
			break;
	}
}


void RGL_DrawWeaponSprites(player_t * p)
{
	bool got_cross = false;

	// special handling for zoom: show viewfinder
	if (viewiszoomed)
	{
		pspdef_t *psp = &p->psprites[ps_weapon];

		if ((p->ready_wp < 0) || (psp->state == S_NULL))
			return;

		weapondef_c *w = p->weapons[p->ready_wp].info;

		if (w->zoom_state <= 0)
			return;

		RGL_DrawPSprite(psp, ps_weapon, p, view_props,
						states + w->zoom_state);
	}
	else
	{
		// add all active psprites
		// Note: order is significant

		for (int i = 0; i < NUMPSPRITES; i++)
		{
			pspdef_t *psp = &p->psprites[i];

			if (psp->state == S_NULL)
				continue;

			RGL_DrawPSprite(psp, i, p, view_props, psp->state);

			if (i == ps_crosshair)
				got_cross = true;
		}
	}

	if (!got_cross && !automapactive && p->health > 0)
		DrawStdCrossHair((screen_hud != HUD_Full) ? 0 : FROM_200(ST_HEIGHT));
}

void RGL_DrawWeaponModel(player_t * p)
{
	if (viewiszoomed)
		return;

	pspdef_t *psp = &p->psprites[ps_weapon];

	if (p->ready_wp < 0)
		return;

	if (psp->state == S_NULL)
		return;

	if (! (psp->state->flags & SFF_Model))
		return;

	weapondef_c *w = p->weapons[p->ready_wp].info;

	modeldef_c *md = W_GetModel(psp->state->sprite);

	int skin_num = p->weapons[p->ready_wp].model_skin;

	const image_c *skin_img = md->skins[skin_num];

	if (! skin_img)  // FIXME: use a dummy image
	{
I_Debugf("Render model: no skin %d\n", skin_num);
		return;
	}

	GLuint skin_tex = W_ImageCache(skin_img, false);

// I_Debugf("Rendering weapon model!\n");
	
	float x = viewx + viewright.x * psp->sx / 8.0;
	float y = viewy + viewright.y * psp->sx / 8.0;
	float z = viewz + viewright.z * psp->sx / 8.0;

	x -= viewup.x * psp->sy / 10.0;
	y -= viewup.y * psp->sy / 10.0;
	z -= viewup.z * psp->sy / 10.0;

	x += viewforward.x * w->model_forward;
	y += viewforward.y * w->model_forward;
	z += viewforward.z * w->model_forward;

	x += viewright.x * w->model_side;
	y += viewright.y * w->model_side;
	z += viewright.z * w->model_side;

	int last_frame = psp->state->frame;
	float lerp = 0.0;

	if (p->weapon_last_frame >= 0)
	{
		SYS_ASSERT(psp->state);
		SYS_ASSERT(psp->state->tics > 1);

		last_frame = p->weapon_last_frame;

		lerp = (psp->state->tics - psp->tics + 1) / (float)(psp->state->tics);
		lerp = CLAMP(0, lerp, 1);
	}

//!!!! FUZZ TEST
skin_tex = fuzz_tex;
	MD2_RenderModel(md->model, skin_tex, true,
			        last_frame, psp->state->frame, lerp,
			        x, y, z, p->mo, view_props,
					1.0f /* scale */, w->model_aspect, w->model_bias);
}

void RGL_DrawCrosshair(player_t * p)
{
	// !!!!! FIXME
}


// ============================================================================
// R2_BSP START
// ============================================================================

int sprite_kludge = 0;

static inline void LinkDrawthingIntoDrawfloor(
		drawfloor_t *dfloor, drawthing_t *dthing)
{
	dthing->props = dfloor->props;
	dthing->next  = dfloor->things;
	dthing->prev  = NULL;

	if (dfloor->things)
		dfloor->things->prev = dthing;

	dfloor->things = dthing;
}

//
// R2_GetThingSprite
//
// Can return NULL, for no image.
//
static const image_c * R2_GetThingSprite2(mobj_t *mo, float mx, float my, bool *flip)
{
	// decide which patch to use for sprite relative to player
	SYS_ASSERT(mo->state);

	if (mo->state->sprite == SPR_NULL)
		return NULL;

	spriteframe_c *frame = W_GetSpriteFrame(mo->state->sprite,
			mo->state->frame);

	if (! frame)
	{
		// -AJA- 2001/08/04: allow the patch to be missing
		(*flip) = false;
		return W_ImageForDummySprite();
	}

	int rot = 0;

	if (frame->rots >= 8)
	{
		angle_t ang = mo->angle;

		MIR_Angle(ang);

		angle_t from_view = R_PointToAngle(viewx, viewy, mx, my);

		ang = from_view - ang + ANG180;

		if (num_active_mirrors % 2)
			ang = (angle_t)0 - ang;

		if (frame->rots == 16)
			rot = (ang + (ANG45 / 4)) >> (ANGLEBITS - 4);
		else
			rot = (ang + (ANG45 / 2)) >> (ANGLEBITS - 3);
	}

	SYS_ASSERT(0 <= rot && rot < 16);

	(*flip) = frame->flip[rot] ? true : false;

	if (num_active_mirrors % 2)
		(*flip) = !(*flip);

	return frame->images[rot];
}

//
// R2_GetOtherSprite
//
// Used for non-object stuff, like weapons and finale.
//
const image_c * R2_GetOtherSprite(int spritenum, int framenum, bool *flip)
{
	if (spritenum == SPR_NULL)
		return NULL;

	spriteframe_c *frame = W_GetSpriteFrame(spritenum, framenum);

	if (! frame)
		return NULL;

	*flip = frame->flip[0] ? true : false;

	return frame->images[0];
}


#define SY_FUDGE  2

static void R2_ClipSpriteVertically(drawsub_c *dsub, drawthing_t *dthing)
{
	drawfloor_t *dfloor = NULL, *df_orig;
	drawthing_t *dnew, *dt_orig;

	float z;
	float f1, c1;
	float f1_orig, c1_orig;

	// find the thing's nominal region.  This section is equivalent to
	// the R_PointInVertRegion() code (but using drawfloors).

	z = (dthing->top + dthing->bottom) / 2.0f;

	std::vector<drawfloor_t *>::iterator DFI;

	for (DFI = dsub->floors.begin(); DFI != dsub->floors.end(); DFI++)
	{
		dfloor = *DFI;

		if (z <= dfloor->top_h)
			break;
	}

	SYS_ASSERT(dfloor);

	// link in sprite.  We'll shrink it if it gets clipped.
	LinkDrawthingIntoDrawfloor(dfloor, dthing);

	// handle never-clip things 
	if (dthing->y_clipping == YCLIP_Never)
		return;

	// Note that sprites are not clipped by the lowest floor or
	// highest ceiling, OR by *solid* extrafloors (even translucent
	// ones) -- UNLESS y_clipping == YCLIP_Hard.

	f1 = dfloor->f_h;
	c1 = dfloor->c_h;

	// handle TRANSLUCENT + THICK floors (a bit of a hack)
	if (dfloor->ef && dfloor->ef->ef_info && dfloor->higher &&
		(dfloor->ef->ef_info->type & EXFL_Thick) &&
		(dfloor->ef->top->translucency <= 0.99f))
	{
		c1 = dfloor->top_h;
	}

	df_orig = dfloor;
	dt_orig = dthing;
	f1_orig = f1;
	c1_orig = c1;

	// Two sections here: Downward clipping (for sprite's bottom) and
	// Upward clipping (for sprite's top).  Both use the exact same
	// algorithm:
	//     
	//    WHILE (current must be clipped)
	//    {
	//       new := COPY OF current
	//       clip both current and new to the clip height
	//       current := new
	//       floor := NEXT floor
	//       link current into floor
	//    }

	// ---- downward section ----

	for (;;)
	{
		if (!dfloor->lower)
			break;

		if ((dthing->bottom >= f1 - SY_FUDGE) ||
			(dthing->top    <  f1 + SY_FUDGE))
			break;

		SYS_ASSERT(dfloor->lower->ef && dfloor->lower->ef->ef_info);

		if (! (dfloor->lower->ef->ef_info->type & EXFL_Liquid))
			break;

		// sprite must be split (bottom), make a copy.

		dnew = R_GetDrawThing();

		dnew[0] = dthing[0];

		// shorten current sprite

		dthing->bottom = f1;

		SYS_ASSERT(dthing->bottom < dthing->top);

		// shorten new sprite

///----		dnew->y_offset += (dnew->top - f1);
		dnew->top = f1;

		SYS_ASSERT(dnew->bottom < dnew->top);

		// time to move on...

		dthing = dnew;
		dfloor = dfloor->lower;

		f1 = dfloor->f_h;
		c1 = dfloor->c_h;

		// handle TRANSLUCENT + THICK floors (a bit of a hack)
		if (dfloor->ef && dfloor->ef->ef_info && dfloor->higher &&
			(dfloor->ef->ef_info->type & EXFL_Thick) &&
			(dfloor->ef->top->translucency <= 0.99f))
		{
			c1 = dfloor->top_h;
		}

		// link new piece in
		LinkDrawthingIntoDrawfloor(dfloor, dthing);
	}

	if (dthing->y_clipping == YCLIP_Hard &&
		dthing->bottom <  f1 - SY_FUDGE &&
		dthing->top    >= f1 + SY_FUDGE)
	{
		// shorten current sprite

		dthing->bottom = f1;

		SYS_ASSERT(dthing->bottom < dthing->top);
	}

	dfloor = df_orig;
	dthing = dt_orig;
	f1 = f1_orig;
	c1 = c1_orig;

	// ---- upward section ----

	for (;;)
	{
		if (!dfloor->higher)
			break;

		if ((dthing->bottom >= c1 - SY_FUDGE) ||
			(dthing->top    <  c1 + SY_FUDGE))
			break;

		SYS_ASSERT(dfloor->ef && dfloor->ef->ef_info);

		if (! (dfloor->ef->ef_info->type & EXFL_Liquid))
			break;

		// sprite must be split (top), make a copy.

		dnew = R_GetDrawThing();

		dnew[0] = dthing[0];

		// shorten current sprite

///----		dthing->y_offset += (dthing->top - c1);
		dthing->top = c1;

		SYS_ASSERT(dthing->bottom < dthing->top);

		// shorten new sprite

		dnew->bottom = c1;

		SYS_ASSERT(dnew->bottom < dnew->top);

		// time to move on...

		dthing = dnew;
		dfloor = dfloor->higher;

		f1 = dfloor->f_h;
		c1 = dfloor->c_h;

		// handle TRANSLUCENT + THICK floors (a bit of a hack)
		if (dfloor->ef && dfloor->ef->ef_info && dfloor->higher &&
			(dfloor->ef->ef_info->type & EXFL_Thick) &&
			(dfloor->ef->top->translucency <= 0.99f))
		{
			c1 = dfloor->top_h;
		}

		// link new piece in
		LinkDrawthingIntoDrawfloor(dfloor, dthing);
	}

	if (dthing->y_clipping == YCLIP_Hard &&
		dthing->bottom <  c1 - SY_FUDGE &&
		dthing->top    >= c1 + SY_FUDGE)
	{
		// shorten current sprite

///----		dthing->y_offset += dthing->top - c1;
		dthing->top = c1;

		SYS_ASSERT(dthing->bottom < dthing->top);
	}
}

//
// RGL_WalkThing
//
// Visit a single thing that exists in the current subsector.
//
void RGL_WalkThing(drawsub_c *dsub, mobj_t *mo)
{
	SYS_ASSERT(mo->state);

	// ignore the player him/herself
	if (mo == players[displayplayer]->mo && num_active_mirrors == 0)
		return;

	// ignore invisible things
	if (mo->visibility == INVISIBLE)
		return;

	// transform the origin point
	float mx = mo->x, my = mo->y;

	MIR_Coordinate(mx, my);

	float tr_x = mx - viewx;
	float tr_y = my - viewy;

	float tz = tr_x * viewcos + tr_y * viewsin;

	// thing is behind view plane?
	if (clip_scope != ANG180 && tz <= 0)
		return;

	float tx = tr_x * viewsin - tr_y * viewcos;

	// too far off the side?
	// -ES- 1999/03/13 Fixed clipping to work with large FOVs (up to 176 deg)
	// rejects all sprites where angle>176 deg (arctan 32), since those
	// sprites would result in overflow in future calculations
	if (tz >= MINZ && fabs(tx) / 32 > tz)
		return;

	bool is_model = (mo->state->flags & SFF_Model) ? true:false;

	float hover_dz = 0;

	if (mo->hyperflags & HF_HOVER)
		hover_dz = GetHoverDZ(mo);

	bool spr_flip = false;
	const image_c *image = NULL;

	float gzt  = 0, gzb  = 0;
	float pos1 = 0, pos2 = 0;

	if (! is_model)
	{
		image = R2_GetThingSprite2(mo, mx, my, &spr_flip);

		if (!image)
			return;

		// calculate edges of the shape
		float sprite_width  = IM_WIDTH(image);
		float sprite_height = IM_HEIGHT(image);
		float side_offset   = IM_OFFSETX(image);
		float top_offset    = IM_OFFSETY(image);

		if (spr_flip)
			side_offset = -side_offset;

		float xscale = mo->info->scale * mo->info->aspect;

		pos1 = (sprite_width/-2.0f - side_offset) * xscale;
		pos2 = (sprite_width/+2.0f - side_offset) * xscale;

		switch (mo->info->yalign)
		{
			case SPYA_TopDown:
				gzt = mo->z + mo->height + top_offset * mo->info->scale;
				gzb = gzt - sprite_height * mo->info->scale;
				break;

			case SPYA_Middle:
			{
				float mz = mo->z + mo->height * 0.5 + top_offset * mo->info->scale;
				float dz = sprite_height * 0.5 * mo->info->scale;

				gzt = mz + dz;
				gzb = mz - dz;
				break;
			}

			case SPYA_BottomUp: default:
				gzb = mo->z +  top_offset * mo->info->scale;
				gzt = gzb + sprite_height * mo->info->scale;
				break;
		}

		if (mo->hyperflags & HF_HOVER)
		{
			gzt += hover_dz;
			gzb += hover_dz;
		}
	} // if (! is_model)


	// fix for sprites that sit wrongly into the floor/ceiling
	int y_clipping = YCLIP_Soft;

	if (is_model || (mo->flags & MF_FUZZY) || (mo->hyperflags & HF_HOVER))
	{
		y_clipping = YCLIP_Never;
	}
	else if (sprite_kludge==0 && gzb < mo->floorz)
	{
		// explosion ?
		if (mo->info->flags & MF_MISSILE)
		{
			y_clipping = YCLIP_Hard;
		}
		else
		{
			gzt += mo->floorz - gzb;
			gzb = mo->floorz;
		}
	}
	else if (sprite_kludge==0 && gzt > mo->ceilingz)
	{
		// explosion ?
		if (mo->info->flags & MF_MISSILE)
		{
			y_clipping = YCLIP_Hard;
		}
		else
		{
			gzb -= gzt - mo->ceilingz;
			gzt = mo->ceilingz;
		}
	}

	if (!is_model && gzb >= gzt)
		return;

	// create new draw thing

	drawthing_t *dthing = R_GetDrawThing();
	dthing->Clear();

	dthing->mo = mo;
	dthing->mx = mx;
	dthing->my = my;
	dthing->props = dsub->floors[0]->props;
	dthing->y_clipping = y_clipping;
	dthing->is_model = is_model;

	dthing->image = image;
	dthing->flip  = spr_flip;

	dthing->tx = tx;
	dthing->tz = tz;

	dthing->top    = dthing->orig_top    = gzt;
	dthing->bottom = dthing->orig_bottom = gzb;
///----	dthing->y_offset = 0;

	dthing->left_dx  = pos1 *  viewsin;
	dthing->left_dy  = pos1 * -viewcos;
	dthing->right_dx = pos2 *  viewsin;
	dthing->right_dy = pos2 * -viewcos;

	// create shadow
#if 0
	if (level_flags.shadows && mo->info->shadow_trans > 0 &&
		mo->floorz < viewz && ! IS_SKY(mo->subsector->sector->floor))
	{
		drawthing_t *dshadow = R_GetDrawThing();

		dshadow[0] = dthing[0];

		dshadow->is_shadow = true;
		dshadow->y_clipping = -1;
		dshadow->tz += 1.5f;

		// shadows are 1/4 the height
		dshadow->iyscale *= 4.0f;

		gzb = mo->floorz;
		gzt = gzb + sprite_height / 4.0f * mo->info->yscale;

		dshadow->top = gzt;
		dshadow->bottom = gzb;

		R2_ClipSpriteVertically(dsub, dshadow);
	}
#endif

	R2_ClipSpriteVertically(dsub, dthing);
}


static void RGL_DrawModel(drawthing_t *dthing)
{
	mobj_t *mo = dthing->mo;

	modeldef_c *md = W_GetModel(mo->state->sprite);

	const image_c *skin_img = md->skins[mo->model_skin];

	if (! skin_img)  // FIXME: use a dummy image
	{
I_Debugf("Render model: no skin %d\n", mo->model_skin);
		return;
	}

	GLuint skin_tex = W_ImageCache(skin_img, false, mo->info->palremap);

	float z = mo->z;

	if (mo->hyperflags & HF_HOVER)
		z += GetHoverDZ(mo);

	int last_frame = mo->state->frame;
	float lerp = 0.0;

	if (mo->model_last_frame >= 0)
	{
		last_frame = mo->model_last_frame;

		SYS_ASSERT(mo->state->tics > 1);

		lerp = (mo->state->tics - mo->tics + 1) / (float)(mo->state->tics);
		lerp = CLAMP(0, lerp, 1);
	}

//!!!! FUZZ TEST
skin_tex = fuzz_tex;

	MD2_RenderModel(md->model, skin_tex, false,
			        last_frame, mo->state->frame, lerp,
					dthing->mx, dthing->my, z, mo, mo->props,
					mo->info->model_scale, mo->info->model_aspect,
					mo->info->model_bias);
}


typedef struct
{
///---	float R, G, B;

	mobj_t *mo;

	vec3_t vert[4];
	vec2_t texc[4];
	vec3_t normal;

	multi_color_c col[4];
}
thing_coord_data_t;


static void ThingCoordFunc(void *d, int v_idx,
		vec3_t *pos, float *rgb, vec2_t *texc,
		vec3_t *normal, vec3_t *lit_pos)
{
	const thing_coord_data_t *data = (thing_coord_data_t *)d;

	*pos    = data->vert[v_idx];
	*texc   = data->texc[v_idx];
	*normal = data->normal;

///---	rgb[0] = data->R;
///---	rgb[1] = data->G;
///---	rgb[2] = data->B;

	*lit_pos = *pos;
}

static void DLIT_Thing(mobj_t *mo, void *dataptr)
{
	thing_coord_data_t *data = (thing_coord_data_t *)dataptr;

	// dynamic lights do not light themselves up!
	if (mo == data->mo)
		return;
	
	SYS_ASSERT(mo->dlight.shader);

	for (int v = 0; v < 4; v++)
	{
		mo->dlight.shader->Sample(data->col + v,
				data->vert[v].x, data->vert[v].y, data->vert[v].z);
	}
}


void RGL_DrawThing(drawfloor_t *dfloor, drawthing_t *dthing)
{
	if (dthing->is_model)
	{
		RGL_DrawModel(dthing);
		return;
	}

	mobj_t *mo = dthing->mo;

	int fuzzy = (mo->flags & MF_FUZZY);

	float trans = fuzzy ? FUZZY_TRANS : mo->visibility;

	float dx = 0, dy = 0;

	if (trans < 0.04f)
		return;

	const image_c *image = dthing->image;

//	float w = IM_WIDTH(image);
	float h = IM_HEIGHT(image);
	float right = IM_RIGHT(image);
	float top   = IM_TOP(image);

	float x1b, y1b, z1b, x1t, y1t, z1t;
	float x2b, y2b, z2b, x2t, y2t, z2t;

	x1b = x1t = dthing->mx + dthing->left_dx;
	y1b = y1t = dthing->my + dthing->left_dy;
	x2b = x2t = dthing->mx + dthing->right_dx;
	y2b = y2t = dthing->my + dthing->right_dy;

	z1b = z2b = dthing->bottom;
	z1t = z2t = dthing->top;

	// Mlook in GL: tilt sprites so they look better
	{
		float h = dthing->orig_top - dthing->orig_bottom;
		float skew2 = h;

		if (mo->radius >= 1.0f && h > mo->radius)
			skew2 = mo->radius;

		float dx = viewcos * sprite_skew * skew2;
		float dy = viewsin * sprite_skew * skew2;

		float top_q    = ((dthing->top - dthing->orig_bottom)  / h) - 0.5f;
		float bottom_q = ((dthing->orig_top - dthing->bottom)  / h) - 0.5f;

		x1t += top_q * dx; y1t += top_q * dy;
		x2t += top_q * dx; y2t += top_q * dy;

		x1b -= bottom_q * dx; y1b -= bottom_q * dy;
		x2b -= bottom_q * dx; y2b -= bottom_q * dy;
	}

	float tex_x1 = 0.01f;
	float tex_x2 = right - 0.01f;

	float tex_y1 = dthing->bottom - dthing->orig_bottom;
	float tex_y2 = tex_y1 + (z1t - z1b);

	float yscale = mo->info->scale;

	SYS_ASSERT(h > 0);
	tex_y1 = top * tex_y1 / (h * yscale);
	tex_y2 = top * tex_y2 / (h * yscale);

	if (dthing->flip)
	{
		float temp = tex_x2;
		tex_x1 = right - tex_x1;
		tex_x2 = right - temp;
	}


	//
	// Special FUZZY effect
	//
	if (false) //!!!! fuzzy)
	{
		float range_x = fabs(dthing->right_dx - dthing->left_dx) / 12.0f;
		float range_y = fabs(dthing->right_dy - dthing->left_dy) / 12.0f;
		float range_z = fabs(z1t - z1b) / 24.0f / 2;

		angle_t adjust = (angle_t)(long)mo << (ANGLEBITS - 14);

		float tl = M_Sin(fuzz_ang_tl + adjust);
		float tr = M_Sin(fuzz_ang_tr + adjust);
		float bl = M_Sin(fuzz_ang_bl + adjust);
		float br = M_Sin(fuzz_ang_br + adjust);

		float ztl = 1.0f + M_Cos(fuzz_ang_tl + adjust);
		float ztr = 1.0f + M_Cos(fuzz_ang_tr + adjust);
		float zbl = 1.0f + M_Cos(fuzz_ang_bl + adjust);
		float zbr = 1.0f + M_Cos(fuzz_ang_br + adjust);

		x1b += bl * range_x; y1b += bl * range_y; z1b += zbl * range_z;
		x1t += tl * range_x; y1t += tl * range_y; z1t += ztl * range_z;
		x2t += tr * range_x; y2t += tr * range_y; z2t += ztr * range_z;
		x2b += br * range_x; y2b += br * range_y; z2b += zbr * range_z;
	}

	GLuint tex_id = W_ImageCache(image, false, dthing->mo->info->palremap);

	// Blended sprites, even if opaque (trans > 0.99), have nicer edges
	int blending = BL_Masked;
	if (trans <= 0.99 || use_smoothing)
		blending |= BL_Alpha;

	if (mo->hyperflags & HF_NOZBUFFER)
		blending |= BL_NoZBuf;

	
	thing_coord_data_t data;

	data.mo = mo;

///---	data.R = fuzzy ? 0 : 1;
///---	data.G = fuzzy ? 0 : 1;
///---	data.B = fuzzy ? 0 : 1;

	data.vert[0].Set(x1b+dx, y1b+dy, z1b);
	data.vert[1].Set(x1t+dx, y1t+dy, z1t);
	data.vert[2].Set(x2t+dx, y2t+dy, z2t);
	data.vert[3].Set(x2b+dx, y2b+dy, z2b);

	data.texc[0].Set(tex_x1, tex_y1);
	data.texc[1].Set(tex_x1, tex_y2);
	data.texc[2].Set(tex_x2, tex_y2);
	data.texc[3].Set(tex_x2, tex_y1);

	data.normal.Set(-viewcos, -viewsin, 0);


	int v_idx;

	for (int jj=0; jj < 4; jj++)
		data.col[jj].Clear();

	if (! fuzzy)
	{
		abstract_shader_c *shader = R_GetColormapShader(dthing->props, mo->state->bright);

		for (int v=0; v < 4; v++)
		{
			shader->Sample(data.col + v, data.vert[v].x, data.vert[v].y, data.vert[v].z);
		}

		if (use_dlights)
		{
			float r = mo->radius + 32;

			P_DynamicLightIterator(
					mo->x - r, mo->y - r, mo->z,
					mo->x + r, mo->y + r, mo->z + mo->height,
					DLIT_Thing, &data);

			P_SectorGlowIterator(mo->subsector->sector,
					mo->x - r, mo->y - r, mo->z,
					mo->x + r, mo->y + r, mo->z + mo->height,
					DLIT_Thing, &data);
		}
	}

	// FIXME DESCRIBE THIS WEIRD SHIT!!!
	
	for (int pass = 0; pass < 1; pass++)   //!!!!! pass < 4
	{
		if (pass > 0 && pass < 3 && GetMulticolMaxRGB(data.col, 4, false) <= 0)
			continue;

		if (pass == 3 && GetMulticolMaxRGB(data.col, 4, true) <= 0)
			continue;

		if (pass >= 1)
		{
			blending &= ~BL_Alpha;
			blending |=  BL_Add;
		}

		bool is_additive = (pass == 3);

#if 0
		local_gl_vert_t * glvert = RGL_BeginUnit(GL_POLYGON, 4,
				 is_additive ? ENV_SKIP_RGB : GL_MODULATE, tex_id,
				 ENV_NONE, 0, pass, blending);
#endif
		//!!!! FUZZ TEST
		blending |= BL_Alpha;

		local_gl_vert_t * glvert = RGL_BeginUnit(GL_POLYGON, 4,
				 GL_MODULATE, tex_id, GL_MODULATE, fuzz_tex,
				 pass, blending);

		for (v_idx=0; v_idx < 4; v_idx++)
		{
			local_gl_vert_t *dest = glvert + v_idx;

			vec3_t lit_pos;

			ThingCoordFunc(&data, v_idx, &dest->pos, dest->rgba,
					&dest->texc[0], &dest->normal, &lit_pos);

			if (! is_additive)
			{
				dest->rgba[0] = data.col[v_idx].mod_R / 255.0;
				dest->rgba[1] = data.col[v_idx].mod_G / 255.0;
				dest->rgba[2] = data.col[v_idx].mod_B / 255.0;

				data.col[v_idx].mod_R -= 256;
				data.col[v_idx].mod_G -= 256;
				data.col[v_idx].mod_B -= 256;
// FUZZ TEST
float ftx = (v_idx >= 2) ? (mo->radius) : 0;
float fty = (v_idx == 1 || v_idx == 2) ? (mo->height) : 0;
{
	float dist = P_ApproxDistance(mo->x - viewx, mo->y - viewy, mo->z - viewz);
	float factor = 2.0 / CLAMP(35, dist, 700);
	ftx *= factor;
	fty *= factor;
}
dest->texc[1].x = ftx;
dest->texc[1].y = fty;
dest->texc[1].x += fmod(mo->x / 900.0, 1.0);
dest->texc[1].y += fmod(mo->y / 900.0, 1.0) + fuzz_yoffset;
trans=1.00;
			}
			else
			{
				dest->rgba[0] = data.col[v_idx].add_R / 255.0;
				dest->rgba[1] = data.col[v_idx].add_G / 255.0;
				dest->rgba[2] = data.col[v_idx].add_B / 255.0;
			}
			dest->rgba[3] = trans;

		}

		RGL_EndUnit(4);
	}
}

//
// RGL_DrawSortThings
//
void RGL_DrawSortThings(drawfloor_t *dfloor)
{
	//
	// As part my move to strip out Z_Zone usage and replace 
	// it with array classes and more standard new and delete 
	// calls, I've removed the QSORT() here and the array. 
	// My main reason for doing that is that since I have to 
	// modify the code here anyway, it is prudent to re-evaluate
	// their usage. 
	//
	// The QSORT() mechanism used does an
	// allocation each time it is used and this is called
	// for each floor drawn in each subsector drawn, it is
	// reasonable to assume that removing it will give
	// something of speed improvement. 
	//
	// This comes at a cost since optimisation is always 
	// a balance between speed and size: drawthing_t now has 
	// to hold 4 additional pointers. Two for the binary tree 
	// (order building) and two for the the final linked list 
	// (avoiding recursive function calls that the parsing the 
	// binary tree would require).
	//
	// -ACB- 2004/08/17
	//
	
	drawthing_t *head_dt;
	
	// Check we have something to draw
	head_dt = dfloor->things;
	if (!head_dt)
		return;

	drawthing_t *curr_dt, *dt, *next_dt;
	float cmp_val;
		
	head_dt->rd_l = head_dt->rd_r = head_dt->rd_prev = head_dt->rd_next = NULL;
	
	dt = NULL;	// Warning removal: This will always have been set
	
	curr_dt = head_dt->next;
	while(curr_dt)
	{
		curr_dt->rd_l = curr_dt->rd_r = NULL;
		
		// Parse the tree to find our place
		next_dt = head_dt;
		do
		{
			dt = next_dt;
			
			cmp_val = dt->tz - curr_dt->tz;
			if (cmp_val == 0.0f)
			{
				// Resolve Z fight by letting the mobj pointer values settle it
				int offset = dt->mo - curr_dt->mo;
				cmp_val = (float)offset;
			}
						
			if (cmp_val < 0.0f)
				next_dt = dt->rd_l;				
			else
				next_dt = dt->rd_r;
		}
		while (next_dt);
		
		// Update our place 
		if (cmp_val < 0.0f)
		{ 
			// Update the binary tree
			dt->rd_l = curr_dt;

			// Update the linked list (Insert behind node)
			if (dt->rd_prev) 
				dt->rd_prev->rd_next = curr_dt; 
			
			curr_dt->rd_prev = dt->rd_prev;
			curr_dt->rd_next = dt; 

			dt->rd_prev = curr_dt;		
		}
		else
		{
			// Update the binary tree
			dt->rd_r = curr_dt; 

			// Update the linked list (Insert infront of node)
			if (dt->rd_next) 
				dt->rd_next->rd_prev = curr_dt; 
			
			curr_dt->rd_next = dt->rd_next;
			curr_dt->rd_prev = dt; 

			dt->rd_next = curr_dt;	
		}
		
		curr_dt = curr_dt->next;
	}
	
	// Find the first to draw
	while(head_dt->rd_prev)
		head_dt = head_dt->rd_prev;

	// Draw...
	for (dt=head_dt; dt; dt=dt->rd_next)
		RGL_DrawThing(dfloor, dt);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
