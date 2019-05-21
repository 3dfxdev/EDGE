/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "epi.h"
#include "errno.h"
#include <vitasdk.h>
#include <vita2d.h>

#define BIGSTACK_SIZE 20 * 1024 * 1024

byte sys_bigstack[BIGSTACK_SIZE];
int sys_bigstack_cursize;
uint8_t is_uma0 = 0;
extern int msaa;

int old_char;
extern int setup_cursor;
extern int lanConfig_cursor;
int isKeyboard;
extern uint64_t rumble_tick;
extern cvar_t psvita_touchmode;
extern cvar_t vid_vsync;
extern int scr_width;
extern int scr_height;
extern int cfg_width;
extern int cfg_height;

bool isDedicated;
uint64_t initialTime = 0;
int hostInitialized = 0;
SceCtrlData pad, oldpad;
/*
===============================================================================

FILE IO

===============================================================================
*/

#define PLATFORM_PSVITA	0x00010000

#define MAX_HANDLES             10
FILE    *sys_handles[MAX_HANDLES];

int Sys_FindHandle(void)
{
	int i;

	for (i=1;i<MAX_HANDLES;i++)
		if (!sys_handles[i]) return i;
	Sys_Error("out of handles");
	return -1;
}

/* void Log(const char *format, ...) {
#ifdef DEBUG
	__gnuc_va_list arg;
	int done;
	va_start(arg, format);
	char msg[512];
	done = vsprintf(msg, format, arg);
	va_end(arg);
	int i;
	sprintf(msg, "%s\n", msg);
	FILE* log = fopen("ux0:/data/Quake/log.txt", "a+");
	if (log != NULL) {
		fwrite(msg, 1, strlen(msg), log);
		fclose(log);
	}
#endif
} */

/* int Sys_FileLength(FILE *f)
{
	int             pos;
	int             end;

	pos = ftell(f);
	fseek(f, 0, SEEK_END);
	end = ftell(f);
	fseek(f, pos, SEEK_SET);

	return end;
}

int Sys_FileOpenRead(char *path, int *hndl)
{
	FILE    *f;
	int             i;

	i = Sys_FindHandle();

	f = fopen(path, "rb");
	if (!f)
	{
		*hndl = -1;
		return -1;
	}
	sys_handles[i] = f;
	*hndl = i;

	return Sys_FileLength(f);
}

int Sys_FileOpenWrite(char *path)
{
	FILE    *f;
	int             i;

	i = Sys_FindHandle();

	f = fopen(path, "wb");
	if (!f)
		Sys_Error("Error opening %s: %s", path, strerror(errno));
	sys_handles[i] = f;

	return i;
}

void Sys_FileClose(int handle)
{
	fclose(sys_handles[handle]);
	sys_handles[handle] = NULL;
}

void Sys_FileSeek(int handle, int position)
{
	fseek(sys_handles[handle], position, SEEK_SET);
}

int Sys_FileRead(int handle, void *dest, int count)
{
	return fread(dest, 1, count, sys_handles[handle]);
}

int Sys_FileWrite(int handle, void *data, int count)
{
	return fwrite(data, 1, count, sys_handles[handle]);
}

int     Sys_FileTime(char *path)
{
	FILE    *f;

	f = fopen(path, "rb");
	if (f)
	{
		fclose(f);
		return 1;
	}

	return -1;
} */

void Sys_mkdir(char *path)
{
	sceIoMkdir(path, 0777);
}

void Sys_BigStackRewind(void)
{
	sys_bigstack_cursize = 0;
}

void* Sys_BigStackAlloc(int size, char* purpose)
{
	void* p;

	p = 0;
	if (sys_bigstack_cursize + size < BIGSTACK_SIZE)
	{
		p = sys_bigstack + sys_bigstack_cursize;
		sys_bigstack_cursize = sys_bigstack_cursize + size;
	}
	else
	{
		I_Error("Sys_BigStackAlloc: %s - failed on %i bytes", purpose, size);
	};
	return p;
}

void Sys_BigStackFree(int size, char* purpose)
{
	if (sys_bigstack_cursize - size >= 0)
	{
		sys_bigstack_cursize = sys_bigstack_cursize - size;
	}
	else
	{
		I_Error("Sys_BigStackFree: %s - underflow on %i bytes", purpose, sys_bigstack_cursize - size);
	};
}
// <<< FIX
}

