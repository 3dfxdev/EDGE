//------------------------------------------------------------------------
//  MENU : Menu handling
//------------------------------------------------------------------------
//
//  Editor for DDF   (C) 2005  The EDGE Team
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

// this includes everything we need
#include "defs.h"


static bool menu_want_to_quit;


#if 0
static void menu_quit_CB(Fl_Widget *w, void *data)
{
	menu_want_to_quit = true;
}
#endif


#ifndef MACOSX
static void menu_do_exit(Fl_Widget *w, void * data)
{
	guix_win->want_quit = true;
}
#endif


//------------------------------------------------------------------------

static void menu_do_prefs(Fl_Widget *w, void * data)
{
}


//------------------------------------------------------------------------

static const char *about_Info =
	"By the EDGE Team (C) 2005";


static void menu_do_about(Fl_Widget *w, void * data)
{
#if 0
  menu_want_to_quit = false;

  Fl_Window *ab_win = new Fl_Window(600, 340, "About " PROG_NAME);
  ab_win->end();

  // non-resizable
  ab_win->size_range(ab_win->w(), ab_win->h(), ab_win->w(), ab_win->h());
  ab_win->position(guix_prefs.manual_x, guix_prefs.manual_y);
  ab_win->callback((Fl_Callback *) menu_quit_CB);

  // add the about image
  Fl_Group *group = new Fl_Group(0, 0, 230, ab_win->h());
  group->box(FL_FLAT_BOX);
  group->color(FL_BLACK, FL_BLACK);
  ab_win->add(group);
  
  Fl_Box *box = new Fl_Box(20, 90, ABOUT_IMG_W+2, ABOUT_IMG_H+2);
  box->image(about_image);
  group->add(box); 


  // about text
  box = new Fl_Box(240, 60, 350, 270, about_Info);
  box->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT | FL_ALIGN_TOP);
  ab_win->add(box);
   
 
  // finally add an "OK" button
  Fl_Button *button = new Fl_Button(ab_win->w()-10-60, ab_win->h()-10-30, 
      60, 30, "OK");
  button->callback((Fl_Callback *) menu_quit_CB);
  ab_win->add(button);
    
  ab_win->set_modal();
  ab_win->show();
  
  // capture initial size
  WindowSmallDelay();
  int init_x = ab_win->x();
  int init_y = ab_win->y();
  
  // run the GUI until the user closes
  while (! menu_want_to_quit)
    Fl::wait();

  // check if the user moved/resized the window
  if (ab_win->x() != init_x || ab_win->y() != init_y)
  {
    guix_prefs.manual_x = ab_win->x();
    guix_prefs.manual_y = ab_win->y();
  }
 
  // this deletes all the child widgets too...
  delete ab_win;
#endif
}


//------------------------------------------------------------------------

static void menu_do_manual(Fl_Widget *w, void * data)
{
}


//------------------------------------------------------------------------

static void menu_do_license(Fl_Widget *w, void * data)
{
}


//------------------------------------------------------------------------

static void menu_do_save_log(Fl_Widget *w, void * data)
{
}


//------------------------------------------------------------------------

#undef FCAL
#define FCAL  (Fl_Callback *)

static Fl_Menu_Item menu_items[] = 
{
    { "&File", 0, 0, 0, FL_SUBMENU },
        { "&Preferences...",    0, FCAL menu_do_prefs },
        { "&Save Log...",       0, FCAL menu_do_save_log },
#ifndef MACOSX
        { "E&xit",   FL_ALT + 'q', FCAL menu_do_exit },
#endif
        { 0 },

    { "&Help", 0, 0, 0, FL_SUBMENU },
        { "&About...",         0,  FCAL menu_do_about },
        { "&License...",       0,  FCAL menu_do_license },
        { "&Manual...",   FL_F+1,  FCAL menu_do_manual },
        { 0 },

    { 0 }
};


//
// MenuCreate
//
#ifdef MACOSX
Fl_Sys_Menu_Bar * MenuCreate(int x, int y, int w, int h)
{
	Fl_Sys_Menu_Bar *bar = new Fl_Sys_Menu_Bar(x, y, w, h);
	bar->menu(menu_items);
	return bar;
}
#else
Fl_Menu_Bar * MenuCreate(int x, int y, int w, int h)
{
	Fl_Menu_Bar *bar = new Fl_Menu_Bar(x, y, w, h);
	bar->menu(menu_items);
	return bar;
}
#endif

//------------------------------------------------------------------------

#if 0  // from EDITOR.CXX

int check_save(void)
{
  if (!changed) return 1;

  int r = fl_choice("The current file has not been saved.\n"
                    "Would you like to save it now?",
                    "Cancel", "Save", "Don't Save");

  if (r == 1)
  {
    save_cb(); // Save the file...
    return !changed;
  }

  return (r == 2) ? 1 : 0;
}

int loading = 0;

char filename[256] = "";

void load_file(char *newfile, int ipos)
{
	loading = 1;
	int insert = (ipos != -1);
	changed = insert;
	if (!insert)
		strcpy(filename, "");

	int r;
	if (!insert)
		r = textbuf->loadfile(newfile);
	else
		r = textbuf->insertfile(newfile, ipos);

	if (r)
		fl_alert("Error reading from file \'%s\':\n%s.", newfile, strerror(errno));
	else
	{
		if (!insert)
			strcpy(filename, newfile);
	}

	loading = 0;
	textbuf->call_modify_callbacks();
}

void save_file(char *newfile)
{
	if (textbuf->savefile(newfile))
		fl_alert("Error writing to file \'%s\':\n%s.", newfile, strerror(errno));
	else
		strcpy(filename, newfile);
	changed = 0;
	textbuf->call_modify_callbacks();
}

void copy_cb(Fl_Widget*, void* v)
{
	EditorWindow* e = (EditorWindow*)v;
	Fl_Text_Editor::kf_copy(0, e->editor);
}

void cut_cb(Fl_Widget*, void* v)
{
	EditorWindow* e = (EditorWindow*)v;
	Fl_Text_Editor::kf_cut(0, e->editor);
}

void delete_cb(Fl_Widget*, void*)
{
	textbuf->remove_selection();
}

void set_title(Fl_Window* w)
{
	if (filename[0] == '\0')
		strcpy(title, "Untitled");
	else
	{
		char *slash = strrchr(filename, '/');
#ifdef WIN32
		if (slash == NULL)
			slash = strrchr(filename, '\\');
#endif
		if (slash != NULL)
			strcpy(title, slash + 1);
		else
			strcpy(title, filename);
	}

	if (changed)
		strcat(title, " (modified)");

	w->label(title);
}

void changed_cb(int, int nInserted, int nDeleted,int, const char*, void* v)
{
	if ((nInserted || nDeleted) && !loading)
		changed = 1;
	EditorWindow *w = (EditorWindow *)v;
	set_title(w);
	if (loading)
		w->editor->show_insert_position();
}

void new_cb(Fl_Widget*, void*)
{
	if (!check_save())
		return;

	filename[0] = '\0';
	textbuf->select(0, textbuf->length());
	textbuf->remove_selection();
	changed = 0;
	textbuf->call_modify_callbacks();
}

void open_cb(Fl_Widget*, void*)
{
	if (!check_save()) return;

	char *newfile = fl_file_chooser("Open File?", "*", filename);
	if (newfile != NULL)
		load_file(newfile, -1);
}

void insert_cb(Fl_Widget*, void *v)
{
	char *newfile = fl_file_chooser("Insert File?", "*", filename);
	EditorWindow *w = (EditorWindow *)v;
	if (newfile != NULL)
		load_file(newfile, w->editor->insert_position());
}

void paste_cb(Fl_Widget*, void* v)
{
	EditorWindow* e = (EditorWindow*)v;
	Fl_Text_Editor::kf_paste(0, e->editor);
}

void close_cb(Fl_Widget*, void* v)
{
	Fl_Window* w = (Fl_Window*)v;
	if (num_windows == 1 && !check_save())
	{
		return;
	}

	w->hide();
	textbuf->remove_modify_callback(changed_cb, w);
	delete w;
	num_windows--;
	if (!num_windows)
		exit(0);
}

void quit_cb(Fl_Widget*, void*)
{
	if (changed && !check_save())
		return;

	exit(0);
}

void replace_cb(Fl_Widget*, void* v)
{
	EditorWindow* e = (EditorWindow*)v;
	e->replace_dlg->show();
}

void save_cb()
{
	if (filename[0] == '\0')
	{
		// No filename - get one!
		saveas_cb();
		return;
	}
	else save_file(filename);
}

void saveas_cb()
{
	char *newfile;

	newfile = fl_file_chooser("Save File As?", "*", filename);
	if (newfile != NULL)
		save_file(newfile);
}

>>>>>>>>>>>>>>>>>>>>

Fl_Menu_Item menuitems[] = {
  { "&File",              0, 0, 0, FL_SUBMENU },
    { "&New File",        0, (Fl_Callback *)new_cb },
    { "&Open File...",    FL_CTRL + 'o', (Fl_Callback *)open_cb },
    { "&Insert File...",  FL_CTRL + 'i', (Fl_Callback *)insert_cb, 0, FL_MENU_DIVIDER },
    { "&Save File",       FL_CTRL + 's', (Fl_Callback *)save_cb },
    { "Save File &As...", FL_CTRL + FL_SHIFT + 's', (Fl_Callback *)saveas_cb, 0, FL_MENU_DIVIDER },
    { "E&xit", FL_CTRL + 'q', (Fl_Callback *)quit_cb, 0 },
    { 0 },

  { "&Edit", 0, 0, 0, FL_SUBMENU },
    { "Cu&t",        FL_CTRL + 'x', (Fl_Callback *)cut_cb },
    { "&Copy",       FL_CTRL + 'c', (Fl_Callback *)copy_cb },
    { "&Paste",      FL_CTRL + 'v', (Fl_Callback *)paste_cb },
    { "&Delete",     0, (Fl_Callback *)delete_cb },
    { 0 },

  { "&Search", 0, 0, 0, FL_SUBMENU },
    { "&Find...",       FL_CTRL + 'f', (Fl_Callback *)find_cb },
    { "F&ind Again",    FL_CTRL + 'g', find2_cb },
    { "&Replace...",    FL_CTRL + 'r', replace_cb },
    { "Re&place Again", FL_CTRL + 't', replace2_cb },
    { 0 },

  { 0 }
};

#endif
