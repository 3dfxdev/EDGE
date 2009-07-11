//------------------------------------------------------------------------
//  EDIT LOOP
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor (C) 2001-2009 Andrew Apted
//                     (C) 1997-2003 André Majorel et al
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
//
//  Based on Yadex which incorporated code from DEU 5.21 that was put
//  in the public domain in 1994 by Raphaël Quinet and Brendon Wyber.
//
//------------------------------------------------------------------------

#include "main.h"

#include "edit2.h"
#include "checks.h"
#include "dialog.h"
#include "editloop.h"
#include "editobj.h"
#include "editsave.h"
#include "gfx.h"
#include "gotoobj.h"
#include "grid2.h"
#include "highlt.h"
#include "linedefs.h"
#include "levels.h"
#include "objects.h"
#include "s_misc.h"
#include "selbox.h"
#include "selectn.h"
#include "selpath.h"
#include "x_mirror.h"
#include "x_hover.h"
#include "xref.h"
#include "r_render.h"
#include "ui_window.h"


int active_when = 0;
int active_wmask = 0;

bool is_butl = false; //FIXME !!!!!


static int zoom_fit ();


Editor_State_c edit;


static Objid object;      // The object under the pointer

static const char *levelname;

static const Objid CANVAS (OBJ_NONE, OBJ_NO_CANVAS);


Editor_State_c::Editor_State_c()
    // FIXME !!!!
{
}

Editor_State_c::~Editor_State_c()
{
}



/* prototypes of private functions */
static int SortLevels (const void *item1, const void *item2);

/*
 *  SelectLevel
 *  Prompt the user for a level name (EnMn or MAPnm). The
 *  name chosen must be present in the master directory
 *  (iwad or pwads).
 *
 *  If <levelno> is 0, the level name can be picked from all
 *  levels present in the master directory. If <levelno> is
 *  non-zero, the level name can be picked only from those
 *  levels in the master directory for which
 *  levelname2levelno() % 1000 is equal to <levelno>. For
 *  example, if <levelno> is equal to 12, only E1M2 and
 *  MAP12 would be listed. This feature is not used anymore
 *  because "e" now requires an argument and tends to deal
 *  with ambiguous level names (like "12") itself.
 */
const char *SelectLevel (int levelno)
{
MDirPtr dir;
static char name[WAD_NAME + 1]; /* AYM it was [7] previously */
char **levels = 0;
int n = 0;           /* number of levels in the dir. that match */

get_levels_that_match:
for (dir = MasterDir; dir; dir = dir->next)
   {
   if (levelname2levelno (dir->dir.name) > 0
    && (levelno==0 || levelname2levelno (dir->dir.name) % 1000 == levelno))
      {
      if (n == 0)
   levels = (char **) GetMemory (sizeof (char *));
      else
   levels = (char **) ResizeMemory (levels, (n + 1) * sizeof (char *));
      levels[n] = dir->dir.name;
      n++;
      }
   }
if (n == 0 && levelno != 0)  /* In case no level matched levelno */
   {
   levelno = 0;               /* List ALL levels instead */
   goto get_levels_that_match;
   }
/* So that InputNameFromList doesn't fail if you
   have both EnMn's and MAPnn's in the master dir. */
qsort (levels, n, sizeof (char *), SortLevels);
strncpy (name, levels[0], WAD_NAME);
name[WAD_NAME] = 0;
//!!!! if (n == 1)
   return name;
///!!!! InputNameFromList (-1, -1, "Level name :", n, levels, name);
FreeMemory (levels);
return name;
}


/*
   compare 2 level names (for sorting)
*/

static int SortLevels (const void *item1, const void *item2)
{
/* FIXME should probably use y_stricmp() instead */
return strcmp (*((const char * const *) item1),
               *((const char * const *) item2));
}



bool DRAWING_MAP;

void ed_highlight_object (Objid& obj)
{
  edit.highlight->set (obj);
}

void ed_forget_highlight ()
{
  if (! edit.highlight->obj.is_nil())
  {
    edit.highlight->unset ();

main_win->canvas->redraw();
  }
}

void ed_draw_all()
{
  DRAWING_MAP = true;

main_win->canvas->DrawMap(); 

main_win->canvas->HighlightSelection (edit.obj_type, edit.Selected); // FIXME should be widgetized

  edit.selbox->draw();
  edit.highlight->draw();

  DRAWING_MAP = false;
}



/*
 *  zoom_fit - adjust zoom factor to make level fit in window
 *
 *  Return 0 on success, non-zero on failure.
 */
static int zoom_fit ()
{
  if (NumVertices == 0)
    return 0;

  update_level_bounds ();
  double xzoom;
  if (MapMaxX - MapMinX)
     xzoom = .95 * ScrMaxX / (MapMaxX - MapMinX);
  else
     xzoom = 1;
  double yzoom;
  if (MapMaxY - MapMinY)
     yzoom = .9 * ScrMaxY / (MapMaxY - MapMinY);
  else
     yzoom = 1;

  grid.NearestScale(MIN(xzoom, yzoom));


  grid.CenterMapAt((MapMinX + MapMaxX) / 2, (MapMinY + MapMaxY) / 2);
  return 0;
}

static void ConvertSelection(int prev_obj_type)
{
  if (! edit.Selected)
    return;

  SelPtr NewSel = 0;

  /* select all linedefs bound to the selected sectors */
  if (prev_obj_type == OBJ_SECTORS && edit.obj_type == OBJ_LINEDEFS)
  {
    int l, sd;

    for (l = 0; l < NumLineDefs; l++)
    {
      sd = LineDefs[l].sidedef1;
      if (sd >= 0 && IsSelected (edit.Selected, SideDefs[sd].sector))
        SelectObject (&NewSel, l);
      else
      {
        sd = LineDefs[l].sidedef2;
        if (sd >= 0 && IsSelected (edit.Selected, SideDefs[sd].sector))
          SelectObject (&NewSel, l);
      }
    }
    ForgetSelection (&edit.Selected);
    edit.Selected = NewSel;
  }
  /* select all Vertices bound to the selected linedefs */
  else if (prev_obj_type == OBJ_LINEDEFS && edit.obj_type == OBJ_VERTICES)
  {
    while (edit.Selected)
    {
      if (!IsSelected (NewSel, LineDefs[edit.Selected->objnum].start))
        SelectObject (&NewSel, LineDefs[edit.Selected->objnum].start);

      if (!IsSelected (NewSel, LineDefs[edit.Selected->objnum].end))
        SelectObject (&NewSel, LineDefs[edit.Selected->objnum].end);

      UnSelectObject (&edit.Selected, edit.Selected->objnum);
    }
    edit.Selected = NewSel;
  }
  /* select all sectors that have their linedefs selected */
  else if (prev_obj_type == OBJ_LINEDEFS && edit.obj_type == OBJ_SECTORS)
  {
    int l, sd;

    /* select all sectors... */
    for (l = 0; l < NumSectors; l++)
      SelectObject (&NewSel, l);
    /* ... then unselect those that should not be in the list */
    for (l = 0; l < NumLineDefs; l++)
      if (!IsSelected (edit.Selected, l))
      {
        sd = LineDefs[l].sidedef1;
        if (sd >= 0)
          UnSelectObject (&NewSel, SideDefs[sd].sector);
        sd = LineDefs[l].sidedef2;
        if (sd >= 0)
          UnSelectObject (&NewSel, SideDefs[sd].sector);
      }
    ForgetSelection (&edit.Selected);
    edit.Selected = NewSel;
  }
  /* select all linedefs that have both ends selected */
  else if (prev_obj_type == OBJ_VERTICES && edit.obj_type == OBJ_LINEDEFS)
  {
    int l;

    for (l = 0; l < NumLineDefs; l++)
      if (IsSelected (edit.Selected, LineDefs[l].start)
          && IsSelected (edit.Selected, LineDefs[l].end))
        SelectObject (&NewSel, l);
    ForgetSelection (&edit.Selected);
    edit.Selected = NewSel;
  }
  /* unselect all */
  else
    ForgetSelection (&edit.Selected);
}

static void HighlightObj(Objid& obj)
{
  edit.highlighted = obj;

  int h_type = obj.type;

  if (obj.is_nil())
    h_type = edit.obj_type;

  if (obj ())
    ed_highlight_object(edit.highlighted);
  else
    ed_forget_highlight();


  int obj_idx = obj.num;

  if (obj.is_nil() && edit.Selected)
  {
    obj_idx = edit.Selected->objnum;
  }

  switch (h_type)
  {
    case OBJ_THINGS:
      main_win->thing_box->SetObj(obj_idx);
      break;

    case OBJ_LINEDEFS:
      main_win->line_box->SetObj(obj_idx);
      break;

    case OBJ_SECTORS:
      main_win->sec_box->SetObj(obj_idx);
      break;

    case OBJ_VERTICES:
      main_win->vert_box->SetObj(obj_idx);
      break;

    //!!!!!!
    default: break;
  }
}


void EditorWheel(int delta)
{
    float S1 = grid.Scale;

    grid.AdjustScale((delta > 0) ? +1 : -1);
    grid.RefocusZoom(edit.map_x, edit.map_y, S1);
}


void EditorKey(int is_key, bool is_shift)
{

  // [Ctrl][L]: force redraw
  if (is_key == '\f')
  {
    edit.RedrawMap = 1;
  }

  // [Esc], [q]: close
  else if (is_key == FL_Escape || is_key == 'q')
  {
    {
      ForgetSelection (&edit.Selected);
      if (!MadeChanges
          || Confirm (-1, -1, "You have unsaved changes."
            " Do you really want to quit?", 0))
        {
          main_win->action = UI_MainWin::QUIT;
          return;
        }
      edit.RedrawMap = 1;
    }
  }

  // [Shift][F1]: save a screen shot.
  // FIXME doesn't work in the Unix port
  else if (is_key == FL_F+1 + FL_SHIFT)
  {
    Beep();
  }


  /* [F2] save level into pwad, prompt for the file name
     every time but keep the same level name. */
  else if (is_key == FL_F+2 && Registered)
  {
    if (! CheckStartingPos ())
      goto cancel_save;
    char *outfile;
    const char *newlevelname;
    if (levelname)
      newlevelname = levelname;
    else
    {
      newlevelname = SelectLevel (0);
      if (! *newlevelname)
        goto cancel_save;
    }
    outfile = GetWadFileName (newlevelname);
    if (! outfile)
      goto cancel_save;
    SaveLevelData (outfile, newlevelname);
    levelname = newlevelname;
    // Sigh. Shouldn't have to do that. Level must die !
    Level = FindMasterDir (MasterDir, levelname);
cancel_save:
    edit.RedrawMap = 1;
  }

  /* [F3] save level into pwad, prompt for the file name and
     level name. */
  else if (is_key == FL_F+3 && Registered)
  {
    char *outfile;
    const char *newlevelname;
    MDirPtr newLevel, oldl, newl;

    if (! CheckStartingPos ())
      goto cancel_save_as;
    newlevelname = SelectLevel (0);
    if (! *newlevelname)
      goto cancel_save_as;
    if (! levelname || y_stricmp (newlevelname, levelname))
    {
      /* horrible but it works... */
      // Horrible indeed -- AYM 1999-07-30
      newLevel = FindMasterDir (MasterDir, newlevelname);
      if (! newLevel)
        nf_bug ("newLevel is NULL");  // Debatable ! -- AYM 2001-05-29
      if (Level)  // If new level ("create" command), Level is NULL
      {
        oldl = Level;
        newl = newLevel;
        for (int m = 0; m < 11; m++)
        {
          newl->wadfile = oldl->wadfile;
          if (m > 0)
            newl->dir = oldl->dir;
          /*
             if (!fncmp (outfile, oldl->wadfile->filename))
             {
             oldl->wadfile = WadFileList;
             oldl->dir = lost...
             }
             */
          oldl = oldl->next;
          newl = newl->next;
        }
      }
      Level = newLevel;
    }
    outfile = GetWadFileName (newlevelname);
    if (! outfile)
      goto cancel_save_as;
    SaveLevelData (outfile, newlevelname);
    levelname = newlevelname;
cancel_save_as:
    edit.RedrawMap = 1;
  }

#if 0 //!!!
  // [F5]: pop up the "Preferences" menu
  else if (is_key == FL_F+5)
  {
    Preferences (-1, -1);
    edit.RedrawMap = 1;
  }
#endif

#if 0 //!!!!!
  // [a]: pop up the "Set flag" menu
  else if (is_key == 'a'
      && edit.menubar->highlighted () < 0
      && (edit.Selected || edit.highlighted ()))
  {
    if (edit.obj_type == OBJ_LINEDEFS)
    {
      menu_linedef_flags->set_title ("Set linedef flag");
      e.modpopup->set (menu_linedef_flags, 0);
    }
    else if (edit.obj_type == OBJ_THINGS)
    {
      menu_thing_flags->set_title ("Set thing flag");
      e.modpopup->set (menu_thing_flags, 0);
    }
  }

  // [b]: pop up the "Toggle flag" menu
  else if (is_key == 'b'
      && edit.menubar->highlighted () < 0
      && (edit.Selected || edit.highlighted ()))
  {
    if (edit.obj_type == OBJ_LINEDEFS)
    {
      menu_linedef_flags->set_title ("Toggle linedef flag");
      e.modpopup->set (menu_linedef_flags, 0);
    }
    else if (edit.obj_type == OBJ_THINGS)
    {
      menu_thing_flags->set_title ("Toggle thing flag");
      e.modpopup->set (menu_thing_flags, 0);
    }
  }

  // [c]: pop up the "Clear flag" menu
  else if (is_key == 'c'
      && edit.menubar->highlighted () < 0
      && (edit.Selected || edit.highlighted ()))
  {
    if (edit.obj_type == OBJ_LINEDEFS)
    {
      menu_linedef_flags->set_title ("Clear linedef flag");
      e.modpopup->set (menu_linedef_flags, 0);
    }
    else if (edit.obj_type == OBJ_THINGS)
    {
      menu_thing_flags->set_title ("Clear thing flag");
      e.modpopup->set (menu_thing_flags, 0);
    }
  }
#endif

  // [M]: pop up the "Misc. operations" menu
  else if (is_key == 'M')
  {
///---    edit.modpopup->set (edit.menubar->get_menu (MBI_MISC), 1);
  }

  // [F9]: pop up the "Insert a standard object" menu
  else if (is_key == FL_F+9)
  {
///---    edit.modpopup->set (edit.menubar->get_menu (MBI_OBJECTS), 1);
  }

  // [F10]: pop up the "Checks" menu
  else if (is_key == FL_F+10)
  {
    CheckLevel (-1, -1);
    edit.RedrawMap = 1;
  }


  // [+], [=], wheel: zooming in
  else if (is_key == '+' || is_key == '=')
  {
    EditorWheel(+1);
  }

  // [-], [_], wheel: zooming out
  else if (is_key == '-' || is_key == '_')
  {
    EditorWheel(-1);
  }

  // [`]: centre window on centre of map
  // and set zoom to view the entire map
  else if (is_key == '`' || is_key == '~')
  {
    zoom_fit ();
    edit.RedrawMap = 1;
  }

  // [1] - [9]: set the zoom factor
  else if (is_key >= '1' && is_key <= '9')
  {
    float S1 = grid.Scale;

    grid.ScaleFromDigit(is_key - '0');
    grid.RefocusZoom(edit.map_x, edit.map_y, S1);
  }

  // [']: centre window on centre of map
  else if (is_key == '\'')
  {
    update_level_bounds ();
    grid.CenterMapAt ((MapMinX + MapMaxX) / 2, (MapMinY + MapMaxY) / 2);
    edit.RedrawMap = 1;
  }

  // [Left], [Right], [Up], [Down]:
  // scroll <scroll_less> percents of a screenful.
  else if (is_key == FL_Left && grid.orig_x > -30000)
  {
    grid.orig_x -= (int) ((double) ScrMaxX * scroll_less / 100 / grid.Scale);
    edit.RedrawMap = 1;
  }
  else if (is_key == FL_Right && grid.orig_x < 30000)
  {
    grid.orig_x += (int) ((double) ScrMaxX * scroll_less / 100 / grid.Scale);
    edit.RedrawMap = 1;
  }
  else if (is_key == FL_Up && grid.orig_y < 30000)
  {
    grid.orig_y += (int) ((double) ScrMaxY * scroll_less / 100 / grid.Scale);
    edit.RedrawMap = 1;
  }
  else if (is_key == FL_Down && grid.orig_y > -30000)
  {
    grid.orig_y -= (int) ((double) ScrMaxY * scroll_less / 100 / grid.Scale);
    edit.RedrawMap = 1;
  }

  // [Pgup], [Pgdn], [Home], [End]:
  // scroll <scroll_more> percents of a screenful.
  else if (is_key == FL_Page_Up && (grid.orig_y) < /*MapMaxY*/ 20000)
  {
    grid.orig_y += (int) ((double) ScrMaxY * scroll_more / 100 / grid.Scale);
    edit.RedrawMap = 1;
  }
  else if (is_key == FL_Page_Down && (grid.orig_y) > /*MapMinY*/ -20000)
  {
    grid.orig_y -= (int) ((double) ScrMaxY * scroll_more / 100 / grid.Scale);
    edit.RedrawMap = 1;
  }
  else if (is_key == FL_Home && (grid.orig_x) > /*MapMinX*/ -20000)
  {
    grid.orig_x -= (int) ((double) ScrMaxX * scroll_more / 100 / grid.Scale);
    edit.RedrawMap = 1;
  }
  else if (is_key == FL_End && (grid.orig_x) < /*MapMaxX*/ 20000)
  {
    grid.orig_x += (int) ((double) ScrMaxX * scroll_more / 100 / grid.Scale);
    edit.RedrawMap = 1;
  }

#if 0
  /* user wants to change the movement speed */
  else if (is_key == ' ')
    edit.move_speed = edit.move_speed == 1 ? 20 : 1;

  else if (is_key == ' ')
  {
    edit.extra_zoom = ! edit.extra_zoom;
    edit_set_zoom (&edit, Scale * (edit.extra_zoom ? 4 : 0.25));
    edit.RedrawMap = 1;
  }
#endif

  // [l], [s], [t], [v]: switch mode
  else if (is_key == 't' || is_key == 'v' || is_key == 'l' || is_key == 's' || is_key == 'r')
  {
    int prev_obj_type = edit.obj_type;

    main_win->SetMode((char)is_key);

    // Set the object type according to the new mode.
    switch((char)is_key)
    {
      case 't': edit.obj_type = OBJ_THINGS;   break;
      case 'l': edit.obj_type = OBJ_LINEDEFS; break;
      case 's': edit.obj_type = OBJ_SECTORS;  break;
      case 'v': edit.obj_type = OBJ_VERTICES; break;
      case 'r': edit.obj_type = OBJ_RSCRIPT;  break;

      default:
        FatalError ("changing mode with %04X", is_key);
    }

    ConvertSelection(prev_obj_type);

    if (GetMaxObjectNum (edit.obj_type) >= 0 && Select0)
    {
      edit.highlighted.type = edit.obj_type;
      edit.highlighted.num  = 0;
    }
    else
      edit.highlighted.nil ();

    edit.RedrawMap = 1;
  }

  // [e]: Select/unselect all linedefs in non-forked path
  else if (is_key == 'e' && edit.highlighted._is_linedef ())
  {
    ForgetSelection (&edit.Selected);
    select_linedefs_path (&edit.Selected, edit.highlighted.num, BOP_ADD);
    edit.RedrawMap = 1;
  }

  // [Ctrl][e] Select/unselect all linedefs in path
  else if (is_key == '\5' && ! is_shift && edit.highlighted._is_linedef ())
  {
    select_linedefs_path (&edit.Selected, edit.highlighted.num, BOP_TOGGLE);
    edit.RedrawMap = 1;
  }
  // [E]: add linedef and split sector -- [AJA]
  else if (is_key == 'E' && edit.obj_type == OBJ_VERTICES)
  {
    if (edit.Selected)
    {
      MiscOperations (edit.obj_type, &edit.Selected, 5);
      edit.RedrawMap = 1;
    }
  }
  // [E]: Select/unselect all 1s linedefs in path
  else if (is_key == 'E' && edit.highlighted._is_linedef ())
  {
    ForgetSelection (&edit.Selected);
    select_1s_linedefs_path (&edit.Selected, edit.highlighted.num, BOP_ADD);
    edit.RedrawMap = 1;
  }

  // [Ctrl][Shift][e]: Select/unselect all 1s linedefs in path
  else if (is_key == '\5' && is_shift && edit.highlighted._is_linedef ())
  {
    select_1s_linedefs_path (&edit.Selected, edit.highlighted.num, BOP_TOGGLE);
    edit.RedrawMap = 1;
  }

  // [G]: to increase the grid step
  else if (is_key == 'G')
  {
    grid.AdjustStep(+1);
  }

  // [g]: decrease the grid step
  else if (is_key == 'g')
  {
    grid.AdjustStep(-1);
  }

  // [h]: display or hide the grid
  else if (is_key == 'h')
  {
    grid.shown = ! grid.shown;
    edit.RedrawMap = 1;
  }

//???  // [H]: reset the grid to grid_step_max
//???  else if (is_key == 'H')
//???  {
//???    e.grid_step = e.grid_step_max;
//???    edit.RedrawMap = 1;
//???  }

  // [f]: toggle the snap_to_grid flag
  else if (is_key == 'f')
  {
    grid.locked = false;
    grid.snap   = ! grid.snap;

    main_win->info_bar->UpdateLock();
  }

  // [k]: toggle the lock_grip_step flag
  else if (is_key == 'k')
  {
    grid.locked = ! grid.locked;
    grid.snap   = true;

    main_win->info_bar->UpdateLock();
  }

  // [r]: toggle the rulers
  else if (is_key == 'r')
    edit.rulers_shown = !edit.rulers_shown;

  // [n], [>]: highlight the next object
  else if ((is_key == 'n' || is_key == '>')
      && ( edit.highlighted ()))
  {
    obj_type_t t = edit.highlighted () ? edit.highlighted.type : edit.obj_type;
    obj_no_t nmax = GetMaxObjectNum (t);
    if (is_obj (nmax))
    {
      if (edit.highlighted.is_nil ())
      {
        edit.highlighted.type = t;
        edit.highlighted.num = 0;
      }
      else
      {
        edit.highlighted.num++;
        if (edit.highlighted.num > nmax)
          edit.highlighted.num = 0;
      }
      GoToObject (edit.highlighted);
      edit.RedrawMap = 1;
    }
  }

  // [p], [<]: highlight the previous object
  else if ((is_key == 'p' || is_key == '<')
      && ( edit.highlighted ()))
  {
    obj_type_t t = edit.highlighted () ? edit.highlighted.type : edit.obj_type;
    obj_no_t nmax = GetMaxObjectNum (t);
    if (is_obj (nmax))
    {
      if (edit.highlighted.is_nil ())
      {
        edit.highlighted.type = t;
        edit.highlighted.num = nmax;
      }
      else
      {
        edit.highlighted.num--;
        if (edit.highlighted.num < 0)
          edit.highlighted.num = nmax;
      }
      GoToObject (edit.highlighted);
      edit.RedrawMap = 1;
    }
  }

  // [j], [#]: jump to object by number
  else if ((is_key == 'j' || is_key == '#')
      && ( edit.highlighted ()))
  {
    const char *buf = fl_input("Enter index", "");

    if (buf)
    {
      Objid target_obj;
      target_obj.type = edit.obj_type;
      target_obj.num  = atoi(buf);

      if (target_obj ())
      {
        GoToObject (target_obj);
        edit.RedrawMap = 1;
      }
    }
  }

  // [f]: find object by type
  else if (is_key == 'f' && ( edit.highlighted ())) 
  {
    Objid find_obj;
    int otype;
    obj_no_t omax,onum;
    find_obj.type = edit.highlighted () ? edit.highlighted.type : edit.obj_type;
    onum = find_obj.num  = edit.highlighted () ? edit.highlighted.num  : 0;
    omax = GetMaxObjectNum(find_obj.type);
    switch (find_obj.type)
    {
      case OBJ_SECTORS:
        if ( ! InputSectorType( 84, 21, &otype))
        {
          for (onum = edit.highlighted () ? onum + 1 : onum; onum <= omax; onum++)
            if (Sectors[onum].special == (wad_stype_t) otype)
            {
              find_obj.num = onum;
              GoToObject(find_obj);
              break;
            }
        }
        break;
      case OBJ_THINGS:
        if ( ! InputThingType( 42, 21, &otype))
        {
          for (onum = edit.highlighted () ? onum + 1 : onum; onum <= omax; onum++)
            if (Things[onum].type == (wad_ttype_t) otype)
            {
              find_obj.num = onum;
              GoToObject(find_obj);
              break;
            }
        }
        break;
      case OBJ_LINEDEFS:
        if ( ! InputLinedefType( 0, 21, &otype))
        {
          for (onum = edit.highlighted () ? onum + 1 : onum; onum <= omax; onum++)
            if (LineDefs[onum].type == (wad_ldtype_t) otype)
            {
              find_obj.num = onum;
              GoToObject(find_obj);
              break;
            }
        }
        break;
    }
    edit.RedrawMap = 1;
  }
#if 0
  // [c]: clear selection and redraw the map
  else if (is_key == 'c')
  {
    ForgetSelection (&edit.Selected);
    edit.RedrawMap = 1;
  }
#endif

  // [o]: copy a group of objects
  else if (is_key == 'o'
      && (edit.Selected || edit.highlighted ()))
  {
    int x, y;

    /* copy the object(s) */
    if (! edit.Selected)
      SelectObject (&edit.Selected, edit.highlighted.num);
    CopyObjects (edit.obj_type, edit.Selected);
    /* enter drag mode */
    /* AYM 19980619 : got to look into this!! */
    //edit.highlight_obj_no = edit.Selected->objnum;

    // Find the "hotspot" in the object(s)
    if (edit.highlighted () && ! edit.Selected)
      GetObjectCoords (edit.highlighted.type, edit.highlighted.num, &x, &y);
    else
      centre_of_objects (edit.obj_type, edit.Selected, &x, &y);

    // Drag the object(s) so that the "hotspot" is under the pointer
    MoveObjectsToCoords (edit.obj_type, 0, x, y, 0);
    MoveObjectsToCoords (edit.obj_type, edit.Selected,
        edit.map_x, edit.map_y, 0);
    edit.RedrawMap = 1;
  }


  // [w]: spin things 1/8 turn counter-clockwise
  else if (is_key == 'w' && edit.obj_type == OBJ_THINGS
      && (edit.Selected || edit.highlighted ()))
  {
    if (! edit.Selected)
    {
      SelectObject (&edit.Selected, edit.highlighted.num);
      spin_things (edit.Selected, 45);
      UnSelectObject (&edit.Selected, edit.highlighted.num);
    }
    else
    {
      spin_things (edit.Selected, 45);
    }
    edit.RedrawMap = 1;  /* FIXME: should redraw only the things */
  }

  // [w]: split linedefs and sectors
  else if (is_key == 'w' && edit.obj_type == OBJ_LINEDEFS
      && edit.Selected && edit.Selected->next && ! edit.Selected->next->next)
  {
    SplitLineDefsAndSector (edit.Selected->next->objnum, edit.Selected->objnum);
    ForgetSelection (&edit.Selected);
    edit.RedrawMap = 1;
  }

  // [w]: split sector between vertices
  else if (is_key == 'w' && edit.obj_type == OBJ_VERTICES
      && edit.Selected && edit.Selected->next && ! edit.Selected->next->next)
  {
    SplitSector (edit.Selected->next->objnum, edit.Selected->objnum);
    ForgetSelection (&edit.Selected);
    edit.RedrawMap = 1;
  }

  // [x]: spin things 1/8 turn clockwise
  else if (is_key == 'x' && edit.obj_type == OBJ_THINGS
      && (edit.Selected || edit.highlighted ()))
  {
    if (! edit.Selected)
    {
      SelectObject (&edit.Selected, edit.highlighted.num);
      spin_things (edit.Selected, -45);
      UnSelectObject (&edit.Selected, edit.highlighted.num);
    }
    else
    {
      spin_things (edit.Selected, -45);
    }
    edit.RedrawMap = 1;  /* FIXME: should redraw only the things */
  }

  // [x]: split linedefs
  else if (is_key == 'x' && edit.obj_type == OBJ_LINEDEFS
      && (edit.Selected || edit.highlighted ()))
  {
    if (! edit.Selected)
    {
      SelectObject (&edit.Selected, edit.highlighted.num);
      SplitLineDefs (edit.Selected);
      UnSelectObject (&edit.Selected, edit.highlighted.num);
    }
    else
      SplitLineDefs (edit.Selected);
    edit.RedrawMap = 1;
  }

  // [Ctrl][x]: exchange objects numbers
  else if (is_key == 24)
  {
    if (! edit.Selected
        || ! edit.Selected->next
        || (edit.Selected->next)->next)
    {
      Beep ();
      Notify (-1, -1, "You must select exactly two objects", 0);
      edit.RedrawMap = 1;
    }
    else
    {
      exchange_objects_numbers (edit.obj_type, edit.Selected, true);
      edit.RedrawMap = 1;
    }
  }

  // [Ctrl][k]: cut a slice out of a sector
  else if (is_key == 11 && edit.obj_type == OBJ_LINEDEFS
      && edit.Selected && edit.Selected->next && ! edit.Selected->next->next)
  {
    sector_slice (edit.Selected->next->objnum, edit.Selected->objnum);
    ForgetSelection (&edit.Selected);
    edit.RedrawMap = 1;
  }

  // [Del]: delete the current object
  else if ((is_key == '\b' || is_key == FL_BackSpace || is_key == FL_Delete)
      && (edit.Selected || edit.highlighted ())) /* 'Del' */
  {
    if (edit.obj_type == OBJ_THINGS
        || Expert
        || Confirm (-1, -1,
          (edit.Selected && edit.Selected->next ?
           "Do you really want to delete these objects?"
           : "Do you really want to delete this object?"),
          (edit.Selected && edit.Selected->next ?
           "This will also delete the objects bound to them."
           : "This will also delete the objects bound to it.")))
    {
      if (edit.Selected)
        DeleteObjects (edit.obj_type, &edit.Selected);
      else
        DeleteObject (edit.highlighted);
    }
    // AYM 1998-09-20 I thought I'd add this
    // (though it doesn't fix the problem : if the object has been
    // deleted, HighlightObject is still called with a bad object#).
    edit.highlighted.nil ();
    edit.RedrawMap = 1;
  }

  // [Ins]: insert a new object
  else if (is_key == 'I' || is_key == FL_Insert + FL_SHIFT) /* 'Ins' */
  {
    SelPtr cur;
    int prev_obj_type = edit.obj_type;

    /* first special case: if several vertices are
       selected, add new linedefs */
    if (edit.obj_type == OBJ_VERTICES
        && edit.Selected && edit.Selected->next)
    {
      int firstv;
      int obj_no = OBJ_NO_NONE;
      
      if (edit.Selected->next->next)
        firstv = edit.Selected->objnum;
      else
        firstv = -1;
      edit.obj_type = OBJ_LINEDEFS;
      /* create linedefs between the vertices */
      for (cur = edit.Selected; cur->next; cur = cur->next)
      {
        /* check if there is already a linedef between the two vertices */
        for (obj_no = 0; obj_no < NumLineDefs; obj_no++)
          if ((LineDefs[obj_no].start == cur->next->objnum
                && LineDefs[obj_no].end   == cur->objnum)
              || (LineDefs[obj_no].end   == cur->next->objnum
                && LineDefs[obj_no].start == cur->objnum))
            break;
        if (obj_no < NumLineDefs)
          cur->objnum = obj_no;
        else
        {
          InsertObject (OBJ_LINEDEFS, -1, 0, 0);
          edit.highlighted.type = OBJ_LINEDEFS;
          edit.highlighted.num  = NumLineDefs - 1;
          LineDefs[edit.highlighted.num].start = cur->next->objnum;
          LineDefs[edit.highlighted.num].end = cur->objnum;
          cur->objnum = edit.highlighted.num;  // FIXME cur = edit.highlighted
        }
      }
      /* close the polygon if there are more than 2 vertices */
      if (firstv >= 0 && is_shift)
      {
        edit.highlighted.type = OBJ_LINEDEFS;
        for (edit.highlighted.num = 0;
            edit.highlighted.num < NumLineDefs;
            edit.highlighted.num++)
          if ((LineDefs[edit.highlighted.num].start == firstv
                && LineDefs[edit.highlighted.num].end   == cur->objnum)
              || (LineDefs[edit.highlighted.num].end   == firstv
                && LineDefs[edit.highlighted.num].start == cur->objnum))
            break;
        if (edit.highlighted.num < NumLineDefs)
          cur->objnum = obj_no;
        else
        {
          InsertObject (OBJ_LINEDEFS, -1, 0, 0);
          edit.highlighted.type = OBJ_LINEDEFS;
          edit.highlighted.num  = NumLineDefs - 1;
          LineDefs[edit.highlighted.num].start = firstv;
          LineDefs[edit.highlighted.num].end   = cur->objnum;
          cur->objnum = edit.highlighted.num;  // FIXME cur = edit.highlighted
        }
      }
      else
        UnSelectObject (&edit.Selected, cur->objnum);
    }
    /* second special case: if several linedefs are selected,
       add new sidedefs and one sector */
    else if (edit.obj_type == OBJ_LINEDEFS && edit.Selected)
    {
      
      for (cur = edit.Selected; cur; cur = cur->next)
        if (LineDefs[cur->objnum].sidedef1 >= 0
            && LineDefs[cur->objnum].sidedef2 >= 0)
        {
          char msg[80];

          Beep ();
          sprintf (msg, "Linedef #%d already has two sidedefs", cur->objnum);
          Notify (-1, -1, "Error: cannot add the new sector", msg);
          break;
        }
      if (! cur)
      {
        edit.obj_type = OBJ_SECTORS;
        InsertObject (OBJ_SECTORS, -1, 0, 0);
        edit.highlighted.type = OBJ_SECTORS;
        edit.highlighted.num  = NumSectors - 1;
        for (cur = edit.Selected; cur; cur = cur->next)
        {
          InsertObject (OBJ_SIDEDEFS, -1, 0, 0);
          SideDefs[NumSideDefs - 1].sector = edit.highlighted.num;
          
          if (LineDefs[cur->objnum].sidedef1 >= 0)
          {
            int s;

            s = SideDefs[LineDefs[cur->objnum].sidedef1].sector;
            if (s >= 0)
            {
              Sectors[edit.highlighted.num].floorh = Sectors[s].floorh;
              Sectors[edit.highlighted.num].ceilh = Sectors[s].ceilh;
              strncpy (Sectors[edit.highlighted.num].floort,
                  Sectors[s].floort, WAD_FLAT_NAME);
              strncpy (Sectors[edit.highlighted.num].ceilt,
                  Sectors[s].ceilt, WAD_FLAT_NAME);
              Sectors[edit.highlighted.num].light = Sectors[s].light;
            }
            LineDefs[cur->objnum].sidedef2 = NumSideDefs - 1;
            LineDefs[cur->objnum].flags = 4;
            strncpy (SideDefs[NumSideDefs - 1].middle,
                "-", WAD_TEX_NAME);
            strncpy (SideDefs[LineDefs[cur->objnum].sidedef1].middle,
                "-", WAD_TEX_NAME);
          }
          else
            LineDefs[cur->objnum].sidedef1 = NumSideDefs - 1;
        }
        ForgetSelection (&edit.Selected);
        SelectObject (&edit.Selected, edit.highlighted.num);
      }
    }
    /* normal case: add a new object of the current type */
    else
    {
      ForgetSelection (&edit.Selected);
      /* FIXME how do you insert a new object of type T if
         no object of that type already exists ? */
      obj_type_t t = edit.highlighted () ? edit.highlighted.type : edit.obj_type;
      InsertObject (t, edit.highlighted.num,
          grid.SnapX(edit.map_x), grid.SnapY(edit.map_y));
      edit.highlighted.type = t;
      edit.highlighted.num  = GetMaxObjectNum (edit.obj_type);
      if (edit.obj_type == OBJ_LINEDEFS)
      {
#if 0  // TODO
        int v1 = LineDefs[edit.highlighted.num].start;
        int v2 = LineDefs[edit.highlighted.num].end;
        if (! Input2VertexNumbers (-1, -1,
              "Choose the two vertices for the new linedef", &v1, &v2))
        {
          DeleteObject (edit.highlighted);
          edit.highlighted.nil ();
        }
        else
        {
          LineDefs[edit.highlighted.num].start = v1;
          LineDefs[edit.highlighted.num].end   = v2;
        }
#endif
      }
      else if (edit.obj_type == OBJ_VERTICES)
      {
        SelectObject (&edit.Selected, edit.highlighted.num);
        if (AutoMergeVertices (&edit.Selected, edit.obj_type, 'i'))
          edit.RedrawMap = 1;
        ForgetSelection (&edit.Selected);
      }
    }

    if (edit.obj_type != prev_obj_type)
    {
      // FIXME !!!
    }

    edit.RedrawMap = 1;
  }

  // [Z] Set sector on surrounding linedefs (AJA)
  else if (is_key == 'Z' && edit.pointer_in_window) 
  {
    if (edit.obj_type == OBJ_SECTORS && edit.Selected)
    {
      SuperSectorSelector (edit.map_x, edit.map_y,
          edit.Selected->objnum);
    }
    else
    {
      SuperSectorSelector (edit.map_x, edit.map_y, OBJ_NO_NONE);
    }
    edit.RedrawMap = 1;
  }

  // [W] limit shown things to specific skill (AJA)
  else if (is_key == 'W' && edit.obj_type == OBJ_THINGS)
  {
    active_wmask ^= 1;
    active_when = active_wmask;
    edit.RedrawMap = 1;
  }

  // [!] Debug info (not documented)
  else if (is_key == '!')
  {
    DumpSelection (edit.Selected);
  }


  // [T] Transfer properties to selected objects (AJA)
  else if (is_key == 'T' && edit.Selected 
      && edit.highlighted.num >= 0)
  {
    switch (edit.obj_type)
    {
      case OBJ_SECTORS:
        TransferSectorProperties (edit.highlighted.num, edit.Selected);
        edit.RedrawMap = 1;
        break;
      case OBJ_THINGS:
        TransferThingProperties (edit.highlighted.num, edit.Selected);
        edit.RedrawMap = 1;
        break;
      case OBJ_LINEDEFS:
        TransferLinedefProperties (edit.highlighted.num, edit.Selected);
        edit.RedrawMap = 1;
        break;
      default:
        Beep ();
        break;
    }
  }


  // [Ctrl][b] Select linedefs whose sidedefs reference non-existant sectors
  else if (is_key == 2)
  {
    bad_sector_number (&edit.Selected);
    edit.RedrawMap = 1;
  }

  // [Ctrl][r] Xref for sidedef (not documented)
  else if (is_key == 18)
  {
    xref_sidedef ();
  }

  // [Ctrl][s] List secret sectors (not documented)
  else if (is_key == 19)
  {
    secret_sectors ();
  }

  // [Ctrl][t] List tagged linedefs or sectors
  else if (is_key == 20)
  {
    if (edit.highlighted._is_sector ())
      list_tagged_linedefs (Sectors[edit.highlighted.num].tag);
    else if (edit.highlighted._is_linedef ())
      list_tagged_sectors (LineDefs[edit.highlighted.num].tag);
    else
      Beep ();
  }

  // [Ctrl][u] Select linedefs with unknown type (not documented)
  else if (is_key == 21)
  {
    unknown_linedef_type (&edit.Selected);
    edit.RedrawMap = 1;
  }


  // [&] Show object numbers
  else if (is_key == '&')
  {
    edit.show_object_numbers = ! edit.show_object_numbers;
    edit.RedrawMap = 1;
  }

  // [%] Show things sprites
  else if (is_key == '%')
  {
    edit.show_things_sprites = ! edit.show_things_sprites;
    edit.show_things_squares = ! edit.show_things_sprites;  // Not a typo !
    edit.RedrawMap = 1;
  }

  /* user likes music */
  else if (is_key)
  {
fprintf(stderr, "UNKNOWN KEY: %d (0x%04x)\n", is_key, is_key);
    Beep ();
  }
}


void EditorMousePress(bool is_ctrl)
{
fprintf(stderr, "MOUSE PRESS !!!\n");

  is_butl = true;

   /* Clicking on an empty space starts a new selection box.
      Unless [Ctrl] is pressed, it also clears the current selection. */
  if (true
      && object.is_nil ())
  {
    edit.clicked    = CANVAS;
    edit.click_ctrl = is_ctrl;
    if (! is_ctrl)
    {
      ForgetSelection (&edit.Selected);
      edit.RedrawMap = 1;
    }
    edit.selbox->set_1st_corner (edit.map_x, edit.map_y);
    edit.selbox->set_2nd_corner (edit.map_x, edit.map_y);

main_win->canvas->redraw();
    return;
  }

  /* Clicking on an unselected object unselects
     everything but that object. Additionally,
     we write the number of the object in case
     the user is about to drag it. */
  if (! is_ctrl
      && ! IsSelected (edit.Selected, object.num))
  {
    edit.clicked        = object;
    edit.click_ctrl     = 0;
    edit.click_time     = 0; // is.time;
    
    //@@@@@
    // if (edit.Selected == NOTHING)
    
    ForgetSelection (&edit.Selected);
    SelectObject (&edit.Selected, object.num);

    /* I don't like having to do that */
    if (object.type == OBJ_THINGS && object ())
      MoveObjectsToCoords (object.type, 0,
          Things[object.num].x, Things[object.num].y, 0);
    else if (object.type == OBJ_VERTICES && object ())
      MoveObjectsToCoords (object.type, 0,
          Vertices[object.num].x, Vertices[object.num].y, 0);
    else
      MoveObjectsToCoords (object.type, 0,
          edit.map_x, edit.map_y, grid.snap ? grid.step : 0);
    edit.RedrawMap = 1;

main_win->canvas->redraw();
    return;
  }

#if 0  /* Second click of a double click on an object */
  if (! is_ctrl
      && IsSelected (edit.Selected, object.num)
      && object  == edit.clicked
      && is.time - edit.click_time <= (unsigned long) double_click_timeout)
  {
    // Very important! If you don't do that, the release of the
    // click that closed the properties menu will drag the object.
    edit.clicked.nil ();
    send_event (YK_RETURN);

main_win->canvas->redraw();
    return;
  }
#endif

  /* Clicking on a selected object does nothing ;
     the user might want to drag the selection. */
  if (! is_ctrl
      && IsSelected (edit.Selected, object.num))
  {
    edit.clicked        = object;
    edit.click_ctrl     = 0;
    edit.click_time     = 0; /// is.time;
    /* I don't like having to do that */
    if (object.type == OBJ_THINGS && object ())
      MoveObjectsToCoords (object.type, 0,
          Things[object.num].x, Things[object.num].y, 0);
    else if (object.type == OBJ_VERTICES && object ())
      MoveObjectsToCoords (object.type, 0,
          Vertices[object.num].x, Vertices[object.num].y, 0);
    else
      MoveObjectsToCoords (object.type, 0,
          edit.map_x, edit.map_y, grid.snap ? grid.step : 0);

main_win->canvas->redraw();
    return;
  }

  /* Clicking on selected object with [Ctrl] pressed unselects it.
     Clicking on unselected object with [Ctrl] pressed selects it. */
  if (is_ctrl
      && object ())
  {
    edit.clicked        = object;
    edit.click_ctrl     = 1;
    if (IsSelected (edit.Selected, object.num))
      UnSelectObject (&edit.Selected, object.num);
    else
      SelectObject (&edit.Selected, object.num);
    edit.RedrawMap = 1;

main_win->canvas->redraw();
    return;
  }

}

void EditorMouseRelease()
{
  is_butl = false;

   /* Releasing the button while there was a selection box
      causes all the objects within the box to be selected. */
  if (true
      && edit.clicked == CANVAS)
  {
    int x1, y1, x2, y2;
    edit.selbox->get_corners (&x1, &y1, &x2, &y2);
    SelectObjectsInBox (&edit.Selected, edit.obj_type, x1, y1, x2, y2);
    edit.selbox->unset_corners ();
    edit.RedrawMap = 1;

    return;
  }

  /* Releasing the button while dragging : drop the selection. */
  // FIXME : should call this automatically when switching tool
  if (true
      && edit.clicked ())
  {
    if (AutoMergeVertices (&edit.Selected, edit.obj_type, 'm'))
      edit.RedrawMap = 1;

    return;
  }

}

void EditorMouseMotion(int x, int y, int map_x, int map_y, bool drag)
{
  edit.map_x = map_x;
  edit.map_y = map_y;
  edit.pointer_in_window = true; // FIXME

  if (edit.pointer_in_window)
    main_win->info_bar->SetMouse(edit.map_x, edit.map_y);

// fprintf(stderr, "MOUSE MOTION: %d,%d  map: %d,%d\n", x, y, edit.map_x, edit.map_y);

  if (! drag)
  {
    obj_type_t t = edit.obj_type;

    GetCurObject (object, t, edit.map_x, edit.map_y);

    if (! (object == edit.highlighted)) //!!!! (no button and no CTRL)
    {
      HighlightObj(object);
    }

  }

        
  /* Moving the pointer with the left button pressed
     and a selection box exists : move the second
     corner of the selection box. */
  else if (true
      && is_butl  /* FIXME: edit.selecting */
      && edit.clicked == CANVAS)
  {
    edit.selbox->set_2nd_corner (edit.map_x, edit.map_y);

    return;
  }

  /* Moving the pointer with the left button pressed
     but no selection box exists and [Ctrl] was not
     pressed when the button was pressed :
     drag the selection. */
  if (true
      && is_butl  /* FIXME: edit.dragging */
      && edit.clicked ()
      && ! edit.click_ctrl)
  {
fprintf(stderr, "DRAGGING OBJECT: %d\n", edit.clicked.num);
    if (! edit.Selected)
    {
      SelectObject (&edit.Selected, edit.clicked.num);
      if (MoveObjectsToCoords (edit.clicked.type, edit.Selected,
            edit.map_x, edit.map_y, grid.snap ? grid.step : 0))
        edit.RedrawMap = 1;
      ForgetSelection (&edit.Selected);
    }
    else
    {
      if (MoveObjectsToCoords (edit.clicked.type, edit.Selected,
            edit.map_x, edit.map_y, grid.snap ? grid.step : 0))
        edit.RedrawMap = 1;
    }

    return;
  }
}


void EditorResize(int is_width, int is_height)
{
  edit.RedrawMap = 1;
}


void EditorAutoScroll()
{
  // Auto-scrolling: scroll the map automatically
  // when the mouse pointer rests near the edge
  // of the window.
  // Scrolling is disabled when a pull-down menu
  // is visible because it would be annoying to see
  // the map scrolling while you're searching
  // through the menus.

#if 0  // DISABLED FOR TIME BEING

  if (is.in_window
      && autoscroll
      && ! is.scroll_lock
      && edit.menubar->pulled_down () < 0)
  {
    unsigned distance;    // In pixels

#define actual_move(total,dist) \
    ((int) (((total * autoscroll_amp / 100)\
        * ((double) (autoscroll_edge - dist) / autoscroll_edge))\
      / Scale))

    distance = is.y;
    // The reason for the second member of the condition
    // is that we don't want to scroll when the user is
    // simply reaching for a menu...
    if (distance <= autoscroll_edge
        && edit.menubar->is_under_menubar_item (is.x) < 0)
    {
      if ((grid.orig_y) < /*MapMaxY*/ 20000)
      {
        grid.orig_y += actual_move (ScrMaxY, distance);
        edit.RedrawMap = 1;
      }
    }

    distance = ScrMaxY - is.y;
    if (distance <= autoscroll_edge)
    {
      if ((grid.orig_y) > /*MapMinY*/ -20000)
      {
        grid.orig_y -= actual_move (ScrMaxY, distance);
        edit.RedrawMap = 1;
      }
    }

    distance = is.x;
    if (distance <= autoscroll_edge)
    {
      if ((grid.orig_x) > /*MapMinX*/ -20000)
      {
        grid.orig_x -= actual_move (ScrMaxX, distance);
        edit.RedrawMap = 1;
      }
    }

    // The reason for the second member of the condition
    // is that we don't want to scroll when the user is
    // simply reaching for the "Help" menu...
    // Note: the ordinate "3 * FONTH" is of course not
    // critical. It's just a rough approximation.
    distance = ScrMaxX - is.x;
    if (distance <= autoscroll_edge && (unsigned) is.y >= 3 * FONTH)
    {
      if ((grid.orig_x) < /*MapMaxX*/ 20000)
      {
        grid.orig_x += actual_move (ScrMaxX, distance);
        edit.RedrawMap = 1;
      }
    }
  }
#endif
}


/*
  the editor main loop
*/

void EditorLoop (const char *_levelname)
{
    levelname = _levelname;

    memset (&edit, 0, sizeof(edit));  /* Catch-all */

    edit.move_speed          = 20;
    edit.extra_zoom          = 0;
    // If you change this, don't forget to change
    // the initialisation of the menu bar below.
    edit.obj_type            = OBJ_THINGS;

    edit.show_object_numbers = false;
    edit.show_things_squares = false;
    edit.show_things_sprites = true;
    edit.rulers_shown        = 0;
    edit.clicked.nil ();
    //edit.click_obj_no        = OBJ_NO_NONE;
    //edit.click_obj_type      = -1;
    edit.click_ctrl          = 0;
    edit.highlighted.nil ();
    //edit.highlight_obj_no    = OBJ_NO_NONE;
    //edit.highlight_obj_type  = -1;
    edit.Selected            = 0;

    edit.selbox              = new selbox_c;
    edit.highlight           = new highlight_c;

    MadeChanges = 0;
    MadeMapChanges = 0;


    zoom_fit ();

    Render3D_Setup(main_win->canvas->w(), main_win->canvas->h());


    main_win->canvas->set_edit_info(&edit);

    edit.RedrawMap = 1;

    while (main_win->action != UI_MainWin::QUIT)
    {
        Fl::wait(0.2);

        if (edit.RedrawMap)
        {
            main_win->canvas->redraw();

            edit.RedrawMap = 0;
        }
    }
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
