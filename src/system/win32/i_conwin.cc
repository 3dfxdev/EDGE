//----------------------------------------------------------------------------
//  EDGE Win32 Output Window Code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2018  The EDGE Team.
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

#include "..\i_defs.h"
#include "../win32/w32_sysinc.h"
#include "dstrings.h"

HWND conwinhandle; // -ACB- 2000/07/04 - Made extern in EPI Local Header

static HFONT oemfont;
static LONG oemwidth, oemheight;
static int conwidth, conheight;
static int inited = 0;

#define CONSOLELINES 25
#define CONSOLELENGTH 80
static char textchar[CONSOLELINES*CONSOLELENGTH];
static int colnum = 0;

#define MSGBUFSIZE 1024

//
// MoveConsoleUp
//
// This moves the console page up.
//
void MoveConsoleUp(void)
{
	char *src;
	RECT conrect;
	HDC conDC;
	HBRUSH brush;
	int len;
	int line;

	// Move the console up in the mem map...
	src = &textchar[CONSOLELENGTH];
	memmove(textchar, src, sizeof(char)*(CONSOLELENGTH*(CONSOLELINES-1)));
	memset(&textchar[(CONSOLELINES-1)*CONSOLELENGTH], '\0', sizeof(char)*CONSOLELENGTH);

	conDC = GetDC(conwinhandle);

	// Clear the console screen
	GetClientRect(conwinhandle, &conrect);       // Get Client Rectangle
	brush = CreateSolidBrush(RGB(64,64,96));     // Get Brush
	FillRect(conDC, &conrect, brush);            // Fill with brush colour
	DeleteObject(brush);                         // Delete Brush

	line=0;
	while (line<(CONSOLELINES-1))
	{
		len = (int)strlen(&textchar[line*CONSOLELENGTH]);

		if (len)
			TextOut(conDC, 0, oemheight*line, &textchar[line*CONSOLELENGTH], len);

		line++;
	}

	ReleaseDC(conwinhandle, conDC);
	return;
}

//
// UpdateConsole
//
void UpdateConsole(void)
{
	PAINTSTRUCT paint;
	HDC dc;
	int line;
	int len;

	dc = BeginPaint (conwinhandle, &paint);
	if (dc)
	{
		line=0;
		while (line<CONSOLELINES)
		{
			len = (int)strlen(&textchar[line*CONSOLELENGTH]);

			if (len)
				TextOut(dc, 0, oemheight*line, &textchar[line*CONSOLELENGTH], len);

			line++;
		}

		EndPaint (conwinhandle, &paint);
	}

	return;
}


//
// ConWindowProc
//
// Console Window Message Handler
//
long FAR PASCAL ConWindowProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch (iMsg)
	{
    	case WM_CLOSE:
    		return 1L;
    
    	case WM_PAINT:
    		UpdateConsole();
    		return 0L;
    
    	default:
    		break;
	}

	return (long)(DefWindowProc(hwnd,iMsg,wParam,lParam));
}

//
// I_StartWinConsole
//
// Initialise the console
//
void I_StartWinConsole(void)
{
	HINSTANCE wininstance;
	HDC conDC;
	WNDCLASS wndclass;
	TEXTMETRIC metrics;
	RECT cRect;
	int width,height;
	int scr_width,scr_height;

	if (inited)
		return;

	wininstance = maininstance;

	wndclass.style         = CS_OWNDC;
	wndclass.lpfnWndProc   = (WNDPROC)ConWindowProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 0;
	wndclass.hInstance     = wininstance;
	wndclass.hIcon         = LoadIcon (wininstance, IDI_WINLOGO);
	wndclass.hCursor       = LoadCursor (NULL,IDC_ARROW);
	wndclass.hbrBackground = CreateSolidBrush(RGB(64,64,96));
	wndclass.lpszMenuName  = OUTPUTTITLE;
	wndclass.lpszClassName = OUTPUTTITLE;

	if (!RegisterClass(&wndclass))
		return;

	width=100;
	height=100;

	conwinhandle = CreateWindow(OUTPUTTITLE,
		OUTPUTTITLE,
		WS_POPUP | WS_CAPTION, 
		0,
		0,
		width,
		height,
		NULL,
		NULL,
		wininstance,
		NULL);

	conDC = GetDC(conwinhandle);

	memset(textchar, '\0', CONSOLELINES*CONSOLELENGTH*sizeof(char));

	oemfont = (HFONT)GetStockObject(SYSTEM_FIXED_FONT);
	SelectObject(conDC, oemfont);
	GetTextMetrics(conDC, &metrics);
	oemwidth = metrics.tmAveCharWidth;
	oemheight = metrics.tmHeight;
	GetClientRect(conwinhandle, &cRect);
	width += ((oemwidth * CONSOLELENGTH) - cRect.right);
	height += ((oemheight * CONSOLELINES) - cRect.bottom);

	scr_width = GetSystemMetrics(SM_CXFULLSCREEN);
	scr_height = GetSystemMetrics(SM_CYFULLSCREEN);

	MoveWindow(conwinhandle, (scr_width-width)/2, (scr_height-height)/2, width, height, TRUE);
	GetClientRect(conwinhandle, &cRect);

	conwidth = cRect.right;
	conheight = cRect.bottom;

	SetBkColor(conDC, RGB(64,64,96));
	SetTextColor(conDC, RGB(255,255,255));
	SetBkMode(conDC, OPAQUE);
	ReleaseDC(conwinhandle,conDC);

	ShowWindow(conwinhandle, SW_SHOW);
	UpdateWindow(conwinhandle);
	BringWindowToTop(conwinhandle);

	memset(textchar, 0, sizeof(char)*CONSOLELENGTH*CONSOLELINES);

	inited = 1;
}

//
// I_SetConsoleTitle
//
// Sets the console title
//
void I_SetConsoleTitle(const char *title)
{
	if (!inited)
		return;

	if (title)
		SetWindowText(conwinhandle, title);
	else
		SetWindowText(conwinhandle, OUTPUTTITLE);
}

//
// I_WinConPrintf
//
// The good old-fashioned printf-style function
//
void I_WinConPrintf(const char *message, ...)
{
	va_list argptr;
	char *string;
	char *lastrow;
	char printbuf[MSGBUFSIZE];
	char testchar;
	HDC conDC;
	RECT conrect;
	HBRUSH brush;
	LONG xpos;

	if (!inited)
		return;

	// clear the buffer
	memset (printbuf, 0, MSGBUFSIZE);

	// Lets ignore a message that could expand outside the buffer
	if (strlen(message) > (MSGBUFSIZE-64))
		return;

	va_start(argptr, message);

	// Print the message into a text string
	vsprintf(printbuf, message, argptr);

	// finish the arg ptr stuff...
	va_end(argptr);

	// Clean up \n\r combinations
	string = printbuf;
	while (*string)
	{
		if (*string == '\n')
		{
			memmove(string + 2, string + 1, strlen(string));
			string[1] = '\r';
			string++;
		}
		string++;
	}

	// Count the number of new lines required
	string = printbuf;
	lastrow = &textchar[CONSOLELENGTH*(CONSOLELINES-1)];
	while (*string)
	{
		testchar = *string;

		if (testchar == '\n')
		{
			colnum=0;
			MoveConsoleUp();
			string++;
		}
		else if(testchar >= 32)
		{
			lastrow[colnum] = *string;

			colnum++;

			if (colnum >= CONSOLELENGTH)
			{
				colnum=0;
				MoveConsoleUp();
			}
		}

		string++;
	}
	lastrow[colnum] = '\0';

	conDC = GetDC(conwinhandle);              // Paint DC for console window
	GetClientRect(conwinhandle, &conrect);    // Client Rectangle Needed
	brush = CreateSolidBrush(RGB(64,64,96));  // Brush Colour

	conrect.top = conrect.bottom - oemheight; // Setup rect for bottom line
	FillRect(conDC, &conrect, brush);         // Clear Bottom Line

	xpos = conrect.left;
	string = lastrow;
	while(*string)
	{
		TextOut(conDC, xpos, conrect.top, string, 1);
		string++;
		xpos += oemwidth;
	}

	DeleteObject(brush);
	ReleaseDC(conwinhandle, conDC);
}

//
// I_ShutdownWinConsole
//
// Closes the WIN32 output window
//
void I_ShutdownWinConsole(void)
{
	PostMessage(conwinhandle, WM_CLOSE, 0, 0);
	inited = false;
}
