//----------------------------------------------------------------------------
//  EDGE Misc: Screenshots, Menu and defaults Code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2001  The EDGE Team.
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
//----------------------------------------------------------------------------
//
// -MH- 1998/07/02  Added key_flyup and key_flydown
// -MH- 1998/07/02 "shootupdown" --> "true3dgameplay"
// -ACB- 2000/06/02 Removed Control Defaults
//

#include "i_defs.h"
#include "m_misc.h"

#include "con_main.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "dstrings.h"
#include "hu_stuff.h"
#include "m_argv.h"
#include "m_menu.h"
#include "m_option.h"
#include "m_swap.h"
#include "p_spec.h"
#include "w_wad.h"
#include "r_main.h"
#include "r2_defs.h"
#include "s_sound.h"
#include "v_ctx.h"
#include "v_res.h"
#include "v_colour.h"
#include "w_image.h"
#include "w_wad.h"
#include "wp_main.h"
#include "z_zone.h"

//
// DEFAULTS
//
int usemouse;
int usejoystick;

int cfgnormalfov;
int cfgzoomedfov;

// toggled by autorun button.
bool autorunning = false;

bool display_disk = false;
static const image_t *disk_image = NULL;
static const image_t *air_images[21] = { NULL, };

unsigned short save_screenshot[160][100];
bool save_screenshot_valid = false;

#ifdef LINUX
char *mousetype;
char *mousedev;
char *videoInterface;
char *vid_path;
#endif

// -ACB- 1999/09/19 Sound API
int dummysndchan;
static int cfgsound;
static int cfgmusic;

default_t defaults[] =
{
    {"screenwidth",  &SCREENWIDTH, 320},
    {"screenheight", &SCREENHEIGHT, 200},
    {"screendepth",  &SCREENBITS, 8},
    {"windowed", (int *)&SCREENWINDOW, 0},
    {"boom_compatility", (int *)&global_flags.compat_mode, 0},
 
    {"mouse_sensitivity", &mouseSensitivity, 5},
    {"sfx_volume",        &cfgsound,         8},
    {"music_volume",      &cfgmusic,         8},
    {"show_messages",     &showMessages,     1},

//    {"autorun", (int *)&autorunning, (int)false},

    {"swapstereo", (int *)&swapstereo, 0},
    {"invertmouse", (int *)&invertmouse, (int)false},
    {"mlookspeed", &mlookspeed, 1000 / 64},
    {"translucency", (int *)&global_flags.trans, 1},
   
    // -ES- 1998/11/28 Save fade settings
    {"telept_effect", &telept_effect, 0},
    {"telept_reverse", &telept_reverse, 0},
    {"telept_flash", &telept_flash, 1},
    {"wipe_method", &wipe_method, WIPE_Melt},
    {"wipe_reverse", &wipe_reverse, 0},
    {"crosshair", &crosshair, 0},
    {"stretchsky", (int *)&global_flags.stretchsky, 1},
    {"rotatemap", (int *)&rotatemap, 0},
    {"newhud", (int *)&newhud, 0},
    {"respawnsetting", (int *)&global_flags.res_respawn, 0},
    {"itemrespawn", (int *)&global_flags.itemrespawn, 0},
    {"respawn", (int *)&global_flags.respawn, 0},
    {"fastparm", (int *)&global_flags.fastparm, 0},
    {"grav", &global_flags.menu_grav, MENU_GRAV_NORMAL},
    {"true3dgameplay", (int *)&global_flags.true3dgameplay, 0},
    {"autoaim", (int *)&global_flags.autoaim, 1},
    {"missileteleport", &missileteleport, 0},
    {"teleportdelay", &teleportdelay, 0},

    // -KM- 1998/07/21 Save the blood setting
    {"blood", (int *)&global_flags.more_blood, 0},
    {"extra", (int *)&global_flags.have_extra, 1},
    {"shadows", (int *)&global_flags.shadows, 1},
    {"halos", (int *)&global_flags.halos, 0},
    {"weaponkick", (int *)&global_flags.kicking, 1},
    {"jumping", (int *)&global_flags.jump, 1},
    {"crouching", (int *)&global_flags.crouch, 1},
    {"mipmapping", &use_mipmapping, 1},
    {"smoothing", (int *)&use_smoothing, 1},
    {"dlights", (int *)&use_dlights, 0},
    {"dither", (int *)&use_dithering, 0},
    {"detail_level", (int *)&detail_level, 1},

#ifdef LINUX
    // -AJA- FIXME: gotta be a better way than this...
    {"mousedev", (int *)&mousedev, (int)"/dev/ttyS0"},
    {"mousetype", (int *)&mousetype, (int)"microsoft"},

    {"video", (int *)&videoInterface, (int)"x"},
    {"vid_path", (int *)&vid_path, (int)"/usr/lib/games/doom"},
#endif

    // -KM- 1998/09/01 Useless mouse/joy stuff removed,
    //                 analogue binding added
    {"use_mouse",   &usemouse,    1},
    {"mouse_xaxis", &mouse_xaxis, 0},
    {"mouse_yaxis", &mouse_yaxis, 0},

    // -ACB- 1998/09/06 Two-stage turning & Speed controls added
    {"twostage_turning",  (int *)&stageturn, 0},
    {"forwardmove_speed", &forwardmovespeed, 0},
    {"angleturn_speed",   &angleturnspeed,   0},
    {"sidemove_speed",    &sidemovespeed,    0},

    {"use_joystick", &usejoystick, 0},
    {"joy_xaxis", &joy_xaxis, 0},
    {"joy_yaxis", &joy_yaxis, 0},

    {"screenblocks", &screenblocks, 10},
    // -ES- 1999/03/30 Added fov stuff.
    {"fieldofview", &cfgnormalfov, 90},
    {"zoomedfieldofview", &cfgzoomedfov, 10},

    {"darken_screen", &darken_screen, 1},
    {"snd_channels",  &dummysndchan, 3},
    {"usegamma",      &current_gamma, 0},

    {"key_right",      &key_right,      0},
    {"key_left",       &key_left,       0},
    {"key_up",         &key_up,         0},
    {"key_down",       &key_down,       0},
    {"key_lookup",     &key_lookup,     0},
    {"key_lookdown",   &key_lookdown,   0},
    {"key_lookcenter", &key_lookcenter, 0},

    // -ES- 1999/03/28 Zoom Key
    {"key_zoom",        &key_zoom,        0},
    {"key_strafeleft",  &key_strafeleft,  0},
    {"key_straferight", &key_straferight, 0},

    // -ACB- for -MH- 1998/07/02 Flying Keys
    {"key_flyup",   &key_flyup,           0},
    {"key_flydown", &key_flydown,         0},

    {"key_fire",       &key_fire,         0},
    {"key_use",        &key_use,          0},
    {"key_strafe",     &key_strafe,       0},
    {"key_speed",      &key_speed,        0},
    {"key_autorun",    &key_autorun,      0},
    {"key_nextweapon", &key_nextweapon,   0},

    {"key_jump",      &key_jump,          0},
    {"key_180",       &key_180,           0},
    {"key_map",       &key_map,           0},
    {"key_talk",      &key_talk,          0},
    {"key_mlook",     &key_mlook,         0},  // -AJA- 1999/07/27.
    {"key_secondatk", &key_secondatk,     0},  // -AJA- 2000/02/08.

    {"chatmacro0", (int *)&chat_macros[0], 0},  //(int) HUSTR_CHATMACRO0 },
    {"chatmacro1", (int *)&chat_macros[1], 0},  //(int) HUSTR_CHATMACRO1 },
    {"chatmacro2", (int *)&chat_macros[2], 0},  //(int) HUSTR_CHATMACRO2 },
    {"chatmacro3", (int *)&chat_macros[3], 0},  //(int) HUSTR_CHATMACRO3 },
    {"chatmacro4", (int *)&chat_macros[4], 0},  //(int) HUSTR_CHATMACRO4 },
    {"chatmacro5", (int *)&chat_macros[5], 0},  //(int) HUSTR_CHATMACRO5 },
    {"chatmacro6", (int *)&chat_macros[6], 0},  //(int) HUSTR_CHATMACRO6 },
    {"chatmacro7", (int *)&chat_macros[7], 0},  //(int) HUSTR_CHATMACRO7 },
    {"chatmacro8", (int *)&chat_macros[8], 0},  //(int) HUSTR_CHATMACRO8 },
    {"chatmacro9", (int *)&chat_macros[9], 0}  //(int) HUSTR_CHATMACRO9 }
};

int numdefaults;
const char *cfgfile = NULL;

// TGA Graphics Header
typedef enum
{
	tga_id = 0,
	tga_cmaptype = 1,
	tga_imgtype = 2,
	tga_cmapfirst = 3,
	tga_cmaplen = 5,
	tga_cmapsize = 7,
	tga_imgorgx = 8,
	tga_imgorgy = 10,
	tga_imgwidth = 12,
	tga_imgheight = 14,
	tga_imgbpp = 16,
	tga_imgbits = 17
}
tgaheader_e;

// ======================= INTERNALS =======================

//
// WriteTGAFile
//
// Each pixel in `buffer' is three bytes (R,G,B).
//
static void WriteTGAFile(const char *filename, int width, int height, 
						 byte *buffer)
{
	FILE *fp;
	byte tgahead[18], *cr, *cb, tmp;
	int i;

	if ((fp = fopen(filename, "wb")) != NULL)
	{
		Z_Clear(tgahead, byte, 18);
    
		tgahead[tga_imgtype] = 2;  // Unmapped RGB
		tgahead[tga_imgbpp] = 24;  // 3 byte colours (B,G,R)
    
		*((short *) &tgahead[tga_imgwidth])  = width;
		*((short *) &tgahead[tga_imgheight]) = height;

		// flip the blue and red color components

		cr = &buffer[2];
		cb = &buffer[0];
    
		for (i=0; i < (width*height); i++, cr += 3, cb += 3)
		{
			tmp = *cb;
			*cb = *cr;
			*cr = tmp;
		}

		fwrite(tgahead, 18, 1, fp);
		fwrite(buffer, sizeof(byte)*width*height*3, 1, fp);
		fclose(fp);
	}
}

// ===================== END OF INTERNALS =====================

//
// M_WriteFile
//
bool M_WriteFile(char const *name, void *source, int length)
{
	int handle;
	int count;

	handle = open(name, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0666);

	if (handle == -1)
		return false;

	count = write(handle, source, length);
	close(handle);

	if (count < length)
		return false;

	return true;
}

//
// M_ReadFile
//
int M_ReadFile(char const *name, byte ** buffer)
{
	int handle, count, length;
	struct stat fileinfo;
	byte *buf;

	handle = open(name, O_RDONLY | O_BINARY);

	if (handle == -1)
		I_Error("Couldn't read file %s", name);
	if (fstat(handle, &fileinfo) == -1)
		I_Error("Couldn't read file %s", name);
	length = fileinfo.st_size;
	buf = Z_New(byte, length);
	count = read(handle, buf, length);
	close(handle);

	if (count < length)
		I_Error("Couldn't read file %s", name);

	*buffer = buf;
	return length;
}

//
// M_SaveDefaults
//
void M_SaveDefaults(void)
{
	int i;
	int v;
	FILE *f;

	// Don't want to save settings in a network game: might not
	// be ours.
	if (netgame)
		return;

	// -ACB- 1999/09/24 idiot proof checking as required by MSVC
	DEV_ASSERT2(cfgfile);

	// -ACB- 1999/10/10 Sound API Values need to be set
	cfgmusic = S_GetMusicVolume();
	cfgsound = S_GetSfxVolume();

	f = fopen(cfgfile, "w");
	if (!f)
	{
		I_Warning("Couldn't open config file %s for writing.", cfgfile);
		return;  // can't write the file, but don't complain
	}

	for (i = 0; i < numdefaults; i++)
	{
		if ((defaults[i].defaultvalue > -0xfff
			 && defaults[i].defaultvalue < 0xfff) || (memcmp(defaults[i].name, "key_", 4) == 0))
		{
			v = *defaults[i].location;

			if (memcmp(defaults[i].name, "key_", 4) == 0)
				fprintf(f, "%s\t\t0x%X\no%s\t\t0x%X\n", defaults[i].name, v, defaults[i].name, v << 16);
			else
				fprintf(f, "%s\t\t%i\n", defaults[i].name, v);
		}
		else
		{
			fprintf(f, "%s\t\t\"%s\"\n", defaults[i].name,
                    *(char **)(defaults[i].location));
		}
	}

	fclose(f);
}

//
// M_LoadDefaults
//
bool M_LoadDefaults(void)
{
	int i;
	FILE *f;
	char def[80];
	char strparm[100];
	char *newstring = 0;
	int parm;
	bool isstring;

	// set everything to base values
	numdefaults = sizeof(defaults) / sizeof(defaults[0]);
	for (i = 0; i < numdefaults; i++)
		*defaults[i].location = defaults[i].defaultvalue;

	I_Printf("  from %s\n", cfgfile);

	// read the file in, overriding any set defaults
	f = fopen(cfgfile, "r");
	if (f)
	{
		while (!feof(f))
		{
			isstring = false;
			if (fscanf(f, "%79s %[^\n]\n", def, strparm) != 2)
				continue;

			if (strparm[0] == '"')
			{
				// get a string default
				isstring = true;
				// overwrite the last "
				strparm[strlen(strparm) - 1] = 0;
				// skip the first "
				newstring = Z_StrDup(strparm + 1);
			}
			else if (strparm[0] == '0' && strparm[1] == 'x')
				sscanf(strparm + 2, "%x", &parm);
			else
				sscanf(strparm, "%i", &parm);

			// backwards compatibility
			if (strcmp(def, "bpp") == 0)
			{
				SCREENBITS = parm * 8;
				continue;
			}
      
			for (i = 0; i < numdefaults; i++)
			{
				if (!strcmp(def, defaults[i].name))
				{
					if (!isstring)
					{
						if (memcmp(defaults[i].name, "key_", 4) != 0)
						{
							*defaults[i].location = parm;
						}
						else
						{
							if (parm != 0)
								*defaults[i].location = parm;
						}
					}
					else
						*defaults[i].location = (int)newstring;
					break;
				}
				if (def[0] == 'o')
					if (!strcmp(def + 1, defaults[i].name))
						if (!isstring)
							if (memcmp(defaults[i].name, "key_", 4) == 0)
								if (parm != 0)
									*defaults[i].location |= parm << 16;
			}
		}

		fclose(f);
	}
	else
	{
		I_Warning("Couldn't open config file %s for reading.\n", cfgfile);
		I_Warning("Resetting config to RECOMMENDED values...\n");

		M_ResetToDefaults(0);
	}

	// -ACB- 1999/10/10 Sound API Values need to be set
	S_SetMusicVolume(cfgmusic);
	S_SetSfxVolume(cfgsound);

	return true;
}

//
// M_DisplayDisk
//
// Displays disk during loading...
//
void M_DisplayDisk(void)
{
	int w, h;

	if (!graphicsmode)
		return;

	if (!display_disk)
		return;
   
	if (!disk_image)
		disk_image = W_ImageFromPatch("STDISK");

	// reset flag
	display_disk = false;

	w = IM_WIDTH(disk_image);
	h = IM_HEIGHT(disk_image);

	VCTX_Image320(314 - w, 164 - h, w, h, disk_image);
}

//
// M_DisplayAir
//
// Displays air indicator when underwater
//
void M_DisplayAir(void)
{
	int i;
  
	if (!graphicsmode)
		return;

	if (consoleplayer->playerstate != PST_LIVE)
		return;

	if (! consoleplayer->underwater)
		return;

	// load in patches for air indicator
	if (!air_images[0])
	{
		for (i=1; i <= 21; i++)
		{
			char buffer[16];
			sprintf(buffer, "AIRBAR%02d", i);
			air_images[i - 1] = W_ImageFromPatch(buffer);
		}
	}

	DEV_ASSERT2(consoleplayer->mo);

	i = 21;

	if (consoleplayer->air_in_lungs > 0)
	{
		int nom = consoleplayer->air_in_lungs;
		int denom = consoleplayer->mo->info->lung_capacity;
    
		i = 1 + (20 * (denom - nom) / denom);

		DEV_ASSERT2(1 <= i && i <= 20);
	}
  
	VCTX_ImageEasy320(0, 0, air_images[i - 1]);
}

#define PIXEL_RED(pix)  (playpal_data[0][pix][0])
#define PIXEL_GRN(pix)  (playpal_data[0][pix][1])
#define PIXEL_BLU(pix)  (playpal_data[0][pix][2])

//
// M_ScreenShot
//
// -ACB- 2002/04/29 Used the ReadScreen() interface to remove #define dependency 
//
void M_ScreenShot(void)
{
	byte *buffer;
	char filename[14];
	int i;

	// find a file name to save it to
	strcpy(filename,"SHOT0000.TGA");

	for (i = 0; i <= 9999; i++)
	{
		filename[4] =  i / 1000 + '0';
		filename[5] = (i % 1000) / 100 + '0';
		filename[6] = (i % 100) / 10 + '0';
		filename[7] = i % 10 + '0';
  
		if (!I_Access(filename))
			break; // file doesn't exist
	}

	buffer = (byte*)Z_Malloc(SCREENWIDTH*SCREENHEIGHT*4);

	vctx.ReadScreen(0, 0, SCREENWIDTH, SCREENHEIGHT, buffer);
	WriteTGAFile(filename, SCREENWIDTH, SCREENHEIGHT, buffer);
	Z_Free(buffer);
}

//
// M_MakeSaveScreenShot
//
void M_MakeSaveScreenShot(void)
{
/* !!!!!!
#ifdef USE_GL
	/// FIXME:
	// buffer = (byte*)Z_Malloc(SCREENWIDTH*SCREENHEIGHT*4);
	// glReadBuffer(GL_FRONT);
	// glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	// glReadPixels(0, 0, SCREENWIDTH, SCREENHEIGHT, GL_RGB, GL_UNSIGNED_BYTE, buffer);
	// ...
	// Z_Free(buffer);
  
#else
	int x, y;
	int ax, ay;

	byte pixel;
	unsigned short rgb;
	truecol_info_t colinfo;

	save_screenshot_valid = true;

	I_GetTruecolInfo(&colinfo);

	switch (main_scr->bytepp)
	{
		case 1:
		{
			byte *top = (byte *) main_scr->data;

			top += viewwindowy * main_scr->pitch + viewwindowx;

			for (y=0; y < 100; y++)
				for (x=0; x < 160; x++)    
				{
					ax = x * viewwindowwidth  / 160;
					ay = y * viewwindowheight / 100;

					DEV_ASSERT2(0 <= ax && ax < SCREENWIDTH);
					DEV_ASSERT2(0 <= ay && ay < SCREENHEIGHT);

					pixel = top[ay * main_scr->pitch + ax];

					rgb = ((PIXEL_RED(pixel) & 0xF8) << 7) |
						((PIXEL_GRN(pixel) & 0xF8) << 2) |
						((PIXEL_BLU(pixel) & 0xF8) >> 3);
        
					save_screenshot[x][y] = rgb;
				}
		}
		break;

		case 2:
		{
			unsigned short *top = (unsigned short *) main_scr->data;

			top += viewwindowy * main_scr->pitch/2 + viewwindowx;

			for (y=0; y < 100; y++)
				for (x=0; x < 160; x++)    
				{
					ax = x * viewwindowwidth  / 160;
					ay = y * viewwindowheight / 100;

					DEV_ASSERT2(0 <= ax && ax < SCREENWIDTH);
					DEV_ASSERT2(0 <= ay && ay < SCREENHEIGHT);

					rgb = top[ay * main_scr->pitch/2 + ax];

					// hack !
					if (colinfo.green_bits == 6)
						rgb = ((rgb & 0xFFC0) >> 1) | (rgb & 0x001F);
        
					save_screenshot[x][y] = rgb;
				}
		}
		break;
	}
#endif
*/
}

//
// M_CheckExtension
//
// Checks a filename extension against one given.
//
// returns:
//   EXT_WEIRD (one of the parameters given was bad)
//   EXT_NONE (filename has no extension)
//   EXT_MATCHING (filename has a matching extension)
//   EXT_NOTMATCHING (filename has an extension, but it does not match)
//
// -ACB- 1999/12/14 Written
//
exttype_e M_CheckExtension(const char *ext, const char* filename)
{
	char filenameext[4];
	int i, ext_len, filename_len, filenameext_len;

	// check extension is valid
	if (ext == NULL)
		return EXT_WEIRD;

	ext_len = strlen(ext);

	if (ext_len < 1 || ext_len > 3)
		return EXT_WEIRD;

	for (i=0; i<ext_len; i++)
	{
		if (!isalnum(ext[i]))
			return EXT_WEIRD;
	}

	// check filename is valid
	if (filename == NULL)
		return EXT_WEIRD;

	filename_len = strlen(filename);

	// Get filename extension
	i=(filename_len-1);
	while (i>=0 && filename[i] != '.')
		i--;

	if (i==-1 || i==(filename_len-1))
		return EXT_NONE;

	filenameext_len = ((filename_len-1)-i);

	// if the extension lengths differ, they can't match
	if (filenameext_len != ext_len)
		return EXT_NOTMATCHING;

	// copy extension to string to compare
	i=0;
	while (i<filenameext_len)
	{
		filenameext[i] = filename[(filename_len-filenameext_len)+i];
		i++;
	}

	// add terminator on the end
	filenameext[filenameext_len] = '\0';

	// compare ignoring case: non-zero means not matching
	if (stricmp(filenameext, ext))
		return EXT_NOTMATCHING;

	return EXT_MATCHING;
}

//
// M_ComposeFileName
//
// Allocates memory for and creates the file name "dir/file", or
// just "file" if it was an absolute address.
//
char *M_ComposeFileName(const char *dir, const char *file)
{
	char *path;

	if (I_PathIsAbsolute(file))
	{
		path = Z_StrDup(file);
	}
	else
	{
		path = Z_New(char, strlen(dir) + strlen(file) + 2);
		sprintf(path, "%s%c%s", dir, DIRSEPARATOR, file);
	}
	return path;
}


//
// M_GetFileData
//
// Loads file into memory. This sets a pointer to the data and
// the length.
//
// NOTE: The data must be freed by Z_Free() when not used.
//
// Returns NULL on failure.
//
// -ACB- 2000/01/08 Written
// -ES-  2000/06/12 Now returns the allocated pointer, or NULL on failure.
//
byte *M_GetFileData(char *filename, int *length)
{
	FILE *lumpfile;
	byte *data;

	// Sanity Checks..
	DEV_ASSERT2(filename);
	DEV_ASSERT2(length);

	lumpfile = fopen(filename, "rb");  
	if (!lumpfile)
	{
		I_Warning("M_GetFileData: Cannot open '%s'\n", filename);
		return NULL;
	}

	fseek(lumpfile, 0, SEEK_END);                   // get the end of the file
	(*length) = ftell(lumpfile);                    // get the size
	fseek(lumpfile, 0, SEEK_SET);                   // reset to beginning
	data = (byte*)Z_New(char, (*length) + 1);       // malloc the size
	fread(data, sizeof(char), (*length), lumpfile); // read file
	fclose(lumpfile);                               // close the file

	return data;
}

//
// M_WarnError
//
// Either displays a warning or produces a fatal error, depending on
// whether the "-strict" option is used.
//
void M_WarnError(const char *error,...)
{
	va_list argptr;
	char message_buf[4096];

	message_buf[4095] = 0;

	va_start(argptr, error);
	vsprintf(message_buf, error, argptr);
	va_end(argptr);

	// I hope nobody is printing strings longer than 4096 chars...
	DEV_ASSERT2(message_buf[4095] == 0);

	if (strict_errors)
		I_Error("%s", message_buf);
	else if (! no_warnings)
		I_Warning("%s", message_buf);
}

//
// L_WriteDebug
//
// Write into the debug file.
//
// -ACB- 1999/09/22: From #define to Procedure
// -AJA- 2001/02/07: Moved here from platform codes.
//
void L_WriteDebug(const char *message,...)
{
	va_list argptr;
	char message_buf[4096];

	if (!debugfile)
		return;

	// -ACB- 2001/02/08 Clear the message buffer
	memset(&message_buf, 0, sizeof(char)*4096);

	// Print the message into a text string
	va_start(argptr, message);
	vsprintf(message_buf, message, argptr);
	va_end(argptr);

	// I hope nobody is printing strings longer than 4096 chars...
	DEV_ASSERT2(message_buf[4095] == 0);

	fprintf(debugfile, "%s", message_buf);
	fflush(debugfile);
}

//
// L_ConvertToDB
// 
// Converts the linear input volume to a DB output volume in the range
// `min..max'.  Input ranges from 0-255.  This call is expensive,
// should be used to build a lookup table.
// 
int L_ConvertToDB(int volume, int min, int max)
{
	float tmp;
	int result;

	DEV_ASSERT2(0 <= volume && volume <= 255);

	tmp = 1.0f - (float)log(256.0f / (volume + 1.0f)) / (float)log(256.0f);

	result = min + (int) ((max - min) * tmp);

	// clamp result, in case of underflow/overflow
	return MAX(min, MIN(max, result));
}

//
// L_CompareTimeStamps
//
// Returns -1 for less, 0 for equals, and +1 for greater.
// I.e. the same values as used by strcmp() and friends.
//
int L_CompareTimeStamps(i_time_t *A, i_time_t *B)
{
	int a_index;
	int b_index;

	// Form an index to compare the dates
	a_index = (A->year * 10000) + (A->month * 100) + A->day;
	b_index = (B->year * 10000) + (B->month * 100) + B->day;

	if (a_index < b_index)
		return -1;
	else if (a_index > b_index)
		return +1;

	// Form an index to compare times
	a_index = (A->hours * 10000) + (A->minutes * 100) + A->secs;
	b_index = (B->hours * 10000) + (B->minutes * 100) + B->secs;

	if (a_index < b_index)
		return -1;
	else if (a_index > b_index)
		return +1;
	else
		return 0;
}


