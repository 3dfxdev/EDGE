//----------------------------------------------------------------------------
//  EDGE Console Interface code.
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2008  The EDGE Team.
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
#include "i_defs_gl.h"

#include "con_defs.h"
#include "e_input.h"
#include "hu_lib.h"
#include "hu_stuff.h"
#include "hu_style.h"
#include "m_argv.h"
#include "r_image.h"
#include "r_modes.h"
#include "r_wipe.h"
#include "z_zone.h"


#define CON_WIPE_TICS  12

#define CON_GFX_HT  (SCREENHEIGHT * 3 / 5)


static visible_t con_visible;

static int con_cursor;

static const image_c *con_font;

// the console's background
static style_c *console_style;


#define T_WHITE   RGB_MAKE(208,208,208)
#define T_YELLOW  RGB_MAKE(255,255,0)

#define MAX_CON_LINES  160

class console_line_c
{
public:
	std::string line;

	rgbcol_t color;

public:
	console_line_c(const std::string& text, rgbcol_t _col = T_WHITE) :
		line(text), color(_col)
	{ }

	console_line_c(const char *text, rgbcol_t _col = T_WHITE) :
		line(text), color(_col)
	{ }

	~console_line_c()
	{ }

	void Append(const char *text)
	{
		line = line + std::string(text);
	}
};

// entry [0] is the bottom-most one
static console_line_c * console_lines[MAX_CON_LINES];

static int con_used_lines = 0;
static bool con_partial_last_line = false;

// the console row that is displayed at the bottom of screen, -1 if cmdline
// is the bottom one.
static int bottomrow = -1;


#define MAX_CON_INPUT  255

static char input_line[MAX_CON_INPUT+2];
static int  input_pos = 0;


// stores the console toggle effect
static int conwipeactive = 0;
static int conwipepos = 0;
static int conwipemethod = WIPE_Crossfade;
static bool conwipereverse = 0;
static int conwipeduration = 10;


#define KEYREPEATDELAY ((250 * TICRATE) / 1000)
#define KEYREPEATRATE  (TICRATE / 15)


#define MAX_CMD_HISTORY  100

static std::string *cmd_history[MAX_CMD_HISTORY];

static int cmd_used_hist = 0;

// when browsing the cmdhistory, this shows the current index. Otherwise it's -1.
static int cmd_hist_pos = -1;


 typedef struct consoleline_s
 {
 	char *s;  // The String
 
 	int len;  // the length of the string, not counting terminating 0.
 
 }
 consoleline_t;

///---// All the output lines of the console (not only those currently visible).
///---// A line can be of any length. If the screen is too narrow for it, the line
///---// will be split up on screen, but it will remain intact in this array.
///---static consoleline_t *linebuffer = NULL;
///---static int linebufferpos = 0;  // shows the index after the last one
///---
///---static int linebuffersize = 0;  // shows the size

///---// lastline: The last output text line. This is different from the others
///---// because it is growable, you can print twice without newline.
///---static char *lastline = NULL;  // the last text line of the console.
///---
///---static int lastlinesize = 0;
///---static int lastlinepos = 0;
///---static int lastlineend = 0;

///---// Properly split up last line.
///---// s: array of string start pointers, l: array of string lengths,
///---// n: number of strings.
///---static char **vislastline_s = NULL;
///---static int *vislastline_l = NULL;
///---static int vislastline_n = 0;

///---// Command Line
///---static char *cmdline = NULL;
///---static int cmdlinesize = 0;
///---static int cmdlinepos = 0;
///---static int cmdlineend = 0;

///--- // Properly split up command line.
///--- static char **viscmdline_s = NULL;
///--- static int *viscmdline_l = NULL;
///--- static int viscmdline_n = 0;

///--- // Command line backup: If you press UPARROW when you've written something
///--- // at the command line, it will be backuped here, and it's possible to restore
///--- // it by pressing DOWNARROW before you've executed another command.
///--- static char *cmdlinebkp = NULL;
///--- static int cmdlinebkpsize = 0;

///---// Command Line History. All the written commands.
///---static consoleline_t *cmdhistory = NULL;
///---static int cmdhistoryend = 0;
///---static int cmdhistorysize = 0;

// when browsing the cmdhistory, this shows the current index. Otherwise it's -1.
///---static int cmdhistorypos = -1;

///---// The text of the console, with lines split up properly for the current
///---// resolution.
///---static char **curlines = NULL;

///---// the length of each of curlines
///---static int *curlinelengths = NULL;
///---
///---// the number of allocated rows in curlines and curlinelengths.
///---static int curlinesize = 0;
///---
///---// number of visible console lines (curlines+vislastline+viscmdline).
///---static int numvislines = 0;

///---// width of console. Measured in characters if in text mode, and in pixels
///---// if in graphics mode.
///---static int conrows;

// if true, nothing will be displayed in the console, and there will be no
// command history.
static bool no_con_history = 0;

// always type ev_keydown
static int RepeatKey;
static int RepeatCountdown;

// tells whether shift is pressed, and pgup/dn should scroll to top/bottom of linebuffer.
static bool KeysShifted;

static bool TabbedLast;

bool CON_HandleKey(int key);

typedef enum
{
	NOSCROLL,
	SCROLLUP,
	SCROLLDN
}
scrollstate_e;

static scrollstate_e scroll_state;


static void CON_AddLine(const char *s, bool partial)
{
	if (con_partial_last_line)
	{
		SYS_ASSERT(console_lines[0]);

		console_lines[0]->Append(s);

		con_partial_last_line = partial;
		return;
	}

	// scroll everything up 

	delete console_lines[MAX_CON_LINES-1];

	for (int i = MAX_CON_LINES-1; i > 0; i--)
		console_lines[i] = console_lines[i-1];

	console_lines[0] = new console_line_c(s);

	con_partial_last_line = partial;

	if (con_used_lines < MAX_CON_LINES)
		con_used_lines++;
}

static void CON_AddCmdHistory(const char *s)
{
	// scroll everything up 
	delete cmd_history[MAX_CMD_HISTORY-1];

	for (int i = MAX_CMD_HISTORY-1; i > 0; i--)
		cmd_history[i] = cmd_history[i-1];

	cmd_history[0] = new std::string(s);

	if (cmd_used_hist < MAX_CMD_HISTORY)
		cmd_used_hist++;
}

static void CON_ClearInputLine(void)
{
	input_line[0] = 0;
	input_pos = 0;
}


void CON_SetVisible(visible_t v)
{
	if (v == vs_toggle)
	{
		v = (con_visible == vs_notvisible) ? vs_maximal : vs_notvisible;

		scroll_state = NOSCROLL;
	}

	if (con_visible == v)
		return;

	con_visible = v;

	if (v == vs_maximal)
	{
///!!!!		cmdhistorypos = -1;
		TabbedLast = false;
	}

	if (! conwipeactive)
	{
		conwipeactive = true;
		conwipepos = (v == vs_maximal) ? 0 : CON_WIPE_TICS;
	}
}


static void StripWhitespace(char *src)
{
	const char *start = src;

	while (*start && isspace(*start))
		start++;

	const char *end = src + strlen(src);

	while (end > start && isspace(end[-1]))
		end--;

	while (start < end)
	{
		*src++ = *start++;
	}

	*src = 0;
}


static void SplitIntoLines(char *src)
{
	char *dest = src;
	char *line = dest;

	while (*src)
	{
		if (*src == '\n')
		{
			*dest++ = 0;

			CON_AddLine(line, false);

			line = dest;

			src++; continue;
		}

		// strip non-printing characters (including backspace)
		if (! isprint(*src))
		{
			src++; continue;
		}

		*dest++ = *src++;
	}

	*dest++ = 0;

	CON_AddLine(line, true);
}

void CON_Printf(const char *message, ...)
{
	va_list argptr;
	char buffer[1024];

	va_start(argptr, message);
	vsprintf(buffer, message, argptr);
	va_end(argptr);

	SplitIntoLines(buffer);
}

void CON_Message(const char *message,...)
{
	va_list argptr;
	char buffer[1024];

	va_start(argptr, message);

	// Print the message into a text string
	vsprintf(buffer, message, argptr);

	va_end(argptr);


	HU_StartMessage(buffer);

	strcat(buffer, "\n");

	SplitIntoLines(buffer);

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

	SplitIntoLines(buffer);
}


void CON_Ticker(void)
{
	con_cursor = (con_cursor + 1) & 31;

	if (con_visible != vs_notvisible)
	{
		// Handle repeating keys
		switch (scroll_state)
		{
		case SCROLLUP:
			if (bottomrow < MAX_CON_LINES-10)
				bottomrow++;

			break;

		case SCROLLDN:
			if (bottomrow > -1)
				bottomrow--;

			break;  

		default:
			if (RepeatCountdown)
			{
				RepeatCountdown -= 1;

				while (RepeatCountdown <= 0)
				{
					RepeatCountdown += KEYREPEATRATE;
					CON_HandleKey(RepeatKey);
				}
			}
			break;
		}
	}

	if (conwipeactive)
	{
		if (con_visible == vs_notvisible)
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


static int SIZE;
static int XMUL;
static int YMUL;


static void WriteChar(int x, int y, char ch, rgbcol_t col)
{
	if (x + SIZE < 0)
		return;

	GLuint tex_id = W_ImageCache(con_font);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tex_id);
 
	glEnable(GL_BLEND);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0);

	float alpha = 1.0f;

	glColor4f(RGB_RED(col)/255.0f, RGB_GRN(col)/255.0f, 
	          RGB_BLU(col)/255.0f, alpha);

	int px =      int((byte)ch) % 16;
	int py = 15 - int((byte)ch) / 16;

	float tx1 = (px  ) / 16.0;
	float tx2 = (px+1) / 16.0;

	float ty1 = (py  ) / 16.0;
	float ty2 = (py+1) / 16.0;


	glBegin(GL_POLYGON);
  
	glTexCoord2f(tx1, ty1);
	glVertex2i(x, y);

	glTexCoord2f(tx1, ty2); 
	glVertex2i(x, y + SIZE);
  
	glTexCoord2f(tx2, ty2);
	glVertex2i(x + SIZE, y + SIZE);
  
	glTexCoord2f(tx2, ty1);
	glVertex2i(x + SIZE, y);
  
	glEnd();


	glDisable(GL_TEXTURE_2D);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);
}

// writes the text on coords (x,y) of the console
static void WriteText(int x, int y, const char *s, rgbcol_t col)
{
	for (; *s; s++)
	{
		WriteChar(x, y, *s, col);

		x += XMUL;

		if (x >= SCREENWIDTH)
			return;
	}

///---	HL_WriteText(console_style, text_type, x, y, buffer, 0.5f);
}


void CON_Drawer(void)
{
	if (! con_font)
	{
		con_font = W_ImageLookup("CON_FONT_2", INS_Graphic, ILF_Exact|ILF_Null);
		if (! con_font)
			I_Error("Cannot find essential image: CON_FONT_2\n");
	}

	if (! console_style)
	{
		styledef_c *def = styledefs.Lookup("CONSOLE");
		if (! def)
			def = default_style;
		console_style = hu_styles.Lookup(def);
	}

	if (con_visible == vs_notvisible && !conwipeactive)
		return;


	// determine font sizing and spacing

	if (SCREENWIDTH < 400)
	{
		SIZE = 10; XMUL = 7; YMUL = 11;
	}
	else if (SCREENWIDTH < 700)
	{
		SIZE = 13;  XMUL = 9;  YMUL = 14;
	}
	else
	{
		SIZE = 16;  XMUL = 11;  YMUL = 18;
	}


	// -- background --

	int y = SCREENHEIGHT;

	if (conwipeactive)
		y = y - CON_GFX_HT * (conwipepos) / CON_WIPE_TICS;
	else
		y = y - CON_GFX_HT;

	console_style->DrawBackground(0, y, SCREENWIDTH, SCREENHEIGHT - y, 1);

	y += 4;

	// -- input line --

	if (bottomrow == -1)
	{
		WriteText(0, y, ">", T_YELLOW);

		if (cmd_hist_pos >= 0)
		{
			const char *text = cmd_history[cmd_hist_pos]->c_str();

			WriteText(XMUL, y, text, T_YELLOW);
		}
		else
		{
			WriteText(XMUL, y, input_line, T_YELLOW);

			if (con_cursor < 16)
			{
				WriteText((input_pos+1) * XMUL, y - 2, "_", T_YELLOW);
			}
		}

		y += YMUL + YMUL / 3;
	}

	// -- text lines --

	for (int i = MAX(0,bottomrow); i < MAX_CON_LINES; i++)
	{
		console_line_c *CL = console_lines[i];

		if (! CL)
			break;

		WriteText(0, y, CL->line.c_str(), CL->color);

		y += YMUL;

		if (y >= SCREENHEIGHT)
			break;
	}

///-- 	i -= curlinesize;
///-- 
///-- 	for (; i < vislastline_n; i++, y += YMUL)
///-- 	{
///-- 		WriteText(0, y, vislastline_s[i], vislastline_l[i], 0);
///-- 	}
///-- 
///-- 	i -= vislastline_n;
///-- 
///-- 	for (; i < viscmdline_n; i++, y += YMUL)
///-- 	{
///-- 		WriteText(0, y, viscmdline_s[i], viscmdline_l[i], 1);
///-- 	}

#if 0  // TODO !!!!!!

	// draw the cursor on the right place of the command line.
	if (con_cursor < 16 && bottomrow == -1)
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

		} while (len > cmdlinepos && i > 0);

		// now draw the cursor on the right x position of the right line.
		// But only draw it if it's on the screen
		if (len <= cmdlinepos)
		{
			len = cmdlinepos - len;
			c = viscmdline_s[i][len];
			// temporarily truncate the cmdline to the cursor position.
			viscmdline_s[i][len] = 0;

			WriteText(16 * strlen(viscmdline_s[i]) / 2, bottom_y + y * YMUL,
				      "_", 1, 1);

			viscmdline_s[i][len] = c;
		}
	}

#endif
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

bool CON_HandleKey(int key)
{
	switch (key)
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
			bottomrow = MAX_CON_LINES - 10;  //!!! FIXME
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
		input_pos = 0;
		con_cursor = 0;
		break;
	
	case KEYD_END:
		// Move cursor to end of line
		while (input_line[input_pos] != 0)
			input_pos++;
		con_cursor = 0;
		break;
	
	case KEYD_LEFTARROW:
		// Move cursor left one character
	
		if (input_pos > 0)
			input_pos--;
		con_cursor = 0;
		break;
	
	case KEYD_RIGHTARROW:
		// Move cursor right one character
	
		if (input_line[input_pos] != 0)
			input_pos++;
		con_cursor = 0;
		break;
	
	case KEYD_BACKSPACE:
		// Erase character to left of cursor
	
		if (input_pos > 0)
		{
			input_pos--;

			// shift characters back
			for (int j = input_pos; j < MAX_CON_INPUT-2; j++)
				input_line[j] = input_line[j+1];
		}
	
		TabbedLast = false;
		con_cursor = 0;
		break;
	
	case KEYD_DELETE:
		// Erase charater under cursor
	
		if (input_line[input_pos] != 0)
		{
			// shift characters back
			for (int j = input_pos; j < MAX_CON_INPUT-2; j++)
				input_line[j] = input_line[j+1];
		}
	
		TabbedLast = false;
		con_cursor = 0;
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
		if (cmd_hist_pos < cmd_used_hist)
			cmd_hist_pos++;

		TabbedLast = false;
		break;
	
	case KEYD_DOWNARROW:
		// Move to next entry in the command history
	
		if (cmd_hist_pos > -1)
			cmd_hist_pos--;
	
		TabbedLast = false;
		break;

	case KEYD_ENTER:
	
		// Execute command line (ENTER)

		StripWhitespace(input_line);

		if (strlen(input_line) == 0)
		{
			CON_Printf(">\n");
		}
		else
		{
			// Add it to history & draw it
			CON_AddCmdHistory(input_line);
			CON_Printf("\n>%s\n", input_line);
		
			// Run it!
			CON_TryCommand(input_line);
		}
	
		CON_ClearInputLine();

		TabbedLast = false;
		break;
	
	case KEYD_ESCAPE:
		// Close console, clear command line, but if we're in the
		// fullscreen console mode, there's nothing to fall back on
		// if it's closed.
		CON_ClearInputLine();
	
		TabbedLast = false;
///---	UpdateCmdLine();
	
		CON_SetVisible(vs_notvisible);
		break;
	
	default:
		if (key < 32 || key > 126)
		{
			// ignore non-printable characters
			break;
		}

		if (input_pos >= MAX_CON_INPUT-1)
			break;

		// make room for new character, shift the trailing NUL too
		{
			for (int j = MAX_CON_INPUT-2; j >= input_pos; j--)
				input_line[j+1] = input_line[j];

			input_line[MAX_CON_INPUT-1] = 0;
		}

		input_line[input_pos++] = key;

///!!!!			// Add keypress to command line
///!!!!			char data = key;
///!!!!			char *c, *e;
///!!!!	
///!!!!///---		GrowLine(&cmdline, &cmdlinesize, cmdlineend + 2);
///!!!!	
///!!!!			// move everything after the cursor, including the 0, one step to the right
///!!!!			e = &cmdline[cmdlineend];
///!!!!			c = &cmdline[cmdlinepos];
///!!!!	
///!!!!			for (; e >= c; e--)
///!!!!			{
///!!!!				*(e + 1) = *e;
///!!!!			}
///!!!!	
///!!!!			// insert the character
///!!!!			*c = data;
///!!!!	
///!!!!			cmdlinepos++;
///!!!!			cmdlineend++;

		TabbedLast = false;
		con_cursor = 0;
	
		break;

	}
	// something in the console has probably changed, so we update it
///---UpdateCmdLine();

	return true;
}

bool CON_Responder(event_t * ev)
{
	if (ev->type == ev_keydown && ev->value.key == key_console)
	{
		CON_SetVisible(vs_toggle);
		return true;
	}

	if (con_visible == vs_notvisible)
		return false;

	if (ev->type == ev_keyup)
	{
		if (ev->value.key == RepeatKey)
			RepeatCountdown = 0;

		switch (ev->value.key)
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
	else if (ev->type == ev_keydown)
	{
		// Okay, fine. Most keys don't repeat
		switch (ev->value.key)
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

		RepeatKey = ev->value.key;

		return CON_HandleKey(RepeatKey);
	}

	return false;
}


//
// Initialises the console with the given dimensions, in characters.
//
void CON_InitConsole(void)
{
	con_used_lines = 0;
	cmd_used_hist  = 0;

	bottomrow = -1;
	cmd_hist_pos = -1;

	CON_ClearInputLine();

	CON_AddLine("", false);
	CON_AddLine("", false);
}

void CON_Start(void)
{
	CON_CreateCVarEnum("conwipemethod",  cf_normal, &conwipemethod, WIPE_EnumStr, WIPE_NUMWIPES);
	CON_CreateCVarInt("conwipeduration", cf_normal, &conwipeduration);
	CON_CreateCVarBool("conwipereverse", cf_normal, &conwipereverse);

	con_visible = vs_notvisible;
	con_cursor  = 0;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
