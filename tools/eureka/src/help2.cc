/*
 *  help2.cc
 *  AYM 1998-08-17
 */


/*
This file is part of Yadex.

Yadex incorporates code from DEU 5.21 that was put in the public domain in
1994 by Raphaël Quinet and Brendon Wyber.

The rest of Yadex is Copyright © 1997-2003 André Majorel and others.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place, Suite 330, Boston, MA 02111-1307, USA.
*/


#include "yadex.h"
#include "credits.h"
#include "cfgfile.h"
#include "gfx.h"
#include "help2.h"


/*
   display the help screen
 */

static const char *help_text[] =
{
  "             Keyboard :",
  "Esc/q Quit                      Pgup  Scroll up",
  "                                Pgdn  Scroll down",
  "Tab   Next mode                 Home  Scroll left",
  "ShiftTab Previous mode          End   Scroll right",
  "l     Linedefs & sidedefs mode  '     Go to centre of map",
  "s     Sectors mode              `     Show whole map",
  "t     Things mode               n,>   Jump to next object",
  "v     Vertices mode             p,<   Jump to previous object",
  "&     Show/hide object numbers  j,#   Jump to object #N",
  "%%     Show/hide sprites",
  "                                +/-   Zoom in/out",
  "Ins   Insert a new object       g/G   Decr./incr. the grid step",
  "Del   Delete the object(s)      h     Hide/show the grid",
  "Retn  Edit object properties    H     Reset grid step to the max",
  "x/w   Spin things cw/ccw        z     Lock the grid step",
  "x     Split linedefs            y     Snap to grid on/off",
  "w     Split linedefs & sector   Space Toggle extra zoom",
  "a     Set things/ld flags",
  "b     Toggle things/ld flags    e     Select linedefs in path",
  "c     Clear things/ld flags     E     Select 1s linedefs in path",
  "F8    Misc. operations",
  "F9    Insert compound object    F5    Preferences",
  "                                F10   Checks",
  "             Mouse :",
  "- Clicking on an object with the left button selects it (and",
  "  unselects everything else unless [Ctrl] is pressed).",
  "- Clicking on an already selected object with the left button with",
  "  [Ctrl] pressed unselects it.",
  "- Double clicking on an object allows to change its properties.",
  "- You can also drag objects with the left button.",
  "- Clicking on an empty space with the left button and moving draws",
  "  a rectangular selection box. Releasing the button selects",
  "  everything in that box (and unselects everything else unless",
  "  [Ctrl] is pressed).",
  "- Wheel or buttons 4 and 5: zoom in and out",
  NULL
};

void DisplayHelp ()
{
  int x0;
  int y0;
  int width;
  int height;
  size_t maxlen = 0;
  int lines = 4;

  for (const char **str = help_text; *str; str++)
  {
    size_t len = strlen (*str);
    maxlen = MAX(maxlen, len);
    lines++;
  }
  width  = (maxlen + 4) * FONTW + 2 * BOX_BORDER;
  height = lines * FONTH + 2 * BOX_BORDER;
  x0 = (ScrMaxX + 1 - width) / 2;
  y0 = (ScrMaxY + 1 - height) / 2;
  /* put in the instructions */
  DrawScreenBox3D (x0, y0, x0 + width - 1, y0 + height - 1);
  set_colour (LIGHTCYAN);
  DrawScreenText (x0 + BOX_BORDER + (width - 5 * FONTW) / 2,
      y0 + BOX_BORDER + FONTH / 2,
      "Yadex");
  set_colour (WINFG);
  DrawScreenText (x0 + BOX_BORDER + 2 * FONTW, y0 + BOX_BORDER + FONTH, "");
  for (const char **str = help_text; *str; str++)
     DrawScreenText (-1, -1, *str);
  set_colour (WINTITLE);
  DrawScreenText (-1, -1, "Press any key to return to the editor...");
}


/*
 *  about_yadex()
 *  The name says it all.
 */
void about_yadex ()
{
  int widthc  = 57;
  int heightc = 19;

  for (const char *const *s = yadex_copyright; *s != 0; s++)
  {
    if (strlen (*s) > size_t (widthc))
      widthc = strlen (*s);
    heightc++;
  }
  int width   = 2 * BOX_BORDER + 2 * WIDE_HSPACING + widthc * FONTW;
  int height  = 2 * BOX_BORDER + 2 * WIDE_VSPACING + heightc * FONTH;
  int x0 = (ScrMaxX + 1 - width) / 2;
  int y0 = (ScrMaxY + 1 - height) / 2;

  DrawScreenBox3D (x0, y0, x0 + width - 1, y0 + height - 1);
  push_colour (WINFG);
  push_colour (WINFG_HL);
  DrawScreenText (x0 + BOX_BORDER + WIDE_HSPACING,
      y0 + BOX_BORDER + WIDE_VSPACING, what ());
  pop_colour ();
  DrawScreenText (-1, -1, "");
  for (const char *const *s = yadex_copyright; *s != 0; s++)
    DrawScreenText (-1, -1, *s);
  DrawScreenText (-1, -1, "");
  push_colour (WINFG_HL);
  DrawScreenText (-1, -1, "Home page :");
  pop_colour ();
  DrawScreenText (-1, -1, "http://www.teaser.fr/~amajorel/yadex/");
  DrawScreenText (-1, -1, "http://www.linuxgames.com/yadex/");
  DrawScreenText (-1, -1, "");
  push_colour (WINFG_HL);
  DrawScreenText (-1, -1, "Mailing lists :");
  pop_colour ();
  DrawScreenText (-1, -1, "you-know-what@freelists.org");
  DrawScreenText (-1, -1, "you-know-what-announce@freelists.org");
  DrawScreenText (-1, -1, "To subscribe, send mail with the subject");
  DrawScreenText (-1, -1, "\"subscribe <list_name>\" to ecartis@freelists.org.");
  DrawScreenText (-1, -1, "");
  push_colour (WINFG_HL);
  DrawScreenText (-1, -1, "Maintainer :");
  pop_colour ();
  DrawScreenText (-1, -1, "André Majorel (http://www.teaser.fr/~amajorel/)");
  DrawScreenText (-1, -1, "Send all email to you-know-what@freelists.org, NOT to me.");
  DrawScreenText (-1, -1, "");
  DrawScreenText (-1, -1, "");
  set_colour (WINTITLE);
  DrawScreenText (-1, -1, "Press any key to return to the editor...");
  pop_colour ();
}


/*
 *  what
 *  Return a static string containing
 *  the name and version number of Yadex.
 */
const char *what ()
{
static char buf[40];
y_snprintf (buf, sizeof buf, "Yadex %s (%s)", yadex_version, yadex_source_date);
return buf;
}


/*
 *  print_usage
 *  Print the program usage.
 */
void print_usage (FILE *fd)
{
fprintf (fd, "%s\n", what ());
fprintf (fd, "Usage: yadex [options] [<pwad_file> ...]\n");
fprintf (fd, "Options:\n");
dump_command_line_options (fd);
fprintf (fd, " %-33sSame as -?\n", "--help");
fprintf (fd, " %-33sPrint version and exit\n", "--version");
fprintf (fd, "Put a \"+\" instead of a \"-\" before boolean options"
             " to reverse their effect.\n");
}


/*
 *  print_welcome
 *  Print the welcome message
 */
void print_welcome (FILE *fd)
{
#ifdef OLD_MESSAGE
fprintf (fd, "\n");
fprintf (fd, "*----------------------------------------------------------------------------*\n");
fprintf (fd, "| Welcome to DEU!  This is a poweful utility and, like all good tools, it    |\n");
fprintf (fd, "| comes with its user's manual.  Please print and read DEU.TXT if you want   |\n");
fprintf (fd, "| to discover all the features of this program.  If you are new to DEU, the  |\n");
fprintf (fd, "| tutorial will show you how to build your first level.                      |\n");
fprintf (fd, "|                                                                            |\n");
fprintf (fd, "| If you are an experienced DEU user and want to know what has changed since |\n");
fprintf (fd, "| the last version, you should read the revision history in README.1ST.      |\n");
fprintf (fd, "|                                                                            |\n");
fprintf (fd, "| And if you have lots of suggestions for improvements, bug reports, or even |\n");
fprintf (fd, "| complaints about this program, be sure to read README.1ST first.           |\n");
fprintf (fd, "| Hint: you can easily disable this message.  Read the docs carefully...     |\n");
fprintf (fd, "*----------------------------------------------------------------------------*\n");
#else

fprintf (fd, "\n"
                "** Welcome to Yadex. Glad you've made it so far. :-)\n");
#if defined Y_ALPHA
fprintf (fd, "**\n"
    "** This is an alpha version. Expect it to have bugs. Do\n"
    "** yourself a favour and make backup copies of your data !\n"
    "**\n");
#elif defined Y_BETA
fprintf (fd, "**\n"
    "** This is a beta version. It is believed to be reasonably\n"
    "** stable but it's been given only limited testing. So do\n"
    "** yourself a favour and make backup copies of your data.\n"
    "**\n");
#else
fprintf (fd, "**\n"
    "** This version is believed to be stable but you never\n"
    "** know so make backup copies of your data anyway.\n"
    "**\n");
#endif
fprintf (fd, "** Yadex is work in progress. Subscribe to yadex-announce\n");
fprintf (fd, "** or keep an eye on the web page.\n");
fprintf (fd, "** To edit an existing level, type \"e <level_name>\".\n");
fprintf (fd, "** To create a new level, type \"c\".\n"
                "\n");
#endif
}
