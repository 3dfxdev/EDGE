//----------------------------------------------------------------------------
//  EDGE TITLE SCREEN
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2009  The EDGE Team.
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

#include "i_defs.h"
#include "i_defs_gl.h"

#include "g_game.h"
#include "r_misc.h"
#include "r_gldefs.h"
#include "r_draw.h"


typedef struct
{
	float x, y;
	float t;
	float size;
}
star_info_t;

static std::list<star_info_t> star_field;

#define STAR_ALPHA(t_diff)  (0.99 - (t_diff) / 2000.0)


static void RemoveDeadStars(void)
{
	for (;;)
	{
		if (star_field.empty())
			break;

		float t_diff = star_field.front().t - millies;

		if (STAR_ALPHA(t_diff) > 0)
			break;

		star_field.pop_front();
	}

}


static void DrawStars(int millies)
{
	float dist = 0;
	if (millies > 1200)
		dist = millies - 1200;

	std::list<star_info_t>::iterator SI;

	while (! star_field.empty() && star_field.

	for (SI = star_field.begin(); SI != star_field.end(); SI++)
	{
		star_info_t& 
		float alpha = 0.99 - (millies - t) / 2000.0; 
    if S.a > 0 and S.z*611 < 6930-dist*100 then
      local w = S.big / (7000 - S.z*611)
      local dx = (S.x - 160) * S.z / 7 
      local dy = (S.y - 100) * S.z / 12
      hud.stretch_image(S.x+dx-w, S.y+dy-w, w*2, w*2, "STAR")
      S.z = S.z + 0.3
      S.a = S.a - 0.015
    end
  end
}


static void DrawName(void)
{
  hud.set_alpha(1)
  hud.solid_box(0, 0, 320, 30, "#000000")
  hud.solid_box(0, 170, 320, 30, "#000000")

  hud.stretch_image(0, 30, 320, 140, "EDGE")

  fade = (mmm - 11) / 30 
  if fade > 1 then fade = 2 - fade end
  if fade > 0 then
    hud.set_alpha(fade ^ 1.5)
    hud.stretch_image(60, 160, 200, 20, "EDGE_MNEM")
  end

}


static void InsertNewStars(int millies)
{
  local num = 25
  if mmm > 26 then num = num - (mmm - 26) * 1 end

  for loop = 1,num do
    local x = math.random() * 320
    local y = math.random() * 200
    local big = (math.random() + 1) * 16000

    table.insert(star_field, { x=x, y=y, z=0.3, a=0.99, big=big })
  end
}


bool RGL_DrawTitle(int millies)
{
	if (millies > 2140)
		return true;  // finished


	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);


	glEnable(GL_BLEND);


	RemoveDeadStars();

	DrawStars(milliest);

	DrawName();

	InsertNewStars(millies);



	glDisable(GL_BLEND);

	I_FinishFrame();
	I_StartFrame();

	return false;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
