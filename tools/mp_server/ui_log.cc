//------------------------------------------------------------------------
//  Log storage and display
//------------------------------------------------------------------------
//
//  Edge MultiPlayer Server (C) 2005  Andrew Apted
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

#include "includes.h"
#include "ui_log.h"
#include "ui_window.h"

//
// LogBox Constructor
//
UI_LogBox::UI_LogBox(int x, int y, int w, int h) :
    Fl_Multi_Browser(x, y, w, h)
{
	// cancel the automatic `begin' in Fl_Group constructor
	// (it's an ancestor of Fl_Browser).
	end();
 
	textfont(FL_COURIER);
	textsize(14);

#ifdef MACOSX
	// the resize box in the lower right corner is a pain, it ends up
	// covering the text box scroll button.  Luckily when both scrollbars
	// are active, FLTK leaves a square space in that corner and it
	// fits in nicely.
	has_scrollbar(BOTH_ALWAYS);
#endif

	nlMutexInit(&save_lock);
}


//
// LogBox Destructor
//
UI_LogBox::~UI_LogBox()
{
	nlMutexDestroy(&save_lock);
}

//------------------------------------------------------------------------

class log_mem_c : public node_c
{
friend class UI_LogBox;

private:
	// FIXME: timestamp

	const char *line;
	Fl_Color col;
	bool bold;

public:
	log_mem_c(const char *_line, Fl_Color _col, bool _bold) :
		node_c(), col(_col), bold(_bold)
	{
		line = strdup(_line);
	}

	~log_mem_c()
	{
		free((void*) line);
		line = NULL;
	}
};

void UI_LogBox::AddMsg(const char *msg, Fl_Color col, // = FL_BLACK,
    bool bold) // = FALSE)
{
	nlMutexLock(&save_lock);

	// FIXME: have a limit on size

	log_mem_c *mem = new log_mem_c(msg, col, bold);

	save_lines.push_back(mem);

	nlMutexUnlock(&save_lock);
}

void UI_LogBox::DoAddMsg(const char *msg, Fl_Color col, // = FL_BLACK,
    bool bold) // = FALSE)
{
	const char *r;

	char buffer[2048];
	int b_idx;

	// setup formatting string
	buffer[0] = 0;

	if (col != FL_BLACK)
		sprintf(buffer, "@C%d", col);

	if (bold)
		strcat(buffer, "@b");

	strcat(buffer, "@.");
	b_idx = strlen(buffer);

	while ((r = strchr(msg, '\n')) != NULL)
	{
		strncpy(buffer+b_idx, msg, r - msg);
		buffer[b_idx + r - msg] = 0;

		if (r - msg == 0)
		{
			// workaround for FLTK bug
			strcpy(buffer+b_idx, " ");
		}

		add(buffer);
		msg = r+1;
	}

	if (strlen(msg) > 0)
	{
		strcpy(buffer+b_idx, msg);
		add(buffer);
	}

	// move browser to last line
	if (size() > 0)
		bottomline(size());
}

void UI_LogBox::Update()
{
	nlMutexLock(&save_lock);

	for (;;)
	{
		log_mem_c *mem = (log_mem_c *) save_lines.pop_front();

		if (! mem)
			break;

		SYS_NULL_CHECK(mem->line);

		DoAddMsg(mem->line, mem->col, mem->bold);
	}

	nlMutexUnlock(&save_lock);
}


void UI_LogBox::AddHorizBar()  // FIXME
{
#if 0
	add(" ");
	add(" ");
	add("@-");
	add(" ");
	add(" ");

	// move browser to last line
	if (size() > 0)
		bottomline(size());
#endif
}


void UI_LogBox::ClearLog()
{
	clear();
}


bool UI_LogBox::SaveLog(const char *filename)
{
	FILE *fp = fopen(filename, "w");

	if (! fp)
		return false;

	for (int y=1; y <= size(); y++)
	{
		const char *L_txt = text(y);

		if (! L_txt)
		{
			fprintf(fp, "\n");
			continue;
		}

		if (L_txt[0] == '@' && L_txt[1] == '-')
		{
			fprintf(fp, "--------------------------------");
			fprintf(fp, "--------------------------------\n");
			continue;
		}

		// remove any `@' formatting info

		while (*L_txt == '@')
		{
			L_txt++;

			if (*L_txt == 0)
				break;

			char fmt_ch = *L_txt++;

			if (fmt_ch == '.')
				break;

			// uppercase formatting chars (e.g. @C) have an int argument
			if (isupper(fmt_ch))
			{
				while (isdigit(*L_txt))
					L_txt++;
			}
		}

		fprintf(fp, "%s\n", L_txt);
	}

	fclose(fp);

	return true;
}


//------------------------------------------------------------------------

//
// LogInit
//
void LogInit(void)
{
}

//
// LogPrintf
//
// Levels are:  0 = informational message.
//              1 = not serious problem.
//              2 = serious problem.
//
void LogPrintf(int level, const char *str, ...)
{
	if (! main_win)
		return;

	char buffer[2048];

	va_list args;

	va_start(args, str);
	vsprintf(buffer, str, args);
	va_end(args);

	main_win->log_box->AddMsg(buffer, (level == 0) ? FL_BLUE :
		(level == 1) ? FL_BLACK : FL_RED, (level != 1));
}

