/* Rise of the Triad MAPS */

/// FIXME: Refactor using EPI file handling, C++-isms, etc.

#include "system/i_defs.h"

#include <stdio.h>

#include <vector>

#include "../epi/endianess.h"

#include "rott_local.h" ///FIXME: This should just be rott_local.h ... 


#define SHAREWARE_TAG     0x4d4b
#define REGISTERED_TAG    0x4344
#define RTL_VERSION       ( 0x0101 )
#define COMMBAT_SIGNATURE ( "RTC" )
#define NORMAL_SIGNATURE  ( "RTL" )
#define RTL_HEADER_OFFSET 8
#define  STANDARDGAMELEVELS   ("DARKWAR.RTL")
static char *ROTTMAPS = STANDARDGAMELEVELS;
//u32_t *mapplanes[3];
int mapwidth;
int mapheight;
unsigned MapSpecials = 0;
/*
======================
=
= GetMapInfo
=
======================
*/
void GetMapInfo(mapfileinfo_t * mapinfo)
{

	//GetMapFileInfo(mapinfo, ROTTMAPS);

}

/*
======================
=
= CheckRTLVersion
=
======================
*/

#if 0
void CheckRTLVersion(char *filename)
{
	int filehandle;
	char RTLSignature[4];
	unsigned long RTLVersion;

	filehandle = SafeOpenRead(filename);

	//
	// Load RTL signature
	//
	SafeRead(filehandle, RTLSignature, sizeof(RTLSignature));

	if ((strcmp(RTLSignature, COMMBAT_SIGNATURE) != 0) &&
		(strcmp(RTLSignature, NORMAL_SIGNATURE) != 0)) {
		Error("The file '%s' is not a valid level file.", filename);
	}
	//
	// Check the version number
	//
	SafeRead(filehandle, &RTLVersion, sizeof(RTLVersion));
	SwapIntelLong(&RTLVersion);
	if (RTLVersion > RTL_VERSION) {
		Error("The file '%s' is a version %d.%d %s file.\n"
			"The highest this version of ROTT can load is %d.%d.",
			filename, RTLVersion >> 8, RTLVersion & 0xff, RTLSignature,
			RTL_VERSION >> 8, RTL_VERSION & 0xff);
	}

	close(filehandle);
}

#endif // 0


/*---------------------------------------------------------------------
   Function: ReadROTTMap

   Read a map from a RTL/RTC file.
   CA~ 6.3.2016 - Use epi for file reading (!!!)
   Ew, ugly hacks inbound!!!!!!
---------------------------------------------------------------------*/#if 0

void ReadROTTMap(char *filename, int mapnum)
{
	RTLMAP RTLMap;
	FILE *fp;
	u32_t pos;
	u32_t compressed;
	u32_t expanded;
	int plane;
	byte *buffer;
	mapfileinfo_t *mapinfo;

#define filehandle fp
	//CheckRTLVersion(filename);
	FILE *fp = fopen("DARKWAR.RTC", "rb");
	if (!fp)
		I_Error("Missing DARKWAR.RTC file!\n");

	//bna++146 start
	//check if that the mapnum exxeds the actuell mapnum
	mapinfo = (mapfileinfo_t *)Z_Malloc2(sizeof(mapfileinfo_t));
	GetMapInfo(mapinfo);
	if (mapnum > mapinfo->nummaps - 1)
		mapnum = 0;
	//bna++146 end

	//
	// Load map header
	//
	fseek(filehandle, RTL_HEADER_OFFSET + mapnum * sizeof(RTLMap),
		SEEK_SET);

	//SafeRead(filehandle, &RTLMap, sizeof(RTLMap));
	fread(&RTLMap, sizeof(RTLMap), 1, fp);

	EPI_LE_U32((long *)&RTLMap.used);
	EPI_LE_U32((long *)&RTLMap.CRC);
	EPI_LE_U32((long *)&RTLMap.RLEWtag);
	EPI_LE_U32((long *)&RTLMap.MapSpecials);
	EPI_LE_U16((long *)&RTLMap.planestart, NUMPLANES);
	EPI_LE_U16((long *)&RTLMap.planelength, NUMPLANES);

	if (!RTLMap.used)
	{
		I_Error("ReadROTTMap: Tried to load a non existent map!");
	}
#if ( SHAREWARE == 1 )
	if (RTLMap.RLEWtag == REGISTERED_TAG) {
		Error
		("Can't use maps from the registered game in shareware version.");
	}

	if (RTLMap.RLEWtag != SHAREWARE_TAG) {
		Error("Can't use modified maps in shareware version.");
	}
#endif

	mapwidth = 128;
	mapheight = 128;

	// Get special map flags
	MapSpecials = RTLMap.MapSpecials;

	//
	// load the planes in
	//
	expanded = mapwidth * mapheight * 2;

	int complen;
	byte *buf1 = new byte[complen];


	int size2 = buf1[0] | ((int)buf1[1] << 8);
	// CHECK STUFF !!!

	byte *buf2 = new byte[size2];

	delete[] buf1;

	int size_r = buf2[0] | ((int)buf2[1] << 8);

	SYS_ASSERT(size_r == expanded);

	u16_t *result = new u16_t[expanded / 2];

	I_Printf("Seeking to %d\n", RTL_HEADER_OFFSET);

	for (plane = 0; plane <= 2; plane++)
	{
		pos = RTLMap.planestart[plane];

		compressed = RTLMap.planelength[plane];

		if (compressed == 0)//bna++146
			return;

		u8_t *buffer = (u8_t*)malloc(compressed);

		if (buffer == 0)//bna++146
			return;

		fseek(filehandle, pos, SEEK_SET);

		fread(filehandle, buffer, compressed);

		mapplanes[plane] = Z_Malloc(expanded, &mapplanes[plane]);

		//
		// unRLEW, skipping expanded length
		//
#if ( SHAREWARE == 1 )
		CA_RLEWexpand((word *)buffer, (word *)mapplanes[plane],
			expanded >> 1, SHAREWARE_TAG);
#else
		//CA_RLEW_Expand((word *)buffer, (word *)mapplanes[plane],
		//	expanded >> 1, RTLMap.RLEWtag);
		WF_RLEW_Expand((u16_t *)(buf2 + 2), result, expanded, RTLMap.RLEWtag);
#endif

		Z_Free(buffer);
	}
	fclose(filehandle);

	//
	// get map name
	//
	strcpy(LevelName, RTLMap.Name);

	CheckMapForAreaErrors();//bna++146
}
#endif // 0


/*
======================
=
= GetMapFileInfo
=
======================
*/
void GetMapFileInfo(mapfileinfo_t * mapinfo, char *filename)
{
	RTLMAP RTLMap[100];
	FILE *fp;
	int i;
	int nummaps;

	//CheckRTLVersion(filename);

	//
	// Load map header
	//
	// move to correct position in file
	fseek(fp, RTL_HEADER_OFFSET, SEEK_SET);
	//lseek(filehandle, RTL_HEADER_OFFSET, SEEK_SET);

	//SafeRead(filehandle, &RTLMap, sizeof(RTLMap));
	fread(&RTLMap, sizeof(RTLMap), 1, fp);

	fclose(fp);

	nummaps = 0;

	for (i = 0; i < 100; i++) 
	{
		if (!RTLMap[i].used) 
		{
			continue;
		}

		mapinfo->maps[nummaps].number = i;

		strcpy(mapinfo->maps[nummaps].mapname, RTLMap[i].Name);

		nummaps++;
	}

	mapinfo->nummaps = nummaps;
}

#if 0

void ReadROTTMapUgly(void)
{
	char *filename,
		int mapnum
		char            RTLSignature[4];
	unsigned long   RTLVersion;
	RTLMAP          RTLMap;
	int             filehandle;
	long            pos;
	long            compressed;
	long            expanded;
	int             plane;
	unsigned short *buffer;

	filehandle = open(filename, O_RDONLY | O_BINARY);

	//
	// Load RTL signature
	//
	read(filehandle, RTLSignature, sizeof(RTLSignature));

	//
	// Read the version number
	//
	read(filehandle, &RTLVersion, sizeof(RTLVersion));

	//
	// Load map header
	//
	lseek(filehandle, mapnum * sizeof(RTLMap), SEEK_CUR);
	read(filehandle, &RTLMap, sizeof(RTLMap));

	if (!RTLMap.used)
	{
		//
		// Exit on error
		//
		I_Error("ReadROTTMap: Tried to load a non existent map!");
	}

	//
	// load the planes in (!!! MAPWIDTH/MAPHEIGHT are ROTT_planes, unlike Wolf3D)
	//
	expanded = MAPWIDTH * MAPHEIGHT * 2;

	for (plane = 0; plane <= 2; plane++)
	{
		pos = RTLMap.planestart[plane];
		compressed = RTLMap.planelength[plane];
		buffer = malloc(compressed);

		lseek(filehandle, pos, SEEK_SET);
		read(filehandle, buffer, compressed);

		mapplanes[plane] = malloc(expanded);

		RLEW_Expand(buffer, mapplanes[plane], expanded >> 1, RTLMap.RLEWtag); ///yuck.

		free(buffer);
	}

	close(filehandle);

}
#endif // 0


