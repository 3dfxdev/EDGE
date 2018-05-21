//------------------------------------------------------------------------
// COOKIE : Unix/FLTK Persistence (INI/RC files)
//------------------------------------------------------------------------
//
//  GL-Friendly Node Builder (C) 2000-2007 Andrew Apted
//
//  Based on 'BSP 2.3' by Colin Reed, Lee Killough and others.
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
#include "local.h"


#define DEBUG_COOKIE  0


static const char *cookie_filename;

// current section parser func
static boolean_g (* cookie_section_parser)
    (const char *name, const char *value) = NULL;


static void CookieDebug(const char *str, ...)
{
#if DEBUG_COOKIE
  char buffer[2048];
  va_list args;

  va_start(args, str);
  vsprintf(buffer, str, args);
  va_end(args);

  fprintf(stderr, "COOKIE: %s", buffer);
#else
  (void) str;
#endif
}

static boolean_g SetBooleanVar(boolean_g& var, const char *value)
{
  if (strchr(value, '0'))
  {
    var = FALSE;
    return TRUE;
  }

  if (strchr(value, '1'))
  {
    var = TRUE;
    return TRUE;
  }
  
  // bad boolean value
  CookieDebug("Bad Boolean [%s]\n", value);
  return FALSE;
}

static boolean_g SetIntegerVar(int& var, const char *value)
{
  var = strtol(value, NULL, 10);

  return TRUE;
}

static boolean_g SetStringVar(const char *& var, const char *value)
{
  int len = strlen(value);
  char buffer[1024];
  
  if (value[0] != '"' || len < 2 || value[len-1] != '"' || len > 1020)
  {
    CookieDebug("Bad String [%s]\n", value);
    return FALSE;
  }

  strncpy(buffer, value+1, len-2);
  buffer[len-2] = 0;

  GlbspFree(var);
  var = GlbspStrDup(buffer);

  return TRUE;
}


//------------------------------------------------------------------------


static boolean_g CookieSetBuildVar(const char *name, const char *value)
{
  // file names and factor
  if (strcasecmp(name, "in_file") == 0)
    return SetStringVar(guix_info.input_file, value);

  if (strcasecmp(name, "out_file") == 0)
    return SetStringVar(guix_info.output_file, value);

  if (strcasecmp(name, "factor") == 0)
    return SetIntegerVar(guix_info.factor, value);


  // boolean values
  if (strcasecmp(name, "no_reject") == 0)
    return SetBooleanVar(guix_info.no_reject, value);

  if (strcasecmp(name, "mini_warnings") == 0)
    return SetBooleanVar(guix_info.mini_warnings, value);

  if (strcasecmp(name, "pack_sides") == 0)
    return SetBooleanVar(guix_info.pack_sides, value);

  // API change to main glbsp code: 'choose_fresh' --> 'fast'.
  if (strcasecmp(name, "choose_fresh") == 0)
  {
    if (! SetBooleanVar(guix_info.fast, value))
      return FALSE;

    guix_info.fast = ! guix_info.fast;
    return TRUE;
  }


  // block limit
  if (strcasecmp(name, "block_limit") == 0)
    return SetIntegerVar(guix_info.block_limit, value);


  // build mode
  if (strcasecmp(name, "gwa_mode") == 0)
    return SetBooleanVar(guix_info.gwa_mode, value);

  if (strcasecmp(name, "no_normal") == 0)
    return SetBooleanVar(guix_info.no_normal, value);

  if (strcasecmp(name, "force_normal") == 0)
    return SetBooleanVar(guix_info.force_normal, value);


  // Unknown build variable !
  CookieDebug("Unknown Build VAR [%s]\n", name);
  return FALSE;
}

static boolean_g CookieSetPrefVar(const char *name, const char *value)
{
  // overwrite warning
  if (strcasecmp(name, "overwrite_warn") == 0)
    return SetBooleanVar(guix_prefs.overwrite_warn, value);

  // same file warning
  if (strcasecmp(name, "same_file_warn") == 0)
    return SetBooleanVar(guix_prefs.same_file_warn, value);

  // missing extension warning
  if (strcasecmp(name, "lack_ext_warn") == 0)
    return SetBooleanVar(guix_prefs.lack_ext_warn, value);

  // save log filename
  if (strcasecmp(name, "save_log_file") == 0)
    return SetStringVar(guix_prefs.save_log_file, value);


  // Unknown preference variable !
  CookieDebug("Unknown Pref VAR [%s]\n", name);
  return FALSE;
}

static boolean_g CookieSetWinPosVar(const char *name, const char *value)
{
  // main window position and size
  if (strcasecmp(name, "window_x") == 0)
    return SetIntegerVar(guix_prefs.win_x, value);

  if (strcasecmp(name, "window_y") == 0)
    return SetIntegerVar(guix_prefs.win_y, value);

  if (strcasecmp(name, "window_w") == 0)
    return SetIntegerVar(guix_prefs.win_w, value);

  if (strcasecmp(name, "window_h") == 0)
    return SetIntegerVar(guix_prefs.win_h, value);


  // other window positions
  if (strcasecmp(name, "progress_x") == 0)
    return SetIntegerVar(guix_prefs.progress_x, value);

  if (strcasecmp(name, "progress_y") == 0)
    return SetIntegerVar(guix_prefs.progress_y, value);

  if (strcasecmp(name, "dialog_x") == 0)
    return SetIntegerVar(guix_prefs.dialog_x, value);

  if (strcasecmp(name, "dialog_y") == 0)
    return SetIntegerVar(guix_prefs.dialog_y, value);

  if (strcasecmp(name, "other_x") == 0)
    return SetIntegerVar(guix_prefs.other_x, value);

  if (strcasecmp(name, "other_y") == 0)
    return SetIntegerVar(guix_prefs.other_y, value);


  // manual window positions
  if (strcasecmp(name, "manual_x") == 0)
    return SetIntegerVar(guix_prefs.manual_x, value);

  if (strcasecmp(name, "manual_y") == 0)
    return SetIntegerVar(guix_prefs.manual_y, value);

  if (strcasecmp(name, "manual_w") == 0)
    return SetIntegerVar(guix_prefs.manual_w, value);

  if (strcasecmp(name, "manual_h") == 0)
    return SetIntegerVar(guix_prefs.manual_h, value);

  if (strcasecmp(name, "manual_page") == 0)
    return SetIntegerVar(guix_prefs.manual_page, value);


  // Unknown window pos variable !
  CookieDebug("Unknown WindowPos VAR [%s]\n", name);
  return FALSE;
}


//------------------------------------------------------------------------


// returns TRUE if line parsed OK.  Note: modifies the buffer.
static boolean_g CookieParseLine(char *buf)
{
  char *name;
  int len;

  while (isspace(*buf))
    buf++;

  // blank line ?
  if (*buf == 0)
    return TRUE;

  // remove trailing whitespace
  len = strlen(buf);

  while (len > 0 && isspace(buf[len-1]))
    buf[--len] = 0;
 
  CookieDebug("PRE-PARSE: '%s'\n", buf);

  // comment ?
  if (*buf == '#')
    return TRUE;

  // section ?
  if (*buf == '[')
  {
    if (strncasecmp(buf+1, "BUILD_INFO]", 11) == 0)
    {
      cookie_section_parser = CookieSetBuildVar;
      return TRUE;
    }

    if (strncasecmp(buf+1, "PREFERENCES]", 12) == 0)
    {
      cookie_section_parser = CookieSetPrefVar;
      return TRUE;
    }

    if (strncasecmp(buf+1, "WINDOW_POS]", 11) == 0)
    {
      cookie_section_parser = CookieSetWinPosVar;
      return TRUE;
    }

    // unknown section !
    cookie_section_parser = NULL;
    CookieDebug("Unknown Section: %s\n", buf);
    return FALSE;
  }

  if (! isalpha(*buf))
    return FALSE;

  // Righteo, line starts with an identifier.  It should be of the
  // form "name = value".  We'll terminate the identifier, and pass
  // the name/value strings to a section-dependent handler.

  name = buf;
  buf++;

  for (buf++; isalpha(*buf) || *buf == '_'; buf++)
  { /* nothing here */ }

  while (isspace(*buf))
    *buf++ = 0;
  
  if (*buf != '=')
    return FALSE;

  *buf++ = 0;

  while (isspace(*buf))
    buf++;

  if (*buf == 0)
    return FALSE;

  CookieDebug("Name: [%s]  Value: [%s]\n", name, buf);

  if (cookie_section_parser)
  {
    return (* cookie_section_parser)(name, buf);
  }

  // variables were found outside of any section
  return FALSE;
}

//
// CookieCheckEm
//
// Increments 'problems' each time something is fixed up.
//
void CookieCheckEm(int& problems)
{
  // check main window size
  if (guix_prefs.win_w < MAIN_WINDOW_MIN_W)
  {
    guix_prefs.win_w = MAIN_WINDOW_MIN_W;
    problems++;
  }

  if (guix_prefs.win_h < MAIN_WINDOW_MIN_H)
  {
    guix_prefs.win_h = MAIN_WINDOW_MIN_H;
    problems++;
  }

  // check manual/license window size
  if (guix_prefs.manual_w < MANUAL_WINDOW_MIN_W)
  {
    guix_prefs.manual_w = MANUAL_WINDOW_MIN_W;
    problems++;
  }

  if (guix_prefs.manual_h < MANUAL_WINDOW_MIN_H)
  {
    guix_prefs.manual_h = MANUAL_WINDOW_MIN_H;
    problems++;
  }

  if (guix_info.factor > 32)
  {
    guix_info.factor = 32;
    problems++;
  }
}

//
// CookieSetPath
//
// Determine the path and filename of the cookie file.
//
void CookieSetPath(const char *argv0)
{
  if (cookie_filename)
    GlbspFree(cookie_filename);

  char buffer[1024];

  buffer[0] = 0;

#if defined(WIN32)

  if (HelperFilenameValid(argv0))
  {
    strcpy(buffer, HelperReplaceExt(argv0, "ini"));
  }
  else
    strcpy(buffer, "glBSPX.ini");

#elif defined(MACOSX)

  if (getenv("HOME"))
  {
    strcpy(buffer, getenv("HOME"));
    strcat(buffer, "/Library/Preferences/");
  }

  strcat(buffer, "glBSPX.rc");

#else  // LINUX

  if (getenv("HOME"))
  {
    strcpy(buffer, getenv("HOME"));
    strcat(buffer, "/");
  }

  // backwards compatibility (glbspX != glBSPX)

  strcat(buffer, ".glbspX_rc");

  if (! HelperFileExists(buffer))
  {
    char *bsp = strrchr(buffer, 'b');

    if (bsp) strncpy(bsp, "BSP", 3);
  }
#endif
 
  cookie_filename = GlbspStrDup(buffer);

  CookieDebug("CookieFilename: '%s'\n", cookie_filename);
}

//
// CookieReadAll
//
// Reads the cookie file.
// Returns one of the COOKIE_E_* values.
// 
// All the variables that can be read here should be initialised to
// default values before calling, in case we fail or the variable is
// missing in the cookie file.
// 
cookie_status_t CookieReadAll(void)
{
  CookieDebug("CookieReadAll BEGUN.\n");

  // open file for reading
  FILE *fp = fopen(cookie_filename, "r");

  if (! fp)
    return COOKIE_E_NO_FILE;

  cookie_section_parser = NULL;

  // simple line-by-line parser
  char buffer[2048];

  int parse_probs = 0;
  int check_probs = 0;

  while (fgets(buffer, sizeof(buffer) - 2, fp))
  {
    if (! CookieParseLine(buffer))
      parse_probs += 1;
  }

  CookieCheckEm(check_probs);

  CookieDebug("CookieReadAll DONE.  %d ParseProbs  %d CheckProbs\n", 
      parse_probs, check_probs);

  // parsing problems take precedence
  if (parse_probs > 0)
    return COOKIE_E_PARSE_ERRORS;

  if (check_probs > 0)
    return COOKIE_E_CHECK_ERRORS;

  return COOKIE_E_OK;
}


//------------------------------------------------------------------------


static void CookieWriteHeader(FILE *fp)
{
  fprintf(fp, "# GLBSPX %s Persistent Data\n", GLBSP_VER);
  fprintf(fp, "# Editing this file by hand is not recommended.\n");

  fprintf(fp, "\n");
  fflush(fp);
}

static void CookieWriteBuildInfo(FILE *fp)
{
  // identifying section
  fprintf(fp, "[BUILD_INFO]\n");

  // file names
  fprintf(fp, "in_file = \"%s\"\n", guix_info.input_file ?
      guix_info.input_file : "");

  fprintf(fp, "out_file = \"%s\"\n", guix_info.output_file ?
      guix_info.output_file : "");

  // factor
  fprintf(fp, "factor = %d\n", guix_info.factor);

  // boolean values
  fprintf(fp, "no_reject = %d\n", guix_info.no_reject ? 1 : 0); 
  fprintf(fp, "mini_warnings = %d\n", guix_info.mini_warnings ? 1 : 0); 
  fprintf(fp, "pack_sides = %d\n", guix_info.pack_sides ? 1 : 0); 
  fprintf(fp, "choose_fresh = %d\n", guix_info.fast ? 0 : 1);  // API change

  // block limit
  fprintf(fp, "block_limit = %d\n", guix_info.block_limit); 

  // build-mode
  fprintf(fp, "gwa_mode = %d\n", guix_info.gwa_mode ? 1 : 0);
  fprintf(fp, "no_normal = %d\n", guix_info.no_normal ? 1 : 0);
  fprintf(fp, "force_normal = %d\n", guix_info.force_normal ? 1 : 0);
 
  // NOT HERE:
  //   no_progress: not useful for GUI mode.
  //   keep_sect, no_prune: not used in GUI mode (yet).
  //   load_all: determined automatically.
 
  // keep a blank line between sections
  fprintf(fp, "\n");
  fflush(fp);
}

static void CookieWritePrefs(FILE *fp)
{
  // identifying section
  fprintf(fp, "[PREFERENCES]\n");
  
  fprintf(fp, "overwrite_warn = %d\n", guix_prefs.overwrite_warn);
  fprintf(fp, "same_file_warn = %d\n", guix_prefs.same_file_warn);
  fprintf(fp, "lack_ext_warn = %d\n", guix_prefs.lack_ext_warn);
 
  fprintf(fp, "save_log_file = \"%s\"\n", guix_prefs.save_log_file ?
      guix_prefs.save_log_file : "");

  fprintf(fp, "\n");
  fflush(fp);
}

static void CookieWriteWindowPos(FILE *fp)
{
  // identifying section
  fprintf(fp, "[WINDOW_POS]\n");

  // main window position and size
  fprintf(fp, "window_x = %d\n", guix_prefs.win_x);
  fprintf(fp, "window_y = %d\n", guix_prefs.win_y);
  fprintf(fp, "window_w = %d\n", guix_prefs.win_w);
  fprintf(fp, "window_h = %d\n", guix_prefs.win_h);

  // other window positions
  fprintf(fp, "progress_x = %d\n", guix_prefs.progress_x);
  fprintf(fp, "progress_y = %d\n", guix_prefs.progress_y);
  fprintf(fp, "dialog_x = %d\n", guix_prefs.dialog_x);
  fprintf(fp, "dialog_y = %d\n", guix_prefs.dialog_y);
  fprintf(fp, "other_x = %d\n", guix_prefs.other_x);
  fprintf(fp, "other_y = %d\n", guix_prefs.other_y);

  fprintf(fp, "manual_x = %d\n", guix_prefs.manual_x);
  fprintf(fp, "manual_y = %d\n", guix_prefs.manual_y);
  fprintf(fp, "manual_w = %d\n", guix_prefs.manual_w);
  fprintf(fp, "manual_h = %d\n", guix_prefs.manual_h);
  fprintf(fp, "manual_page = %d\n", guix_prefs.manual_page);

  fprintf(fp, "\n");
  fflush(fp);
}


//
// CookieWriteAll
//
// Writes the cookie file.
// Returns one of the COOKIE_E_* values.
//
cookie_status_t CookieWriteAll(void)
{
  CookieDebug("CookieWriteAll BEGUN.\n");
  
  // create file (overwrite if exists)
  FILE *fp = fopen(cookie_filename, "w");

  if (! fp)
    return COOKIE_E_NO_FILE;

  CookieWriteHeader(fp);
  CookieWriteBuildInfo(fp);
  CookieWritePrefs(fp);
  CookieWriteWindowPos(fp);

  // all done
  fclose(fp);

  CookieDebug("CookieWriteAll DONE.\n");

  return COOKIE_E_OK;
}

