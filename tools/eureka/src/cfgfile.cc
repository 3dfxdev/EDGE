/*
 *  cfgfile.cc
 *  AYM 1998-09-30
 */

/*
This file is part of Yadex.

Yadex incorporates code from DEU 5.21 that was put in the public domain in
1994 by Raphaël Quinet and Brendon Wyber.

The rest of Yadex is Copyright © 1997-2003 André Majorel.

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

#include <sys/types.h>
#include <sys/stat.h>

#include "cfgfile.h"
#include "gfx.h"
#include "help2.h"
#include "levels.h"
#include "locate.h"



/*
 *  Description of the command line arguments and config file keywords
 */
typedef enum
{
  // Boolean (toggle)
  // Receptacle is of type bool
  // data_ptr is of type (bool *)
  OPT_BOOLEAN,

  // "yes", "no", "ask"
  // Receptacle is of type confirm_t
  // data_ptr is of type confirm_t
  OPT_CONFIRM,

  // Integer number,
  // Receptacle is of type int
  // data_ptr is of type (int *)
  OPT_INTEGER,

  // Unsigned long integer
  // Receptacle is of type unsigned
  // data_ptr is of type (unsigned long *)
  OPT_UNSIGNED,

  // String
  // Receptacle is of type (char[9])
  // data_ptr is of type (char *)
  OPT_STRINGBUF8,

  // String
  // Receptacle is of type (const char *)
  // data_ptr is of type (const char **)
  OPT_STRINGPTR,

  // String, but store in a list
  // Receptacle is of type ??
  // data_ptr is of type ??
  OPT_STRINGPTRACC,

  // List of strings
  // Receptacle is of type (const char *[])
  // data_ptr is of type (const char ***)
  OPT_STRINGPTRLIST,

  // End of the options description
  OPT_END
} opt_type_t;

typedef struct
{
  const char *long_name;  // Command line arg. or keyword
  const char *short_name; // Abbreviated command line argument
  opt_type_t opt_type;    // Type of this option
  const char *flags;    // Flags for this option :
        // "1" = process only on pass 1 of
        //       parse_command_line_options()
  const char *desc;   // Description of the option
  void *data_ptr;   // Pointer to the data
} opt_desc_t;

/* The first option has neither long name nor short name.
   It is used for "lonely" arguments (i.e. file names). */
opt_desc_t options[] =    // Description of the command line options
{
  { 0,
    0,
    OPT_STRINGPTRACC,
    0,
    "Patch wad file",
    &PatchWads },

  { "autoscroll",
    0,
    OPT_BOOLEAN,
    0,
    "Enable autoscrolling",
    &autoscroll },

  { "autoscroll_amp",
    0,
    OPT_UNSIGNED,
    0,
    "Amp. of scrolling (% of screen size)",
    &autoscroll_amp },

  { "autoscroll_edge",
    0,
    OPT_UNSIGNED,
    0,
    "Max. dist. to edge (pixels)",
    &autoscroll_edge },

  { 0,
    "b",
    OPT_STRINGPTR,
    0,
    "Run benchmark and exit successfully",
    &bench },

  { "blindly_swap_sidedefs",
    0,
    OPT_BOOLEAN,
    0,
    "Blindly swap sidedefs on a linedef",
    &blindly_swap_sidedefs },

  { "config_file",
    "f",
    OPT_STRINGPTR,
    "1",
    "Config file",
    &config_file },

  { "copy_linedef_reuse_sidedefs",
    0,
    OPT_BOOLEAN,
    0,
    "Use same sidedefs as original linedef",
    &copy_linedef_reuse_sidedefs },

  { "debug",
    "d",
    OPT_BOOLEAN,
    0,
    "Debug mode",
    &Debug },

  { "default_ceiling_height",
    0,
    OPT_INTEGER,
    0,
    "Default ceiling height",
    &default_ceiling_height },

  { "default_ceiling_texture",
    0,
    OPT_STRINGBUF8,
    0,
    "Default ceiling texture",
    default_ceiling_texture },

  { "default_floor_height",
    0,
    OPT_INTEGER,
    0,
    "Default floor height",
    &default_floor_height },

  { "default_floor_texture",
    0,
    OPT_STRINGBUF8,
    0,
    "Default floor texture",
    default_floor_texture },

  { "default_light_level",
    0,
    OPT_INTEGER,
    0,
    "Default light level",
    &default_light_level },

  { "default_lower_texture",
    0,
    OPT_STRINGBUF8,
    0,
    "Default lower texture",
    default_lower_texture },

  { "default_middle_texture",
    0,
    OPT_STRINGBUF8,
    0,
    "Default middle texture",
    default_middle_texture },

  { "default_thing",
    0,
    OPT_INTEGER,
    0,
    "Default thing number",
    &default_thing },

  { "default_upper_texture",
    0,
    OPT_STRINGBUF8,
    0,
    "Default upper texture",
    default_upper_texture },

  { "digit_zoom_base",
    0,
    OPT_INTEGER,
    0,
    "[0]-[9]: base zoom factor (in %)",
    &digit_zoom_base },

  { "digit_zoom_step",
    0,
    OPT_INTEGER,
    0,
    "[0]-[9]: step between factors (in %)",
    &digit_zoom_step },

  { "double_click_timeout",
    0,
    OPT_INTEGER,
    0,
    "Max delay in ms between clicks",
    &double_click_timeout },

  { "expert",
    0,
    OPT_BOOLEAN,
    0,
    "Expert mode",
    &Expert },

  { "file",
    0,
    OPT_STRINGPTRLIST,
    0,
    "Patch wad file",
    &PatchWads },

  { "font",
    "fn",
    OPT_STRINGPTR,
    0,
    "(X11 only) Font name",
    &font_name },

  { "game",
    "g",
    OPT_STRINGPTR,
    0,
    "Game",
    &Game },

  { "help",
    "?",
    OPT_BOOLEAN,
    "1",
    "Show usage summary",
    &show_help },

  { "idle_sleep_ms",
    0,
    OPT_INTEGER,
    0,
    "ms to sleep before XPending()",
    &idle_sleep_ms },

  { "info_bar",
    0,
    OPT_BOOLEAN,
    0,
    "Show the info bar",
    &InfoShown },

  { "insert_vertex_split_linedef",
    0,
    OPT_CONFIRM,
    0,
    "Split ld after ins. vertex",
    &insert_vertex_split_linedef },

  { "insert_vertex_merge_vertices",
    0,
    OPT_CONFIRM,
    0,
    "Merge vertices after ins. vertex",
    &insert_vertex_merge_vertices },

  { "iwad1",
    "i1",
    OPT_STRINGPTR,
    0,
    "The name of the Doom/Ultimate D. iwad",
    &Iwad1 },

  { "iwad2",
    "i2",
    OPT_STRINGPTR,
    0,
    "The name of the Doom II/Final D. iwad",
    &Iwad2 },

  { "iwad3",
    "i3",
    OPT_STRINGPTR,
    0,
    "The name of the Heretic iwad",
    &Iwad3 },

  { "iwad4",
    "i4",
    OPT_STRINGPTR,
    0,
    "The name of the Hexen iwad",
    &Iwad4 },

  { "iwad5",
    "i5",
    OPT_STRINGPTR,
    0,
    "The name of the Strife iwad",
    &Iwad5 },

  { "iwad6",
    "i6",
    OPT_STRINGPTR,
    0,
    "The name of the Doom alpha 0.2 iwad",
    &Iwad6 },

  { "iwad7",
    "i7",
    OPT_STRINGPTR,
    0,
    "The name of the Doom alpha 0.4 iwad",
    &Iwad7 },

  { "iwad8",
    "i8",
    OPT_STRINGPTR,
    0,
    "The name of the Doom alpha 0.5 iwad",
    &Iwad8 },

  { "iwad9",
    "i9",
    OPT_STRINGPTR,
    0,
    "The name of the Doom press rel. iwad",
    &Iwad9 },

  { "iwad10",
    "i10",
    OPT_STRINGPTR,
    0,
    "The name of the Strife 1.0 iwad",
    &Iwad10 },

  { "pwad",
    "pw",
    OPT_STRINGPTRACC,
    0,
    "Pwad file to load",
    &PatchWads },

  { "quiet",
    "q",
    OPT_BOOLEAN,
    0,
    "Quiet mode",
    &Quiet },

  { "quieter",
    "qq",
    OPT_BOOLEAN,
    0,
    "Quieter mode",
    &Quieter },

  { "scroll_less",
    0,
    OPT_UNSIGNED,
    0,
    "Amp. of scrolling (% of screen size)",
    &scroll_less },

  { "scroll_more",
    0,
    OPT_UNSIGNED,
    0,
    "Amp. of scrolling (% of screen size)",
    &scroll_more },

  { "select0",
    "s0",
    OPT_BOOLEAN,
    0,
    "Automatic selection of 0th object",
    &Select0 },

  { "sprite_scale",
    0,
    OPT_INTEGER,
    0,
    "Relative scale of sprites",
    &sprite_scale },

  { "swap_buttons",
    "sb",
    OPT_BOOLEAN,
    0,
    "Swap mouse buttons",
    &SwapButtons },

  { "text_dot",
    "td",
    OPT_BOOLEAN,
    0,
    "DrawScreenText debug flag",
    &text_dot },

  { "verbose",
    "v",
    OPT_BOOLEAN,
    "1",
    "Verbose mode",
    &verbose },

  { "warp",
    "w",
    OPT_STRINGPTR,
    0,
    "Warp map",
    &Warp },

  { "welcome_message",
    0, 
    OPT_BOOLEAN,
    0,
    "Print welcome message",
    &welcome_message },

  { "zoom_default",
    "z",
    OPT_INTEGER,
    0,
    "Initial zoom factor",
    &zoom_default },

  { "zoom_step",
    0,
    OPT_INTEGER,
    0,
    "Step between zoom factors (in %)",
    &zoom_step },

  { 0,
    0,
    OPT_END,
    0,
    0,
    0 }
};


static void append_item_to_list (const char ***list, const char *item);
static int parse_config_file (const char *filename);
static confirm_t confirm_e2i (const char *external);
static const char *confirm_i2e (confirm_t internal);


/*
 *  parse_config_file_default - parse the default config file(s)
 *
 *  Return non-zero if parse error occurred (i.e. at least
 *  one call to parse_config_file() returned non-zero), zero
 *  otherwise.
 */
int parse_config_file_default ()
{
  int rc = 0;
  int matches;
  const char *pathname;
  const char *name = "eureka.cfg";
  Locate locate (yadex_etc_path, name, true);

  for (matches = 0; (pathname = locate.get_next ()) != NULL; matches++)
  {
    printf ("Reading config file \"%s\".\n", pathname);
    int r = parse_config_file (pathname);
    if (r != 0)
      rc = 1;
  }
  if (matches == 0)
    warn ("%s: not found\n", name);
  return rc;
}


/*
 *  parse_config_file_user - parse a user-specified config file
 *
 *  Return non-zero if the file couldn't be found or if
 *  parse error occurred (i.e. parse_config_file() returned
 *  non-zero), zero otherwise.
 */
int parse_config_file_user (const char *name)
{
  const char *pathname;
  Locate locate (yadex_etc_path, name, false);
  
  pathname = locate.get_next ();
  if (pathname == NULL)
  {
    err ("%s: not found", name);
    return 1;
  }
  printf ("Reading config file \"%s\".\n", pathname);
  return parse_config_file (pathname);
}


/*
 *  parse_config_file - try to parse a config file by pathname.
 *
 *  Return 0 on success, <>0 on failure.
 */
#define RETURN_FAILURE do { rc = 1; goto byebye; } while (0)

static int parse_config_file (const char *filename)
{
  int   rc = 0;
  FILE *cfgfile;
  char  line[1024];

  cfgfile = fopen (filename, "r");
  if (cfgfile == NULL)
  {
    err ("Can't open config file \"%s\" (%s)", filename, strerror (errno));
    RETURN_FAILURE;
  }

///---  /* The first line of the configuration file must
///---     contain exactly config_file_magic. */
///---  if (fgets (line, sizeof line, cfgfile) == NULL
///---      || memcmp (line, config_file_magic, sizeof config_file_magic - 1)
///---      || line[sizeof config_file_magic - 1] != '\n'
///---      || line[sizeof config_file_magic] != '\0')
///---  {
///---    if (flags & CFG_PARSE_MAGIC_ERROR)
///---    {
///---      err ("%s(1): bad magic, not a valid Yadex configuration file", filename);
///---      err ("Perhaps a leftover from a previous version of Yadex ?");
///---      RETURN_FAILURE;
///---    }
///---    if (flags & CFG_PARSE_MAGIC_WARN)
///---      warn ("%s(1): bad magic, perhaps from a different version of Yadex\n",
///---          filename);
///---    rewind (cfgfile);
///---  }

  // Execute one line on each iteration
  for (unsigned lnum = 1; fgets (line, sizeof line, cfgfile) != NULL; lnum++)
  {
    char *name  = 0;
    char *value = 0;
    char *p     = line;

    // Skip leading whitespace
    while (isspace (*p))
      p++;

    // Skip comments
    if (*p == '#')
      continue;

    // Remove trailing newline
    {
      size_t len = strlen (p);
      if (len >= 1 && p[len - 1] == '\n')
        p[len - 1] = '\0';
    }

    // Skip empty lines
    if (*p == '\0')
      continue;

    // Make <name> point on the <name> field
    name = p;
    while (y_isident (*p))
      p++;
    if (*p == '\0')
    {
      warn ("%s(%u): expected an \"=\", skipping\n", filename, lnum);
      goto next_line;
    }
    if (*p == '=')
    {
      // Mark the end of the option name
      *p = '\0';
    }
    else
    {
      // Mark the end of the option name
      *p = '\0';
      p++;
      // Skip blanks after the option name
      while (isspace ((unsigned char) *p))
        p++;
      if (*p != '=')
      {
        warn ("%s(%u,%d): expected an \"=\", skipping\n",
            filename, lnum, 1 + (int) (p - line));
        goto next_line;
      }
    }
    p++;

    /* First parameter : <value> points on the first character.
       Put a NUL at the end. If there is a second parameter,
       holler. */
    while (isspace ((unsigned char) *p))
      p++;
    value = p;
    {
      unsigned char *p2 = (unsigned char *) value;
      while (*p2 != '\0' && ! isspace (*p2))
        p2++;
      if (*p2 != '\0')  // There's trailing whitespace after 1st parameter
      {
        for (unsigned char *p3 = p2; *p3 != '\0'; p3++)
          if (! isspace (*p3))
          {
            err ("%s(%u,%d): extraneous argument",
                filename, lnum, 1 + (int) ((char *) p3 - line));
            RETURN_FAILURE;
          }
      }
      *p2 = '\0';
    }

    for (const opt_desc_t *o = options + 1; ; o++)
    {
      if (o->opt_type == OPT_END)
      {
          warn ("%s(%u): invalid variable \"%s\", skipping\n",
              filename, lnum, name);
          goto next_line;
      }
      if (! o->long_name || strcmp (name, o->long_name) != 0)
        continue;

      if (o->flags != NULL && strchr (o->flags, '1'))
        break;
      switch (o->opt_type)
      {
        case OPT_BOOLEAN:
          if (! strcmp (value, "yes") || ! strcmp (value, "true")
              || ! strcmp (value, "on") || ! strcmp (value, "1"))
          {
            if (o->data_ptr)
              *((bool *) (o->data_ptr)) = true;
          }
          else if (! strcmp (value, "no") || ! strcmp (value, "false")
              || ! strcmp (value, "off") || ! strcmp (value, "0"))
          {
            if (o->data_ptr)
              *((bool *) (o->data_ptr)) = false;
          }
          else
          {
            err ("%s(%u): invalid value for option %s: \"%s\"",
                filename, lnum, name, value);
            RETURN_FAILURE;
          }
          break;

        case OPT_CONFIRM:
          if (o->data_ptr)
            *((confirm_t *) o->data_ptr) = confirm_e2i (value);
          break;

        case OPT_INTEGER:
          if (o->data_ptr)
            *((int *) (o->data_ptr)) = atoi (value);
          break;

        case OPT_UNSIGNED:
          if (o->data_ptr)
          {
            if (*value == '\0')
            {
              err ("%s(%u,%d): missing argument",
                  filename, lnum, 1 + (int) (value - line));
              RETURN_FAILURE;
            }
            bool neg = false;
            if (value[0] == '-')
              neg = true;
            char *endptr;
            errno = 0;
            *((unsigned long *) (o->data_ptr)) = strtoul (value, &endptr, 0);
            if (*endptr != '\0' && ! isspace (*endptr))
            {
              err ("%s(%u,%d): illegal character in unsigned integer",
                  filename, lnum, 1 + (int) (endptr - line));
              RETURN_FAILURE;
            }
            /* strtoul() sets errno to ERANGE if overflow. In
               addition, we don't want any non-zero negative
               numbers. In terms of regexp, /^(0x)?0*$/i. */
            if
              (
               errno != 0
               || neg
               && !
               (
                strspn (value + 1, "0") == strlen (value + 1)
                || value[1] == '0'
                && tolower (value[2]) == 'x'
                && strspn (value + 3, "0") == strlen (value + 3)
               )
              )
              {
                err ("%s(%u,%d): unsigned integer out of range",
                    filename, lnum, 1 + (int) (value - line));
                RETURN_FAILURE;
              }
          }
          break;

        case OPT_STRINGBUF8:
          if (o->data_ptr)
            strncpy ((char *) o->data_ptr, value, 8);
            ((char *) o->data_ptr)[8] = 0;
          break;

        case OPT_STRINGPTR:
          {
            char *dup = (char *) GetMemory (strlen (value) + 1);
            strcpy (dup, value);
            if (o->data_ptr)
              *((char **) (o->data_ptr)) = dup;
            break;
          }

        case OPT_STRINGPTRACC:
          {
            char *dup = (char *) GetMemory (strlen (value) + 1);
            strcpy (dup, value);
            if (o->data_ptr)
              append_item_to_list ((const char ***) o->data_ptr, dup);
            break;
          }

        case OPT_STRINGPTRLIST:
          while (*value != '\0')
          {
            char *v = value;
            while (*v != '\0' && ! isspace ((unsigned char) *v))
              v++;
            char *dup = (char *) GetMemory (v - value + 1);
            memcpy (dup, value, v - value);
            dup[v - value] = '\0';
            if (o->data_ptr)
              append_item_to_list ((const char ***) o->data_ptr, dup);
            while (isspace (*v))
              v++;
            value = v;
          }
          break;

        default:
          {
            nf_bug ("%s(%u): unknown option type %d",
                filename, lnum, (int) o->opt_type);
            RETURN_FAILURE;
          }
      }
      break;
    }
next_line:;
  }

byebye:
  if (cfgfile != 0)
    fclose (cfgfile);
  return rc;
}


/*
 *  parse_command_line_options
 *  If <pass> is set to 1, ignores all options except those
 *  that have the "1" flag.
 *  Else, ignores all options that have the "1" flag.
 *  If an error occurs, report it with err()
 *  and returns non-zero. Else, returns 0.
 */
int parse_command_line_options (int argc, const char *const *argv, int pass)
{
  const opt_desc_t *o;

  while (argc > 0)
  {
    int ignore;

    // Which option is this ?
    if (**argv != '-' && **argv != '+')
    {
      o = options;
      argc++;
      argv--;
    }
    else
      for (o = options + 1; ; o++)
      {
  if (o->opt_type == OPT_END)
  {
    err ("invalid option: \"%s\"", argv[0]);
    return 1;
  }
  if (o->short_name && ! strcmp (argv[0]+1, o->short_name)
   || o->long_name  && ! strcmp (argv[0]+1, o->long_name))
    break;
      }

    // If this option has the "1" flag but pass is not 1
    // or it doesn't but pass is 1, ignore it.
    ignore = (o->flags != NULL && strchr (o->flags, '1')) != (pass == 1);

    switch (o->opt_type)
    {
      case OPT_BOOLEAN:
  if (argv[0][0] == '-')
  {
    if (o->data_ptr && ! ignore)
       *((bool *) (o->data_ptr)) = true;
  }
  else
  {
    if (o->data_ptr && ! ignore)
       *((bool *) (o->data_ptr)) = false;
  }
  break;

      case OPT_CONFIRM:
  if (argc <= 1)
  {
    err ("missing argument after \"%s\"", argv[0]);
    return 1;
  }
  argv++;
  argc--;
  if (o->data_ptr && ! ignore)
     *((confirm_t *) o->data_ptr) = confirm_e2i (argv[0]);
  break;

      case OPT_INTEGER:
  if (argc <= 1)
  {
    err ("missing argument after \"%s\"", argv[0]);
    return 1;
  }
  argv++;
  argc--;
  if (o->data_ptr && ! ignore)
    *((int *) (o->data_ptr)) = atoi (argv[0]);
  break;

      case OPT_UNSIGNED:
        if (argc <= 1)
  {
    err ("missing argument after \"%s\"", argv[0]);
    return 1;
  }
  argv++;
  argc--;
  if (o->data_ptr && ! ignore)
  {
    const char *value = argv[0];
    if (*value == '\0')
    {
      err ("not an unsigned integer \"%s\"", value);
      return 1;
    }
    bool neg = false;
    if (*value == '-')
      neg = true;
    char *endptr;
    errno = 0;
    *((unsigned long *) (o->data_ptr)) = strtoul (value, &endptr, 0);
    while (*endptr != '\0' && isspace (*endptr))
      endptr++;
    if (*endptr != '\0')
    {
      err ("illegal characters in unsigned int \"%s\"", endptr);
      return 1;
    }
    /* strtoul() sets errno to ERANGE if overflow. In
       addition, we don't want any non-zero negative
       numbers. In terms of regexp, /^(0x)?0*$/i. */
    if
    (
      errno != 0
      || neg
        && !
        (
    strspn (value + 1, "0") == strlen (value + 1)
    || value[1] == '0'
      && tolower (value[2]) == 'x'
      && strspn (value + 3, "0") == strlen (value + 3)
        )
    )
    {
      err ("unsigned integer out of range \"%s\"", value);
      return 1;
    }
  }
  break;

      case OPT_STRINGBUF8:
  if (argc <= 1)
  {
    err ("missing argument after \"%s\"", argv[0]);
    return 1;
  }
  argv++;
  argc--;
  if (o->data_ptr && ! ignore)
    strncpy ((char *) o->data_ptr, argv[0], 8);
    ((char *) o->data_ptr)[8] = 0;
  break;

      case OPT_STRINGPTR:
  if (argc <= 1)
  {
    err ("missing argument after \"%s\"", argv[0]);
    return 1;
  }
  argv++;
  argc--;
  if (o->data_ptr && ! ignore)
    *((const char **) (o->data_ptr)) = argv[0];
  break;

      case OPT_STRINGPTRACC:
  if (argc <= 1)
  {
    err ("missing argument after \"%s\"", argv[0]);
    return 1;
  }
  argv++;
  argc--;
  if (o->data_ptr && ! ignore)
    append_item_to_list ((const char ***) o->data_ptr, argv[0]);
  break;

      case OPT_STRINGPTRLIST:
  if (argc <= 1)
  {
    err ("missing argument after \"%s\"", argv[0]);
    return 1;
  }
  while (argc > 1 && argv[1][0] != '-' && argv[1][0] != '+')
  {
    argv++;
    argc--;
    if (o->data_ptr && ! ignore)
      append_item_to_list ((const char ***) o->data_ptr, argv[0]);
  }
  break;

      default:
  {
    nf_bug ("unknown option type (%d)", (int) o->opt_type);
    return 1;
  }
    }
    argv++;
    argc--;
  }
  return 0;
}


/*
 *  dump_parameters
 *  Print a list of the parameters with their current value.
 */
void dump_parameters (FILE *fp)
{
  const opt_desc_t *o;
  int desc_maxlen = 0;
  int name_maxlen = 0;

  for (o = options + 1; o->opt_type != OPT_END; o++)
  {
    int len = strlen (o->desc);
    desc_maxlen = al_amax (desc_maxlen, len);
    if (o->long_name)
    {
      len = strlen (o->long_name);
      name_maxlen = al_amax (name_maxlen, len);
    }
  }

  for (o = options + 1; o->opt_type != OPT_END; o++)
  {
    if (! o->long_name)
      continue;
    fprintf (fp, "%-*s  %-*s  ",name_maxlen, o->long_name, desc_maxlen, o->desc);
    if (o->opt_type == OPT_BOOLEAN)
      fprintf (fp, "%s", *((bool *) o->data_ptr) ? "enabled" : "disabled");
    else if (o->opt_type == OPT_CONFIRM)
      fputs (confirm_i2e (*((confirm_t *) o->data_ptr)), fp);
    else if (o->opt_type == OPT_STRINGBUF8)
      fprintf (fp, "\"%s\"", (char *) o->data_ptr);
    else if (o->opt_type == OPT_STRINGPTR)
    {
      if (o->data_ptr)
  fprintf (fp, "\"%s\"", *((char **) o->data_ptr));
      else
  fprintf (fp, "--none--");
    }
    else if (o->opt_type == OPT_INTEGER)
      fprintf (fp, "%d", *((int *) o->data_ptr));
    else if (o->opt_type == OPT_UNSIGNED)
      fprintf (fp, "%lu", *((unsigned long *) o->data_ptr));
    else if (o->opt_type == OPT_STRINGPTRACC
    || o->opt_type == OPT_STRINGPTRLIST)
    {
      if (o->data_ptr)
      {
  char **list;
  for (list = *((char ***) o->data_ptr); list && *list; list++)
    fprintf (fp, "\"%s\" ", *list);
  if (list == *((char ***) o->data_ptr))
    fprintf (fp, "--none--");
      }
      else
  fprintf (fp, "--none--");
    }
    fputc ('\n', fp);
  }
}


/*
 *  dump_command_line_options
 *  Print a list of all command line options (usage message).
 */
void dump_command_line_options (FILE *fd)
{
  const opt_desc_t *o;
  int desc_maxlen = 0;
  int name_maxlen = 0;

  for (o = options + 1; o->opt_type != OPT_END; o++)
  {
    int len;
    if (! o->short_name)
      continue;
    len = strlen (o->desc);
    desc_maxlen = al_amax (desc_maxlen, len);
    if (o->long_name)
    {
      len = strlen (o->long_name);
      name_maxlen = al_amax (name_maxlen, len);
    }
  }

  for (o = options; o->opt_type != OPT_END; o++)
  {
    if (! o->short_name)
      continue;
    if (o->short_name)
      fprintf (fd, " -%-3s ", o->short_name);
    else
      fprintf (fd, "      ");
    if (o->long_name)
      fprintf (fd, "-%-*s ", name_maxlen, o->long_name);
    else
      fprintf (fd, "%*s", name_maxlen + 2, "");
    switch (o->opt_type)
    {
      case OPT_BOOLEAN:       fprintf (fd, "            "); break;
      case OPT_CONFIRM:       fprintf (fd, "yes|no|ask  "); break;
      case OPT_STRINGBUF8:
      case OPT_STRINGPTR:
      case OPT_STRINGPTRACC:  fprintf (fd, "<string>    "); break;
      case OPT_INTEGER:       fprintf (fd, "<integer>   "); break;
      case OPT_UNSIGNED:      fprintf (fd, "<unsigned>  "); break;
      case OPT_STRINGPTRLIST: fprintf (fd, "<string> ..."); break;
      case OPT_END: ;  // This line is here only to silence a GCC warning.
    }
    fprintf (fd, " %s\n", o->desc);
  }
}


/*
 *  confirm_e2i
 *  Convert the external representation of a confirmation
 *  flag ("yes", "no", "ask", "ask_once") to the internal
 *  representation (YC_YES, YC_NO, YC_ASK, YC_ASK_ONCE or
 *  '\0' if none).
 */
static confirm_t confirm_e2i (const char *external)
{
  if (external != NULL)
  {
    if (! strcmp (external, "yes"))
      return YC_YES;
    if (! strcmp (external, "no"))
      return YC_NO;
    if (! strcmp (external, "ask"))
      return YC_ASK;
    if (! strcmp (external, "ask_once"))
      return YC_ASK_ONCE;
  }
  return YC_ASK;
}


/*
 *  confirm_i2e
 *  Convert the internal representation of a confirmation
 *  flag (YC_YES, YC_NO, YC_ASK, YC_ASK_ONCE) to the external
 *  representation ("yes", "no", "ask", "ask_once" or "?").
 */
static const char *confirm_i2e (confirm_t internal)
{
  if (internal == YC_YES)
    return "yes";
  if (internal == YC_NO)
    return "no";
  if (internal == YC_ASK)
    return "ask";
  if (internal == YC_ASK_ONCE)
    return "ask_once";
  return "?";
}


/*
 *  append_item_to_list
 *  Append a string to a null-terminated string list
 */
static void append_item_to_list (const char ***list, const char *item)
{
  int i;

  i = 0;
  if (*list != 0)
  {
    // Count the number of elements in the list (last = null)
    while ((*list)[i] != 0)
      i++;
    // Expand the list
    *list = (const char **) ResizeMemory (*list, (i + 2) * sizeof **list);
  }
  else
  {
    // Create a new list
    *list = (const char **) GetMemory (2 * sizeof **list);
  }
  // Append the new element
  (*list)[i] = item;
  (*list)[i + 1] = 0;
}


#include <string>
#include <vector>

const size_t MAX_TOKENS = 10;

/*
 *  word_splitting - perform word splitting on a string
 */
int word_splitting (std::vector<std::string>& tokens, const char *string)
{
  size_t      ntokens     = 0;
  const char *iptr        = string;
  const char *token_start = 0;
  bool        in_token    = false;
  bool        quoted      = false;

  /* break the line into whitespace-separated tokens.
     whitespace can be enclosed in double quotes. */
  for (; ; iptr++)
  {
    if (*iptr == '\n' || *iptr == '\0')
      break;

    else if (*iptr == '"')
      quoted = ! quoted;

    // "#" at the beginning of a token
    else if (! in_token && ! quoted && *iptr == '#')
      break;

    // First character of token
    else if (! in_token && (quoted || ! isspace (*iptr)))
    {
      ntokens++;
      if (ntokens > MAX_TOKENS)
  return 2;  // Too many tokens
      in_token = true;
    }

    // First space between two tokens
    else if (in_token && ! quoted && isspace (*iptr))
    {
      tokens.push_back (std::string (token_start, iptr - token_start));
      in_token = false;
    }
  }
  
  if (in_token)
    tokens.push_back (std::string (token_start, iptr - token_start));

  if (quoted)
    return 1;  // Unmatched double quote
  return 0;
}


