//------------------------------------------------------------------------
//  EDITOR : Editing widget
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
//
//  Based on the "editor.cxx" sample program from FLTK 1.1.6,
//  as described in Chapter 4 of the FLTK Programmer's Guide.
//  
//  Copyright 1998-2004 by Bill Spitzak, Mike Sweet and others.
//
//------------------------------------------------------------------------

#include "defs.h"


#define FLOWOVER_STYLE(ch)  ((ch) == 'S' || (ch) == 'T' || (ch) == 'I')


void style_unfinished_cb(int, void *);
void style_update_cb(int, int, int, int, const char *, void *);


W_Editor::W_Editor(int X, int Y, int W, int H, const char *label) :
	Fl_Text_Editor(X, Y, W, H, label),
	cur_scheme(-1)
{
	textbuf  = new Fl_Text_Buffer;
	stylebuf = new Fl_Text_Buffer;

	buffer(textbuf);

	SetScheme(SCHEME_Dark, 18);

	textbuf->add_modify_callback(style_update_cb, this);
}

W_Editor::~W_Editor()
{
	delete textbuf;
	delete stylebuf;
}

void W_Editor::SetScheme(int kind, int font_h)
{
	textfont(FL_COURIER);

	if (true) // kind == SCHEME_Dark)
	{
		color(FL_BLACK /* background */, FL_WHITE /* selection */);
		cursor_color(FL_WHITE);
	}
	else
	{
		color(FL_WHITE /* background */, FL_BLACK /* selection */);
		cursor_color(FL_BLACK);
	}

	for (int i = 0; i < TABLE_SIZE; i++)
	{
		table_dark [i].size = font_h;
		// table_light[i].size = font_h;
	}

	highlight_data(stylebuf, table_dark,
		// (kind == SCHEME_Dark) ? table_dark : table_dark,
		TABLE_SIZE, 'Z', style_unfinished_cb, this);

	cur_scheme = kind;
}

int W_Editor::handle(int event)
{
	if (event == FL_KEYDOWN)
	{
		int key = Fl::event_key();

		if (key == FL_Tab)
			return 1;
	}

	// pass it along to daddy...
	return Fl_Text_Editor::handle(event);
}

//---------------------------------------------------------------------------

Fl_Text_Display::Style_Table_Entry W_Editor::table_dark[W_Editor::TABLE_SIZE] =
{
	{ FL_RED,        FL_COURIER,        14, 0 }, // 'A' - All else
	{ FL_GREEN,      FL_COURIER_BOLD,   14, 0 }, // 'B'
	{ FL_BLUE,       FL_COURIER,        14, 0 }, // 'C' - Comments //
	{ FL_MAGENTA,    FL_COURIER,        14, 0 }, // 'D' - Directives #
	{ FL_GREEN,      FL_COURIER_BOLD,   14, 0 }, // 'E' - ERRORS
	{ FL_MAGENTA,    FL_COURIER,        14, 0 }, // 'F' - Flags (specials)
	{ FL_DARK2,      FL_COURIER,        14, 0 }, // 'G'
	{ FL_DARK2,      FL_COURIER,        14, 0 }, // 'H'
	{ FL_GREEN,      FL_COURIER_BOLD,   14, 0 }, // 'I' - Items [ ]
	{ FL_CYAN,       FL_COURIER,        14, 0 }, // 'J' - States
	{ FL_LIGHT2,     FL_COURIER,        14, 0 }, // 'K' - Keyword (command)
	{ FL_DARK2,      FL_COURIER,        14, 0 }, // 'L'
	{ FL_DARK2,      FL_COURIER,        14, 0 }, // 'M'
	{ FL_YELLOW,     FL_COURIER,        14, 0 }, // 'N' - Numbers
	{ FL_DARK2,      FL_COURIER,        14, 0 }, // 'O'
	{ FL_DARK2,      FL_COURIER,        14, 0 }, // 'P'
	{ FL_CYAN,       FL_COURIER_BOLD,   14, 0 }, // 'Q' - Quoted char \X
	{ FL_DARK2,      FL_COURIER_BOLD,   14, 0 }, // 'R'
	{ FL_GREEN,      FL_COURIER,        14, 0 }, // 'S' - Strings ""
	{ FL_MAGENTA,    FL_COURIER_BOLD,   14, 0 }, // 'T' - Tags < >
	{ FL_RED,        FL_COURIER_BOLD,   14, 0 }, // 'U' - Unknown words
	{ FL_DARK_YELLOW,FL_COURIER,        14, 0 }, // 'V' - invalid text
	{ FL_DARK2,      FL_COURIER,        14, 0 }, // 'W' - Word
	{ FL_DARK2,      FL_COURIER,        14, 0 }, // 'X' - no comma (command)
	{ FL_DARK2,      FL_COURIER,        14, 0 }, // 'Y' - no comma (states)
	{ FL_DARK2,      FL_COURIER,        14, 0 }  // 'Z' - no comma (flags)
};


//
// 'style_unfinished_cb()' - Update unfinished styles.
//

void style_unfinished_cb(int, void*)
{
}


//
// 'style_update()' - Update the style buffer...
//

void style_update_cb(int pos,		// Position of update
					 int nInserted,	// Number of inserted chars
					 int nDeleted,	// Number of deleted chars
					 int nRestyled,	// Number of restyled chars
					 const char * deletedText,// Text that was deleted
					 void *cbArg) 	// Callback data
{
	(void)nRestyled;
	(void)deletedText;

	W_Editor *WE = (W_Editor*) cbArg;

	// If this is just a selection change, just unselect the style buffer...
	if (nInserted == 0 && nDeleted == 0)
	{
		WE->stylebuf->unselect();
		return;
	}

	// Track changes in the text buffer...
	if (nInserted > 0)
	{
		// Insert characters into the style buffer...
		char *style = new char[nInserted + 1];
		memset(style, 'A', nInserted);
		style[nInserted] = '\0';

		WE->stylebuf->replace(pos, pos + nDeleted, style);
		delete[] style;
	}
	else
	{
		// Just delete characters in the style buffer...
		WE->stylebuf->remove(pos, pos + nDeleted);
	}

	// Select the area that was just updated to avoid unnecessary
	// callbacks...
	WE->stylebuf->select(pos, pos + nInserted - nDeleted);

	// Re-parse the changed region; we do this by parsing from the
	// beginning of the line of the changed region to the end of
	// the line of the changed region...  Then we check the last
	// style character and keep updating if necessary.

	int start = WE->textbuf->line_start(pos);
	int end   = WE->textbuf->line_end(pos + nInserted);

	if (end < WE->textbuf->length())
		end++;

	if (start >= WE->textbuf->length())
		return;

	for (;;)
	{
		// NOTE: 'end-1' could be invalid if last line has no trailing NUL.
		//       The character() method returns NUL in this case.
		char last_before = WE->stylebuf->character(end-1);

		WE->UpdateStyleRange(start, end);

		char last_after = WE->stylebuf->character(end-1);

		if (last_before == last_after)
			break;

		// The newline ('\n') on the end line changed styles, so
		// reparse another chunk.

		if (end >= WE->textbuf->length())
			break;

		start = end;
		end   = WE->textbuf->skip_lines(start, 5);

		SYS_ASSERT(start < end);
	}
}

void W_Editor::UpdateStyleRange(int start, int end)
{
	// get style of previous line (from the '\n' character)
	char context = 'A';

	if (start > 0)
		context = stylebuf->character(start - 1);

	char *text  = textbuf->text_range(start, end);
	char *style = stylebuf->text_range(start, end);
	{
		Parse(text, text + (end - start), style, context);

		stylebuf->replace(start, end, style);
	}
	free(text);
	free(style);

	redisplay_range(start, end);
}

bool W_Editor::Load(const char *filename)
{
	//!!!!! FIXME: remove <CR> characters
	if (textbuf->insertfile(filename, 0) != 0)
	{
		fl_alert("Loading file %s failed\n", filename);
		exit(9); //!!!!
	}

	return true;
}

//!!!!! FIXME: Save method


//---------------------------------------------------------------------------
//  STYLE PARSING CODE
//---------------------------------------------------------------------------

void W_Editor::Parse(const char *text, const char *t_end, char *style, char context)
{
	SYS_ASSERT(text < t_end);

	// --- LOOP over each line ---
	// (Sometimes a group of lines, e.g. when a string is mal-formed)

	bool partial_line = false;

	while (text < t_end)
	{
		bool at_col0 = ! partial_line;
		partial_line = false;

		if (FLOWOVER_STYLE(context))
		{
			int len = ParseBadStuff(text, t_end, style, &context);
			text  += len;
			style += len;
			continue;
		}

		// skip leading whitespace
		while (text < t_end && *text != '\n' && isspace(*text))
			text++, *style++ = 'A', at_col0 = false;

		int line_len = 0;
		while (text+line_len < t_end && text[line_len] != '\n')
			line_len++;

		// look for '=' separator and comments
		int equal_pos = -1;
		int comment_pos = -1;
		bool in_string = false;

		for (int j=0; j < line_len; j++)
		{
			if (text[j] == '"')
			{
				in_string = !in_string;
				continue;
			}

			if (!in_string && text[j] == '=' && equal_pos < 0)
				equal_pos = j;

			if (!in_string && j+1 < line_len && strncmp(text+j, "//", 2) == 0)
			{
				comment_pos = j;
				break;
			}
		}

		// check for missing semicolon or comma (in previous line)
		bool is_special = (at_col0 && (*text == '<' || *text == '#' || *text == '['));
		bool no_comma = (context >= 'X' && context <= 'Z');

		if (context != 'A' && (is_special || equal_pos >= 0 || no_comma))
		{
			memset(style, 'E', line_len);

			text  += line_len;
			style += line_len;

			if (is_special || equal_pos >= 0)
			{
				context = 'A';
			}
			else switch (context)
			{
				case 'X': context = 'K'; break;
				case 'Y': context = 'J'; break;
				case 'Z': context = 'F'; break;
				default:  context = 'A'; break;
			}

			if (text < t_end)
				text++, *style++ = context;
			continue;
		}

		int len = CheckMalformedString(text, t_end, style, &context);
		if (len > 0)
		{
			text  += len;
			style += len;
			continue;
		}

		// --- special stuff ---

		if (at_col0 && *text == '<')  // Tags
		{
			int len = ParseTag(text, t_end, style, &context);
			text  += len;
			style += len;
			continue;
		}
		else if (at_col0 && *text == '#')  // Directives
		{
			int len = ParseDirective(text, t_end, style, &context);
			text  += len;
			style += len;
			continue;
		}
		else if (at_col0 && *text == '[')  // Items
		{
			int len = ParseItem(text, t_end, style, &context);
			text  += len;
			style += len;
			
			if (context == 'A')
				partial_line = true;
			continue;
		}

		// --- normal stuff ---

		ParseLine(text, text + ((comment_pos >= 0) ? comment_pos : line_len),
			style, equal_pos, &context);

		if (comment_pos >= 0)
			ParseComment(text + comment_pos, text + line_len, style + comment_pos);

		text  += line_len;
		style += line_len;

		SYS_ASSERT(text <= t_end);

		// update newline to the correct context
		if (text < t_end)
			text++, *style++ = context;
	}
}

int W_Editor::CheckMalformedString(const char *text, const char *t_end, char *style,
	char *context)
{
	const char *t_orig = text;
	const char *last_quote = NULL;

	bool in_string = false;

	for (; text < t_end && *text != '\n'; text++)
	{
		// check for comments
		if (!in_string && text+1 < t_end && strncmp(text, "//", 2) == 0)
			break;

		if (*text == '\"')
		{
			if (!in_string)
				last_quote = text;

			in_string = !in_string;
		}
		else if (text[0] == '\\' && text+1 < t_end && !isspace(text[1]))
		{
			text++; // Escape sequence, handles \" too
		}
	}

	if (! in_string)
		return 0;

	SYS_ASSERT(last_quote);

	// mark bordering characters as invalid
//	in_string = false;
	for (text = t_orig; text < last_quote; )
	{
		if (*text == '\"')
		{
			int len = ParseString(text, t_end, style);
			text  += len;
			style += len;
			continue;
		}

		text++, *style++ = 'V'; // (in_string || *text == '\"') ? 'S' : 'V';
	}

	SYS_ASSERT(text == last_quote);
	SYS_ASSERT(text < t_end);

	SYS_ASSERT(*text == '\"');

	// mark this quote as an error
	text++, *style++ = 'E';

	*context = 'S';

	return (text - t_orig);
}

int W_Editor::ParseString(const char *text, const char *t_end, char *style)
{
	const char *t_orig = text;

	SYS_ASSERT(text[0] == '\"');

	text++, *style++ = 'S';

	while (text < t_end)
	{
		if (*text == '\"')
		{
			// End-quote of string
			text++, *style++ = 'S';
			break;
		}

		if (text[0] == '\\' && text+1 < t_end && !isspace(text[1]))
		{
			// Escape sequence, handles \" too
			text++, *style++ = 'Q';
			text++, *style++ = 'Q';
			continue;
		}

		text++, *style++ = 'S';
	}

	return (text - t_orig);
}

int W_Editor::ParseBadStuff(const char *text, const char *t_end, char *style,
	char *context)
{
	const char *t_orig = text;

	char end_symbol = 0;

	switch (*context)
	{
		case 'S': end_symbol = '"'; break;
		case 'T': end_symbol = '>'; break;
		case 'I': end_symbol = ']'; break;

		default:
			AssertFail("OVERFLOW Context '%c' not handled.\n", *context);
			break; /* NOT REACHED */
	}
	
	while (text < t_end)
	{
		if (text[0] == '\\' && *context == 'S' &&
			text+1 < t_end && !isspace(text[1]))
		{
			// Escape sequence, handles \" too
			text++, *style++ = 'Q';
			text++, *style++ = 'Q';
			continue;
		}

		text++, *style++ = *context;

		if (text[-1] == end_symbol)
			break;
	}

	// mark bordering characters as invalid
	while (text < t_end && *text != '\n')
		text++, *style++ = 'V';

	*context = 'A';

	// update newline to correct context
	if (text < t_end)
		text++, *style++ = *context;

	return (text - t_orig);
}

int W_Editor::ParseComment(const char *text, const char *t_end, char *style)
{
	const char *t_orig = text;

	while (text < t_end && *text != '\n')
	{
		text++, *style++ = 'C';
	}

	return (text - t_orig);
}

int W_Editor::ParseTag(const char *text, const char *t_end, char *style,
	char *context)
{
	const char *t_orig = text;

	SYS_ASSERT(*text == '<');

	// check for invalid tag (missing '>')
	int pos;
	for (pos = 0; text+pos < t_end && text[pos] != '\n'; pos++)
		if (text[pos] == '>')
			break;
	
	if (text+pos >= t_end || text[pos] != '>')
	{
		// mark initial '<' as an error
		if (text < t_end)
			text++, *style++ = 'E';

		*context = 'T';
		return (text - t_orig);
	}

	ValidateTag(text+1, text+pos, style+1);

	style[0] = style[pos] = 'T';

	text  += pos+1;
	style += pos+1;

	// mark anything after the closing '>' as an error
	while (text < t_end && *text != '\n' && isspace(*text))
		text++, *style++ = 'A';

	while (text < t_end && *text != '\n')
	{
		if (text+1 < t_end && strncmp(text, "//", 2) == 0)
		{
			*context = 'A';
			return (text - t_orig);
		}

		text++, *style++ = 'E';
	}

	*context = 'A';

	// update newline to correct context
	if (text < t_end)
		text++, *style++ = *context;

	return (text - t_orig);
}

int W_Editor::ParseItem(const char *text, const char *t_end, char *style,
	char *context)
{
	const char *t_orig = text;

	SYS_ASSERT(*text == '[');

	// check for invalid item (missing ']')
	int pos;
	for (pos = 0; text+pos < t_end && text[pos] != '\n'; pos++)
		if (text[pos] == ']')
			break;

	if (text+pos >= t_end || text[pos] != ']')
	{
		// mark initial '[' as an error
		if (text < t_end)
			text++, *style++ = 'E';

		*context = 'I';
		return (text - t_orig);
	}

	ValidateItem(text+1, text+pos, style+1);

	style[0] = style[pos] = 'I';

	text  += pos+1;
	style += pos+1;

	*context = 'A';
	return (text - t_orig);
}

int W_Editor::ParseDirective(const char *text, const char *t_end, char *style,
	char *context)
{
	const char *t_orig = text;

	while (text < t_end && *text != '\n')
	{
		if (text+1 < t_end && strncmp(text, "//", 2) == 0)
		{
			*context = 'A';
			return (text - t_orig);
		}

		text++, *style++ = 'D';
	}

	*context = 'A';

	// update newline to correct context
	if (text < t_end)
		text++, *style++ = 'A';

	return (text - t_orig);
}

#if 0
int W_Editor::ParseBadItem(const char *text, const char *t_end, char *style,
	char *context)
{
	const char *t_orig = text;

	while (text < t_end)
	{
		text++, *style++ = 'I';

		if (text[-1] == ']')
			break;
	}

	// handle mal-formed items like mal-formed strings

	while (text < t_end && *text != '\n')
		text++, *style++ = 'V';

	*context = 'A';

	if (text < t_end)
		text++, *style++ = *context;

	return (text - t_orig);
}
#endif

int W_Editor::ParseWord(const char *text, const char *t_end, char *style)
{
	const char *t_orig = text;

	while (text < t_end)
	{
		if (! isalnum(*text) && *text != '_' && *text != '#')
			break;

		text++, *style++ = 'W';
	}

	return (text - t_orig);
}

int W_Editor::ParseNumber(const char *text, const char *t_end, char *style)
{
	const char *t_orig = text;

	if (*text == '-')
	{
		text++, *style++ = 'N';
	}

	while (text < t_end)
	{
		if (*text == '%')
		{
			text++, *style++ = 'N';
			break;
		}

		if (! isdigit(*text) && *text != '.')
			break;

		text++, *style++ = 'N';
	}

	return (text - t_orig);
}

//------------------------------------------------------------------------

void W_Editor::ParseLine(const char *text, const char *t_end, char *style,
	int equal_pos, char *context)
{
	bool has_comma = CheckComma(text, t_end);
	int semi_pos = CheckSemicolon(text, t_end, equal_pos, style);

	if (semi_pos >= 0)
		t_end = text + semi_pos;

	switch (*context)
	{
		case 'A': ParseCommand(text, t_end, style, equal_pos, context);
			break;

		case 'K': ParseCommandData(text, t_end, style, context);
			break;

		case 'J': ParseStateData(text, t_end, style, context);
			break;

		case 'F': ParseFlagData(text, t_end, style, context);
			break;

		default:
			AssertFail("LINE Context '%c' not handled.\n", *context);
			break; /* NOT REACHED */
	}

	// semicolons mark the end of the current command
	if (semi_pos >= 0)
	{
		*context = 'A';
	}
	else if (! has_comma)
	{
		switch (*context)
		{
			case 'K': *context = 'X'; break;
			case 'J': *context = 'Y'; break;
			case 'F': *context = 'Z'; break;
			default: /* nothing to do */ break;
		}
	}

	// for command lines, brackets before '=' are validated elsewhere
	if (equal_pos >= 0)
		ValidateBrackets(text + equal_pos, t_end, style + equal_pos);
	else
		ValidateBrackets(text, t_end, style);
}

int W_Editor::CheckSemicolon(const char *text, const char *t_end, int equal_pos, char *style)
{
	bool in_string = false;
	int pos = (equal_pos >= 0) ? (equal_pos+1) : 0; 

	for (; pos < (t_end - text); pos++)
	{
		if (text[pos] == '"')
		{
			in_string = !in_string;
			continue;
		}

		if (!in_string && text[pos] == ';')
			break;
	}

	if (pos >= (t_end - text))
		return -1;  // NOT FOUND

	// mark everything after the semicolon as error
	text  += pos;
	style += pos;

	text++, *style++ = 'K';

	while (text < t_end && isspace(*text))
		text++, *style++ = 'A';

	while (text < t_end)
		text++, *style++ = 'E';

	return pos;
}

bool W_Editor::CheckComma(const char *text, const char *t_end)
{
	for (t_end--; t_end > text; t_end--)
	{
		if (*t_end == ',')
			return true;

		if (! isspace(*t_end))
			break;
	}

	return false;  // not found
}

void W_Editor::ParseCommand(const char *text, const char *t_end, char *style,
	int equal_pos, char *context)
{
	if (text >= t_end)
		return;

	if (equal_pos < 0)
	{
		memset(style, 'V', t_end - text);
		return;
	}

	// @@@@

	memset(style, 'W', equal_pos+1);

	bool special = false;
	bool states  = false;

	if (strncmp(text, "SPECIAL", 7) == 0)
		special = true;
	else if (strncmp(text, "STATES", 6) == 0)
		states = true;

	text  += equal_pos+1;
	style += equal_pos+1;

	if (special)
	{
		*context = 'F';
		ParseFlagData(text, t_end, style, context);
	}
	else if (states)
	{
		*context = 'J';
		ParseStateData(text, t_end, style, context);
	}
	else
	{
		*context = 'K';
		ParseCommandData(text, t_end, style, context);
	}
	return;

}

void W_Editor::ParseCommandData(const char *text, const char *t_end, char *style,
	char *context)
{
#if 0 //!!!!
		if (*text == '\"')
		{
			int len = ParseString(text, t_end, style, false);
			text  += len;
			style += len;
			at_col0 = false;
		}
		else if (isdigit(*text) || (*text == '-' && text+1 < t_end && isdigit(text[1])))
		{
			int len = ParseNumber(text, t_end, style);
			text += len;
			style += len;
			at_col0 = false;
		}
		else if (isalpha(*text) || *text == '_' || *text == '#')
		{
			int len = ParseWord(text, t_end, style);
			text += len;
			style += len;
			at_col0 = false;
		}
		else
		{
			text++, *style++ = 'A';
		}
#endif
	while (text < t_end)
	{
		text++, *style++ = *context;
	}
}

void W_Editor::ParseStateData(const char *text, const char *t_end, char *style,
	char *context)
{
#if 0 //!!!!
		if (*text == '\"')
		{
			int len = ParseString(text, t_end, style, false);
			text  += len;
			style += len;
			at_col0 = false;
		}
		else if (isdigit(*text) || (*text == '-' && text+1 < t_end && isdigit(text[1])))
		{
			int len = ParseNumber(text, t_end, style);
			text += len;
			style += len;
			at_col0 = false;
		}
		else if (isalpha(*text) || *text == '_' || *text == '#')
		{
			int len = ParseWord(text, t_end, style);
			text += len;
			style += len;
			at_col0 = false;
		}
		else
		{
			text++, *style++ = 'A';
		}
#endif

	while (text < t_end)
	{
		text++, *style++ = *context;
	}
}

void W_Editor::ParseFlagData(const char *text, const char *t_end, char *style,
	char *context)
{
	while (text < t_end)
	{
		text++, *style++ = *context;
	}
}

//------------------------------------------------------------------------

void W_Editor::ValidateTag(const char *text, const char *t_end, char *style)
{
#if 0 //!!!!
		char buffer[84];
		int len = 0;

		while (len < 80 && text+len < t_end && isalnum(text[len]))
		{
			buffer[len] = text[len];
			len++;
		}

		buffer[len] = 0;

		if (len > 0)
		{
			bool known = false;

			text += len;

			keyword_box_c *ddf = config.Find(keyword_box_c::TP_Files, "ddf");
			if (ddf)
			{
				if (ddf->Find(buffer) != NULL)
					known = true;
			}

			memset(style, known ? 'T' : 'U', len);
#endif

	memset(style, 'T', t_end - text);
}

void W_Editor::ValidateItem(const char *text, const char *t_end, char *style)
{
	memset(style, 'I', t_end - text);
}

char ValidateCommand(const char *text, const char *t_end, char *style)
{
	// !!!!! FIXME XXX

	return 'K';
}

void W_Editor::ValidateBrackets(const char *text, const char *t_end, char *style)
{
	// NOTE: this routine is a bit special, since it runs after all
    //       other syntax highlighting is done.

	const char *t_orig = text;

	int brackets_open = 0;

	for (; text < t_end; text++)
	{
		if (style[text - t_orig] == 'S')
			continue;

		if (*text == '(')
		{
			brackets_open++;
		}
		else if (*text == ')')
		{
			if (brackets_open == 1)
			{
				// finish the last group, start new group
				style += (text - t_orig) + 1;
				t_orig = text + 1;
				brackets_open = 0;
			}
			else if (brackets_open == 0)
				style[text - t_orig] = 'E';
			else
				brackets_open--;
		}
	}

	if (brackets_open > 0)
	{
		for (text = t_orig; text < t_end; text++)
		{
			if (style[text - t_orig] != 'S' && *text == '(')
			{
				style[text - t_orig] = 'E';
				break;
			}
		}

		SYS_ASSERT(text < t_end);
	}

	// check for invalid /*..*/ comments
	for (const char *T = t_orig; T+1 < t_end; T++)
	{
		if (style[T - t_orig] == 'S')
			continue;

		if ((T[0] == '/' && T[1] == '*') ||
		    (T[0] == '*' && T[1] == '/'))
		{
			style[T - t_orig] = style[T+1 - t_orig] = 'E';
		}
	}
}

