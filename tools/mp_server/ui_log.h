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

#ifndef __UI_LOG_H__
#define __UI_LOG_H__

#include "lib_list.h"

class UI_LogBox : public Fl_Multi_Browser
{
public:
	UI_LogBox(int x, int y, int w, int h);
	virtual ~UI_LogBox();

private:
	NLmutex save_lock;

	list_c save_lines;

public:
	void Update();

	// adds the message to the text box.  The message may contain
	// newlines ('\n').  The message doesn't need a trailing `\n', i.e.
	// it is implied.
	//
	void AddMsg(const char *msg, Fl_Color col = FL_BLACK, 
			bool bold = false);
private:
	void DoAddMsg(const char *msg, Fl_Color col = FL_BLACK, 
			bool bold = false);
public:
	// add a horizontal dividing bar
	void AddHorizBar();

	// routine to clear the text box
	void ClearLog();

	// routine to save the log to a file.  Will overwrite the file if
	// already exists.  Returns TRUE if successful or FALSE on error.
	//
	bool SaveLog(const char *filename);
};

void LogInit(void);
void LogPrintf(int level, const char *str, ...);

#endif /* __UI_LOG_H__ */
