//----------------------------------------------------------------------------
//  EDGE GUI Main
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2001  The EDGE Team.
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
#include "gui_main.h"

#include "dm_type.h"
#include "dm_state.h"
#include "gui_gui.h"
#include "con_main.h"
#include "con_cvar.h"
#include "v_res.h"
#include "w_wad.h"
#include "z_zone.h"

static int mouse_x = 0, mouse_y = 0;
static bool mouse_visible = false;

static int mouse;

static gui_t *main_gui;

void GUI_Init(gui_t **gui)
{
  *gui = Z_ClearNew(gui_t, 1);

  (*gui)->prev = *gui;
  (*gui)->next = *gui;
  (*gui)->gui = gui;
}

gui_t **GUI_NULL(void)
{
  return &main_gui;
}

void GUI_Start(gui_t ** gui, gui_t * g)
{
  guievent_t spawn =
  {gev_spawn, 0,0,0,0};

  g->next = *gui;
  g->prev = (*gui)->prev;
  g->next->prev = g;
  g->prev->next = g;
  g->gui = gui;
  GUI_Responder(&g, &spawn);
  GUI_SetFocus(gui, g);
}

void GUI_Destroy(gui_t * g)
{
  guievent_t destroy =
  {gev_destroy, 0,0,0,0};

  if (!GUI_Responder(&g, &destroy))
  {
    g->next->prev = g->prev;
    g->prev->next = g->next;
    if (*g->gui == g)
      *g->gui = g->next;
    Z_Free(g);
  }
}

void GUI_SetFocus(gui_t ** gui, gui_t * g)
{
  guievent_t lose =
  {gev_losefocus, 0,0,0,0};

  if (!GUI_Responder(gui, &lose))
    *gui = g;
}

void GUI_Ticker(gui_t ** gui)
{
  gui_t *g = *gui;

  do
  {
    if (g->Ticker)
      g->Ticker(g);

    g = g->next;
  }
  while (g != *gui);
}

static bool GUI_InBox(int x, int y, gui_t * g)
{
  return (x <= g->right && x >= g->left && y <= g->bottom && y >= g->top);
}

bool GUI_Responder(gui_t ** gui, guievent_t * e)
{
  gui_t *g = *gui;
  guievent_t event = *e;
  bool eat;
  const visible_t *v;

  switch (event.type)
  {
    case gev_analogue:
      mouse_x += event.data2;
      if (mouse_x >= SCREENWIDTH)
        mouse_x = SCREENWIDTH - 1;
      else if (mouse_x < 0)
        mouse_x = 0;

      mouse_y += event.data4;
      if (mouse_y >= SCREENHEIGHT)
        mouse_y = SCREENHEIGHT - 1;
      else if (mouse_y < 0)
        mouse_y = 0;

      event.type = gev_hover;
      event.data1 = event.data2;
      event.data2 = event.data4;
      event.data3 = mouse_x;
      event.data4 = mouse_y;
      if (!mouse_visible)
        return false;
      break;
    case gev_keydown:
      if (event.data1 == KEYD_TILDE)
      {
        CON_GetCVar("constate", (const void **)&v);
        CON_SetVisible((visible_t)(((*v) + 1) % NUMVIS));
        return true;
      }
      break;
    default:
      break;
  }

  if (event.type == gev_keydown
      && event.data1 >= KEYD_MOUSE1
      && event.data1 <= KEYD_MOUSE4)
    eat = GUI_InBox(mouse_x, mouse_y, g) && mouse_visible;
  else
    eat = true;
  if (g->Responder && eat)
  {
    eat = g->Responder(g, &event);
    if (!eat)
    {
      switch (e->type)
      {
        case gev_move:
          g->left += event.data1;
          g->right += event.data1;
          g->top += event.data2;
          g->bottom += event.data2;
          return true;
        case gev_size:
          g->right += event.data1;
          g->bottom += event.data2;
          return true;
        default:
          return false;
      }
    }
    return eat;
  }
  else if (!eat && mouse_visible)
  {
    g = g->next;
    do
    {
      if (GUI_InBox(mouse_x, mouse_y, g))
      {
        GUI_SetFocus(gui, g);
        if (g->Responder)
          return g->Responder(g, &event);
        else
          return false;
      }

      g = g->next;
    }
    while (g != *gui);
  }

  return false;
}

void GUI_Drawer(gui_t ** gui)
{
  gui_t *g = (*gui)->prev;

  do
  {
    if (g->Drawer)
      g->Drawer(g);

    g = g->prev;
  }
  while (g != (*gui)->prev);

#if 0  // V_DrawPatch no longer usable
  if (mouse_visible)
  {
    const patch_t *p;
    p = W_CacheLumpNum(mouse);
    V_DrawPatch(main_scr, mouse_x, mouse_y, p);
    W_DoneWithLump(p);
  }
#endif
}

void GUI_MainTicker(void)
{
  GUI_Ticker(&main_gui);
}

bool GUI_MainResponder(event_t * ev)
{
  guievent_t ge;

  if (ev->type == ev_analogue)
  {
    ge.type = gev_analogue;
    ge.data1 = ev->value.analogue.axis;
    ge.data2 = ev->value.analogue.amount;
    ge.data3 = AXIS_DISABLE;
  }
  else
  {
    if (ev->type == ev_keyup)
    {
      ge.type = gev_keyup;
    }
    else
    {
//      DEV_ASSERT2(ev->type == gev_keydown);
      DEV_ASSERT2(ev->type == ev_keydown);
      ge.type = gev_keydown;
    }

    ge.data1 = ev->value.key;
  }
  
  return GUI_Responder(&main_gui, &ge);
}

void GUI_MainDrawer(void)
{
  GUI_Drawer(&main_gui);
}

//
// GUI_MainInit
//
bool GUI_MainInit(void)
{
  mouse = W_GetNumForName("mouse");
  GUI_Init(&main_gui);
  CON_Start(&main_gui);
  return true;
}

void GUI_InitResolution(void)
{
  CON_InitResolution();
}

void GUI_MainSetMouseVisibility(bool visible)
{
  mouse_visible = visible;
}

bool GUI_MainGetMouseVisibility(void)
{
  return mouse_visible;
}

bool GUI_SetMouse(char *name)
{
  mouse = W_CheckNumForName(name);
  if (mouse == -1)
  {
    mouse = W_GetNumForName("mouse");
    return false;
  }
  return true;
}
