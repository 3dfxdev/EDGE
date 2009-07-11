//------------------------------------------------------------------------
//  Information Bar (bottom of window)
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

#ifndef __UI_FILTER_H__
#define __UI_FILTER_H__


class UI_FChoice : public Fl_Box
{
private:
  bool filtered;  // won't be shown
  bool pic_mode;  // show as picture

  std::string name;
  std::string size;

  int type_id;
  int category;

  UI_Pic *pic;

  int pic_size;
  int name_w;

public:
  UI_FChoice(int X, int Y, int W, int H);
  virtual ~UI_FChoice();

public:


private:
  // FLTK method for drawing this widget
  void draw();

};


class UI_FPanel : public Fl_Group
{
private:
  Fl_Box *title_lab;

  FL_Choice *mode;  // Names vs Images
  Fl_Choice *category;

  Fl_Box *show_lab;

  UI_Pic *pic;

  FL_Button *cancel_btn;

public:
  UI_FPanel(int X, int Y, int W, int H, const char *);
  virtual ~UI_FPanel();

public:
  void SetCategories(const char *cats);


private:

};


class UI_Filter : public Fl_Double_Window
{
private:
  UI_FPanel *panel;

  std::vector<UI_FChoice *> choices;

public:
  UI_Filter(int X, int Y, int W, int H, const char *title, const char *flags = "");
  virtual ~UI_Filter();

public:
  void SetCategories(const char *cats);


private:


};


#endif // __UI_FILTER_H__

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
