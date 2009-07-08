/*
 *  editloop.h
 *  AYM 1998-09-06
 */

#include "objid.h"

void EditorLoop (const char *);
const char *SelectLevel (int levelno);
extern int InputSectorType(int x0, int y0, int *number);
extern int InputLinedefType(int x0, int y0, int *number);
extern int InputThingType(int x0, int y0, int *number);


void EditorKey(int is_key, bool is_shift = false);
void EditorWheel(int delta);
void EditorMousePress(bool is_ctrl);
void EditorMouseRelease (void);
void EditorMouseMotion(int x, int y, int map_x, int map_y, bool drag);


void ed_draw_all();
void ed_highlight_object (Objid& obj);
void ed_forget_highlight ();
