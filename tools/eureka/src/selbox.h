/*
 *  selbox.h
 *  Selection box stuff.
 *  Header for selbox.c
 *  AYM 1998-07-04
 */


#include "edwidget.h"


class selbox_c : public edwidget_c
{
  public :

    /* Methods used by edit_t */
    selbox_c            ();
    void set_1st_corner (int x, int y);
    void set_2nd_corner (int x, int y);
    void get_corners    (int *x1, int *y1, int *x2, int *y2);
    void unset_corners  ();
    /* Methods declared in edwidget_c */
    void unset         ();
    void draw          ();
    void undraw        ();

    int can_undraw ()
      { return 1; }  // I have the ability to undraw myself

    int need_to_clear ()
      { return 0; }  // I know how to undraw myself.

    void clear         ();

  private :

    int x1;   /* Coordinates of the first corner */
    int y1;
    int x2;   /* Coordinates of the second corner */
    int y2;
    int x1_disp;  /* x1 and y1 as they were last time draw() was called */
    int y1_disp;
    int x2_disp;  /* x2 and y2 as they were last time draw() was called */
    int y2_disp;
    int flags;
};


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
