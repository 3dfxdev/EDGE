//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (Game settings)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2004  The EDGE Team.
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
// Overall Game Setup and Parser Code
//

#include "i_defs.h"

#include "ddf_locl.h"
#include "ddf_main.h"

#include "z_zone.h"

#include "./epi/epistring.h"

#undef  DF
#define DF  DDF_CMD

gamedef_container_c gamedefs;

static gamedef_c buffer_gamedef;
static wi_animdef_c buffer_animdef;
static wi_framedef_c buffer_framedef;

static gamedef_c* dynamic_gamedef;

static void DDF_GameGetPic (const char *info, void *storage);
static void DDF_GameGetFrames (const char *info, void *storage);
static void DDF_GameGetMap (const char *info, void *storage);

#define DDF_CMD_BASE  buffer_gamedef

static const commandlist_t gamedef_commands[] = 
{
	DF ("INTERMISSION GRAPHIC", background, DDF_MainGetInlineStr10),
	DF ("INTERMISSION CAMERA", bg_camera, DDF_MainGetInlineStr32),
	DF ("INTERMISSION MUSIC", music, DDF_MainGetNumeric),
	DF ("SPLAT GRAPHIC", splatpic, DDF_MainGetInlineStr10),
	DF ("YAH1 GRAPHIC", yah[0], DDF_MainGetInlineStr10),
	DF ("YAH2 GRAPHIC", yah[1], DDF_MainGetInlineStr10),
	DF ("PERCENT SOUND", percent, DDF_MainLookupSound),
	DF ("DONE SOUND", done, DDF_MainLookupSound),
	DF ("ENDMAP SOUND", endmap, DDF_MainLookupSound),
	DF ("NEXTMAP SOUND", nextmap, DDF_MainLookupSound),
	DF ("ACCEL SOUND", accel_snd, DDF_MainLookupSound),
	DF ("FRAG SOUND", frag_snd, DDF_MainLookupSound),
	DF ("FIRSTMAP", firstmap, DDF_MainGetInlineStr10),
	DF ("NAME GRAPHIC", namegraphic, DDF_MainGetInlineStr10),
	DF ("TITLE MUSIC", titlemusic, DDF_MainGetNumeric),
	DF ("TITLE TIME", titletics, DDF_MainGetTime),
	DF ("SPECIAL MUSIC", special_music, DDF_MainGetNumeric),

	// these don't quite fit in yet
	DF ("TITLE GRAPHIC", ddf, DDF_GameGetPic),
	DF ("MAP", ddf, DDF_GameGetMap),
	{"ANIM", DDF_GameGetFrames, &buffer_framedef, NULL},

	DDF_CMD_END
};


//
//  DDF PARSE ROUTINES
//

static bool GameStartEntry (const char *name)
{
	gamedef_c *existing = NULL;

	if (name && name[0])
		existing = gamedefs.Lookup(name);

	// not found, create a new one
	if (! existing)
	{
		dynamic_gamedef = new gamedef_c;

		dynamic_gamedef->ddf.name = (name && name[0]) ? Z_StrDup(name) :
			DDF_MainCreateUniqueName("UNNAMED_GAMEDEF", gamedefs.GetSize());

		gamedefs.Insert(dynamic_gamedef);
	}
	else
	{
		dynamic_gamedef = existing;
	}

	dynamic_gamedef->ddf.number = 0;

	// instantiate the static entries
	buffer_gamedef.Default();
	buffer_animdef.Default();
	buffer_framedef.Default();

	return (existing != NULL);
}

static void GameParseField (const char *field, const char *contents,
		int index, bool is_last)
{
#if (DEBUG_DDF)
	L_WriteDebug ("GAME_PARSE: %s = %s;\n", field, contents);
#endif

	if (!DDF_MainParseField (gamedef_commands, field, contents))
		DDF_WarnError ("Unknown games.ddf command: %s\n", field);
}

static void GameFinishEntry (void)
{
	// FIXME: check stuff...

	// transfer static entry to dynamic entry
	dynamic_gamedef->CopyDetail(buffer_gamedef);

	// compute CRC...
	// FIXME: Use EPI CRC Class
	CRC32_Init (&dynamic_gamedef->ddf.crc);
	// FIXME: more stuff...
	CRC32_Done (&dynamic_gamedef->ddf.crc);
}

static void GameClearAll (void)
{
	// safe to delete game entries
	gamedefs.Clear();
}


void DDF_ReadGames (void *data, int size)
{
	readinfo_t games;

	games.memfile = (char *) data;
	games.memsize = size;
	games.tag = "GAMES";
	games.entries_per_dot = 1;

	if (games.memfile)
	{
		games.message = NULL;
		games.filename = NULL;
		games.lumpname = "DDFGAME";
	}
	else
	{
		games.message = "DDF_InitGames";
		games.filename = "games.ddf";
		games.lumpname = NULL;
	}

	games.start_entry = GameStartEntry;
	games.parse_field = GameParseField;
	games.finish_entry = GameFinishEntry;
	games.clear_all = GameClearAll;

	DDF_MainReadFile (&games);
}

void DDF_GameInit(void)
{
	gamedefs.Clear();			// <-- Consistent with existing behaviour (-ACB- 2004/05/23)
}

void DDF_GameCleanUp(void)
{
	if (gamedefs.GetSize() == 0)
		I_Error("There are no games defined in DDF !\n");
		
	gamedefs.Trim();
}

static void DDF_GameAddFrame(void)
{
	wi_framedef_c *f = new wi_framedef_c(buffer_framedef);
	
	buffer_animdef.frames.Insert(f);
	buffer_framedef.Default();
}

static void DDF_GameAddAnim(void)
{
	wi_animdef_c *a = new wi_animdef_c(buffer_animdef);
	
	if (a->level[0])
		a->type = wi_animdef_c::WI_LEVEL;
	else
		a->type = wi_animdef_c::WI_NORMAL;	
		
	buffer_gamedef.anims.Insert(a);
	buffer_animdef.Default();
}

static void DDF_GameGetFrames(const char *info, void *storage)
{
	epi::string_c s, s2;
	wi_framedef_c *f;
	int i, pos;
	
	struct { int type; void *dest; } f_dest[4];
	
	f = (wi_framedef_c *) storage;
	s = info;
	
	f_dest[0].type = 1;
	f_dest[0].dest = &f->pic;
	f_dest[1].type = 0;
	f_dest[1].dest = &f->tics;
	f_dest[2].type = 0;
	f_dest[2].dest = &f->pos.x;
	f_dest[3].type = 2;
	f_dest[3].dest = &f->pos.y;

	if (s.GetLength() && s.GetAt(0) == '#')
	{
		pos = s2.Find(':');

		s2 = s;
		s2.TruncateAt(pos);
		
		if (buffer_animdef.frames.GetSize() == 0)
		{
			// Remove the hash
			s2.RemoveLeft(1);

			// Copy 
			buffer_animdef.level.Set(s2.GetString());
			 
			// Strip out the map from the working buffer
			s.RemoveLeft(pos+1);
		}
		else if (!s2.CompareNoCase("#END"))
		{
			DDF_GameAddAnim();
			return;
		}
		else
			DDF_Error ("Invalid # command '%s'\n", s2.GetString());
	}
	
	for (i = 0; i < 4; i++)
	{
		pos = s.Find(':');

		s2 = s;

		if (pos >= 0)
		{
			s2.TruncateAt(pos);
			s.RemoveLeft(pos+1);
		}

		if (f_dest[i].type & 1)
		{	
			lumpname_c *ln = (lumpname_c*)f_dest[i].dest;
			ln->Set(s2.GetString());
		}
		else
		{
			*(int *) f_dest[i].dest = atoi (s2);
		}
		
		if (f_dest[i].type & 2)
		{
			DDF_GameAddFrame();
			return;
		}

		if (pos<0)
			break;
	}

	DDF_Error ("Bad Frame command '%s'\n", info);
}

static void DDF_GameGetMap(const char *info, void *storage)
{
	epi::string_c s, s2;
	int i, pos;
	wi_mapposdef_c *mp;
	
	struct { int type; void *dest; } dest[3];

	s = info;
	mp = new wi_mapposdef_c();
	
	dest[0].type = 1;
	dest[0].dest = &mp->name;
	dest[1].type = 0;
	dest[1].dest = &mp->pos.x;
	dest[2].type = 2;
	dest[2].dest = &mp->pos.y;

	for (i = 0; i < 3; i++)
	{
		pos = s.Find(':');

		s2 = s;

		if (pos >= 0)
		{
			s2.TruncateAt(pos);
			s.RemoveLeft(pos+1);
		}

		if (dest[i].type & 1)
		{
			lumpname_c *ln = (lumpname_c*)dest[i].dest;
			ln->Set(s2);
		}
		else
		{
			*(int *) dest[i].dest = atoi (s2);
		}
			
		if (dest[i].type & 2)
		{
			buffer_gamedef.mappos.Insert(mp);
			return;
		}
		
		if (pos < 0)
			break;
	}
	
	if (mp)
		delete mp;
	
	DDF_Error ("Bad Map command '%s'\n", info);
}

static void DDF_GameGetPic (const char *info, void *storage)
{
	buffer_gamedef.titlepics.Insert(info);
}

// --> world intermission mappos class

//
// wi_mapposdef_c Constructor
//
wi_mapposdef_c::wi_mapposdef_c()
{
}

//
// wi_mapposdef_c Copy constructor
//
wi_mapposdef_c::wi_mapposdef_c(wi_mapposdef_c &rhs)
{
	Copy(rhs);
}

//
// wi_mapposdef_c Destructor
//
wi_mapposdef_c::~wi_mapposdef_c()
{
}

//
// wi_mapposdef_c::Copy()
//
void wi_mapposdef_c::Copy(wi_mapposdef_c &src)
{
	name = src.name;
	pos = src.pos;
}

//
// wi_mapposdef_c assignment operator
//
wi_mapposdef_c& wi_mapposdef_c::operator=(wi_mapposdef_c &rhs)
{
	if (&rhs != this)
		Copy(rhs);

	return *this;
}

// --> world intermission mappos container class

//
// wi_mapposdef_container_c Constructor
//
wi_mapposdef_container_c::wi_mapposdef_container_c() 
	: epi::array_c(sizeof(wi_mapposdef_c*))
{
}

//
// wi_mapposdef_container_c Copy constructor
//
wi_mapposdef_container_c::wi_mapposdef_container_c(wi_mapposdef_container_c &rhs)
	: epi::array_c(sizeof(wi_mapposdef_c*))
{
	Copy(rhs);
}

//
// wi_mapposdef_container_c Destructor
//
wi_mapposdef_container_c::~wi_mapposdef_container_c()
{
	Clear();
}

//
// wi_mapposdef_container_c::CleanupObject()
//
void wi_mapposdef_container_c::CleanupObject(void *obj)
{
	wi_mapposdef_c *mp = *(wi_mapposdef_c**)obj;

	if (mp)
		delete mp;

	return;
}

//
// wi_mapposdef_container_c::Copy()
//
void wi_mapposdef_container_c::Copy(wi_mapposdef_container_c &src)
{
	epi::array_iterator_c it;
	wi_mapposdef_c *wi;
	wi_mapposdef_c *wi2;
	
	Size(src.GetSize());

	for (it = src.GetBaseIterator(); it.IsValid(); it++)
	{
		wi = ITERATOR_TO_TYPE(it, wi_mapposdef_c*);
		if (wi)
		{
			wi2 = new wi_mapposdef_c(*wi);
			Insert(wi2);
		}
	}

	Trim();
}

//
// wi_mapposdef_container_c assignment operator
//
wi_mapposdef_container_c& wi_mapposdef_container_c::operator=(wi_mapposdef_container_c &rhs)
{
	if (&rhs != this)
	{
		Clear();
		Copy(rhs);
	}

	return *this;
}

// --> world intermission framedef class

//
// wi_framedef_c Constructor
//
wi_framedef_c::wi_framedef_c()
{
	Default();
}

//
// wi_framedef_c Copy constructor
//
wi_framedef_c::wi_framedef_c(wi_framedef_c &rhs)
{
	Copy(rhs); 
}

//
// wi_framedef_c Destructor
//
wi_framedef_c::~wi_framedef_c()
{
}

//
// wi_framedef_c::Copy()
//
void wi_framedef_c::Copy(wi_framedef_c &src)
{
	tics = src.tics;
	pos = src.pos;	
	pic = src.pic;
}

//
// wi_framedef_c::Default()
//
void wi_framedef_c::Default()
{
	tics = 0;
	pos.x = 0;	
	pos.y = 0;	
	pic.Clear(); 
}

//
// wi_framedef_c assignment operator
//
wi_framedef_c& wi_framedef_c::operator=(wi_framedef_c &rhs)
{
	if (&rhs != this)
		Copy(rhs);

	return *this;
}

// --> world intermission framedef container class

//
// wi_framedef_container_c Constructor
//
wi_framedef_container_c::wi_framedef_container_c()
	: epi::array_c(sizeof(wi_framedef_c*))
{
}

//
// wi_framedef_container_c Copy constructor
//
wi_framedef_container_c::wi_framedef_container_c(wi_framedef_container_c &rhs)
	: epi::array_c(sizeof(wi_framedef_c*))
{
	Copy(rhs);
}

//
// wi_framedef_container_c Destructor
//
wi_framedef_container_c::~wi_framedef_container_c()
{
	Clear();
}

//
// wi_framedef_container_c::CleanupObject()
//
void wi_framedef_container_c::CleanupObject(void *obj)
{
	wi_framedef_c *f = *(wi_framedef_c**)obj;

	if (f)
		delete f;

	return;
}

//
// wi_framedef_container_c::Copy()
//
void wi_framedef_container_c::Copy(wi_framedef_container_c &src)
{
	epi::array_iterator_c it;
	wi_framedef_c *f;
	wi_framedef_c *f2;
	
	Size(src.GetSize());

	for (it = src.GetBaseIterator(); it.IsValid(); it++)
	{
		f = ITERATOR_TO_TYPE(it, wi_framedef_c*);
		if (f)
		{
			f2 = new wi_framedef_c(*f);
			Insert(f2);
		}
	}

	Trim();
}

//
// wi_framedef_container_c assignment operator
//
wi_framedef_container_c& wi_framedef_container_c::operator=(wi_framedef_container_c& rhs)
{
	if (&rhs != this)
	{
		Clear();
		Copy(rhs);
	}

	return *this;
}

// --> world intermission animdef class

//
// wi_animdef_c Constructor
//
wi_animdef_c::wi_animdef_c()
{
	Default();
}

//
// wi_animdef_c Copy constructor
//
wi_animdef_c::wi_animdef_c(wi_animdef_c &rhs)
{
	Copy(rhs);
}

//
// wi_animdef_c Destructor
//
wi_animdef_c::~wi_animdef_c()
{
}

//
// void Copy()
//
void wi_animdef_c::Copy(wi_animdef_c &src)
{
	type = src.type;
	level = src.level;
	frames = src.frames;
}

//
// wi_animdef_c::Default()
//
void wi_animdef_c::Default()
{
	type = WI_NORMAL;
	level.Clear();
	frames.Clear();
}


//
// wi_animdef_c assignment operator
//
wi_animdef_c& wi_animdef_c::operator=(wi_animdef_c &rhs)
{
	if (&rhs != this)
		Copy(rhs);

	return *this;
}

// --> world intermission anim container class

//
// wi_animdef_container_c Constructor
//
wi_animdef_container_c::wi_animdef_container_c() : epi::array_c(sizeof(wi_animdef_c*))
{
}

//
// wi_animdef_container_c Copy constructor
//
wi_animdef_container_c::wi_animdef_container_c(wi_animdef_container_c &rhs)
	: epi::array_c(sizeof(wi_animdef_c*))
{
	Copy(rhs);
}

//
// wi_animdef_container_c Destructor
//
wi_animdef_container_c::~wi_animdef_container_c()
{
	Clear();
}

//
// wi_animdef_container_c::CleanupObject()
//
void wi_animdef_container_c::CleanupObject(void *obj)
{
	wi_animdef_c *a = *(wi_animdef_c**)obj;

	if (a)
		delete a;

	return;
}

//
// wi_animdef_container_c::Copy()
//
void wi_animdef_container_c::Copy(wi_animdef_container_c &src)
{
	epi::array_iterator_c it;
	wi_animdef_c *a;
	wi_animdef_c *a2;
	
	Size(src.GetSize());

	for (it = src.GetBaseIterator(); it.IsValid(); it++)
	{
		a = ITERATOR_TO_TYPE(it, wi_animdef_c*);
		if (a)
		{
			a2 = new wi_animdef_c(*a);
			Insert(a2);
		}
	}

	Trim();
}


//
// wi_animdef_container_c assignment operator
//
wi_animdef_container_c& wi_animdef_container_c::operator=(wi_animdef_container_c &rhs)
{
	if (&rhs != this)
	{
		Clear();
		Copy(rhs);
	}

	return *this;
}


// --> game definition class 

//
// gamedef_c Constructor
//
gamedef_c::gamedef_c()
{
	Default();
}

//
// gamedef_c Copy constructor
//
gamedef_c::gamedef_c(gamedef_c &rhs)
{
	Copy(rhs);
}

//
// gamedef_c Destructor
//
gamedef_c::~gamedef_c()
{
}

//
// gamedef_c::Copy()
//
void gamedef_c::Copy(gamedef_c &src)
{
	ddf = src.ddf;
	CopyDetail(src);	
}

//
// gamedef_c::CopyDetail()
//
void gamedef_c::CopyDetail(gamedef_c &src)
{
	anims = src.anims;
	mappos = src.mappos;
		
	background = src.background;
	splatpic = src.splatpic;

	yah[0] = src.yah[0];
	yah[1] = src.yah[1];
	
	strcpy(bg_camera, src.bg_camera);
	music = src.music;

	percent = src.percent;
	done = src.done;
	endmap = src.endmap;
	nextmap = src.nextmap;
	accel_snd = src.accel_snd;
	frag_snd = src.frag_snd;

	firstmap = src.firstmap;
	namegraphic = src.namegraphic;
	
	titlepics = src.titlepics;
	
	titlemusic = src.titlemusic;
	titletics = src.titletics;
	
	special_music = src.special_music;
}

//
// gamedef_c::Default()
//
void gamedef_c::Default()
{
	// FIXME: ddf.Clear() ?
	ddf.name	= "";
	ddf.number	= 0;	
	ddf.crc		= 0;

	anims.Clear();
	mappos.Clear();
		
	background.Clear();
	splatpic.Clear();

	yah[0].Clear();
	yah[1].Clear();
	
	bg_camera[0] = '\0';
	music = 0;

	percent = sfx_None;
	done = sfx_None;
	endmap = sfx_None;
	nextmap = sfx_None;
	accel_snd = sfx_None;
	frag_snd = sfx_None;

	firstmap.Clear();
	namegraphic.Clear();
	
	titlepics.Clear();
	
	titlemusic = 0;
	titletics = TICRATE * 4;
	
	special_music = 0;
}

//
// gamedef_c assignment operator
//
gamedef_c& gamedef_c::operator=(gamedef_c &rhs)
{
	if (&rhs != this)
		Copy(rhs);
		
	return *this;
}

// --> game definition container class

//
// gamedef_container_c Constructor
//
gamedef_container_c::gamedef_container_c() : epi::array_c(sizeof(gamedef_c*))
{
}

//
// gamedef_container_c Destructor
//
gamedef_container_c::~gamedef_container_c()
{
	Clear();
}

//
// gamedef_container_c::CleanupObject()
//
void gamedef_container_c::CleanupObject(void *obj)
{
	gamedef_c *g = *(gamedef_c**)obj;

	if (g)
		delete g;

	return;
}

//
// gamedef_c* gamedef_container_c::Lookup()
//
// Looks an gamedef by name, returns a fatal error if it does not exist.
//
gamedef_c* gamedef_container_c::Lookup(const char *refname)
{
	epi::array_iterator_c it;
	gamedef_c *g;

	if (!refname || !refname[0])
		return NULL;

	for (it = GetBaseIterator(); it.IsValid(); it++)
	{
		g = ITERATOR_TO_TYPE(it, gamedef_c*);
		if (DDF_CompareName(g->ddf.name, refname) == 0)
			return g;
	}

	// FIXME!!! Throw an epi:error_c here
	return NULL;
}
