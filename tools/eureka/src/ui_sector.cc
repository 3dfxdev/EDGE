//------------------------------------------------------------------------
//  Information Panel
//------------------------------------------------------------------------
//
//  RTS Layout Tool (C) 2007 Andrew Apted
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

#include "main.h"
#include "ui_window.h"

#include "levels.h"
#include "things.h"
#include "w_structs.h"


//
// UI_SectorBox Constructor
//
UI_SectorBox::UI_SectorBox(int X, int Y, int W, int H, const char *label) : 
    Fl_Group(X, Y, W, H, label),
    obj(-1)
{
  end();  // cancel begin() in Fl_Group constructor

  box(FL_FLAT_BOX); // (FL_THIN_UP_BOX);


  X += 6;
  Y += 5;

  W -= 12;
  H -= 10;


  which = new UI_Nombre(X, Y, W-10, 28, "Sector");

  add(which);

  Y += which->h() + 4;


  type = new Fl_Int_Input(X+58, Y, 64, 24, "Type: ");
  type->align(FL_ALIGN_LEFT);
  type->callback(type_callback, this);

  add(type);

  choose = new Fl_Button(X+W/2+24, Y, 80, 24, "Choose");
  choose->callback(button_callback, this);

  add(choose);

  Y += type->h() + 3;


  desc = new Fl_Output(X+58, Y, W-64, 24, "Desc: ");
  desc->align(FL_ALIGN_LEFT);

  add(desc);

  Y += desc->h() + 3;


  tag = new Fl_Int_Input(X+58, Y, 64, 24, "Tag: ");
  tag->align(FL_ALIGN_LEFT);
  tag->callback(tag_callback, this);

  add(tag);

  Y += tag->h() + 3;


  light = new Fl_Int_Input(X+58, Y, 64, 24, "Light: ");
  light->align(FL_ALIGN_LEFT);
  light->callback(light_callback, this);

  add(light);

  lt_down = new Fl_Button(X+W/2+20, Y+1, 30, 22, "-");
  lt_up   = new Fl_Button(X+W/2+60, Y+1, 30, 22, "+");

  lt_down->labelfont(FL_HELVETICA_BOLD);
  lt_up  ->labelfont(FL_HELVETICA_BOLD);
  lt_down->labelsize(16);
  lt_up  ->labelsize(16);

  lt_down->callback(button_callback, this);
  lt_up  ->callback(button_callback, this);

  add(lt_down); add(lt_up);


  Y += light->h() + 3;

  Y += 14;


  c_pic = new UI_Pic(X+W-76, Y+2,  64, 64);
  f_pic = new UI_Pic(X+W-76, Y+78, 64, 64);

  c_pic->callback(button_callback, this);
  f_pic->callback(button_callback, this);

  add(f_pic);
  add(c_pic);


  c_tex = new Fl_Input(X+58, Y, 96, 24, "Ceiling: ");
  c_tex->align(FL_ALIGN_LEFT);
  c_tex->callback(tex_callback, this);

  add(c_tex);

  Y += c_tex->h() + 3;


  ceil_h = new Fl_Int_Input(X+58, Y, 64, 24, "");
  ceil_h->align(FL_ALIGN_LEFT);
  ceil_h->callback(height_callback, this);

  add(ceil_h);


  ce_down = new Fl_Button(X+24,    Y+1, 30, 22, "-");
  ce_up   = new Fl_Button(X+58+68, Y+1, 30, 22, "+");

  ce_down->labelfont(FL_HELVETICA_BOLD);
  ce_up  ->labelfont(FL_HELVETICA_BOLD);
  ce_down->labelsize(16);
  ce_up  ->labelsize(16);

  ce_down->callback(button_callback, this);
  ce_up  ->callback(button_callback, this);

  add(ce_down); add(ce_up);


  Y += ceil_h->h() + 3;


  Y += 5;

  headroom = new Fl_Int_Input(X+100, Y, 56, 24, "Headroom: ");
  headroom->align(FL_ALIGN_LEFT);
  headroom->callback(height_callback, this);

  add(headroom);

  Y += headroom->h() + 3;

  Y += 5;


  floor_h = new Fl_Int_Input(X+58, Y, 64, 24, "");
  floor_h->align(FL_ALIGN_LEFT);
  floor_h->callback(height_callback, this);

  add(floor_h);


  fl_down = new Fl_Button(X+24,    Y+1, 30, 22, "-");
  fl_up   = new Fl_Button(X+58+68, Y+1, 30, 22, "+");

  fl_down->labelfont(FL_HELVETICA_BOLD);
  fl_up  ->labelfont(FL_HELVETICA_BOLD);
  fl_down->labelsize(16);
  fl_up  ->labelsize(16);

  fl_down->callback(button_callback, this);
  fl_up  ->callback(button_callback, this);

  add(fl_down); add(fl_up);


  Y += floor_h->h() + 3;


  f_tex = new Fl_Input(X+58, Y, 96, 24, "Floor:   ");
  f_tex->align(FL_ALIGN_LEFT);
  f_tex->callback(tex_callback, this);

  add(f_tex);

  Y += f_tex->h() + 3;


  Y += 29;
}


//
// UI_SectorBox Destructor
//
UI_SectorBox::~UI_SectorBox()
{
}


//------------------------------------------------------------------------


void UI_SectorBox::height_callback(Fl_Widget *w, void *data)
{
  UI_SectorBox *box = (UI_SectorBox *)data;

  int N = box->obj;
  if (! is_sector(N))
    return;

  if (w == box->floor_h)
  {
    int new_h = atoi(box->floor_h->value());
    new_h = MIN(32767, MAX(-32767, new_h));

    Sectors[N].floorh = new_h;
  }

  if (w == box->ceil_h)
  {
    int new_h = atoi(box->ceil_h->value());
    new_h = MIN(32767, MAX(-32767, new_h));

    Sectors[N].ceilh = new_h;
  }

  if (w == box->headroom)
  {
    int new_h = Sectors[N].floorh + atoi(box->headroom->value());
    new_h = MIN(32767, MAX(-32767, new_h));

    Sectors[N].ceilh = new_h;
  }

  box-> floor_h->value(Int_TmpStr(Sectors[N].floorh));
  box->  ceil_h->value(Int_TmpStr(Sectors[N].ceilh));
  box->headroom->value(Int_TmpStr(Sectors[N].ceilh - Sectors[N].floorh));
}


void UI_SectorBox::tex_callback(Fl_Widget *w, void *data)
{
  UI_SectorBox *box = (UI_SectorBox *)data;

  int N = box->obj;
  if (! is_sector(N))
    return;

  if (w == box->f_tex)
  {
    box->FlatFromWidget(Sectors[N].floor_tex, box->f_tex);
    box->f_pic->GetFlat(Sectors[N].floor_tex);
  }

  if (w == box->c_tex)
  {
    box->FlatFromWidget(Sectors[N].ceil_tex, box->c_tex);
    box->c_pic->GetFlat(Sectors[N].ceil_tex);
  }
}


void UI_SectorBox::type_callback(Fl_Widget *w, void *data)
{
  UI_SectorBox *box = (UI_SectorBox *)data;

  int N = box->obj;
  if (! is_sector(N))
    return;

  int new_type = atoi(box->type->value());

  Sectors[N].type = new_type;

  box->desc->value(GetSectorTypeName(new_type));

///  main_win->canvas->redraw();
}


void UI_SectorBox::light_callback(Fl_Widget *w, void *data)
{
  UI_SectorBox *box = (UI_SectorBox *)data;

  int N = box->obj;
  if (! is_sector(N))
    return;

  int new_lt = atoi(box->light->value());

  // we allow 256 if explicitly typed, otherwise limit to 255
  if (new_lt > 256)
    new_lt = 255;

  new_lt = MAX(0, new_lt);

  Sectors[N].light = new_lt;
}


void UI_SectorBox::tag_callback(Fl_Widget *w, void *data)
{
  UI_SectorBox *box = (UI_SectorBox *)data;

  int N = box->obj;
  if (! is_sector(N))
    return;

  int new_tag = atoi(box->tag->value());

  Sectors[N].tag = new_tag;
}


void UI_SectorBox::button_callback(Fl_Widget *w, void *data)
{
  UI_SectorBox *box = (UI_SectorBox *)data;

  int N = box->obj;
  if (! is_sector(N))
    return;


  if (w == box->choose)
  {
    // FIXME: sector type selection
    return;
  }

  if (w == box->f_pic)
  {
printf("SELECT FLOOR PIC .........\n");
    return;
  }
  else if (w == box->c_pic)
  {
printf("SELECT CEIL PIC .........\n");
    return;
  }


  int dist = 8;
  if (Fl::event_shift())
    dist = 1;
  else if (Fl::event_ctrl())
    dist = 64;

  if (w == box->lt_up)
    box->AdjustLight(&Sectors[N].light, +dist);
  else if (w == box->lt_down)
    box->AdjustLight(&Sectors[N].light, -dist);

  if (w == box->ce_up)
    box->AdjustHeight(&Sectors[N].ceilh, +dist);
  else if (w == box->ce_down)
    box->AdjustHeight(&Sectors[N].ceilh, -dist);

  if (w == box->fl_up)
    box->AdjustHeight(&Sectors[N].floorh, +dist);
  else if (w == box->fl_down)
    box->AdjustHeight(&Sectors[N].floorh, -dist);

  box->light->value(Int_TmpStr(Sectors[N].light));

  box-> floor_h->value(Int_TmpStr(Sectors[N].floorh));
  box->  ceil_h->value(Int_TmpStr(Sectors[N].ceilh));
  box->headroom->value(Int_TmpStr(Sectors[N].ceilh - Sectors[N].floorh));
}


//------------------------------------------------------------------------


void UI_SectorBox::SetObj(int index)
{
  if (index == obj)
    return;

  obj = index;

  which->SetIndex(index);
  which->SetTotal(NumSectors);

  if (is_sector(obj))
  {
     floor_h->value(Int_TmpStr(Sectors[obj].floorh));
      ceil_h->value(Int_TmpStr(Sectors[obj].ceilh));
    headroom->value(Int_TmpStr(Sectors[obj].ceilh - Sectors[obj].floorh));

    FlatToWidget(f_tex, Sectors[obj].floor_tex);
    FlatToWidget(c_tex, Sectors[obj].ceil_tex);

    f_pic->GetFlat(Sectors[obj].floor_tex);
    c_pic->GetFlat(Sectors[obj].ceil_tex);

     type->value(Int_TmpStr(Sectors[obj].type));
     desc->value(GetSectorTypeName(Sectors[obj].type));

    light->value(Int_TmpStr(Sectors[obj].light));
      tag->value(Int_TmpStr(Sectors[obj].tag));
  }
  else
  {
     floor_h->value("");
      ceil_h->value("");
    headroom->value("");

    f_tex->value("");
    c_tex->value("");

    f_pic->Nil();
    c_pic->Nil();

     type->value("");
     desc->value("");
    light->value("");
      tag->value("");
  }
}
    


// FIXME: make a method of Sector class
void UI_SectorBox::AdjustHeight(s16_t *h, int delta)
{
  // prevent overflow
  if (delta > 0 && (int(*h) + delta) > 32767)
  {
    *h = 32767; return;
  }
  else if (delta < 0 && (int(*h) + delta) < -32767)
  {
    *h = -32767; return;
  }

  *h = *h + delta;
}


// FIXME: make a method of Sector class
void UI_SectorBox::AdjustLight(s16_t *L, int delta)
{
  if (abs(delta) > 16)
  {
    *L = (delta > 0) ? 255 : 0;
    return;
  }

  if (abs(delta) < 4)
    *L += delta;
  else
  {
    if (delta > 0)
      *L = (*L | 15) + 1;
    else
      *L = (*L - 1) & ~15;
  }

  if (*L < 0)
    *L = 0;
  else if (*L > 255)
    *L = 255;
}


void UI_SectorBox::FlatFromWidget(wad_flat_name_t& fname, Fl_Input *w)
{
  memset(fname, 0, WAD_FLAT_NAME);

  strncpy(fname, w->value(), WAD_FLAT_NAME);

  for (int i = 0; i < WAD_FLAT_NAME; i++)
    fname[i] = toupper(fname[i]);
}

void UI_SectorBox::FlatToWidget(Fl_Input *w, const wad_flat_name_t& fname)
{
  char buffer[WAD_FLAT_NAME + 1];

  strncpy(buffer, fname, WAD_FLAT_NAME);

  buffer[WAD_FLAT_NAME] = 0;

  w->value(buffer);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
