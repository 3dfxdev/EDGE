//----------------------------------------------------------------------------
//  EDGE Sector Lighting Code
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
//
// -KM- 1998/09/27 Lights generalised for ddf
//

#include "i_defs.h"

#include "dm_defs.h"
#include "dm_state.h"
#include "m_random.h"
#include "p_local.h"
#include "r_state.h"
#include "s_sound.h"
#include "z_zone.h"

#include <list>

light_t *lights = NULL;

//
// GENERALISED LIGHT
//
// -AJA- 2000/09/20: added FADE type.
//
static void DoLight(light_t * light)
{
	const lightdef_c *type = light->type;

	if (type->type == LITE_None || --light->count)
		return;

	// Flashing lights
	switch (type->type)
	{
		case LITE_Set:
		{
			light->sector->props.lightlevel = light->maxlight;

			// count is 0, i.e. this light is now disabled
			return;
		}

		case LITE_Fade:
		{
			int diff = light->maxlight - light->minlight;

			if (ABS(diff) < type->step)
			{
				light->sector->props.lightlevel = light->maxlight;

				// count is 0, i.e. this light is now disabled
				return;
			}

			// step towards the target light level
			if (diff < 0)
				light->minlight -= type->step;
			else
				light->minlight += type->step;

			light->sector->props.lightlevel = light->minlight;
			light->count = type->brighttime;
			break;
		}

		case LITE_Flash:
		{
			// Dark
			if (M_RandomTest(type->chance))
			{
				light->sector->props.lightlevel = light->minlight;
				light->count = type->darktime;
			}
			else
			{
				light->sector->props.lightlevel = light->maxlight;
				light->count = type->brighttime;
			}
			break;
		}

		case LITE_Strobe:
			if (light->sector->props.lightlevel == light->maxlight)
			{
				// Go dark
				light->sector->props.lightlevel = light->minlight;
				light->count = type->darktime;
			}
			else
			{
				// Go Bright
				light->sector->props.lightlevel = light->maxlight;
				light->count = type->brighttime;
			}
			break;

		case LITE_Glow:
			if (light->direction == -1)
			{
				// Go dark
				light->sector->props.lightlevel -= type->step;
				if (light->sector->props.lightlevel <= light->minlight)
				{
					light->sector->props.lightlevel = light->minlight;
					light->count = type->brighttime;
					light->direction = +1;
				}
				else
				{
					light->count = type->darktime;
				}
			}
			else
			{
				// Go Bright
				light->sector->props.lightlevel += type->step;
				if (light->sector->props.lightlevel >= light->maxlight)
				{
					light->sector->props.lightlevel = light->maxlight;
					light->count = type->darktime;
					light->direction = -1;
				}
				else
				{
					light->count = type->brighttime;
				}
			}
			break;

		case LITE_FireFlicker:
		{
			// -ES- 2000/02/13 Changed this to original DOOM style flicker
			int amount = (M_Random() & 7) * type->step;

			if (light->sector->props.lightlevel - amount < light->minlight)
			{
				light->sector->props.lightlevel = light->minlight;
				light->count = type->darktime;
			}
			else
			{
				light->sector->props.lightlevel = light->maxlight - amount;
				light->count = type->brighttime;
			}
		}

		default:
			break;
	}
}

//
// TURN LINE'S TAG LIGHTS ON
//
void EV_LightTurnOn(int tag, int bright)
{
	int i;
	int j;
	sector_t *sector;
	sector_t *temp;
	line_t *templine;

	sector = sectors;

	for (i = 0; i < numsectors; i++, sector++)
	{
		if (sector->tag == tag)
		{
			// bright == 0 means to search for highest light level
			// surrounding sector
			if (!bright)
			{
				for (j = 0; j < sector->linecount; j++)
				{
					templine = sector->lines[j];
					temp = P_GetNextSector(templine, sector);

					if (!temp)
						continue;

					if (temp->props.lightlevel > bright)
						bright = temp->props.lightlevel;
				}
			}
			sector->props.lightlevel = bright;
		}
	}
}

#if 0 // Currently unused
//
// P_DestroyLight
//
// Unlink and destroy light.
void P_DestroyLight(light_t *light)
{
	if (light->next)
		light->next->prev = light->prev;
	if (light->prev)
		light->prev->next = light->next;
	else
		lights = light->next;
	Z_Free(light);
}
#endif

//
// P_DestroyAllLights
//
void P_DestroyAllLights(void)
{
	light_t *l, *next;

	for (l = lights; l; l = next)
	{
		next = l->next;
		Z_Free(l);
	}
	lights = NULL;
}

//
// P_NewLight
//
// Allocate and link in light.
//
light_t *P_NewLight(void)
{
	light_t *light;

	light = Z_New(light_t, 1);
	light->next = lights;
	light->prev = NULL;
	if (lights)
		lights->prev = light;
	lights = light;

	return light;
}

//
// EV_Lights
//
bool EV_Lights(sector_t * sec, const lightdef_c * type)
{
	light_t *light;

	// check if a light effect already is running on this sector.
	for (light=lights; light; light=light->next)
	{
		if (light->count == 0 || light->sector == sec)
			break;
	}

	if (!light)
	{
		// didn't already exist, create a new one
		light = P_NewLight();
	}

	light->type = type;
	light->sector = sec;
	light->direction = -1;

	switch (type->type)
	{
		case LITE_Set:
		case LITE_Fade:
		{
			light->minlight = sec->props.lightlevel;
			light->maxlight = type->level;
			light->count = type->brighttime;
			break;
		}

		default:
		{
			light->minlight = P_FindMinSurroundingLight(sec, sec->props.lightlevel);
			light->maxlight = sec->props.lightlevel;
			light->count = type->sync ? (leveltime % type->sync) + 1 : 
				type->darktime;
			break;
		}
	}

	return true;
}

//
// P_RunLights
//
// Executes all light effects of this tic
// Lights are quite simple to handle, since they never destroy
// themselves. Therefore, we do not need to bother about stuff like
// removal queues
//
void P_RunLights(void)
{
	light_t *l;

	for (l = lights; l; l = l->next)
	{
		if (l->count)
			DoLight(l);
	}
}


//----------------------------------------------------------------------------
//  AMBIENT SOUND CODE
//----------------------------------------------------------------------------

#define SECSFX_TIME  7  // every 7 tics (i.e. 5 times per second)

class ambientsfx_c
{
public:
	sector_t *sector;

	sfx_t *sfx;
 
	// tics to go before next update
	int count;

public:
	ambientsfx_c(sector_t *_sec, sfx_t *_fx) :
		sector(_sec), sfx(_fx), count(SECSFX_TIME)
	{ }

	~ambientsfx_c()
	{ }
};

std::list<ambientsfx_c *> ambient_sounds;


void P_AddAmbientSFX(sector_t *sec, sfx_t *sfx)
{
	ambient_sounds.push_back(new ambientsfx_c(sec, sfx));
}

void P_DestroyAllAmbientSFX(void)
{
	while (! ambient_sounds.empty())
	{
		ambientsfx_c *amb = * ambient_sounds.begin();

		ambient_sounds.pop_front();

		S_StopFX(&amb->sector->sfx_origin);

		delete amb;
	}
}

void P_RunAmbientSFX(void)
{
	std::list<ambientsfx_c *>::iterator S;

	for (S = ambient_sounds.begin(); S != ambient_sounds.end(); S++)
	{
		ambientsfx_c *amb = *S;
		
		if (amb->count > 0)
			amb->count--;
		else
		{
			amb->count = SECSFX_TIME;

			S_StartFX(amb->sfx, SNCAT_Level, &amb->sector->sfx_origin);
		}
	}
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
