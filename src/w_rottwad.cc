//----------------------------------------------------------------------------
//  EDGE2 Packed Data Support Code
//  Rise of the Triad IWAD handler
//----------------------------------------------------------------------------
//
//  Copyright © 1999-2017  The EDGE2 Team.
//  Copyright © 2008, 2017 Birger N. Andreasen 
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
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//
//  Portions based on the Rise of the Triad source code, released by Apogee
//  Software under the following copyright:
//
//	  Copyright (C) 1994-1995 Apogee Software, Ltd.
//   
//----------------------------------------------------------------------------
//
// This file contains various levels of support for using sprites and
// flats directly from a PWAD as well as some minor optimisations for
// patches. Because there are some PWADs that do arcane things with
// sprites, it is possible that this feature may not always work (at
// least, not until I become aware of them and support them) and so
// this feature can be turned off from the command line if necessary.
//
// -MH- 1998/03/04
//
// -CA- 2017/28/10
//		 TODO: 
//       All of the HWND and Windows specific stuff needs to be replaced by platform-agnostic
//       sprintf instead (the universal EDGE2 console window). Also, this needs to be all automated
//       and the created IWAD needs to be stored and cached inside of the /cache folder
//

#include "system/i_defs.h"

#include <limits.h>

#include <list>

#include "../epi/bytearray.h"
#include "../epi/endianess.h"
#include "../epi/file.h"
#include "../epi/file_sub.h"
#include "../epi/filesystem.h"
#include "../epi/math_md5.h"
#include "../epi/path.h"
#include "../epi/rawdef_pcx.h" //PCX defines via EPI
#include "../epi/str_format.h"
#include "../epi/utility.h"

#include "../ddf/main.h"
#include "../ddf/anim.h"
#include "../ddf/colormap.h"
#include "../ddf/font.h"
#include "../ddf/image.h"
#include "../ddf/style.h"
#include "../ddf/switch.h"

#include "dm_data.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "dm_structs.h"
#include "dstrings.h"
#include "e_main.h"
#include "e_search.h"
#include "l_deh.h"
#include "l_glbsp.h"
#include "m_misc.h"
#include "r_colormap.cc"
#include "r_doomtex.h"
#include "r_image.h"
#include "rad_trig.h"
#include "vm_coal.h"
#include "games/wolf3d/wlf_local.h"
#include "games/wolf3d/wlf_rawdef.h"
#include "w_wad.h"
#include "z_zone.h"

#include "games/rott/rott_local.h" // ResourceFile from WinROTT GL

#ifdef HAVE_PHYSFS
#include <physfs.h>
#endif

extern byte g_FolderPath[260];
extern byte g_FileType[260];
extern HINSTANCE	hInst;



int GFX_wallstart=0,    GFX_wallstop=0;
int GFX_floorstart=0,   GFX_floorstop=0;
int GFX_shapestart=0,   GFX_shapestop=0;
int GFX_gunsstart=0,    GFX_gunsstop=0;
int GFX_doorstart=0,    GFX_doorstop=0;
int GFX_sidestart=0,    GFX_sidestop=0;
int GFX_elevatorstart=0,GFX_elevatorstop=0;
int GFX_exitstart=0,    GFX_exitstop=0;
int GFX_maskedstart=0,  GFX_maskedstop=0;
int GFX_mishstart=0,	GFX_mishstop=0;

int GFX_diversestart=0,	GFX_diversestop=0;
int GFX_pic_tstart=0,	GFX_pic_tstop=0;

int GFX_T_single[100];
int GFX_T_LBMsingle[10];
int GFX_T_Picsingle[10];

int GFX_sky_start=0,	GFX_sky_stop=0;

int GFX_norm_1_start=0,	GFX_norm_1_stop=0;
int GFX_norm_2_start=0,	GFX_norm_2_stop=0;
int GFX_pic_tstart2=0,	GFX_pic_tstop2=0;

//menu stuff
int GFX_m_infostart=0,	GFX_m_infostop=0;
int GFX_m_q_yesstart=0,	GFX_m_q_nostop=0;
int GFX_m_newstart=0,	GFX_m_newstop=0;

int GFX_pic_tstart1=0,	GFX_pic_tstop1=0;
int GFX_pcstart=0,	GFX_pcstop=0;
int GFX_adstart=0,	GFX_adstop=0;
int GFX_animstart=0,	GFX_animstop=0;


int GFX_TEST=0;
int GFX_TEST1=0;

int GFX_lmb_1=0;
int GFX_lmb_2=0;
int GFX_lmb_3=0;
int GFX_lmb_4=0;
int GFX_lmb_5=0;
int GFX_specstart=0,	GFX_specstop=0;

int GFX_maskedBotLimit=0,  GFX_maskedTopLimit=0;
int GFX_maskedExclude=0;

int GFX_abovewallstart=0,GFX_abovewallstop=0;
int GFX_abovemaskwallstart=0,GFX_abovemaskwallstop=0;
int GFX_himaskstart=0,  GFX_himaskstop=0;
int GFX_titlestart=0,   GFX_titlestop=0;
int GFX_ap_world=0,     GFX_ap_titl=0;
int GFX_trilogo = 0;
int GFX_plane = 0;


void AddTxtToListbox(char *txt);
bool WriteLumpToWadTable(int filehandle, int lump,HWND hWndSizeLB,HWND hWndFilePosLB,HWND hWndNameLB);
int  ExtractWadLumpTable(int filehandle,HWND hWndSizeLB,HWND hWndFilePosLB,HWND hWndNameLB);
bool BreakRottWad(HWND hwnd);
bool MakeRottWad(HWND hwnd);
void CheckforSTRT (int lumpnb, char*name);
void HandleLumpBMP(	long filepos,long filesize,char*dirpath,char *tmpbuf1,int w, int h,FILE* txtstream,FILE* instream);
void HandleLumpShapeBMP(long filepos,long filesize,char*dirpath,char *tmpbuf1,int w, int h,FILE* txtstream,FILE* instream);
void HandleLumpPic_tBMP(int lump,long filepos,long filesize,char*dirpath,char *tmpbuf1,int w, int h,FILE* txtstream,FILE* instream);
void HandleLumpIntro(int lump,long filepos,long filesize,char*dirpath,char *tmpbuf1,int w, int h,FILE* txtstream,FILE* instream);
void HandleLumpLBM(int lump,long filepos,long filesize,char*dirpath,char *tmpbuf1,int w, int h,FILE* txtstream,FILE* instream);
void HandleLumpShapeBMPSpecial(long filepos,long filesize,char*dirpath,char *tmpbuf1,int w, int h,FILE* txtstream,FILE* instream);


void Break_listinfo(char*txt1);
void Assem_listinfo(char*txt1);


extern HWND		hMainWnd;


HWND  hWndNameLB,hWndSizeLB,hWndFilePosLB,hWndCheckLB,hWndNewSizeLB,hWndLBBreak,hWndLBAssem;
char szFileName[256*2];
long oldfilepos = 12;

extern byte *gMAP_screenmem;
	extern UINT APIENTRY OFNHookProc(HWND hDlg,
                                 UINT uMsgId,
                                 WPARAM wParam,
                                 LPARAM lParam);
extern LRESULT CALLBACK EXP_Proc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);


//PCX header defined in epi/rawdef_pcx.h!
#define GAP_SIZE  (128 - sizeof (epi::raw_pcx_header_t))

typedef struct
{
	char	identification[4];
	long	numlumps;
	long	infotableofs;
} 
wadinfo_t;


typedef struct
{
	long		filepos;
	long		size;
	char		name[8];

#ifdef HAVE_PHYSFS
	// pathname for PHYSFS file - wasteful, but no biggy on a PC
	char path[256];
#endif
} 
lumpinfo_t;

lumpinfo_t glumpinfo;

LRESULT CALLBACK WTO_AssembleWadProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WTO_BreakWadProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WTO_BreakWadProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{

	switch (message)
	{
		case WM_INITDIALOG:
			{
				int i;
				char tmp[24];
				//ShutdownClientControls();

				hWndLBBreak = GetDlgItem(hDlg,IDC_LIST_INFO);

				// create a ListBox to store dir string 
				hWndNameLB = GetDlgItem(hDlg,IDC_LIST1);
				hWndSizeLB = GetDlgItem(hDlg,IDC_LIST2);
				hWndFilePosLB = GetDlgItem(hDlg,IDC_LIST3);
				hWndCheckLB = GetDlgItem(hDlg,IDC_LIST4);
				SetWindowText(GetDlgItem(hDlg,IDC_IWADRESPATH_EDIT),"C:\\Temp\\Wadres\\");

			}
			//SetWindowPos(hDlg,HWND_TOPMOST,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE);

			//return TRUE;
		case WM_ACTIVATE:
		case WM_MOVE:
		case WM_SIZE:
			 MAP_UpdateMAPWnd(gMAP_screenmem);
			break;
		case WM_COMMAND:


			//EDGE: g_FileType is incompatible with byte??
			if (LOWORD(wParam) == IDC_BUTT_BROWSE1)
			{ 
				char szFile[260]="";
				lstrcpy(g_FileType,"*.wad"); 					
				DialogBox(hInst, (LPCTSTR)IDD_EXPLORER, hDlg, (DLGPROC)EXP_Proc);
				if (g_FolderPath[0]!=0){
				//if (GetCommFilename(szFile)==1)
					SetWindowText(GetDlgItem(hDlg,IDC_IWADPATH_EDIT),g_FolderPath);
				
				Break_listinfo (g_FolderPath);
				}
				break;

/*
				OPENFILENAME ofn;       // common dialog box structure
				char szFile[260]="";       // buffer for file name

				// Initialize OPENFILENAME
				ZeroMemory(&ofn, sizeof(OPENFILENAME));
				ofn.lStructSize = sizeof(OPENFILENAME);
				ofn.hwndOwner = hDlg;
				ofn.lpstrFile = szFile;
				ofn.nMaxFile = sizeof(szFile);
				ofn.lpstrFilter = "*.wad\0*.wad\0All\0*.*\0";
				ofn.nFilterIndex = 1;
				ofn.lpstrFileTitle = "Open IWAD for breaking to RLF files";
				ofn.nMaxFileTitle = 0;
				ofn.lpstrInitialDir = NULL;
				ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST|OFN_ENABLEHOOK|OFN_EXPLORER |OFN_ENABLESIZING;
				ofn.lpfnHook = OFNHookProc; 
				// Display the Open dialog box. 
				if (GetOpenFileName(&ofn)==TRUE) {
					SetWindowText(GetDlgItem(hDlg,IDC_IWADPATH_EDIT),ofn.lpstrFile);
				}
				break;*/
			}


			if (LOWORD(wParam) == IDC_BUTT_BROWSE2 )
			{ 
				BROWSEINFO		bi;         
				LPITEMIDLIST	pidl; 
				char			gPATH[256]="";

				OleInitialize(NULL);

				ZeroMemory(&bi,sizeof(bi));  
				bi.hwndOwner = hDlg;
			//    bi.pszDisplayName = s; 
				bi.lpszTitle = "Select folderpath to store IWAD resource files";  
				bi.pidlRoot = 0;
				bi.ulFlags = BIF_EDITBOX | BIF_RETURNONLYFSDIRS ;//BIF_STATUSTEXT;
				pidl = SHBrowseForFolder(&bi);            
				if (pidl) {
				if (SHGetPathFromIDList(pidl,gPATH)) 
				{
						if (gPATH[0]!=0){
							lstrcat (gPATH,"\\");
							SetWindowText(GetDlgItem(hDlg,IDC_IWADRESPATH_EDIT),gPATH);
							Break_listinfo (gPATH);
						}
				}
				} 
				break;
			}


			if (LOWORD(wParam) == IDOK )
			{   
				int is;

				BreakRottWad(hDlg);
				// setup initial button state


			//	EndDialog(hDlg, LOWORD(wParam));
			//	StartupClientControls();
				return TRUE;
			}

			if (LOWORD(wParam) == IDCANCEL )
			{
				EndDialog(hDlg, LOWORD(wParam));
				//StartupClientControls();
				return TRUE;
			}


	}
    return FALSE;
}



LRESULT CALLBACK WTO_AssembleWadProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	switch (message)
	{
		case WM_INITDIALOG:
			{ 
				int i;
				char tmp[24];
				//ShutdownClientControls();
				hWndLBAssem = GetDlgItem(hDlg,IDC_LIST_INFO2);
				// create a ListBox to store dir string 
				hWndNameLB = GetDlgItem(hDlg,IDC_A_LIST1);
				hWndSizeLB = GetDlgItem(hDlg,IDC_A_LIST2);
				hWndFilePosLB = GetDlgItem(hDlg,IDC_A_LIST3);
				hWndCheckLB = GetDlgItem(hDlg,IDC_A_LIST4);
				hWndNewSizeLB = GetDlgItem(hDlg,IDC_A_LIST5);;
				SetWindowText(GetDlgItem(hDlg,IDC_A_IWADPATH_EDIT),"C:\\Temp\\Wadres\\Darkwar.lst");
				SetWindowText(GetDlgItem(hDlg,IDC_A_IWADRESPATH_EDIT),"C:\\Rott3Deditor_cleanup\\Darkwar.wad");
			}
			//SetWindowPos(hDlg,HWND_TOPMOST,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE);

			return TRUE;
	/*	case WM_ACTIVATE:
		case WM_MOVE:
		case WM_SIZE:
			 MAP_UpdateMAPWnd(gMAP_screenmem);
			break;*/

		case WM_COMMAND:
			wmId    = LOWORD(wParam); 
			wmEvent = HIWORD(wParam); 
			// Parse the menu selections:MAP01.wad
			switch (wmId) 
			{
				case IDC_A_BUTT_BROWSE1:
					{ 
						char szFile[260]="";
						lstrcpy(g_FileType,"*.lst"); 					
						DialogBox(hInst, (LPCTSTR)IDD_EXPLORER, hDlg, (DLGPROC)EXP_Proc);
						//if (GetCommFilename(szFile)==1)
						if (g_FolderPath[0]!=0){
							SetWindowText(GetDlgItem(hDlg,IDC_A_IWADPATH_EDIT),g_FolderPath);
							Assem_listinfo (g_FolderPath);
						}
						break;
					}
				case IDC_A_BUTT_BROWSE2:
					{ 
						char szFile[260]="";
						lstrcpy(g_FileType,"*.wad"); 					
						DialogBox(hInst, (LPCTSTR)IDD_EXPLORER, hDlg, (DLGPROC)EXP_Proc);
						//if (GetCommFilename(szFile)==1)
						if (g_FolderPath[0]!=0){
							Assem_listinfo (g_FolderPath);
							SetWindowText(GetDlgItem(hDlg,IDC_A_IWADRESPATH_EDIT),g_FolderPath);
						}
						break;
					}
				case IDOK:
					{ 
						int is;
						MakeRottWad(hDlg);
					//	EndDialog(hDlg, LOWORD(wParam));
					//	StartupClientControls();
						return TRUE;
					}
				case IDCANCEL:
					{ 
						EndDialog(hDlg, LOWORD(wParam));
						//StartupClientControls();
						return TRUE;
					}
			}
			break;


	}
    return FALSE;
}





bool BreakRottWad(HWND hwnd)
{

	wadinfo_t *wadinfo;
	lumpinfo_t *lumpinfo;
	char path_buffer[_MAX_PATH*2];
    char drive[_MAX_DRIVE];
    char dir[_MAX_DIR+5];
    char fname[_MAX_FNAME];
    char ext[_MAX_EXT];
    FILE	*instream,*outstream,*txtstream,*wadinfostream;  
	BYTE *ptr;
	char lumpfilename[260];
	char dirpath[512];
	char we[64];
	char tmpbuf1[512], tmpbuf2[512];
    unsigned int i,x,lump,count=0,numlumps;
    u8_t c;
	long filepos,filesize;

/*
	// create a ListBox to store dir string 
    hWndNameLB = CreateWindowEx(0L,"LISTBOX", "", WS_CHILD | WS_BORDER,-6000,-6000, 700, 200,hwnd,(HMENU)NULL,hInst,NULL);
    hWndSizeLB = CreateWindowEx(0L,"LISTBOX", "", WS_CHILD | WS_BORDER,-6000,-6000, 700, 200,hwnd,(HMENU)NULL,hInst,NULL);
	hWndFilePosLB = CreateWindowEx(0L,"LISTBOX", "", WS_CHILD | WS_BORDER,-6000,-6000, 700, 200,hwnd,(HMENU)NULL,hInst,NULL);
	hWndCheckLB = CreateWindowEx(0L,"LISTBOX", "", WS_CHILD | WS_BORDER,-6000,-6000, 700, 200,hwnd,(HMENU)NULL,hInst,NULL);
*/		
	SendMessage(hWndNameLB,LB_RESETCONTENT,(WPARAM)0, (LPARAM) 0);
	SendMessage(hWndSizeLB,LB_RESETCONTENT,(WPARAM)0, (LPARAM) 0);
	SendMessage(hWndFilePosLB,LB_RESETCONTENT,(WPARAM)0, (LPARAM) 0);
	SendMessage(hWndCheckLB,LB_RESETCONTENT,(WPARAM)0, (LPARAM) 0);

	SendMessage(hWndLBBreak,LB_RESETCONTENT,(WPARAM)0, (LPARAM) 0);


	GetWindowText(GetDlgItem(hwnd,IDC_IWADPATH_EDIT),szFileName,sizeof(szFileName));
	if (access (szFileName, 0) != 0){ //does sfxfile exists
		sprintf(tmpbuf1,"File '%s' does not exist",szFileName);
		Break_listinfo (tmpbuf1);
		return 0;
	} 


	sprintf(tmpbuf1,"Splitting '%s'",szFileName);
	Break_listinfo (tmpbuf1);

	if( (instream  = fopen( szFileName, "r+b" )) == NULL ) 
	{
		Break_listinfo ("Opening of ROTT WAD file failed!");
		return FALSE;
	}	
	
	//get the name to save to
	GetWindowText(GetDlgItem(hwnd,IDC_IWADRESPATH_EDIT),path_buffer,sizeof(path_buffer));
	
	
	//get the name to save to
	_splitpath( szFileName, drive, dir, fname, ext );
	CreateDirectory(path_buffer, NULL);
	lstrcpy (dirpath,path_buffer);
	lstrcat (path_buffer,fname);
	lstrcat (path_buffer,".lst");
	DeleteFile(path_buffer);

	Break_listinfo (path_buffer);



	memset (tmpbuf1,0 ,sizeof(tmpbuf1) );
	for (i=0;i<sizeof (lumpinfo_t);i++)
	{
		tmpbuf1[i] = fgetc(instream);
	}
	ptr = (BYTE*)tmpbuf1;
	wadinfo = (wadinfo_t*)ptr;
	numlumps = wadinfo->numlumps ;

	memset (tmpbuf2,0 ,sizeof(tmpbuf2) );
	fseek(instream,wadinfo->infotableofs ,SEEK_SET);
	if ((wadinfo->identification[0]!='I')||
		(wadinfo->identification[1]!='W')||
		(wadinfo->identification[2]!='A')||
		(wadinfo->identification[3]!='D'))
	{
		sprintf(tmpbuf1,"File '%s' is not a main ROTT IWAD file",szFileName);
		Break_listinfo (tmpbuf1);
		return FALSE;
	}
	if( (txtstream  = fopen( path_buffer, "w+t" )) == NULL ) 
	{
		Break_listinfo ("Opening of ROTT LIST file failed!");
		return FALSE;
	}/*
	if( (wadinfostream  = fopen( "WadInfoTabel.txt", "w+t" )) == NULL ) {
		Break_listinfo ("Opening of WadInfoTabel.txt file failed!");
	}*/

	fputs("IWAD lump list for ROTT, made by Rott3Dmapeditor\n",txtstream );
	fputs("\n",txtstream );
	Break_listinfo ("Writing Rott Lump Files ...");

	//EDGE: What are these used for??
	disableBreakButtons( hwnd,IDCANCEL);
	disableBreakButtons( hwnd,IDOK);
	disableBreakButtons( hwnd,IDC_BUTT_BROWSE1);
	disableBreakButtons( hwnd,IDC_BUTT_BROWSE2);

	//read table into LB's
	for (lump = 0; lump < numlumps ;lump++){
		//read lump info
		for (i=0;i<sizeof (lumpinfo_t);i++){
			tmpbuf2[i] = fgetc(instream);
		}
		ptr = (BYTE*)tmpbuf2;
		lumpinfo = (lumpinfo_t*)ptr;
		//lstrcpy (lumpfilename,dirpath);
		//lstrcpy(lumpfilename,lumpinfo->name );
		//lstrcat(lumpfilename,".RLF" );
		//fputs(lumpfilename,txtstream );
		//count++;

		//lstrcpy(lumpfilename,lumpinfo->name );
		//lstrcat(lumpfilename,".RLF" );
		SendMessage(hWndNameLB, LB_ADDSTRING , 0, (LPARAM)lumpinfo->name);
		sprintf(tmpbuf1,"%d",lumpinfo->size );
		SendMessage(hWndSizeLB, LB_ADDSTRING , 0, (LPARAM)tmpbuf1);
		sprintf(tmpbuf1,"%d",lumpinfo->filepos );
		SendMessage(hWndFilePosLB, LB_ADDSTRING , 0, (LPARAM)tmpbuf1);

		CheckforSTRT (lump, lumpinfo->name);
/*
		//write to WadInfoTabel.txt
		sprintf(tmpbuf1,"%s %d %d \n",lumpinfo->name,lumpinfo->size,lumpinfo->filepos);
		fputs(tmpbuf1,wadinfostream );
*/
	} 

	x=SendMessage(hWndNameLB,LB_GETCOUNT,(WPARAM)0,(LPARAM) 0);
	//extract files from wad based on info from LB's
	count = 0;
	for (lump = 0; lump < numlumps ;lump++)
	{
	//for (lump=0;lump<x;lump++){
		SendMessage(hWndSizeLB,LB_GETTEXT,(WPARAM)lump,(LPARAM)tmpbuf1);
		filesize = atol(tmpbuf1);
		SendMessage(hWndFilePosLB,LB_GETTEXT,(WPARAM)lump,(LPARAM)tmpbuf1);
		filepos = atol(tmpbuf1);
		SendMessage(hWndNameLB,LB_GETTEXT,(WPARAM)lump,(LPARAM)tmpbuf1);

/*
//these gets us into trouble
if (strstr(tmpbuf1,"BULLETHO")!=0)
goto SkipLump;*/
if (strstr(tmpbuf1,"TINYFONT")!=0)
goto SkipLump;
if (strstr(tmpbuf1,"SMALLFON")!=0)
goto SkipLump;
if (strstr(tmpbuf1,"FANFARE2")!=0)
goto SkipLump;
if (strstr(tmpbuf1,"M_FLIP")!=0)
goto SkipLump;


if (strstr(tmpbuf1,"SHARTIT")!=0)
goto SkipLump;


/*
if (strstr(tmpbuf1,"DSTRIP")!=0)
goto SkipLump;
if (strstr(tmpbuf1,"TP2")!=0)
goto SkipLump;
if (strstr(tmpbuf1,"TP3")!=0)
goto SkipLump;*/



if ((strstr(tmpbuf1,"PAL")!=0)&&(filesize==768))
goto SkipLump;
if ((strstr(tmpbuf1,"MAP")!=0)&&(filesize==8192))
goto SkipLump;
if ((strstr(tmpbuf1,"MENUCMAP")!=0)&&(filesize==4096))
goto SkipLump;




if (strstr(tmpbuf1,"LICENSE")!=0)
goto SkipLump;
if (strstr(tmpbuf1,"GUSMIDI")!=0)
goto SkipLump;

testtest:;

		I_Printf("Extracting/Checking Wall graphics");
		if ( lump > 0){
			for (x=0;x<100;x++)
			{
				if ( lump == GFX_T_single[x])
				{
					HandleLumpShapeBMP(filepos,filesize,dirpath,tmpbuf1,0,0,txtstream,instream);
					goto botloop;
				}
			}
			for (x=0;x<10;x++)
			{
				if ( lump == GFX_T_LBMsingle[x]){
					HandleLumpLBM(lump,filepos,filesize,dirpath,tmpbuf1,320,200,txtstream,instream);
					goto botloop;
				}
			}
			for (x=0;x<10;x++)
			{
				if ( lump == GFX_T_Picsingle[x]){
					HandleLumpPic_tBMP(lump,filepos,filesize,dirpath,tmpbuf1,0,0,txtstream,instream);
					goto botloop;
				}
			}
		}

		if ((lump>=GFX_wallstart)&&(lump<=GFX_wallstop))
		{
			HandleLumpBMP(filepos,filesize,dirpath,tmpbuf1,64,64,txtstream,instream);
		}
		else if ((lump>=GFX_floorstart)&&(lump<=GFX_floorstop))
		{
			HandleLumpBMP(filepos+8,128*128,dirpath,tmpbuf1,128,128,txtstream,instream);
		
		}
		else if ((lump>=GFX_sky_start)&&(lump<=GFX_sky_stop))
		{
			HandleLumpBMP(filepos+2,256*200,dirpath,tmpbuf1,200,256,txtstream,instream);
		}
		else if ((lump>=GFX_shapestart)&&(lump<=GFX_shapestop))
		{
			HandleLumpShapeBMP(filepos,filesize,dirpath,tmpbuf1,0,0,txtstream,instream);
		}
		else if ((lump>=GFX_gunsstart)&&(lump<=GFX_gunsstop))
		{
			HandleLumpShapeBMP(filepos,filesize,dirpath,tmpbuf1,0,0,txtstream,instream);
		}
		else if ((lump>=GFX_titlestart)&&(lump<=GFX_titlestop))
		{
			HandleLumpShapeBMP(filepos,filesize,dirpath,tmpbuf1,0,0,txtstream,instream);
		}
		else if ((lump>=GFX_doorstart)&&(lump<=GFX_doorstop))
		{
			if ((filesize == 4096)&&(strstr(tmpbuf1,"SDOOR4A")==0))
				HandleLumpBMP(filepos,filesize,dirpath,tmpbuf1,64,64,txtstream,instream);
			else		
				HandleLumpShapeBMP(filepos,filesize,dirpath,tmpbuf1,0,0,txtstream,instream);
		}
		else if ((lump>=GFX_sidestart)&&(lump<=GFX_sidestop))
		{
			HandleLumpBMP(filepos,filesize,dirpath,tmpbuf1,64,64,txtstream,instream);
		}
		else if ((lump>=GFX_elevatorstart)&&(lump<=GFX_elevatorstop))
		{
			HandleLumpBMP(filepos,filesize,dirpath,tmpbuf1,64,64,txtstream,instream);
		}
		else if ((lump>=GFX_exitstart)&&(lump<=GFX_exitstop))
		{
			if (filesize != 4096)
				HandleLumpShapeBMP(filepos,filesize,dirpath,tmpbuf1,0,0,txtstream,instream);
			else
				HandleLumpBMP(filepos,filesize,dirpath,tmpbuf1,64,64,txtstream,instream);
		}
		else if ((lump>=GFX_abovewallstart)&&(lump<=GFX_abovewallstop))
		{
			if (filesize != 4096)
				HandleLumpShapeBMP(filepos,filesize,dirpath,tmpbuf1,0,0,txtstream,instream);
			else
				HandleLumpBMP(filepos,filesize,dirpath,tmpbuf1,64,64,txtstream,instream);
		}
		else if ((lump>=GFX_abovemaskwallstart)&&(lump<=GFX_abovemaskwallstop))
		{
			if (filesize != 4096)
				HandleLumpShapeBMP(filepos,filesize,dirpath,tmpbuf1,0,0,txtstream,instream);
			else
				HandleLumpBMP(filepos,filesize,dirpath,tmpbuf1,64,64,txtstream,instream);
		
		}
		else if ((lump>=GFX_abovewallstart)&&(lump<=GFX_abovewallstop))
		{
			if (filesize != 4096)
				HandleLumpShapeBMP(filepos,filesize,dirpath,tmpbuf1,0,0,txtstream,instream);
			else
				HandleLumpBMP(filepos,filesize,dirpath,tmpbuf1,64,64,txtstream,instream);
		}
		else if ((lump>=GFX_mishstart)&&(lump<=GFX_mishstop))
		{
			HandleLumpShapeBMP(filepos,filesize,dirpath,tmpbuf1,0,0,txtstream,instream);
		}
		else if ((lump>=GFX_pic_tstart)&&(lump<=GFX_pic_tstop)){
			HandleLumpPic_tBMP(lump,filepos,filesize,dirpath,tmpbuf1,0,0,txtstream,instream);

			I_Printf("Finished scanning end markers, continuing...\n");


		}
		else if ((lump>=GFX_norm_2_start)&&(lump<=GFX_norm_2_stop)){
			//HandleLumpShapeBMP(filepos,filesize,dirpath,tmpbuf1,0,0,txtstream,instream);
				HandleLumpPic_tBMP(lump,filepos,filesize,dirpath,tmpbuf1,0,0,txtstream,instream);
			//HandleLumpShapeBMPSpecial(filepos,filesize,dirpath,tmpbuf1,0,0,txtstream,instream);
		




		}
		else if ((lump>=GFX_pic_tstart1)&&(lump<=GFX_pic_tstop1)){
			HandleLumpPic_tBMP(lump,filepos,filesize,dirpath,tmpbuf1,0,0,txtstream,instream);
			//	HandleLumpShapeBMP(filepos,filesize,dirpath,tmpbuf1,0,0,txtstream,instream);
					
			/*		GFX_TEST
GFX_m_infostart
GFX_ap_titl 
*/
		}else if (lump==GFX_ap_world)
		{
			HandleLumpIntro(lump,filepos,filesize,dirpath,tmpbuf1,320,200,txtstream,instream);
		}else if (lump==GFX_plane)
		{
			HandleLumpIntro(lump,filepos,filesize,dirpath,tmpbuf1,320,200,txtstream,instream);


		}else if (lump==GFX_TEST)
		{
			HandleLumpPic_tBMP(lump,filepos,filesize,dirpath,tmpbuf1,320,200,txtstream,instream);


	//	}else if (lump==GFX_TEST){
			//HandleLumpLBM(lump,filepos,filesize,dirpath,tmpbuf1,320,200,txtstream,instream);
	//		HandleLumpShapeBMP(filepos,filesize,dirpath,tmpbuf1,0,0,txtstream,instream);

		}else if ((lump>=GFX_specstart)&&(lump<=GFX_specstop))
		{
			if ((strstr(tmpbuf1, "PAL") != 0) && (filesize == 768))
				I_Printf("PAL lumps: skipping...\n");
				goto SkipLump;
			HandleLumpShapeBMPSpecial(filepos,filesize,dirpath,tmpbuf1,0,0,txtstream,instream);



	//	}else if ((lump==GFX_lmb_1)||(lump==GFX_lmb_2)||(lump==GFX_lmb_3)||(lump==GFX_lmb_4)||(lump==GFX_lmb_5)){
	//		HandleLumpLBM(lump,filepos,filesize,dirpath,tmpbuf1,320,200,txtstream,instream);
		}else if ((lump>=GFX_pic_tstart2)&&(lump<=GFX_pic_tstop2))
		{
			HandleLumpPic_tBMP(lump,filepos,filesize,dirpath,tmpbuf1,0,0,txtstream,instream);
	



/* these is just a line
		}else if ((lump>=GFX_pcstart)&&(lump<=GFX_pcstop)){
			HandleLumpPic_tBMP(lump,filepos,filesize,dirpath,tmpbuf1,0,0,txtstream,instream);
			//	HandleLumpShapeBMP(filepos,filesize,dirpath,tmpbuf1,0,0,txtstream,instream);
			//	HandleLumpShapeBMPSpecial(filepos,filesize,dirpath,tmpbuf1,0,0,txtstream,instream);

		}else if ((lump>=GFX_adstart)&&(lump<=GFX_adstop)){
			HandleLumpPic_tBMP(lump,filepos,filesize,dirpath,tmpbuf1,0,0,txtstream,instream);
			//	HandleLumpShapeBMP(filepos,filesize,dirpath,tmpbuf1,0,0,txtstream,instream);
			//	HandleLumpShapeBMPSpecial(filepos,filesize,dirpath,tmpbuf1,0,0,txtstream,instream);

			*/	


		}
		else if ((lump>=GFX_animstart)&&(lump<=GFX_animstop))
		{
			if (filesize != 4096)
				HandleLumpShapeBMP(filepos,filesize,dirpath,tmpbuf1,0,0,txtstream,instream);
			else
				HandleLumpBMP(filepos,filesize,dirpath,tmpbuf1,64,64,txtstream,instream);


		}
		else if ((lump>=GFX_m_infostart)&&(lump<=GFX_m_infostop))
		{
			HandleLumpShapeBMP(filepos,filesize,dirpath,tmpbuf1,0,0,txtstream,instream);
		}
		else if ((lump>=GFX_m_q_yesstart)&&(lump<=GFX_m_q_nostop))
		{
			HandleLumpShapeBMP(filepos,filesize,dirpath,tmpbuf1,0,0,txtstream,instream);
		}
		else if ((lump>=GFX_m_newstart)&&(lump<=GFX_m_newstop))
		{
			HandleLumpShapeBMP(filepos,filesize,dirpath,tmpbuf1,0,0,txtstream,instream);

		}
		// EDGE: origpal/shadingtable? Maybe origpal should be rott_pallette from r_playpal.cc (which converts the ROTT base palette)
		else if (lump==GFX_ap_titl)
		{
			byte tmppal[768];
			byte*tmpshadingtable = shadingtable;
			memcpy (tmppal,&origpal[0],768);
			memcpy (&origpal[0], W_CacheLumpName ("ap_pal", PU_CACHE, CvtNull, 1), 768);
			shadingtable = colormap + (1 << 12);
			VL_NormalizePalette (&origpal[0]);
			HandleLumpShapeBMP(filepos,filesize,dirpath,tmpbuf1,0,0,txtstream,instream);

			memcpy (&origpal[0],tmppal,768);
			shadingtable = tmpshadingtable;

		}else if (lump==GFX_trilogo){
			//HandleLumpIntro(lump,filepos,filesize,dirpath,tmpbuf1,320,200,txtstream,instream);
			HandleLumpBMP(filepos+8,320*200,dirpath,tmpbuf1,200,320,txtstream,instream);


		}else if ((lump>=GFX_maskedstart)&&(lump<=GFX_maskedstop)){
			HandleLumpShapeBMP(filepos,filesize,dirpath,tmpbuf1,0,0,txtstream,instream);
		}else if ((lump>=GFX_himaskstart)&&(lump<=GFX_himaskstop)){
			HandleLumpShapeBMP(filepos,filesize,dirpath,tmpbuf1,0,0,txtstream,instream);
			

		}else{
SkipLump:;
			we[0]=0;
			x = fread( we, 1, 25, instream );
			//if (i > 0){
			if (filesize > 0){
				if (strstr(we,"Creative Voice File")!=0)
				{
					lstrcat(tmpbuf1,".VOC");
				}
				else if (strstr(tmpbuf1,"M_FLIP")!=0)
				{
					lstrcat(tmpbuf1,".VOC");
				}
				else if (strstr(we,"MThd")!=0)
				{
					lstrcat(tmpbuf1,".MID");
				}
				else if ((strstr(tmpbuf1,"PAL")!=0)&&(filesize==768))
				{
					lstrcat(tmpbuf1,".PAL");
				}
				else if ((strstr(tmpbuf1,"MAP")!=0)&&(filesize==8192))
				{
					lstrcat(tmpbuf1,".MAP");
				}
				else if ((strstr(tmpbuf1,"MENUCMAP")!=0)&&(filesize==4096))
				{
					lstrcat(tmpbuf1,".MAP");
				}
				else if (strstr(tmpbuf1,"SHARTIT")!=0)
				{
					lstrcat(tmpbuf1,".TXT");
				}
				else if (strstr(tmpbuf1,"SHARTIT2")!=0)
				{
					lstrcat(tmpbuf1,".TXT");
				}
				else if (strstr(tmpbuf1,"FANFARE2")!=0)
				{
					lstrcat(tmpbuf1,".MID");
				}
				else if (lstrcmpi(tmpbuf1,"IFNT")==0)
				{
					lstrcat(tmpbuf1,".FNT");
				}
				else if (lstrcmpi(tmpbuf1,"ITNYFONT")==0)
				{
					lstrcat(tmpbuf1,".FNT");
				}
				else if (lstrcmpi(tmpbuf1,"LIFONT")==0)
				{
					lstrcat(tmpbuf1,".FNT");
				}
				else if (lstrcmpi(tmpbuf1,"NEWFNT1")==0)
				{
					lstrcat(tmpbuf1,".FNT");
				}
				else if (lstrcmpi(tmpbuf1,"SIFONT")==0)
				{
					lstrcat(tmpbuf1,".FNT");
				}
				else if (lstrcmpi(tmpbuf1,"SMALLFON")==0)
				{
					lstrcat(tmpbuf1,".FNT");
				}
				else if (lstrcmpi(tmpbuf1,"TINYFONT")==0)
				{
					lstrcat(tmpbuf1,".FNT");
				}
				else if (lstrcmpi(tmpbuf1,"LICENSE")==0)
				{
					lstrcat(tmpbuf1,".DOC");
				}
				else if (lstrcmpi(tmpbuf1,"GUSMIDI")==0)
				{
					lstrcat(tmpbuf1,".INF");
				}
				else
				{
					lstrcat(tmpbuf1,".LMP");
				}
			}else{

			}

			//lstrcat(tmpbuf1,".RLF");
			//add name to lst file
	
			fputs(tmpbuf1,txtstream );
			fputs("\n",txtstream );

			lstrcpy (lumpfilename,dirpath);
			lstrcat(lumpfilename,tmpbuf1 );

			fseek(instream,filepos,SEEK_SET);	
			DeleteFile(lumpfilename);

			if((outstream = fopen(lumpfilename, "w+b" )) == NULL )
			{
				Break_listinfo ("Opening of ROTT LMP file failed!");
				enableBreakButtons(hwnd,IDCANCEL);
				return FALSE;
			}		 
			for (x=0;x<filesize;x++)
			{
				c = fgetc(instream);//read from wad
				fputc(c,outstream); //write to new file
			}
			_flushall(  );
			fclose(outstream);

			Break_listinfo(lumpfilename);
		}

//	  	WriteBmp (filename, buffer,64, iGLOBAL_SCREENHEIGHT, colrs  ,255);

botloop:;
		count++;
	}
	
	sprintf(tmpbuf1,"%d files extracted",count);
	Break_listinfo (tmpbuf1);

	lstrcpy(lumpfilename,"\n" );
	fputs(lumpfilename,txtstream );
	_flushall(  );
	fclose(txtstream);
	fclose(instream);

	enableBreakButtons(hwnd,IDCANCEL);
	SetWindowText(GetDlgItem(hwnd,IDCANCEL),"Done");
}


/*

bool BreakRottWad(HWND hwnd)
{
	wadinfo_t *wadinfo;
	lumpinfo_t *lumpinfo;
	int handle;
	char path_buffer[_MAX_PATH*2];
    char drive[_MAX_DRIVE];
    char dir[_MAX_DIR+5];
    char fname[_MAX_FNAME];
    char ext[_MAX_EXT];
    FILE	*instream,*outstream,*txtstream;  
	BYTE *ptr;
	char lumpfilename[260];
	char we[260],checklstfname[256];
	char dirpath[512];
	char tmpbuf1[512], tmpbuf2[512];
    unsigned int i,x,lump,count=0,numlumps;
    u8_t c;
	long oldfilepos,filepos,filesize;


	SendMessage(hWndNameLB,LB_RESETCONTENT,(WPARAM)0, (LPARAM) 0);
	SendMessage(hWndSizeLB,LB_RESETCONTENT,(WPARAM)0, (LPARAM) 0);
	SendMessage(hWndFilePosLB,LB_RESETCONTENT,(WPARAM)0, (LPARAM) 0);
	SendMessage(hWndCheckLB,LB_RESETCONTENT,(WPARAM)0, (LPARAM) 0);

	GetWindowText(GetDlgItem(hwnd,IDC_IWADPATH_EDIT),szFileName,sizeof(szFileName));

	if( (instream  = fopen( szFileName, "r+b" )) == NULL ) {
	//	AddTxtToListbox ("Opening of ROTT WAD file failed!");
		return FALSE;
	}	
	
	//get the name to save to
	GetWindowText(GetDlgItem(hwnd,IDC_IWADRESPATH_EDIT),path_buffer,sizeof(path_buffer));
	//get the name to save to
	_splitpath( szFileName, drive, dir, fname, ext );
	CreateDirectory(path_buffer, NULL);
	lstrcpy (dirpath,path_buffer);
	lstrcat (path_buffer,fname);
	lstrcat (path_buffer,".lst");
	DeleteFile(path_buffer);
	if( (txtstream  = fopen( path_buffer, "w+t" )) == NULL ) {
		AddTxtToListbox ("Opening of ROTT LIST file failed!");
		return FALSE;
	}

	memset (tmpbuf1,0 ,sizeof(tmpbuf1) );

	for (i=0;i<sizeof (lumpinfo_t);i++){
		tmpbuf1[i] = fgetc(instream);
	}
	ptr = (BYTE*)tmpbuf1;
	wadinfo = (wadinfo_t*)ptr;
	numlumps = wadinfo->numlumps ;

	memset (tmpbuf2,0 ,sizeof(tmpbuf2) );
	fseek(instream,wadinfo->infotableofs ,SEEK_SET);

	AddTxtToListbox ("Writing Rott Lump Files ...");

	//read table into LB's
	for (lump = 0; lump < numlumps ;lump++){
		//read lump info
		for (i=0;i<sizeof (lumpinfo_t);i++){
			tmpbuf2[i] = fgetc(instream);
		}
		ptr = (BYTE*)tmpbuf2;
		lumpinfo = (lumpinfo_t*)ptr;
		//lstrcpy (lumpfilename,dirpath);
		//lstrcpy(lumpfilename,lumpinfo->name );
		//lstrcat(lumpfilename,".RLF" );
		//fputs(lumpfilename,txtstream );
		//count++;

//lstrcpy(lumpfilename,lumpinfo->name );
//lstrcat(lumpfilename,".RLF" );
SendMessage(hWndNameLB, LB_ADDSTRING , 0, (LPARAM)lumpinfo->name);

sprintf(tmpbuf1,"%d",lumpinfo->size );
SendMessage(hWndSizeLB, LB_ADDSTRING , 0, (LPARAM)tmpbuf1);
sprintf(tmpbuf1,"%d",lumpinfo->filepos );
SendMessage(hWndFilePosLB, LB_ADDSTRING , 0, (LPARAM)tmpbuf1);

	}

	x=SendMessage(hWndNameLB,LB_GETCOUNT,(WPARAM)0,(LPARAM) 0);
	//extract files from wad based on info from LB's
	count = 0;
	for (lump = 0; lump < numlumps ;lump++){
		MSG msg;
	//for (lump=0;lump<x;lump++){
		UpdateWindow(hWndNameLB);
		UpdateWindow(hWndCheckLB);
		UpdateWindow(hWndSizeLB);
		UpdateWindow(hWndFilePosLB);

		GetMessage(&msg, NULL, 0, 0);
		TranslateMessage(&msg);  // Translates virtual key codes
		DispatchMessage(&msg);   // Dispatches message to window

		SendMessage(hWndNameLB,LB_GETTEXT,(WPARAM)lump,(LPARAM)tmpbuf1);
		//check if name previus is used
/*		if (SendMessage(hWndCheckLB,LB_FINDSTRINGEXACT,(WPARAM)-1,(LPARAM)tmpbuf1)!=LB_ERR){
			//yes filename is used create a new
			//add filename to checklst
			SendMessage(hWndCheckLB, LB_ADDSTRING , 0, (LPARAM)tmpbuf1);
			lstrcat(tmpbuf1,".RLX");
		}else{
			//add filename to checklst
			SendMessage(hWndCheckLB, LB_ADDSTRING , 0, (LPARAM)tmpbuf1);
			lstrcat(tmpbuf1,".RLF");
		}
		//add name to lst file
		fputs(tmpbuf1,txtstream );
		fputs("\n",txtstream );*//*
			
		//add filename to checklst
		SendMessage(hWndCheckLB, LB_ADDSTRING , 0, (LPARAM)tmpbuf1);
		lstrcpy (checklstfname,tmpbuf1);



		lstrcpy (lumpfilename,dirpath);
		lstrcat(lumpfilename,tmpbuf1 );

		SendMessage(hWndSizeLB,LB_GETTEXT,(WPARAM)lump,(LPARAM)tmpbuf1);
		filesize = atol(tmpbuf1);
		SendMessage(hWndFilePosLB,LB_GETTEXT,(WPARAM)lump,(LPARAM)tmpbuf1);
		filepos = atol(tmpbuf1);

		fseek(instream,filepos,SEEK_SET);	
		DeleteFile(lumpfilename);

		if((outstream = fopen(lumpfilename, "w+b" )) == NULL ){
			AddTxtToListbox ("Opening of ROTT LMP file failed!");
			return FALSE;
		}
						
		SetWindowText(GetDlgItem(hwnd,IDC_IWADINFO_EDIT2),lumpfilename);


		
		for (x=0;x<filesize;x++){
			c = fgetc(instream);//read from wad
			fputc(c,outstream); //write to new file
		}

		//try to identify file type
		rewind(outstream);
		we[0]=0;
		i = fread( we, 1, 25, outstream );


		if (i > 0){
			if (strstr(we,"Creative Voice File")!=0){
				lstrcat(checklstfname,".VOC");
				lstrcpy (tmpbuf1,dirpath);
				lstrcat(tmpbuf1,checklstfname );
			}else if (strstr(we,"MThd")!=0){
				lstrcat(checklstfname,".MID");
				lstrcpy (tmpbuf1,dirpath);
				lstrcat(tmpbuf1,checklstfname );
			}else{
				//add filename to checklst
				lstrcat(checklstfname,".RLF");
				lstrcpy (tmpbuf1,dirpath);
				lstrcat(tmpbuf1,checklstfname );
			}
		}

		//add name to lst file
		fputs(checklstfname,txtstream );
		fputs("\n",txtstream );

		fclose(outstream);

		if (i>0)
			MoveFile(lumpfilename,tmpbuf1);

		count++;
	
	}
	
	sprintf(tmpbuf1,"%d files extracted",count);
	AddTxtToListbox (tmpbuf1);

	lstrcpy(lumpfilename,"\n" );
	fputs(lumpfilename,txtstream );
	fclose(txtstream);
	fclose(instream);





}
*/
byte BigMem1[0x1FFFF];
byte BigMem2[0x1FFFF];
HDC  hMemDCTemp = 0;

// EDGE:
// This can be called by IdentifyVersion() in e_main, which is the function to detect an IWAD:
// 	if (rott_mode)
//{
//	I_Printf("Rise of the Triad: Darkwar detected\n");
//  MakeRottWad();
//}
//
// Reason is that we need a platform-agnostic solution to WTO_AssembleWadProc(), so my goal here is to
// make the whole process happen in a "new" GLBSP Startup-Progress Bar via e_main.cc
// So what do I need to replace HWND hwnd to?
bool MakeRottWad(HWND hwnd)
{
	wadinfo_t wadinfo, *w;
	lumpinfo_t lumpinfo, *l;


	int handle;
	char path_buffer[_MAX_PATH*2];
    char drive[_MAX_DRIVE];
    char dir[_MAX_DIR+5];
    char fname[_MAX_FNAME];
    char ext[_MAX_EXT];
    FILE	*instream,*lumpstream,*wadstream,*tmpstream,*txtstream;  
	BYTE *ptr,*xtr;
	char lumpfilename[260];
	char tmpfilename[260];
	char wadfilename[260];
	char dirpath[512];
	char tmpbuf1[512], tmpbuf2[512];
    unsigned int i,x,lump,count=0,listcount=0;
    u8_t c,p;
	//long oldfilepos;
	long lumpfilesize=0;
	long lumpfilepos=0x0C;//iwad header
	int filehandle;
	u8_t *src,Type[2];
	int origsize ,leftoff ,topoff,translevel ;

	oldfilepos = 12;

	SendMessage(hWndNewSizeLB,LB_RESETCONTENT,(WPARAM)0, (LPARAM) 0);
	SendMessage(hWndNameLB,LB_RESETCONTENT,(WPARAM)0, (LPARAM) 0);
	SendMessage(hWndSizeLB,LB_RESETCONTENT,(WPARAM)0, (LPARAM) 0);
	SendMessage(hWndFilePosLB,LB_RESETCONTENT,(WPARAM)0, (LPARAM) 0);
	SendMessage(hWndCheckLB,LB_RESETCONTENT,(WPARAM)0, (LPARAM) 0);

	GetWindowText(GetDlgItem(hwnd,IDC_A_IWADPATH_EDIT),szFileName,sizeof(szFileName));
	GetWindowText(GetDlgItem(hwnd,IDC_A_IWADRESPATH_EDIT),path_buffer,sizeof(path_buffer));
	_setmaxstdio (2048); //??

	if (access (szFileName, 0) != 0)
	{ //does sfxfile exists
		sprintf(tmpbuf1,"File '%s' does not exist",szFileName);
		Assem_listinfo (tmpbuf1);
		return 0;
	}

	if( (txtstream  = fopen( szFileName, "r+t" )) == NULL ) 
	{
		Assem_listinfo ("Opening of ROTT LIST file failed!");
		return 0;
	}
	
	//make the dirpath
	_splitpath( szFileName, drive, dir, fname, ext );
	lstrcpy (dirpath,drive);
	lstrcat (dirpath,dir);

	//make the new wad to save to
	//lstrcpy (path_buffer,dirpath);
	//lstrcat (path_buffer,fname);
	//lstrcat (path_buffer,".wad");
	DeleteFile(path_buffer);
	lstrcpy (wadfilename,path_buffer);

	if((wadstream = fopen(path_buffer, "w+b" )) == NULL )
	{
		Assem_listinfo ("Creation of ROTT WAD file failed!");
		return FALSE;
	}

	// EDGE: This should be cached as a RWA cached wad in /cache (with the HWA/GWA wadfiles)
	Assem_listinfo ("Creating WAD file");

	//write the temp wad header
	Assem_listinfo ("Writing IWAD header");

	lstrcpy(wadinfo.identification,"IWAD"); 
	wadinfo.infotableofs = 0;
	wadinfo.numlumps = 0;
	w = &wadinfo;
	ptr = (BYTE*)w;
	for(i=0;i<sizeof(wadinfo_t);i++)
	{
		fputc(*(ptr+i),wadstream);
	}	
	/* 
	lstrcpy (path_buffer,dirpath);
	lstrcat (path_buffer,"wadtmp.tmp");
	lstrcpy (tmpfilename,path_buffer);
	DeleteFile(path_buffer);
	if((tmpstream = fopen(path_buffer, "w+b" )) == NULL ){AddTxtToListbox ("Creation of ROTT tmp WAD file failed!");return FALSE;}
	*/
    fgets( tmpbuf1,sizeof(tmpbuf1),  txtstream );
	if (strstr(tmpbuf1,"IWAD lump list")==0)
	{
		sprintf(tmpbuf1,"File '%s' is not a IWAD lump list",szFileName);
		Assem_listinfo (tmpbuf1);
		return 0;
	}
    fgets( tmpbuf1,sizeof(tmpbuf1),  txtstream );
	disableBreakButtons( hwnd,IDCANCEL);
	disableBreakButtons( hwnd,IDOK);
	disableBreakButtons( hwnd,IDC_A_BUTT_BROWSE1);
	disableBreakButtons( hwnd,IDC_A_BUTT_BROWSE2);

	Assem_listinfo ("Writing RLF files to IWAD");
	listcount=0;
   /* Cycle until end of file reached: */
   while( !feof( txtstream ) )
   {
      /* Attempt to read in a lumpname */
      fgets( tmpbuf1,sizeof(tmpbuf1),  txtstream );
      if( ferror( txtstream ) )     
	  {
		  Assem_listinfo ("Error in reading wadlist!");
		  enableBreakButtons(hwnd,IDCANCEL);
		  return FALSE;
      }

	  listcount++;

	  //find lumpfilesize
	  i = lstrlen(tmpbuf1);
	  if ((i>1)&&(i<sizeof(tmpbuf1)))
			tmpbuf1[i-1]=0;//remove end of line marker
	  translevel = 0;
	  //do we have extra data stored in line
	  ptr = strstr(tmpbuf1,"#");
	  Type[0]=0;
	  if (ptr!=0){
		  Type[0]=*(ptr+1);
		  xtr = strstr(tmpbuf1,"#origs");
		  if (xtr!=0)
			origsize = atol(xtr+6);
		  xtr = strstr(tmpbuf1,"#loff");
		  if (xtr!=0)
			leftoff = atol(xtr+5);
		  xtr = strstr(tmpbuf1,"#toff");
		  if (xtr!=0)
			topoff = atol(xtr+5);
		  xtr = strstr(tmpbuf1,"#trans");
		  if (xtr!=0)
			translevel = atol(xtr+6);

		  *(ptr-1) = 0;
  	  }

 	  lstrcpy (path_buffer,dirpath);
	  lstrcat (path_buffer,tmpbuf1);

	  if (tmpbuf1[0]<=0x20){
			Assem_listinfo ("0x20");
		  break;
	  }

	  if((lumpstream = fopen(path_buffer, "r+b" )) == NULL )
	  {
			sprintf(tmpbuf1,"Opening of ROTT RLF file '%s' failed!  ",path_buffer);
			Assem_listinfo (tmpbuf1);
			enableBreakButtons(hwnd,IDCANCEL);
			return 0;

		  return FALSE;
	  }
//	  SendMessage(hWndStatus, SB_SETTEXT, 0, (LPARAM)path_buffer);
	  //AddTmpTxtToListbox (path_buffer);
	  fseek(lumpstream,0,SEEK_END);
	  lumpfilesize = ftell(lumpstream);

	  if (lumpfilesize < 0)
	  {
			sprintf(tmpbuf1,"lumpfilesize '%d' ",lumpfilesize);
			Assem_listinfo (tmpbuf1);
		    enableBreakButtons(hwnd,IDCANCEL);
			return 0;
	  } 

	  //copy lumpname to lumpinfo struct
	  if (strstr(tmpbuf1,".")!=0)
	  {
		i = lstrlen(tmpbuf1);
	    if ((i>4)&&(i<sizeof(tmpbuf1)))
			tmpbuf1[i-4]=0;//remove end of line marker
	  }

	  //lstrcpy(lumpinfo.name,tmpbuf1);
	  SendMessage(hWndNameLB, LB_ADDSTRING , 0, (LPARAM)tmpbuf1);
	  sprintf(tmpbuf1,"%d",lumpfilesize);
	  SendMessage(hWndSizeLB, LB_ADDSTRING , 0, (LPARAM)tmpbuf1);
	  sprintf(tmpbuf1,"%d",lumpfilepos);
	  SendMessage(hWndFilePosLB, LB_ADDSTRING , 0, (LPARAM)tmpbuf1);

	  Assem_listinfo(path_buffer);
	  //save the lumpfile itself to the wad file
	  fseek(lumpstream,0,SEEK_SET);

	  if (lumpfilesize > 0)
	  {
		    CharUpper(path_buffer);
			if (strstr(path_buffer,".BMP")!=0)
			{
				HBITMAP hBmp;	
				BITMAP  bm;
				RGBQUAD rgb[256];

				fclose(lumpstream);

				if (hMemDCTemp == 0)  // Create a memory DC and select the DIBSection into it
					hMemDCTemp = CreateCompatibleDC( NULL );

				hBmp = (HBITMAP)LoadImage(NULL,path_buffer,IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION|LR_LOADFROMFILE );
				GetObject(hBmp, sizeof(BITMAP), (LPSTR) &bm);

				if ((bm.bmWidth > 320)||(bm.bmHeight > 320))
				{
					HBITMAP hnew = CopyImage(hBmp,IMAGE_BITMAP,320,200,LR_CREATEDIBSECTION);
					DeleteObject(hBmp);
					hBmp = hnew;
					GetObject(hnew, sizeof(BITMAP), (LPSTR) &bm);
					Assem_listinfo ("Picture to large, rescaling.");
				}

				if (bm.bmBitsPixel != 8)
				{
					Assem_listinfo ("Converting 32/24 bit bmp picture to 8 bit.");
					Convert24bitBmpTo8bitExt (BigMem2,hBmp);
					//Convert24bitBmpTo8bit((byte*)bm.bmBits,BigMem2,bm.bmWidth,bm.bmHeight,bm.bmWidthBytes);
					bm.bmBits=(byte*)BigMem2;
				}
				SelectObject( hMemDCTemp, hBmp );
				// Get the DIBSection's color table
				GetDIBColorTable( hMemDCTemp, 0, 256, rgb );

				lumpfilesize = bm.bmWidth*bm.bmHeight;

				//floor ceil seems to be missing 8 byte, why
				if (strstr(path_buffer,"FLRCL")!=0)
				{
					c = 128;fputc(c,wadstream);
					c = 0;  fputc(c,wadstream);
					c = 128;fputc(c,wadstream);
					c = 0;  fputc(c,wadstream);

					c = 0;  fputc(c,wadstream);
					c = 0;  fputc(c,wadstream);
					c = 0;  fputc(c,wadstream);
					c = 0;  fputc(c,wadstream);
					/*
					for (i=0;i<8;i++){
						fputc(0,wadstream);
					}*/
					lumpfilesize += 8;
				
				}
				if (strstr(path_buffer,"SKY")!=0)
				{
					c = 63;  fputc(c,wadstream);
					c = 63;  fputc(c,wadstream);
					/*for (i=0;i<2;i++){
						fputc(0,wadstream);
					}*/
					lumpfilesize += 2;
				}
				src = (u8_t*)bm.bmBits;
				//check for type (norm, masked , shape osv)
				if (Type[0]=='S'){//Shape
					BYTE  *RottSpriteMemptr = BigMem1;
					memset(BigMem1,0,sizeof(BigMem1));
					//BYTE  *RottSpriteMemptr = GetMemBlock (lumpfilesize+32767);
					lumpfilesize = ConvertPCXIntoRottSprite (RottSpriteMemptr,src,0,bm.bmWidth*bm.bmHeight,origsize,bm.bmHeight,bm.bmWidth,leftoff,topoff);
					src = RottSpriteMemptr;
					for (i=0;i<lumpfilesize;i++)
					{
						c = *src++;
						fputc(c,wadstream);
					}			
					//FreeMemBlock (RottSpriteMemptr);
				}else if (Type[0]=='M'){//masked
					BYTE  *RottSpriteMemptr = BigMem1;
					memset(BigMem1,0,sizeof(BigMem1));
					//BYTE  *RottSpriteMemptr = GetMemBlock (lumpfilesize+32767);
					lumpfilesize = ConvertPCXIntoRottTransSprite (RottSpriteMemptr,src,0,bm.bmWidth*bm.bmHeight,origsize,bm.bmHeight,bm.bmWidth,leftoff,topoff,translevel);
					src = RottSpriteMemptr;

					for (i=0;i<lumpfilesize;i++)
					{
						c = *src++;
						fputc(c,wadstream);
					}

				}
				// EDGE: BigMem1 - byte cannot be used to convert to type lbm_t (??)
				else if (Type[0]=='L') 
				{//LBM format
					lbm_t *lbm = BigMem1;
					BYTE  *tgt = BigMem1;
					int ix;
					byte nboffull7Einline = bm.bmWidth/0x7E;
					byte nbofrest7Einline = bm.bmWidth-(nboffull7Einline*0x7E);

					memset(BigMem1,0,sizeof(BigMem1));

					lbm->width =  bm.bmWidth;
					lbm->height = bm.bmHeight ;
					src = BigMem1;

					for (i=0;i<4;i++)
					{
						c = *src++;
						fputc(c,wadstream);
					}

					for (i=0;i<256;i++)
					{
						fputc(rgb[i].rgbRed,wadstream);
						fputc(rgb[i].rgbGreen,wadstream);
						fputc(rgb[i].rgbBlue,wadstream);
					}

					lumpfilesize = 0; 
					//flip picture
				/*	FlipPictureUpsideDown(bm.bmWidth, bm.bmHeight,(u8_t*)bm.bmBits);
					src = (u8_t*)bm.bmBits;
					for (i = bm.bmHeight-1; i > 0; i-- ){
						memcpy(tgt ,(src+(i*bm.bmWidth)),bm.bmWidth);
						tgt += bm.bmWidth;
					}*/
					//make lbm data
					src = bm.bmBits;
					for (ix=0;ix<bm.bmHeight;ix++)
					{
						for (x = 0; x < nboffull7Einline;x++)
						{
							c = 0x7D;
							fputc(c,wadstream);
							for (i=0;i<0x7E;i++)
							{
								c = *src++;
								fputc(c,wadstream);
							}
						}

						c = nbofrest7Einline-1;
						fputc(c,wadstream);

						for (i=0;i<nbofrest7Einline;i++)
						{
							c = *src++;
							fputc(c,wadstream);
						}

						lumpfilesize += 1 + nboffull7Einline;
					}

					lumpfilesize += 4+768+(bm.bmWidth*bm.bmHeight);

				}
				// EDGE: BigMem1 - byte cannot be used to initialize type lpic_t (??)
				else if (Type[0]=='I')
				{//lpic_t
					BYTE  *tgt = BigMem1;
					lpic_t *lp = BigMem1;
					memset(BigMem1,0,sizeof(BigMem1));
					src = BigMem1;

					lp->width = bm.bmHeight ;
					lp->height = bm.bmWidth;
					lp->orgx = 0;
					lp->orgy = 0;

					for (i=0;i<8;i++)
					{
						c = *src++;
						fputc(c,wadstream);
					}

					FlipPictureUpsideDown(bm.bmWidth, bm.bmHeight,(u8_t*)bm.bmBits);

					src = (u8_t*)bm.bmBits;
					if (strstr(path_buffer,"PLANE") != 0){
						RotatePicture ( (u8_t*)src,bm.bmWidth, bm.bmHeight);//only plane.lmp
					}
					for (i=0;i<lumpfilesize;i++){
						c = *src++;
						fputc(c,wadstream);
					}
					lumpfilesize += 8;
				}else if (Type[0]=='P'){//    pic_t *pic;
					//BYTE  *tgt = BigMem1;
					c = bm.bmWidth/4 ;
					fputc(c,wadstream);
					c = bm.bmHeight;
					fputc(c,wadstream);

					FlipPictureUpsideDown(bm.bmWidth, bm.bmHeight,(u8_t*)bm.bmBits);

					src = (u8_t*)bm.bmBits;
					ConvertPicToPPic (bm.bmWidth,bm.bmHeight,src,BigMem1);

					src = (u8_t*)BigMem1;

					for (i=0;i<bm.bmWidth*bm.bmHeight;i++)

					{
						c = *src++;
						fputc(c,wadstream);
					}	

					fputc(0,wadstream);
					fputc(0,wadstream);
					lumpfilesize += 4;

				}
				else

				{//norm bmp data
					FlipPictureUpsideDown(bm.bmWidth, bm.bmHeight,(u8_t*)bm.bmBits);
					RotatePicture ( bm.bmBits,bm.bmWidth,bm.bmHeight);
					for (i=0;i<bm.bmWidth*bm.bmHeight;i++)
					{
						c = *src++;
						fputc(c,wadstream);
					}
				}
				DeleteObject(hBmp);


			}else{
				while( !feof( lumpstream ) )
				{
					c = fgetc(lumpstream);
					if (feof(lumpstream))
					{
						break;
					}
					fputc(c,wadstream);
				}

				fclose(lumpstream);
			}

	  }
//	  if (lumpfilepos==0){lumpfilepos += 0x0C;}//iwad header

	  sprintf(tmpbuf2,"%d",lumpfilesize);
	  SendMessage(hWndNewSizeLB, LB_ADDSTRING , 0, (LPARAM)tmpbuf2);
	  lumpfilepos += lumpfilesize;



   }
	wadinfo.infotableofs = ftell(wadstream);
		_flushall(  );
   fclose( txtstream );
//   fclose( tmpstream );
   fclose( wadstream );
//hWndNameLB,hWndSizeLB,hWndFilePosLB;

	Assem_listinfo ("Writing Lump infotable to IWAD");
	if((wadstream = fopen(wadfilename, "a+b" )) == NULL ){AddTxtToListbox ("Creation of ROTT WAD file failed!");return FALSE;}
    filehandle = _fileno(wadstream);
	fseek(wadstream,0,SEEK_END);	


	//write all table names to wad table in the end of wad
	x = SendMessage(hWndNameLB,LB_GETCOUNT,(WPARAM)0,(LPARAM) 0);
    for (lump=0;lump < x;lump++){
		WriteLumpToWadTable(filehandle,lump,hWndSizeLB,hWndFilePosLB,hWndNameLB);
   }


    fclose( wadstream );
	if((wadstream = fopen(wadfilename, "r+b" )) == NULL ){AddTxtToListbox ("Creation of ROTT WAD file failed!");return FALSE;}
    filehandle = _fileno(wadstream);

	//rewind(wadstream);
	wadinfo.numlumps = lump;

    _write(filehandle,&wadinfo,sizeof(wadinfo_t) );
	_flushall(  );
    fclose( wadstream );


	Assem_listinfo ("Creation of IWAD done.");
//	SendMessage(hWndStatus, SB_SETTEXT, 0, (LPARAM)"");
			  
	enableBreakButtons(hwnd,IDCANCEL);
	SetWindowText(GetDlgItem(hwnd,IDCANCEL),"Done");
}




bool WriteLumpToWadTable(int filehandle, int lump,HWND hWndSizeLB,HWND hWndFilePosLB,HWND hWndNameLB)
{
	static lumpinfo_t lumpinfo;
	int i;
	char tmpbuf1[128];

	SendMessage(hWndNewSizeLB,LB_GETTEXT,(WPARAM)lump,(LPARAM)tmpbuf1);
	lumpinfo.size = atol(tmpbuf1);

	//SendMessage(hWndNewSizeLB, LB_ADDSTRING , 0, (LPARAM)tmpbuf2);

	//SendMessage(hWndFilePosLB,LB_GETTEXT,(WPARAM)lump,(LPARAM)tmpbuf1);

	if (lumpinfo.size != 0){
		lumpinfo.filepos = oldfilepos;//atol(tmpbuf1);
	}else{
		lumpinfo.filepos = 0;
	}
	oldfilepos += lumpinfo.size;

	SendMessage(hWndNameLB,LB_GETTEXT,(WPARAM)lump,(LPARAM)tmpbuf1);  
	lstrcpy(lumpinfo.name ,tmpbuf1);
    i = _write(filehandle,&lumpinfo,sizeof(lumpinfo_t) );
	sprintf(tmpbuf1,"Writing tableinfoname -> %s",lumpinfo.name);
//	SendMessage(hWndStatus, SB_SETTEXT, 0, (LPARAM)tmpbuf1);

	return TRUE;
}


int  ExtractWadLumpTable(int filehandle,HWND hWndSizeLB,HWND hWndFilePosLB,HWND hWndNameLB)
{
	static wadinfo_t wadinfo;
	static lumpinfo_t lumpinfo;
	int numlumps,lump;
	long fpos;
	long lsize;
	char buf[512];

	_read(filehandle,&wadinfo,sizeof(wadinfo_t) );

	numlumps = wadinfo.numlumps ;
	_lseek(filehandle,wadinfo.infotableofs,SEEK_SET);

	lstrcpy(lumpinfo.name,"        ");
	//read table into LB's
	for (lump = 0; lump < numlumps ;lump++){
		//read lump info
		_read(filehandle,&lumpinfo,sizeof(lumpinfo) );
		SendMessage(hWndNameLB, LB_ADDSTRING , 0, (LPARAM)lumpinfo.name);
		sprintf(buf,"%d",lumpinfo.size );
		SendMessage(hWndSizeLB, LB_ADDSTRING , 0, (LPARAM)buf);
		sprintf(buf,"%d",lumpinfo.filepos );
		SendMessage(hWndFilePosLB, LB_ADDSTRING , 0, (LPARAM)buf);
	}
/*
	for (i=0;i<sizeof (lumpinfo_t);i++){
		tmpbuf1[i] = fgetc(instream);
	}
	ptr = (BYTE*)tmpbuf1;
	wadinfo = (wadinfo_t*)ptr;


	memset (tmpbuf2,0 ,sizeof(tmpbuf2) );
	fseek(instream,wadinfo->infotableofs ,SEEK_SET);

	AddTxtToListbox ("Writing Rott Lump Files ...");

	//read table into LB's
	for (lump = 0; lump < numlumps ;lump++){
		//read lump info
		for (i=0;i<sizeof (lumpinfo_t);i++){
			tmpbuf2[i] = fgetc(instream);
		}
		ptr = (BYTE*)tmpbuf2;
		lumpinfo = (lumpinfo_t*)ptr;
		//lstrcpy (lumpfilename,dirpath);
		//lstrcpy(lumpfilename,lumpinfo->name );
		//lstrcat(lumpfilename,".RLF" );
		//fputs(lumpfilename,txtstream );
		//count++;

//lstrcpy(lumpfilename,lumpinfo->name );
//lstrcat(lumpfilename,".RLF" );
SendMessage(hWndNameLB, LB_ADDSTRING , 0, (LPARAM)lumpinfo->name);

sprintf(tmpbuf1,"%d",lumpinfo->size );
*/



return 0;
}



void AddTxtToListbox(char *txt)
{

}


void CheckforSTRT (int lumpnb, char*name)
{
	if (lstrcmpi(name,"SMALLTRI")==0){
		GFX_norm_2_start = lumpnb;
		 
	}
	if (lstrcmpi(name,"TRI2PIC")==0){
		GFX_norm_2_stop = lumpnb;
		 
	}
	if (lstrcmpi(name,"ANIMSTRT")==0){
		GFX_animstart = lumpnb+1;
		 
	}
	if (lstrcmpi(name,"EXITSTRT")==0){
		GFX_animstop = lumpnb-1;
		//  no return
	}
	if (lstrcmpi(name,"NEW11")==0){
		GFX_m_newstart = lumpnb;
	}

	if (lstrcmpi(name,"NEW34")==0){
		GFX_m_newstop = lumpnb;
	}

	if (lstrcmpi(name,"WALLSTRT")==0){
		GFX_wallstart = lumpnb + 1;
		 
	}
	if (lstrcmpi(name,"WALLSTOP")==0){
		GFX_wallstop = lumpnb - 1;
		 
	}

	if (lstrcmpi(name,"UPDNSTRT")==0){
		GFX_floorstart = lumpnb + 1;
		 
	}
	if (lstrcmpi(name,"UPDNSTOP")==0){
		GFX_floorstop = lumpnb - 1;
		 
	}

	if (lstrcmpi(name,"SHAPSTRT")==0){
		GFX_shapestart = lumpnb + 1;
		 
	}
	if (lstrcmpi(name,"SHAPSTOP")==0){
		GFX_shapestop = lumpnb - 1;
		 
	}
	if (lstrcmpi(name,"GUNSTART")==0){
		GFX_gunsstart = lumpnb + 1;
		 
	}
	if (lstrcmpi(name,"PAL")==0){
		GFX_gunsstop = lumpnb - 1;
		 
	}

	if (lstrcmpi(name,"DOORSTRT")==0){
		GFX_doorstart = lumpnb + 1;
		 
	}
	if (lstrcmpi(name,"DOORSTOP")==0){
		GFX_doorstop = lumpnb - 1;
		 
	}

	if (lstrcmpi(name,"SIDESTRT")==0){
		GFX_sidestart = lumpnb + 1;
		 
	}
	if (lstrcmpi(name,"SIDESTOP")==0){
		GFX_sidestop = lumpnb - 1;
		 
	}

	if (lstrcmpi(name,"ELEVSTRT")==0){
		GFX_elevatorstart = lumpnb + 1;
		 
	}
	if (lstrcmpi(name,"ELEVSTOP")==0){
		GFX_elevatorstop = lumpnb - 1;
		 
	}

	if (lstrcmpi(name,"MASKSTRT")==0){
		GFX_maskedstart = lumpnb + 1;
		 
	}
	if (lstrcmpi(name,"MASKSTOP")==0){
		GFX_maskedstop = lumpnb - 1;
		 
	}

	GFX_maskedBotLimit = GFX_maskedstart;
	if (lstrcmpi(name,"MASKED3A")==0){
		GFX_maskedTopLimit = lumpnb;
		 
	}
	if (lstrcmpi(name,"RAILING")==0){
		GFX_maskedExclude = lumpnb;
		 
	}

	if (lstrcmpi(name,"EXITSTRT")==0){
		GFX_exitstart = lumpnb + 1;
		 
	}
	if (lstrcmpi(name,"EXITSTOP")==0){
		GFX_exitstop = lumpnb - 1;
		 
	}

	if (lstrcmpi(name,"HMSKSTRT")==0){
		GFX_himaskstart = lumpnb + 1;
		 
	}
    GFX_himaskstop =  GFX_gunsstart-2;

	if (lstrcmpi(name,"ABVMSTRT")==0){
		GFX_abovemaskwallstart = lumpnb + 1;
		 
	}
    GFX_abovemaskwallstop =  GFX_himaskstart-2;


	if (lstrcmpi(name,"ABVWSTRT")==0){
		GFX_abovewallstart = lumpnb + 1;
		 
	}
    GFX_abovewallstop =  GFX_abovemaskwallstart-2;


	if (lstrcmpi(name,"ADSTOP")==0){
		GFX_titlestart = lumpnb + 1;
		 
	}
	if (lstrcmpi(name,"SHARTITL")==0){
		GFX_titlestop = lumpnb - 1;
		 
	}


	if (lstrcmpi(name,"AP_WRLD")==0){
		GFX_ap_world = lumpnb ;
	}
	if (lstrcmpi(name,"PLANE")==0){
		GFX_plane = lumpnb ;
	}
	if (lstrcmpi(name,"AP_TITL")==0){
		GFX_ap_titl = lumpnb;
		 
	}
	if (lstrcmpi(name,"TRILOGO")==0){
		GFX_trilogo = lumpnb ;
		 
	}

	if (lstrcmpi(name,"SKYSTOP")==0){
		GFX_mishstart = lumpnb + 1;
		 
	}
	if (lstrcmpi(name,"SMALLC09")==0){
		GFX_mishstop = lumpnb;
		 
	}



	if (lstrcmpi(name,"WINDOW1")==0){
		GFX_pic_tstart = lumpnb;
		 
	}
	if (lstrcmpi(name,"WINDOW9")==0){
		GFX_pic_tstop = lumpnb;
		 
	}

	if (lstrcmpi(name,"NEWG1")==0){
		GFX_pic_tstart1 = lumpnb;
		 
	}
	if (lstrcmpi(name,"MINUS")==0){
		GFX_pic_tstop1 = lumpnb;
		 
	}

	if (lstrcmpi(name,"INFO1")==0){
		GFX_m_infostart = lumpnb;
		 
	}
	if (lstrcmpi(name,"INFO9")==0){
		GFX_m_infostop = lumpnb;
		 
	}


	if (lstrcmpi(name,"Q_YES")==0){
		GFX_m_q_yesstart = lumpnb;
		 
	}
	if (lstrcmpi(name,"Q_NO")==0){
		GFX_m_q_nostop = lumpnb;
		 
	}


	if (lstrcmpi(name,"MMBK")==0){
		GFX_TEST = lumpnb;
		 
	}







	if (lstrcmpi(name,"WAIT")==0){
		GFX_pic_tstart2 = lumpnb;
		 
	}
	if (lstrcmpi(name,"T_BAR")==0){
		GFX_pic_tstop2 = lumpnb;
		 
	}

	if (lstrcmpi(name,"SKYSTART")==0)
		GFX_sky_start = lumpnb+1;
	if (lstrcmpi(name,"SKYSTOP")==0)
		GFX_sky_stop = lumpnb-1;


	if (lstrcmpi(name,"PCSTART")==0)
		GFX_pcstart = lumpnb+1;
	if (lstrcmpi(name,"PCSTOP")==0)
		GFX_pcstop = lumpnb-1;
	if (lstrcmpi(name,"ADSTART")==0)
		GFX_adstart = lumpnb+1;
	if (lstrcmpi(name,"ADSTOP")==0)
		GFX_adstop = lumpnb-1;



	//HandleLumpPic_tBMP cases
	if (lstrcmpi(name,"SCREEN1")==0)
		GFX_T_Picsingle[0] = lumpnb;
	if (lstrcmpi(name,"DSTRIP")==0)
		GFX_T_Picsingle[1] = lumpnb;



	//HandleLumpLBM cases
	if (lstrcmpi(name,"BOOTBLOD")==0)
		GFX_T_LBMsingle[0] = lumpnb;
	if (lstrcmpi(name,"BOOTNORM")==0)
		GFX_T_LBMsingle[1] = lumpnb;
	if (lstrcmpi(name,"DEADBOSS")==0)
		GFX_T_LBMsingle[2] = lumpnb;
	if (lstrcmpi(name,"IMFREE")==0)
		GFX_T_LBMsingle[3] = lumpnb;
//	if (lstrcmpi(name,"REGEND")==0)
//		GFX_T_LBMsingle[4] = lumpnb;
//	if (lstrcmpi(name,"SHAREEND")==0)
//		GFX_T_LBMsingle[5] = lumpnb;


	//HandleLumpShapeBMP cases
	if (lstrcmpi(name,"CACHEBAR")==0)
		GFX_T_single[0] = lumpnb;
	if (lstrcmpi(name,"LED1")==0)
		GFX_T_single[1] = lumpnb;
	if (lstrcmpi(name,"LED2")==0)
		GFX_T_single[2] = lumpnb;
	if (lstrcmpi(name,"ESTERHAT")==0)
		GFX_T_single[3] = lumpnb;
	if (lstrcmpi(name,"AMFLAG")==0)
		GFX_T_single[4] = lumpnb;
	if (lstrcmpi(name,"SOMBRERO")==0)
		GFX_T_single[5] = lumpnb;
	if (lstrcmpi(name,"SANTAHAT")==0)
		GFX_T_single[6] = lumpnb;
	if (lstrcmpi(name,"WITCHHAT")==0)
		GFX_T_single[7] = lumpnb;
	if (lstrcmpi(name,"RSAC")==0)
		GFX_T_single[8] = lumpnb;
	if (lstrcmpi(name,"TMAIN")==0)
		GFX_T_single[9] = lumpnb;
	if (lstrcmpi(name,"TRESTORE")==0)
		GFX_T_single[10] = lumpnb;
	if (lstrcmpi(name,"TSAVE")==0)
		GFX_T_single[11] = lumpnb;
	if (lstrcmpi(name,"TCONTROL")==0)
		GFX_T_single[13] = lumpnb;
	if (lstrcmpi(name,"TCUSTOM")==0)
		GFX_T_single[14] = lumpnb;
	if (lstrcmpi(name,"TSTICK")==0)
		GFX_T_single[15] = lumpnb;
	if (lstrcmpi(name,"TMOUSE")==0)
		GFX_T_single[16] = lumpnb;
	if (lstrcmpi(name,"TPLAYER")==0)
		GFX_T_single[17] = lumpnb;
	if (lstrcmpi(name,"TMCARD")==0)
		GFX_T_single[18] = lumpnb;
	if (lstrcmpi(name,"TFXCARD")==0)
		GFX_T_single[19] = lumpnb;
	if (lstrcmpi(name,"TNUMCHAN")==0)
		GFX_T_single[20] = lumpnb;
	if (lstrcmpi(name,"TSTEREO")==0)
		GFX_T_single[21] = lumpnb;
	if (lstrcmpi(name,"TRES")==0)
		GFX_T_single[22] = lumpnb;
	if (lstrcmpi(name,"THARD")==0)
		GFX_T_single[23] = lumpnb;
	if (lstrcmpi(name,"TSENS")==0)
		GFX_T_single[24] = lumpnb;
	if (lstrcmpi(name,"TTHRESH")==0)
		GFX_T_single[25] = lumpnb;
	if (lstrcmpi(name,"TCALSTK")==0)
		GFX_T_single[26] = lumpnb;
	if (lstrcmpi(name,"TSCORES")==0)
		GFX_T_single[27] = lumpnb;
	if (lstrcmpi(name,"TMVOLUME")==0)
		GFX_T_single[28] = lumpnb;
	if (lstrcmpi(name,"TFVOLUME")==0)
		GFX_T_single[29] = lumpnb;
	if (lstrcmpi(name,"TMIDI")==0)
		GFX_T_single[30] = lumpnb;
	if (lstrcmpi(name,"TVOICES")==0)
		GFX_T_single[31] = lumpnb;
	if (lstrcmpi(name,"TFSPEED")==0)
		GFX_T_single[32] = lumpnb;
	if (lstrcmpi(name,"TOPTIONS")==0)
		GFX_T_single[33] = lumpnb;
	if (lstrcmpi(name,"TUSEROPT")==0)
		GFX_T_single[34] = lumpnb;
	if (lstrcmpi(name,"TDETAIL")==0)
		GFX_T_single[35] = lumpnb;
	if (lstrcmpi(name,"TBATTLE")==0)
		GFX_T_single[36] = lumpnb;
	if (lstrcmpi(name,"TCKEYB")==0)
		GFX_T_single[37] = lumpnb;
	if (lstrcmpi(name,"TNKEY")==0)
		GFX_T_single[38] = lumpnb;
	if (lstrcmpi(name,"TSKEY")==0)
		GFX_T_single[39] = lumpnb;
	if (lstrcmpi(name,"THELP")==0)
		GFX_T_single[40] = lumpnb;
	if (lstrcmpi(name,"TVLEVEL")==0)
		GFX_T_single[41] = lumpnb;
	if (lstrcmpi(name,"TPAS")==0)
		GFX_T_single[42] = lumpnb;
	if (lstrcmpi(name,"TVMENU")==0)
		GFX_T_single[43] = lumpnb;
	if (lstrcmpi(name,"TCBGAME")==0)
		GFX_T_single[44] = lumpnb;
	if (lstrcmpi(name,"TCBMODE")==0)
		GFX_T_single[45] = lumpnb;
	if (lstrcmpi(name,"TCBOPT")==0)
		GFX_T_single[46] = lumpnb;
	if (lstrcmpi(name,"TGRAV")==0)
		GFX_T_single[47] = lumpnb;
	if (lstrcmpi(name,"TSPEED")==0)
		GFX_T_single[48] = lumpnb;
	if (lstrcmpi(name,"TAMMO")==0)
		GFX_T_single[49] = lumpnb;
	if (lstrcmpi(name,"THPNTS")==0)
		GFX_T_single[50] = lumpnb;
	if (lstrcmpi(name,"TLIGHT")==0)
		GFX_T_single[51] = lumpnb;
	if (lstrcmpi(name,"TPNTGOAL")==0)
		GFX_T_single[52] = lumpnb;
	if (lstrcmpi(name,"TDAMG")==0)
		GFX_T_single[53] = lumpnb;
	if (lstrcmpi(name,"TTIME")==0)
		GFX_T_single[54] = lumpnb;
	if (lstrcmpi(name,"TCOLOR")==0)
		GFX_T_single[55] = lumpnb;
	if (lstrcmpi(name,"LEVELSEL")==0)
		GFX_T_single[56] = lumpnb;
	if (lstrcmpi(name,"CODENAME")==0)
		GFX_T_single[57] = lumpnb;
	if (lstrcmpi(name,"TRADICAL")==0)
		GFX_T_single[58] = lumpnb;

//	if (lstrcmpi(name,"IMFREE")==0)
//		GFX_T_single[59] = lumpnb;
//	if (lstrcmpi(name,"BOOTNORM")==0)
//		GFX_T_single[60] = lumpnb;
	if (lstrcmpi(name,"FINALE")==0)
		GFX_T_single[61] = lumpnb;
//	if (lstrcmpi(name,"DEADBOSS")==0)
//		GFX_T_single[62] = lumpnb;
	if (lstrcmpi(name,"TRANSMIT")==0)
		GFX_T_single[63] = lumpnb;
	if (lstrcmpi(name,"EPI12")==0)
		GFX_T_single[64] = lumpnb;
	if (lstrcmpi(name,"EPI23")==0)
		GFX_T_single[65] = lumpnb;


	if (lstrcmpi(name,"EPI34")==0)
		GFX_T_single[66] = lumpnb;
	if (lstrcmpi(name,"FINLDOOR")==0)
		GFX_T_single[67] = lumpnb;
	if (lstrcmpi(name,"FINLFIRE")==0)
		GFX_T_single[68] = lumpnb;
	if (lstrcmpi(name,"OUREARTH")==0)
		GFX_T_single[69] = lumpnb;
	if (lstrcmpi(name,"GROUPPIC")==0)
		GFX_T_single[56] = lumpnb;
	if (lstrcmpi(name,"DEADROBO")==0)
		GFX_T_single[70] = lumpnb;
	if (lstrcmpi(name,"DEADJOE")==0)
		GFX_T_single[71] = lumpnb;

	if (lstrcmpi(name,"DEADSTEV")==0)
		GFX_T_single[72] = lumpnb;
	if (lstrcmpi(name,"DEADTOM")==0)
		GFX_T_single[73] = lumpnb;
	if (lstrcmpi(name,"CUTMARK")==0)
		GFX_T_single[74] = lumpnb;
	if (lstrcmpi(name,"CUTPAT")==0)
		GFX_T_single[75] = lumpnb;
	if (lstrcmpi(name,"CUTMARI")==0)
		GFX_T_single[76] = lumpnb;
	if (lstrcmpi(name,"CUTANN")==0)
		GFX_T_single[77] = lumpnb;
	if (lstrcmpi(name,"CUTWILL")==0)
		GFX_T_single[78] = lumpnb;

	if (lstrcmpi(name,"CUTSTEV")==0)
		GFX_T_single[79] = lumpnb;
	if (lstrcmpi(name,"NICOLAS")==0)
		GFX_T_single[80] = lumpnb;
	if (lstrcmpi(name,"ONEYEAR")==0)
		GFX_T_single[81] = lumpnb;
	if (lstrcmpi(name,"BUDGCUT")==0)
		GFX_T_single[82] = lumpnb;
	if (lstrcmpi(name,"BINOCULR")==0)
		GFX_T_single[83] = lumpnb;
	if (lstrcmpi(name,"BINOSEE")==0)
		GFX_T_single[84] = lumpnb;
	if (lstrcmpi(name,"BOATGARD")==0)
		GFX_T_single[85] = lumpnb;
	if (lstrcmpi(name,"BOATBLOW")==0)
		GFX_T_single[86] = lumpnb;


	if (lstrcmpi(name,"ORDERBKG")==0)
		GFX_T_single[87] = lumpnb;
	if (lstrcmpi(name,"ORDER1")==0)
		GFX_T_single[88] = lumpnb;
	if (lstrcmpi(name,"ORDER2")==0)
		GFX_T_single[89] = lumpnb;
	if (lstrcmpi(name,"ORDER3")==0)
		GFX_T_single[90] = lumpnb;
	if (lstrcmpi(name,"ORDER4")==0)
		GFX_T_single[91] = lumpnb;

	if (lstrcmpi(name,"P_GMASK")==0)
		GFX_T_single[92] = lumpnb;
	if (lstrcmpi(name,"HELP")==0)
		GFX_T_single[93] = lumpnb;

	if (lstrcmpi(name,"TP1")==0)
		GFX_T_single[94] = lumpnb;
//	if (lstrcmpi(name,"DSTRIP")==0)
//		GFX_T_single[95] = lumpnb;
	if (lstrcmpi(name,"LEVELSEL")==0)
		GFX_T_single[96] = lumpnb;
//	if (lstrcmpi(name,"REGEND")==0)
//		GFX_T_single[87] = lumpnb;


	if (lstrcmpi(name,"SHAPSTRT")==0){
		GFX_norm_1_stop = lumpnb;
		return; 
	}

}





void LoadBMPtoBuffer(char*filename)
{
	HBITMAP hBmp;	
	BITMAP  bm;

	hBmp = (HBITMAP)LoadImage(NULL,filename,IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION|LR_LOADFROMFILE );
	GetObject(hBmp, sizeof(BITMAP), (LPSTR) &bm);

}

void HandleLumpBMP(	long filepos,long filesize,char*dirpath,char *tmpbuf1,int w, int h,FILE* txtstream,FILE* instream)
{
	FILE* outstream;
	char lumpfilename[256];

	if ((filesize>0x1FFFF)||(filesize<0))
		return ;


	if (filesize > 0)
	{
		if (lstrcmpi(tmpbuf1,"trilogo")==0)
		{
			sprintf(lumpfilename,"%s.BMP #I",tmpbuf1);
			fputs(lumpfilename,txtstream );
			fputs("\n",txtstream );
			lstrcpy (lumpfilename,dirpath);
			lstrcat(lumpfilename,tmpbuf1 );
			lstrcat(lumpfilename,".BMP");
		}
		else
		{
			lstrcat(tmpbuf1,".BMP");
			lstrcpy (lumpfilename,dirpath);
			lstrcat(lumpfilename,tmpbuf1 );
			fputs(tmpbuf1,txtstream );
			fputs("\n",txtstream );
		}

		fseek(instream,filepos,SEEK_SET);	
		Break_listinfo(lumpfilename);
		DeleteFile(lumpfilename);
		WriteResBMP(lumpfilename,instream,w,h);
	}
	else
	{
		lstrcpy (lumpfilename,dirpath);
		lstrcat(lumpfilename,tmpbuf1 );

		if((outstream = fopen(lumpfilename, "w+b" )) == NULL )
		{
			Break_listinfo ("Opening of ROTT LMP file failed!");
			return ;
		}

		fclose(outstream);

		fputs(tmpbuf1,txtstream );
		fputs("\n",txtstream );
	}

}


void HandleLumpShapeBMP(long filepos,long filesize,char*dirpath,char *tmpbuf1,int w, int h,FILE* txtstream,FILE* instream)
{
	FILE* outstream;
	char lumpfilename[256];
	byte *mem;
    char tmp[256];
    char tmp2[256*2];
	patch_t *p;
	transpatch_t *pt;
	BYTE  *RottSpriteMemptr;
	BYTE  *RottPCXptr;
	u8_t Type[2];
	int cofs,width;
	
	memset(BigMem1,0,sizeof(BigMem1));
	RottSpriteMemptr = BigMem1;//GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT,100000);	

	memset(BigMem2,0,sizeof(BigMem2));
	RottPCXptr = BigMem2;//GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT,100000+5000);


	fseek(instream,filepos,SEEK_SET);	
	fread( RottSpriteMemptr,1, filesize, instream );
/*
    if (filesize <= 10) 
		return 0; // Too short for a patch
*/
	p = (patch_t*)RottSpriteMemptr;
    cofs=IntelShort(p->collumnofs[0]);
    width=IntelShort(p->width);


//if (strstr(tmpbuf1,"BULLETHO")!=0)
//width=width;


	//if (p->collumnofs [0]>p->collumnofs [1]){
    if (cofs == (10 + width * 2)) {
		p = (patch_t*)RottSpriteMemptr;
		lstrcpy (Type,"S");
		ConvertRottSpriteIntoPCX (RottSpriteMemptr, RottPCXptr);
	}else{
		pt = (transpatch_t*)RottSpriteMemptr;
		lstrcpy (Type,"M");
		ConvertMaskedRottSpriteIntoPCX (RottSpriteMemptr, RottPCXptr);
	}

	if (filesize > 0)
	{
		lstrcat(tmpbuf1,".BMP");
		lstrcpy (lumpfilename,dirpath);
		lstrcat(lumpfilename,tmpbuf1 );
		Break_listinfo(lumpfilename);

		if (Type[0]=='S')
			sprintf(tmp2,"%s #%s #origs %d #loff %d #toff %d",tmpbuf1,Type,p->origsize,p->leftoffset,p->topoffset);
		else{
			sprintf(tmp2,"%s #%s #origs %d #loff %d #toff %d #trans %d",tmpbuf1,Type,pt->origsize,pt->leftoffset,pt->topoffset, pt->translevel);
		}
		
		fputs(tmp2,txtstream );
		fputs("\n",txtstream );

		DeleteFile(lumpfilename);

		WriteBufferBMP(lumpfilename,RottPCXptr,p->width,p->height);

	}
	else
	{
		lstrcpy (lumpfilename,dirpath);
		lstrcat(lumpfilename,tmpbuf1 );

		if((outstream = fopen(lumpfilename, "w+b" )) == NULL )
		{
			AddTxtToListbox ("Opening of ROTT LMP file failed!");
			return FALSE;
		}		 

		fclose(outstream);
		fputs(tmpbuf1,txtstream );
		fputs("\n",txtstream );
	}

	//GlobalFree(RottSpriteMemptr);//release mem;
	//GlobalFree(RottPCXptr);//release mem;
}

void HandleLumpShapeBMPSpecial(long filepos,long filesize,char*dirpath,char *tmpbuf1,int w, int h,FILE* txtstream,FILE* instream)
{

	if (strstr(tmpbuf1,"FINLDOOR")!=0)
	{//FINDRPAL
		byte tmppal[768];
		byte*tmpshadingtable = shadingtable;
		memcpy (tmppal,&origpal[0],768);
		fseek(instream,filepos+filesize,SEEK_SET);	
		fread( &origpal[0], 1, 768, instream );
		shadingtable = colormap + (1 << 12);
		VL_NormalizePalette (&origpal[0]);
		fseek(instream,filepos,SEEK_SET);	
		HandleLumpShapeBMP( filepos,filesize,dirpath,tmpbuf1, w,  h, txtstream,instream);
		memcpy (&origpal[0],tmppal,768);
		shadingtable = tmpshadingtable;
		return;
	}
	if (strstr(tmpbuf1,"FINLFIRE")!=0)
	{//FINFRPAL
		byte tmppal[768];
		byte*tmpshadingtable = shadingtable;
		memcpy (tmppal,&origpal[0],768);
		fseek(instream,filepos+filesize,SEEK_SET);	
		fread( &origpal[0], 1, 768, instream );
		shadingtable = colormap + (1 << 12);
		VL_NormalizePalette (&origpal[0]);
		fseek(instream,filepos,SEEK_SET);	
		HandleLumpShapeBMP( filepos,filesize,dirpath,tmpbuf1, w,  h, txtstream,instream);
		memcpy (&origpal[0],tmppal,768);
		shadingtable = tmpshadingtable;
		return;
	}
	if (strstr(tmpbuf1,"NICOLAS")!=0)
	{//NICPAL
		byte tmppal[768];
		byte*tmpshadingtable = shadingtable;
		memcpy (tmppal,&origpal[0],768);
		memcpy(&origpal[0],W_CacheLumpName("nicpal",PU_CACHE, CvtNull, 1),768);
		shadingtable = colormap + (1 << 12);
		VL_NormalizePalette (&origpal[0]);
		fseek(instream,filepos,SEEK_SET);	
		HandleLumpShapeBMP( filepos,filesize,dirpath,tmpbuf1, w,  h, txtstream,instream);
		memcpy (&origpal[0],tmppal,768);
		shadingtable = tmpshadingtable;
		return;

	}
	if (strstr(tmpbuf1,"BOATBLOW")!=0)
	{//BOATPAL
		byte tmppal[768];
		byte*tmpshadingtable = shadingtable;
		memcpy (tmppal,&origpal[0],768);
		fseek(instream,filepos+filesize,SEEK_SET);	
		fread( &origpal[0], 1, 768, instream );
		shadingtable = colormap + (1 << 12);
		VL_NormalizePalette (&origpal[0]);
		fseek(instream,filepos,SEEK_SET);	
		HandleLumpShapeBMP( filepos,filesize,dirpath,tmpbuf1, w,  h, txtstream,instream);
		memcpy (&origpal[0],tmppal,768);
		shadingtable = tmpshadingtable;
		return;
	}
		
	HandleLumpShapeBMP( filepos,filesize,dirpath,tmpbuf1, w,  h, txtstream,instream);

}



byte BigMem6[0x1FFFF];
byte BigMem7[0x1FFFF];

void HandleLumpPic_tBMP(int lump,long filepos,long filesize,char*dirpath,char *tmpbuf1,int w, int h,FILE* txtstream,FILE* instream)
{
	BYTE  *shape,*tgt;
	pic_t *Win1;
	char lumpfilename[256];
    char tmp2[256*2];

	//shape = GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT,0X1FFFF);	
	//tgt = GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT,0X1FFFF);
	memset(BigMem6,0,sizeof(BigMem6));
	memset(BigMem7,0,sizeof(BigMem7));
	shape = BigMem6;
	tgt = BigMem7;

	fseek(instream,filepos,SEEK_SET);	
	fread( shape,1, filesize, instream );

	Win1 = (pic_t *) shape;
			
	if (strstr(tmpbuf1,".BMP") == 0)
		lstrcat(tmpbuf1,".BMP");
	lstrcpy (lumpfilename,dirpath);
	lstrcat(lumpfilename,tmpbuf1 );
	sprintf(tmp2,"%s #%P",tmpbuf1);
	fputs(tmp2,txtstream );
	fputs("\n",txtstream );

	DeleteFile(lumpfilename);

	ConvertPPicToPic (Win1->width, Win1->height,shape+2,tgt);



	WriteBufferBMP(lumpfilename,tgt,Win1->width*4,Win1->height);
//	GlobalFree(shape);//release mem;
//	GlobalFree(tgt);//release mem;
tgt = tgt ;
}



void Break_listinfo(char*txt1)
{
	int x;	
	MSG msg;
	SendMessage(hWndLBBreak, LB_ADDSTRING , 0, (LPARAM)txt1);
	x = SendMessage(hWndLBBreak,LB_GETCOUNT,(WPARAM)0,(LPARAM) 0);
	SendMessage(hWndLBBreak,LB_SETCURSEL,(WPARAM)x-1, (LPARAM) 0);
	
	UpdateWindow(hWndLBBreak);

	PeekMessage(&msg, NULL, 0, 0,PM_REMOVE);
	TranslateMessage(&msg);  // Translates virtual key codes
	DispatchMessage(&msg);   // Dispatches message to window
}

// EDGE:
// This should be absorbed by a standard sprintf (the EDGE2 console window class), and by I_Debugf() for the console logger!
void Assem_listinfo(char*txt1)
{
	int x;	
	MSG msg;
	SendMessage(hWndLBAssem, LB_ADDSTRING , 0, (LPARAM)txt1);
	x = SendMessage(hWndLBAssem,LB_GETCOUNT,(WPARAM)0,(LPARAM) 0);
	SendMessage(hWndLBAssem,LB_SETCURSEL,(WPARAM)x-1, (LPARAM) 0);
	
	UpdateWindow(hWndLBAssem);
/*
	PeekMessage(&msg, NULL, 0, 0,PM_REMOVE);
	TranslateMessage(&msg);  // Translates virtual key codes
	DispatchMessage(&msg);   // Dispatches message to window
	*/
}


void HandleLumpIntro(int lump,long filepos,long filesize,char*dirpath,char *tmpbuf1,int w, int h,FILE* txtstream,FILE* instream)
{ 
	lpic_t *lp;
	char header[64];
	char lumpfilename[260];
	byte tmppal[768];
	byte*tmpshadingtable = shadingtable;

	fseek(instream,filepos,SEEK_SET);	
	fread( header, 1, 60, instream );
	lp = (lpic_t*)header;

	lstrcpy (lumpfilename,dirpath);
	lstrcat(lumpfilename,tmpbuf1 );
	lstrcat(lumpfilename,".BMP");
	Break_listinfo(lumpfilename);
	DeleteFile(lumpfilename);
	fseek(instream,filepos+8,SEEK_SET);	


	if (lump == GFX_plane)
	{
		char *l = BigMem2;
		fread( l, 1, filesize, instream );
		RotatePicture (l, lp->height,lp->width);
		WriteBufferBMP(lumpfilename,l,lp->width, lp->height);
		sprintf(lumpfilename,"%s.BMP #I",tmpbuf1);
		fputs(lumpfilename,txtstream );
		fputs("\n",txtstream );
		return;
	}


	memcpy (tmppal,&origpal[0],768);

	memcpy (&origpal[0], W_CacheLumpName ("ap_pal", PU_CACHE, CvtNull, 1), 768);
	shadingtable = colormap + (1 << 12);
	VL_NormalizePalette (&origpal[0]);

	WriteResBMP(lumpfilename,instream,lp->height,lp->width);
	
	memcpy (&origpal[0],tmppal,768);
	shadingtable = tmpshadingtable;

	sprintf(lumpfilename,"%s.BMP #I",tmpbuf1);
	fputs(lumpfilename,txtstream );
	fputs("\n",txtstream );

}


void HandleLumpLBM(int lump,long filepos,long filesize,char*dirpath,char *tmpbuf1,int w, int h,FILE* txtstream,FILE* instream)
{ 
	lbm_t *lbm;
	char header[64];
	char lumpfilename[260];
	byte tmppal[768];
  // byte *screen = (byte *)bufferofs;
   byte *orig;
	int  count;
	byte b,  rept;
   byte *source;
   byte *buf = BigMem2;
   int  ht;
	byte*tmpshadingtable = shadingtable;
   int  x = 0;
   int  y;
   byte *origbuf;
   byte pal[768];

	lstrcpy (lumpfilename,dirpath);
	lstrcat(lumpfilename,tmpbuf1 );
	lstrcat(lumpfilename,".BMP");
	Break_listinfo(lumpfilename);
	DeleteFile(lumpfilename);
	memcpy (tmppal,&origpal[0],768);
	memset(BigMem1,0,sizeof(BigMem1));
	memset(BigMem2,0,sizeof(BigMem2));

	fseek(instream,filepos,SEEK_SET);	
	fread( BigMem1, 1, filesize, instream );

	lbm = (lbm_t*)BigMem1;
	source = (byte *)&lbm->data;
	ht = lbm->height;


   memcpy(&origpal[0],lbm->palette,768);
   VL_NormalizePalette (&origpal[0]);
   while (ht--)
   {
      count = 0;

   	do
	   {
		   rept = *source++;

   		if (rept > 0x80)
	   	{
		   	rept = (rept^0xff)+2;
			   b = *source++;
   			memset (buf, b, rept);
	   		buf += rept;
		   }
   		else if (rept < 0x80)
	   	{
		   	rept++;
			   memcpy (buf, source, rept);
   			buf += rept;
	   		source += rept;
		   }
   		else
	   		rept = 0;               // rept of 0x80 is NOP

		   count += rept;

   	} while (count < lbm->width);
   }

	WriteBufferBMP(lumpfilename,BigMem2,lbm->width,lbm->height);

	memcpy (&origpal[0],tmppal,768);
	shadingtable = tmpshadingtable;

	sprintf(lumpfilename,"%s.BMP #L",tmpbuf1);
	fputs(lumpfilename,txtstream );
	fputs("\n",txtstream );
/*
	byte*tmpshadingtable = shadingtable;

	memset(BigMem1,0,sizeof(BigMem1));
	memcpy (tmppal,&origpal[0],768);

	fseek(instream,filepos,SEEK_SET);	
	fread( BigMem1, 1, filesize, instream );

	lbm = (lbm_t*)BigMem1;

	lstrcpy (lumpfilename,dirpath);
	lstrcat(lumpfilename,tmpbuf1 );
	lstrcat(lumpfilename,".BMP");
	Break_listinfo(lumpfilename);
	DeleteFile(lumpfilename);

	memcpy (&origpal[0], lbm->palette, 768);
	shadingtable = colormap + (1 << 12);
	VL_NormalizePalette (&origpal[0]);

	fseek(instream,filepos+768+4,SEEK_SET);	
	//WriteResBMP(lumpfilename,instream,lbm->width,lbm->height);
	WriteBufferBMP(lumpfilename,&lbm->data,lbm->width,lbm->height);
	memcpy (&origpal[0],tmppal,768);
	shadingtable = tmpshadingtable;

	sprintf(lumpfilename,"%s.BMP #I",tmpbuf1);
	fputs(lumpfilename,txtstream );
	fputs("\n",txtstream );*/

}

