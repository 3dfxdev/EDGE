#if 0
//----------------------------------------------------------------------------
//  EDGE WIN32 IWAD Selection Dialogue Box
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2009  Marisa Heit.
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

#include "../i_defs.h"
#include "../i_sdlinc.h"

#include "w32_sysinc.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define USE_WINDOWS_DWORD
//#include "resource.h"
//#include "version.h"
//#include "wl_iwad.h"
#include "w_wad.h"
//#include "zstring.h"

#define FString std::string

#define Window ConWindow
extern HINSTANCE g_hInst;
extern HWND ConWindow;
extern bool queryiwad;

static WadStuff *WadList;
static int NumWads;
static int DefaultWad;

//==========================================================================
//
// SetQueryIWAD
//
// The user had the "Don't ask again" box checked when they closed the
// IWAD selection dialog.
//
//==========================================================================

static void SetQueryIWad(HWND dialog)
{
	HWND checkbox = GetDlgItem(dialog, IDC_DONTASKIWAD);
	int state = (int)SendMessage(checkbox, BM_GETCHECK, 0, 0);
	bool query = (state != BST_CHECKED);

	if (!query && queryiwad)
	{
		MessageBox(dialog,
			"You have chosen not to show this dialog box in the future.\n"
			"If you wish to see it again, hold down SHIFT while starting " GAMENAME ".",
			"Don't ask me this again",
			MB_OK | MB_ICONINFORMATION);
	}

	queryiwad = query;
}

//==========================================================================
//
// IWADBoxCallback
//
// Dialog proc for the IWAD selector.
//
//==========================================================================

BOOL CALLBACK IWADBoxCallback(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND ctrl;
	int i;

	switch (message)
	{
	case WM_INITDIALOG:
		// Add our program name to the window title
	{
		TCHAR label[256];
		FString newlabel;

		GetWindowText(hDlg, label, countof(label));
		newlabel.Format("%s: %s", GetGameCaption(), label);
		SetWindowText(hDlg, newlabel.GetChars());
	}
	// Populate the list with all the IWADs found
	ctrl = GetDlgItem(hDlg, IDC_IWADLIST);
	for (i = 0; i < NumWads; i++)
	{
		FString work;
		FString filepart;
		if (WadList[i].Path.Size() == 1)
		{
			filepart = strrchr(WadList[i].Path[0], '/');
			if (filepart.IsEmpty())
				filepart = WadList[i].Path[0];
			else
				filepart = filepart.Mid(1);
		}
		else
			filepart.Format("*.%s", WadList[i].Extension.GetChars());
		work.Format("%s (%s)", WadList[i].Name.GetChars(), filepart.GetChars());
		SendMessage(ctrl, LB_ADDSTRING, 0, (LPARAM)work.GetChars());
		SendMessage(ctrl, LB_SETITEMDATA, i, (LPARAM)i);
	}
	SendMessage(ctrl, LB_SETCURSEL, DefaultWad, 0);
	SetFocus(ctrl);
	// Set the state of the "Don't ask me again" checkbox
	ctrl = GetDlgItem(hDlg, IDC_DONTASKIWAD);
	SendMessage(ctrl, BM_SETCHECK, queryiwad ? BST_UNCHECKED : BST_CHECKED, 0);
	// Make sure the dialog is in front. If SHIFT was pressed to force it visible,
	// then the other window will normally be on top.
	SetForegroundWindow(hDlg);
	break;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, -1);
		}
		else if (LOWORD(wParam) == IDOK ||
			(LOWORD(wParam) == IDC_IWADLIST && HIWORD(wParam) == LBN_DBLCLK))
		{
			SetQueryIWad(hDlg);
			ctrl = GetDlgItem(hDlg, IDC_IWADLIST);
			EndDialog(hDlg, SendMessage(ctrl, LB_GETCURSEL, 0, 0));
		}
		break;
	}
	return FALSE;
}

//==========================================================================
//
// I_PickIWad
//
// Open a dialog to pick the IWAD, if there is more than one found.
//
//==========================================================================

// CVar in ZDoom
#define queryiwad_key "shift"
int I_PickIWad(WadStuff *wads, int numwads, bool showwin, int defaultiwad)
{
	int vkey;

	if (stricmp(queryiwad_key, "shift") == 0)
	{
		vkey = VK_SHIFT;
	}
	else if (stricmp(queryiwad_key, "control") == 0 || stricmp(queryiwad_key, "ctrl") == 0)
	{
		vkey = VK_CONTROL;
	}
	else
	{
		vkey = 0;
	}
	if (showwin || (vkey != 0 && GetAsyncKeyState(vkey)))
	{
		WadList = wads;
		NumWads = numwads;
		DefaultWad = defaultiwad;

		return (int)DialogBox(g_hInst, MAKEINTRESOURCE(IDD_IWADDIALOG),
			(HWND)Window, (DLGPROC)IWADBoxCallback);
	}
	return defaultiwad;
}

#endif // 0
