//----------------------------------------------------------------------------
//  EDGE Console Interface code.
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2005  The EDGE Team.
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
//
// Partially based on the ZDoom console code, by Randy Heit
// (rheit@iastate.edu).  Randy Heit has given his permission to
// release this code under the GPL, for which the EDGE Team is very
// grateful.  The original GPL'd version `c_consol.c' can be found in
// the contrib/ directory.
//

#include "i_defs.h"
#include "con_defs.h"

#include "con_gui.h"
#include "hu_lib.h"
#include "hu_stuff.h"
#include "hu_style.h"
#include "m_argv.h"
#include "v_res.h"
#include "wp_main.h"
#include "z_zone.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define CON_WIPE_TICS  12

#define CON_GFX_HT  (SCREENHEIGHT * 3 / 5)

typedef struct coninfo_s 
{
	visible_t visible;
	int cursor;
	//   char s[SCREENROWS][SCREENCOLS];
	//   char input[BACKBUFFER+1][SCREENCOLS];
	int x, y, pos;
	int backbuffer;
	int page;
	bool echo;
}
coninfo_t;

static coninfo_t con_info;

gui_t console =
{
    0,
	&CON_Ticker,
	&CON_Responder,
	&CON_Drawer,
	(void *)&con_info,
	0, 0, 0, 0, 0, 0, 0
};

// stores the console toggle effect
/// wipeinfo_t *conwipe = NULL;
static int conwipeactive = 0;
static int conwipepos = 0;
static int conwipemethod = WIPE_Crossfade;
static bool conwipereverse = 0;
static int conwipeduration = 10;

#define KEYREPEATDELAY ((250 * TICRATE) / 1000)
#define KEYREPEATRATE  (TICRATE / 15)

// the console's screen
static style_c *console_style;

typedef struct consoleline_s
{
	char *s;  // The String

	int len;  // the length of the string, not counting terminating 0.

}
consoleline_t;

// All the output lines of the console (not only those currently visible).
// A line can be of any length. If the screen is too narrow for it, the line
// will be split up on screen, but it will remain intact in this array.
static consoleline_t *linebuffer = NULL;
static int linebufferpos = 0;  // shows the index after the last one

static int linebuffersize = 0;  // shows the size

// lastline: The last output text line. This is different from the others
// because it is growable, you can print twice without newline.
char *lastline = NULL;  // the last text line of the console.

int lastlinesize = 0;
int lastlinepos = 0;
int lastlineend = 0;

// Properly split up last line.
// s: array of string start pointers, l: array of string lengths,
// n: number of strings.
char **vislastline_s = NULL;
int *vislastline_l = NULL;
int vislastline_n = 0;

// Command Line
char *cmdline = NULL;
int cmdlinesize = 0;
int cmdlinepos = 0;
int cmdlineend = 0;

// Properly split up command line.
char **viscmdline_s = NULL;
int *viscmdline_l = NULL;
int viscmdline_n = 0;

// Command line backup: If you press UPARROW when you've written something
// at the command line, it will be backuped here, and it's possible to restore
// it by pressing DOWNARROW before you've executed another command.
char *cmdlinebkp = NULL;
int cmdlinebkpsize = 0;

// Command Line History. All the written commands.
consoleline_t *cmdhistory = NULL;
int cmdhistoryend = 0;
int cmdhistorysize = 0;

// when browsing the cmdhistory, this shows the current index. Otherwise it's -1.
int cmdhistorypos = -1;

// The text of the console, with lines split up properly for the current
// resolution.
char **curlines = NULL;

// the length of each of curlines
int *curlinelengths = NULL;

// the number of allocated rows in curlines and curlinelengths.
int curlinesize = 0;

// number of visible console lines (curlines+vislastline+viscmdline).
int numvislines = 0;

// width of console. Measured in characters if in text mode, and in pixels
// if in graphics mode.
static int conwidth;
static int conrows;

// the console row that is displayed at the bottom of screen, -1 if cmdline
// is the bottom one.
int bottomrow = -1;

// if true, nothing will be displayed in the console, and there will be no
// command history.
bool no_con_history = 0;

// always type ev_keydown
guievent_t RepeatEvent;
int RepeatCountdown;

// tells whether shift is pressed, and pgup/dn should scroll to top/bottom of linebuffer.
bool KeysShifted;

bool TabbedLast;

bool CON_HandleKey(guievent_t * ev);

typedef enum
{
	NOSCROLL,
	SCROLLUP,
	SCROLLDN
}
scrollstate_e;

static scrollstate_e scroll_state;

static int (*MaxTextLen) (const char *s);

static int MaxTextLen_gfx(const char *s)
{
	if (! console_style)
		return MIN(100, strlen(s));

	return console_style->fonts[0]->MaxFit(conwidth, s);
}

static int MaxTextLen_text(const char *s)
{
	int len = (int)strlen(s);

	if (len > conwidth)
		return conwidth;
	else
		return len;
}

// Adds a line of text to either cmdhistory or linebuffer
static void AddLine(consoleline_t ** line, int *pos, int *size, char *s)
{
	consoleline_t *l;

	// we always have two free lines at the end, these can be used temporarily
	// for easier lastline and cmdline handling
	if (*pos >= *size - 2)
		Z_Resize(*line, consoleline_t, *size += 8);

	l = &(*line)[*pos];
	l->len = (int)strlen(s);
	l->s = Z_New(char, l->len + 1);
	Z_MoveData(l->s, s, char, l->len + 1);

	(*pos)++;
}

//
// GrowLine
//
// helper function for use with cmdline and lastline. Verifies that
// *line can contain newlen characters
//
static void GrowLine(char **line, int *len, int newlen)
{
	if (newlen <= *len)
		return;  // don't need to do anything

	// always grow 128 byte at a time
	newlen = (newlen + 127) & ~127;

	Z_Resize(*line, char, newlen);
	if (!*line)
		I_Error("GrowLine: Out of memory!");

	*len = newlen;
}

// splits up s in conwidth wide chunks, and stores pointers to the start of
// each line at the end of the *lines array.
static void AddSplitRow(char ***lines, int **lengths, int *size, char *s)
{
	int len;

	do
	{
		(*size)++;
		Z_Resize(*lines, char *, *size);
		Z_Resize(*lengths, int, *size);

		len = MaxTextLen(s);
		(*lines)[(*size) - 1] = s;
		(*lengths)[(*size) - 1] = len;
		s += len;
	}
	while (*s);
}

static void UpdateNumvislines(void)
{
	numvislines = curlinesize + vislastline_n + viscmdline_n;
}

static void AddConsoleLine(char *s)
{
	if (no_con_history)
		return;

	AddLine(&linebuffer, &linebufferpos, &linebuffersize, s);
	AddSplitRow(&curlines, &curlinelengths, &curlinesize, linebuffer[linebufferpos - 1].s);
	UpdateNumvislines();
}

// updates cmdline after it has been changed.
static void UpdateCmdLine(void)
{
	viscmdline_n = 0;
	if (cmdlinepos == cmdlineend)
	{  // the cursor is at the end of the command line, allocate space for it too.

		GrowLine(&cmdline, &cmdlinesize, cmdlineend + 2);
		cmdline[cmdlineend] = '_';
		cmdline[cmdlineend + 1] = 0;
	}
	AddSplitRow(&viscmdline_s, &viscmdline_l, &viscmdline_n, cmdline);
	cmdline[cmdlineend] = 0;
	// the cursor should not blink when you're writing.
	con_info.cursor = 0;
	UpdateNumvislines();
}
static void UpdateLastLine(void)
{
	vislastline_n = 0;
	AddSplitRow(&vislastline_s, &vislastline_l, &vislastline_n, lastline);
	UpdateNumvislines();
}

static void AddCommandToHistory(char *s)
{
	// Don't add the string if it's the same as the previous one in history.
	// Add it if history is empty, though.
	if (no_con_history)
		return;
	if (!cmdhistory || strcmp(cmdhistory[cmdhistoryend - 1].s, s))
		AddLine(&cmdhistory, &cmdhistoryend, &cmdhistorysize, s);
}

static void UpdateConsole(void)
{
	int i;

	curlinesize = 0;

	for (i = 0; i < linebufferpos; i++)
	{
		AddSplitRow(&curlines, &curlinelengths, &curlinesize, linebuffer[i].s);
	}

	bottomrow = -1;

	UpdateLastLine();
	UpdateCmdLine();
}

//
// CON_InitConsole
//
// Initialises the console with the given dimensions, in characters.
// gfxmode tells whether it should be initialised to work in graphics mode.
//
void CON_InitConsole(int width, int height, int gfxmode)
{
	conwidth = width;

	if (gfxmode)
	{
		MaxTextLen = MaxTextLen_gfx;
		conrows = (height * 200 / SCREENHEIGHT) / 4;
	}
	else
	{
		MaxTextLen = MaxTextLen_text;
		conrows = height;
	}

	if (lastline == NULL)
	{
		// First time. Init lastline and cmdline and cmdhistory, and add dummy
		// elements
		GrowLine(&lastline, &lastlinesize, 128);
		lastline[0] = 0;
		GrowLine(&cmdline, &cmdlinesize, 128);
		cmdline[0] = '>';
		cmdline[1] = 0;
		cmdlinepos = 1;
		cmdlineend = 1;
		AddCommandToHistory(cmdline);
		AddConsoleLine("");
		no_con_history = M_CheckParm("-noconhistory")?true:false;
	}

	UpdateConsole();
}

void CON_SetVisible(visible_t v)
{
	if (v == vs_toggle)
	{
		v = (con_info.visible == vs_notvisible) ? vs_maximal : vs_notvisible;
	}

	if (con_info.visible == v)
		return;

	con_info.visible = v;

	if (v == vs_maximal)
	{
		GUI_SetFocus(console.gui, &console);
		cmdhistorypos = -1;
		TabbedLast = false;
	}

	if (! conwipeactive)
	{
		conwipeactive = true;
		conwipepos = (v == vs_maximal) ? 0 : CON_WIPE_TICS;
	}
}

static void PrintString(const char *s)
{
	const char *src;

	for (src = s; *src; src++)
	{
		if (*src == '\n')
		{  // new line, add it to the console history.

			lastline[lastlineend] = 0;
			AddConsoleLine(lastline);
			lastlinepos = lastlineend = 0;
			continue;
		}

		if (*src == '\r')
		{
			lastlinepos = 0;
			continue;
		}

		if (*src == '\x8' && lastlinepos > 0)
		{  // backspace
			lastlinepos--;
			continue;
		}

		lastline[lastlinepos] = *src;
		lastlinepos++;

		if (lastlinepos > lastlineend)
			lastlineend = lastlinepos;

		// verify that lastline is big enough to contain the final \0
		GrowLine(&lastline, &lastlinesize, lastlineend + 1);
	}

	lastline[lastlineend] = 0;

	UpdateLastLine();
}

void CON_Printf(const char *message, ...)
{
	va_list argptr;
	char buffer[1024];

	va_start(argptr, message);
	vsprintf(buffer, message, argptr);
	va_end(argptr);

	PrintString(buffer);
}

void CON_MessageLDF(const char *lookup, ...)
{
	va_list argptr;
	char buffer[1024];

	lookup = language[lookup];

	va_start(argptr, lookup);
	vsprintf(buffer, lookup, argptr);
	va_end(argptr);

	HU_StartMessage(buffer);

	strcat(buffer, "\n");

	PrintString(buffer);
}

void CON_Message(const char *message,...)
{
	va_list argptr;
	char buffer[1024];

	va_start(argptr, message);

	// Print the message into a text string
	vsprintf(buffer, message, argptr);

	HU_StartMessage(buffer);

	strcat(buffer, "\n");

	PrintString(buffer);

	va_end(argptr);
}

void CON_Ticker(gui_t * gui)
{
	coninfo_t *info = (coninfo_t *)gui->process;

	info->cursor = (info->cursor + 1) & 31;

	if (info->visible != vs_notvisible)
	{
		// Handle repeating keys
		switch (scroll_state)
		{
		case SCROLLUP:
			if (bottomrow > 0)
				bottomrow--;
			if (bottomrow == -1)
				bottomrow = numvislines - 2;  // numvislines-1 (commandline) is the last line

			break;

		case SCROLLDN:
			if (bottomrow == -1)
				break;  // already at bottom. Can't scroll down.

			if (bottomrow < numvislines - 2)
				bottomrow++;
			else
				bottomrow = -1;
			break;

		default:
			if (RepeatCountdown)
			{
				RepeatCountdown -= 1;

				while (RepeatCountdown <= 0)
				{
					RepeatCountdown += KEYREPEATRATE;
					CON_HandleKey(&RepeatEvent);
				}
			}
			break;
		}
	}

	if (conwipeactive)
	{
		if (info->visible == vs_notvisible)
		{
			conwipepos--;
			if (conwipepos <= 0)
				conwipeactive = false;
		}
		else
		{
			conwipepos++;
			if (conwipepos >= CON_WIPE_TICS)
				conwipeactive = false;
		}
	}
}

// writes the text on coords (x,y) of the console
static void WriteText(int x, int y, char *s, int len, int text_type)
{
	char buffer[1024];

	if (len > 1020)
		len = 1020;

	Z_StrNCpy(buffer, s, len);

	HL_WriteText(console_style, text_type, x, y, buffer, 0.5f);
}

//
// CON_Drawer
//
// Draws the console in graphics mode.
//
void CON_Drawer(gui_t * gui)
{
	if (! console_style)
	{
		styledef_c *def = styledefs.Lookup("CONSOLE");
		if (! def)
			def = default_style;
		console_style = hu_styles.Lookup(def);
	}

	coninfo_t *info = (coninfo_t *)gui->process;
	int i;
	int y;
	int bottom;
	int len, c;

	if (info->visible == vs_notvisible && !conwipeactive)
	{
		return;
	}

	int wiping_y = 0;
	
	if (conwipeactive)
	{
		wiping_y = (CON_GFX_HT-4) * (CON_WIPE_TICS - conwipepos) / CON_WIPE_TICS;
	}

	console_style->DrawBackground(0, 0, SCREENWIDTH, CON_GFX_HT - wiping_y, 1);

	if (bottomrow == -1)
		bottom = numvislines;
	else
		bottom = bottomrow;

	y = 0;
	i = bottom - conrows;
	if (i < 0)
	{  // leave some blank lines before the top

		y -= i;
		i = 0;
	}

	// !!!! FIXME: y * 4 shite

	wiping_y = wiping_y * 200 / SCREENHEIGHT;

	for (; i < curlinesize && y < conrows; i++, y++)
	{
		WriteText(0, y * 4 - wiping_y, curlines[i], curlinelengths[i], 0);
	}
	i -= curlinesize;
	for (; i < vislastline_n && y < conrows; i++, y++)
	{
		WriteText(0, y * 4 - wiping_y, vislastline_s[i], vislastline_l[i], 0);
	}
	i -= vislastline_n;
	for (; i < viscmdline_n && y < conrows; i++, y++)
	{
		WriteText(0, y * 4 - wiping_y, viscmdline_s[i], viscmdline_l[i], 1);
	}

	// draw the cursor on the right place of the command line.
	if (info->cursor < 16 && bottomrow == -1)
	{
		// the command line can be more than one row high, so we must first search
		// for the line containing the cursor.
		i = viscmdline_n;
		y = conrows;
		// if the cursor is alone, we must add an extra char for it
		len = cmdlineend;  //==cmdlinepos?cmdlineend+1:cmdlineend;

		do
		{
			i--;
			y--;
			len -= viscmdline_l[i];
		}
		while (len > cmdlinepos && i > 0);
		// now draw the cursor on the right x position of the right line.
		// But only draw it if it's on the screen
		if (len <= cmdlinepos)
		{
			len = cmdlinepos - len;
			c = viscmdline_s[i][len];
			// temporarily truncate the cmdline to the cursor position.
			viscmdline_s[i][len] = 0;
			WriteText(console_style->fonts[1]->StringWidth(viscmdline_s[i]) / 2,
				y * 4 - wiping_y, "_", 1, 1);
			viscmdline_s[i][len] = c;
		}
	}
}

#if 0
static void TabComplete(void)
{  // -ES- fixme - implement these

}
static void AddTabCommand(char *name)
{
}
static void RemoveTabCommand(char *name)
{
}
#endif

bool CON_HandleKey(guievent_t * ev)
{
	switch (ev->data1)
	{
#if 0  // -ES- fixme - implement tab stuff (need commands first, though)
	case KEYD_TAB:
		// Try to do tab-completion
		TabComplete();
		break;
#endif
	
	case KEYD_PGUP:
		if (KeysShifted)
			// Move to top of console buffer
			bottomrow = 0;
		else
			// Start scrolling console buffer up
			scroll_state = SCROLLUP;
		break;
	
	case KEYD_PGDN:
		if (KeysShifted)
			// Move to bottom of console buffer
			bottomrow = -1;
		else
			// Start scrolling console buffer down
			scroll_state = SCROLLDN;
		break;
	
	case KEYD_HOME:
		// Move cursor to start of line
		cmdlinepos = 1;
		break;
	
	case KEYD_END:
		// Move cursor to end of line
		cmdlinepos = cmdlineend;
		break;
	
	case KEYD_LEFTARROW:
		// Move cursor left one character
	
		if (cmdlinepos > 1)
			cmdlinepos--;
		break;
	
	case KEYD_RIGHTARROW:
		// Move cursor right one character
	
		if (cmdlinepos < cmdlineend)
			cmdlinepos++;
		break;
	
	case KEYD_BACKSPACE:
		// Erase character to left of cursor
	
		if (cmdlinepos > 1)
		{
			char *c, *e;
	
			e = &cmdline[cmdlineend];
			c = &cmdline[cmdlinepos];
	
			for (; c <= e; c++)
				*(c - 1) = *c;
	
			cmdlineend--;
			cmdlinepos--;
		}
	
		TabbedLast = false;
		break;
	
	case KEYD_DELETE:
		// Erase charater under cursor
	
		if (cmdlinepos < cmdlineend)
		{
			char *c, *e;
	
			e = &cmdline[cmdlineend];
			c = &cmdline[cmdlinepos + 1];
	
			for (; c <= e; c++)
				*(c - 1) = *c;
	
			cmdlineend--;
		}
	
		TabbedLast = false;
		break;
	
	case KEYD_RALT:
	case KEYD_RCTRL:
		// Do nothing
		break;
	
	case KEYD_RSHIFT:
		// SHIFT was pressed
		KeysShifted = true;
		break;
	
	case KEYD_UPARROW:
		// Move to previous entry in the command history
		if (cmdhistorypos == -1)
		{
			cmdhistorypos = cmdhistoryend - 1;
			// backup cmdline temporarily: It can be recovered until the next
			// command you execute. another command.
			GrowLine(&cmdlinebkp, &cmdlinebkpsize, cmdlineend + 1);
			Z_MoveData(cmdlinebkp, cmdline, char, cmdlineend + 1);
	
			// add to history unofficially, so that it will be overwritten.
			cmdhistory[cmdhistoryend].s = cmdlinebkp;
			cmdhistory[cmdhistoryend].len = (int)strlen(cmdlinebkp);
		}
		else if (cmdhistorypos)
		{
			cmdhistorypos--;
		}
	
		// set command line to the history index.
		cmdlineend = cmdlinepos = cmdhistory[cmdhistorypos].len;
		GrowLine(&cmdline, &cmdlinesize, cmdlineend + 1);
		Z_MoveData(cmdline, cmdhistory[cmdhistorypos].s, char, cmdlineend + 1);
	
		TabbedLast = false;
		break;
	
	case KEYD_DOWNARROW:
		// Move to next entry in the command history
	
		if (cmdhistorypos != -1 && cmdhistorypos < cmdhistoryend)
		{
			cmdhistorypos++;
			// set command line to the history item.
			cmdlineend = cmdlinepos = cmdhistory[cmdhistorypos].len;
			GrowLine(&cmdline, &cmdlinesize, cmdlineend + 1);
			Z_MoveData(cmdline, cmdhistory[cmdhistorypos].s, char, cmdlineend + 1);
	
			if (cmdhistorypos == cmdhistoryend)
			{  // we just restored the cmdline backup, now we aren't browsing history anymore.
	
				cmdhistorypos = -1;
			}
	
			TabbedLast = false;
		}
		break;
	case KEYD_ENTER:
	
		// Execute command line (ENTER)
	
		// Add it to history & draw it
		AddCommandToHistory(cmdline);
		CON_Printf("\n%s\n", cmdline);
	
		// Run it!
		CON_TryCommand(cmdline + 1);
	
		// clear cmdline
		cmdline[1] = 0;
		cmdlinepos = 1;
		cmdlineend = 1;
		cmdhistorypos = -1;
		TabbedLast = false;
		break;
	
	case KEYD_TILDE:
	case KEYD_ESCAPE:
		// Close console, clear command line, but if we're in the
		// fullscreen console mode, there's nothing to fall back on
		// if it's closed.
		cmdline[1] = 0;
		cmdlinepos = 1;
		cmdlineend = 1;
		cmdhistorypos = -1;
	
		TabbedLast = false;
		UpdateCmdLine();
	
		CON_SetVisible(vs_notvisible);
		break;
	
	default:
		if (ev->data1 < 32 || ev->data1 > 126)
		{
			// Do nothing
		}
		else
		{
			// Add keypress to command line
			char data = ev->data1;
			char *c, *e;
	
			GrowLine(&cmdline, &cmdlinesize, cmdlineend + 2);
	
			// move everything after the cursor, including the 0, one step to the right
			e = &cmdline[cmdlineend];
			c = &cmdline[cmdlinepos];
	
			for (; e >= c; e--)
			{
				*(e + 1) = *e;
			}
	
			// insert the character
			*c = data;
	
			cmdlinepos++;
			cmdlineend++;
		}
		TabbedLast = false;
	
		break;

	}
	// something in the console has probably changed, so we update it
	UpdateCmdLine();

	return true;
}

bool CON_Responder(gui_t * gui, guievent_t * event)
{
	coninfo_t *info = (coninfo_t *)gui->process;

	if (info->visible == vs_notvisible)
	{
#if 0  // CURRENTLY UNREACHABLE CODE
		if (event->type == gev_keydown && event->data1 == KEYD_TILDE)
		{
			CON_SetVisible(vs_toggle);
			return true;
		}
#endif
		return false;
	}

	if (event->type == gev_keyup)
	{
		if (event->data1 == RepeatEvent.data1)
			RepeatCountdown = 0;

		switch (event->data1)
		{
			case KEYD_PGUP:
			case KEYD_PGDN:
				scroll_state = NOSCROLL;
				break;
			case KEYD_RSHIFT:
				KeysShifted = false;
				break;
			default:
				return false;
		}
	}
	else if (event->type == gev_keydown)
	{
		// Okay, fine. Most keys don't repeat
		switch (event->data1)
		{
			case KEYD_RIGHTARROW:
			case KEYD_LEFTARROW:
			case KEYD_UPARROW:
			case KEYD_DOWNARROW:
			case KEYD_SPACE:
			case KEYD_BACKSPACE:
			case KEYD_DELETE:
				RepeatCountdown = KEYREPEATDELAY;
				break;
			default:
				RepeatCountdown = 0;
				break;
		}

		RepeatEvent = *event;
		if (CON_HandleKey(event))
		{
			return true;
		}
	}

	return false;
}

bool CON_InitResolution(void)
{
	CON_InitConsole(SCREENWIDTH, CON_GFX_HT, true);
	return true;
}

void CON_Start(gui_t ** gui)
{
	CON_CreateCVarEnum("conwipemethod", cf_normal, &conwipemethod, WIPE_EnumStr, WIPE_NUMWIPES);
	CON_CreateCVarInt("conwipeduration", cf_normal, &conwipeduration);
	CON_CreateCVarBool("conwipereverse", cf_normal, &conwipereverse);

	GUI_Start(gui, &console);
	CON_SetVisible(vs_maximal);
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
