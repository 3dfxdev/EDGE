//------------------------------------------------------------------------
//  TEXT Strings
//------------------------------------------------------------------------
//
//  DEH_EDGE  Copyright (C) 2004  The EDGE Team
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License (in COPYING.txt) for more details.
//
//------------------------------------------------------------------------
//
//  DEH_EDGE is based on:
//
//  +  DeHackEd source code, by Greg Lewis.
//  -  DOOM source code (C) 1993-1996 id Software, Inc.
//  -  Linux DOOM Hack Editor, by Sam Lantinga.
//  -  PrBoom's DEH/BEX code, by Ty Halderman, TeamTNT.
//
//------------------------------------------------------------------------

#include "i_defs.h"
#include "text.h"

#include "info.h"
#include "english.h"
#include "frames.h"
#include "mobj.h"
#include "sounds.h"
#include "system.h"
#include "util.h"
#include "wad.h"


typedef struct
{
	const char *orig_text;
	const char *ldf_name;
	const char *deh_name;

	// holds modified version (NULL means not modified).  Guaranteed to
	// have space for an additional four (4) characters.
	char *new_text;
}
langinfo_t;

langinfo_t lang_list[] =
{
    { GOTCHAINSAW, "GotChainSaw", NULL, NULL },
    { AMSTR_MARKSCLEARED, "AutoMapMarksClear", NULL, NULL },
    { STSTR_FAADDED, "AmmoAdded", NULL, NULL },
    { NIGHTMARE, "NightmareCheck", NULL, NULL },
    { ENDGAME, "EndGameCheck", NULL, NULL },
    { GOTBERSERK, "GotBerserk", NULL, NULL },
    { HUSTR_PLRBROWN, "Player3Name", NULL, NULL },
    { STSTR_CLEV, "LevelChange", NULL, NULL },
    { HUSTR_CHATMACRO7, "DefaultCHATMACRO7", NULL, NULL },
    { GOTMAP, "GotMap", NULL, NULL },
    { C5TEXT, "Level31Text", NULL, NULL },
    { C6TEXT, "Level32Text", NULL, NULL },
    { STSTR_DQDOFF, "GodModeOFF", NULL, NULL },
    { STSTR_DQDON, "GodModeON", NULL, NULL },
    { D_DEVSTR, "DevelopmentMode", NULL, NULL },
    { STSTR_CHOPPERS, "ChoppersNote", NULL, NULL },
    { QLPROMPT, "QuickLoad", NULL, NULL },
    { HUSTR_E1M1, "E1M1Desc", NULL, NULL },
    { HUSTR_E1M2, "E1M2Desc", NULL, NULL },
    { HUSTR_E1M3, "E1M3Desc", NULL, NULL },
    { HUSTR_E1M4, "E1M4Desc", NULL, NULL },
    { HUSTR_E1M5, "E1M5Desc", NULL, NULL },
    { HUSTR_E1M6, "E1M6Desc", NULL, NULL },
    { HUSTR_E1M7, "E1M7Desc", NULL, NULL },
    { HUSTR_E1M8, "E1M8Desc", NULL, NULL },
    { HUSTR_E1M9, "E1M9Desc", NULL, NULL },
    { HUSTR_E2M1, "E2M1Desc", NULL, NULL },
    { HUSTR_E2M2, "E2M2Desc", NULL, NULL },
    { HUSTR_E2M3, "E2M3Desc", NULL, NULL },
    { HUSTR_E2M4, "E2M4Desc", NULL, NULL },
    { HUSTR_E2M5, "E2M5Desc", NULL, NULL },
    { HUSTR_E2M6, "E2M6Desc", NULL, NULL },
    { HUSTR_E2M7, "E2M7Desc", NULL, NULL },
    { HUSTR_E2M8, "E2M8Desc", NULL, NULL },
    { HUSTR_E2M9, "E2M9Desc", NULL, NULL },
    { HUSTR_E3M1, "E3M1Desc", NULL, NULL },
    { HUSTR_E3M2, "E3M2Desc", NULL, NULL },
    { HUSTR_E3M3, "E3M3Desc", NULL, NULL },
    { HUSTR_E3M4, "E3M4Desc", NULL, NULL },
    { HUSTR_E3M5, "E3M5Desc", NULL, NULL },
    { HUSTR_E3M6, "E3M6Desc", NULL, NULL },
    { HUSTR_E3M7, "E3M7Desc", NULL, NULL },
    { HUSTR_E3M8, "E3M8Desc", NULL, NULL },
    { HUSTR_E3M9, "E3M9Desc", NULL, NULL },
    { HUSTR_E4M1, "E4M1Desc", NULL, NULL },
    { HUSTR_E4M2, "E4M2Desc", NULL, NULL },
    { HUSTR_E4M3, "E4M3Desc", NULL, NULL },
    { HUSTR_E4M4, "E4M4Desc", NULL, NULL },
    { HUSTR_E4M5, "E4M5Desc", NULL, NULL },
    { HUSTR_E4M6, "E4M6Desc", NULL, NULL },
    { HUSTR_E4M7, "E4M7Desc", NULL, NULL },
    { HUSTR_E4M8, "E4M8Desc", NULL, NULL },
    { HUSTR_E4M9, "E4M9Desc", NULL, NULL },
    { EMPTYSTRING, "EmptySlot", NULL, NULL },
    { AMSTR_FOLLOWOFF, "AutoMapFollowOff", NULL, NULL },
    { AMSTR_FOLLOWON, "AutoMapFollowOn", NULL, NULL },
    { GGSAVED, "GameSaved", NULL, NULL },
    { GAMMALVL1, "GammaLevelOne", NULL, NULL },
    { GAMMALVL2, "GammaLevelTwo", NULL, NULL },
    { GAMMALVL3, "GammaLevelThree", NULL, NULL },
    { GAMMALVL4, "GammaLevelFour", NULL, NULL },
    { GAMMALVL0, "GammaOff", NULL, NULL },
    { HUSTR_PLRGREEN, "Player1Name", NULL, NULL },
    { AMSTR_GRIDOFF, "AutoMapGridOff", NULL, NULL },
    { AMSTR_GRIDON, "AutoMapGridOn", NULL, NULL },
    { HUSTR_CHATMACRO4, "DefaultCHATMACRO4", NULL, NULL },
    { HUSTR_CHATMACRO8, "DefaultCHATMACRO8", NULL, NULL },
    { HUSTR_CHATMACRO3, "DefaultCHATMACRO3", NULL, NULL },
    { HUSTR_CHATMACRO2, "DefaultCHATMACRO2", NULL, NULL },
    { STSTR_NOMUS, "ImpossibleChange", NULL, NULL },
    { HUSTR_CHATMACRO1, "DefaultCHATMACRO1", NULL, NULL },
    { HUSTR_PLRINDIGO, "Player2Name", NULL, NULL },
    { GOTINVUL, "GotInvulner", NULL, NULL },
    { STSTR_BEHOLD, "BEHOLDNote", NULL, NULL },
    { HUSTR_10, "Map10Desc", NULL, NULL },
    { HUSTR_11, "Map11Desc", NULL, NULL },
    { HUSTR_12, "Map12Desc", NULL, NULL },
    { HUSTR_13, "Map13Desc", NULL, NULL },
    { HUSTR_14, "Map14Desc", NULL, NULL },
    { HUSTR_15, "Map15Desc", NULL, NULL },
    { HUSTR_16, "Map16Desc", NULL, NULL },
    { HUSTR_17, "Map17Desc", NULL, NULL },
    { HUSTR_18, "Map18Desc", NULL, NULL },
    { HUSTR_19, "Map19Desc", NULL, NULL },
    { HUSTR_1, "Map01Desc", NULL, NULL },
    { HUSTR_20, "Map20Desc", NULL, NULL },
    { HUSTR_21, "Map21Desc", NULL, NULL },
    { HUSTR_22, "Map22Desc", NULL, NULL },
    { HUSTR_23, "Map23Desc", NULL, NULL },
    { HUSTR_24, "Map24Desc", NULL, NULL },
    { HUSTR_25, "Map25Desc", NULL, NULL },
    { HUSTR_26, "Map26Desc", NULL, NULL },
    { HUSTR_27, "Map27Desc", NULL, NULL },
    { HUSTR_28, "Map28Desc", NULL, NULL },
    { HUSTR_29, "Map29Desc", NULL, NULL },
    { HUSTR_2, "Map02Desc", NULL, NULL },
    { HUSTR_30, "Map30Desc", NULL, NULL },
    { HUSTR_31, "Map31Desc", NULL, NULL },
    { HUSTR_32, "Map32Desc", NULL, NULL },
    { HUSTR_3, "Map03Desc", NULL, NULL },
    { HUSTR_4, "Map04Desc", NULL, NULL },
    { HUSTR_5, "Map05Desc", NULL, NULL },
    { HUSTR_6, "Map06Desc", NULL, NULL },
    { HUSTR_7, "Map07Desc", NULL, NULL },
    { HUSTR_8, "Map08Desc", NULL, NULL },
    { HUSTR_9, "Map09Desc", NULL, NULL },
    { GOTVISOR, "GotVisor", NULL, NULL },
    { AMSTR_MARKEDSPOT, "AutoMapMarkedSpot", NULL, NULL },
    { GOTMSPHERE, "GotMega", NULL, NULL },
    { HUSTR_MESSAGESENT, "Sent", NULL, NULL },
    { MSGOFF, "MessagesOff", NULL, NULL },
    { MSGON, "MessagesOn", NULL, NULL },
    { HUSTR_MSGU, "UnsentMsg", NULL, NULL },
    { STSTR_MUS, "MusChange", NULL, NULL },
    { HUSTR_CHATMACRO6, "DefaultCHATMACRO6", NULL, NULL },
    { STSTR_NCOFF, "ClipOFF", NULL, NULL },
    { STSTR_NCON, "ClipON", NULL, NULL },
    { HUSTR_CHATMACRO0, "DefaultCHATMACRO0", NULL, NULL },
    { E1TEXT, "Episode1Text", NULL, NULL },
    { GOTINVIS, "GotInvis", NULL, NULL },
    { GOTSHELLS, "GotShells", NULL, NULL },
    { GOTBACKPACK, "GotBackpack", NULL, NULL },
    { GOTBLUECARD, "GotBlueCard", NULL, NULL },
    { GOTBLUESKUL, "GotBlueSkull", NULL, NULL },
    { GOTCLIPBOX, "GotClipBox", NULL, NULL },
    { GOTROCKBOX, "GotRocketBox", NULL, NULL },
    { GOTSHELLBOX, "GotShellBox", NULL, NULL },
    { GOTCLIP, "GotClip", NULL, NULL },
    { GOTHTHBONUS, "GotHealthPotion", NULL, NULL },
    { GOTMEDIKIT, "GotMedi", NULL, NULL },
    { GOTMEDINEED, "GotMediNeed", NULL, NULL },   // not yet supported by EDGE
    { GOTARMBONUS, "GotArmourHelmet", NULL, NULL },
    { GOTCELL, "GotCell", NULL, NULL },
    { GOTCELLBOX, "GotCellPack", NULL, NULL },
    { GOTREDCARD, "GotRedCard", NULL, NULL },
    { GOTREDSKULL, "GotRedSkull", NULL, NULL },
    { GOTROCKET, "GotRocket", NULL, NULL },
    { GOTSTIM, "GotStim", NULL, NULL },
    { GOTYELWCARD, "GotYellowCard", NULL, NULL },
    { GOTYELWSKUL, "GotYellowSkull", NULL, NULL },
    { GOTARMOR, "GotArmour", NULL, NULL },
    { GOTMEGA, "GotMegaArmour", NULL, NULL },
    { STSTR_BEHOLDX, "BEHOLDUsed", NULL, NULL },
    { PRESSKEY, "PressKey", NULL, NULL },
    { PRESSYN, "PressYorN", NULL, NULL },
    { DOSY, "PressToQuit", NULL, NULL },
    { QSPROMPT, "QuickSaveOver", NULL, NULL },
    { GOTSUIT, "GotSuit", NULL, NULL },
    { HUSTR_PLRRED, "Player4Name", NULL, NULL },
    { GOTSUPER, "GotSoul", NULL, NULL },
    { C4TEXT, "EndGameText", NULL, NULL },
    { E3TEXT, "Episode3Text", NULL, NULL },
    { E4TEXT, "Episode4Text", NULL, NULL },
    { STSTR_KFAADDED, "VeryHappyAmmo", NULL, NULL },
    { HUSTR_TALKTOSELF2, "TALKTOSELF2", NULL, NULL },
    { HUSTR_CHATMACRO9, "DefaultCHATMACRO9", NULL, NULL },
    { C3TEXT, "Level21Text", NULL, NULL },
    { NETEND, "EndNetGame", NULL, NULL },
    { LOADNET, "NoLoadInNetGame", NULL, NULL },
    { QLOADNET, "NoQLoadInNetGame", NULL, NULL },
    { SAVEDEAD, "SaveWhenNotPlaying", NULL, NULL },
    { NEWGAME, "NewNetGame", NULL, NULL },
    { GOTBFG9000, "GotBFG", NULL, NULL },
    { GOTCHAINGUN, "GotChainGun", NULL, NULL },
    { GOTPLASMA, "GotPlasmaGun", NULL, NULL },
    { GOTLAUNCHER, "GotRocketLauncher", NULL, NULL },
    { GOTSHOTGUN, "GotShotgun", NULL, NULL },
    { GOTSHOTGUN2, "GotDoubleBarrel", NULL, NULL },
    { C1TEXT, "Level7Text", NULL, NULL },
    { QSAVESPOT, "NoQuickSaveSlot", NULL, NULL },
    { C2TEXT, "Level12Text", NULL, NULL },
    { HUSTR_TALKTOSELF1, "TALKTOSELF1", NULL, NULL },
    { PD_BLUEO, "NeedBlueForObject", NULL, NULL },
    { PD_BLUEK, "NeedBlueForDoor", NULL, NULL },
    { PD_REDO, "NeedRedForObject", NULL, NULL },
    { PD_REDK, "NeedRedForDoor", NULL, NULL },
    { PD_YELLOWO, "NeedYellowForObject", NULL, NULL },
    { PD_YELLOWK, "NeedYellowForDoor", NULL, NULL },
    { HUSTR_TALKTOSELF3, "TALKTOSELF3", NULL, NULL },
    { HUSTR_TALKTOSELF4, "TALKTOSELF4", NULL, NULL },
    { HUSTR_CHATMACRO5, "DefaultCHATMACRO5", NULL, NULL },
    { E2TEXT, "Episode2Text", NULL, NULL },
    { HUSTR_TALKTOSELF5, "TALKTOSELF5", NULL, NULL },

	// TNT strings
    { THUSTR_10, "Tnt10Desc", NULL, NULL },
    { THUSTR_11, "Tnt11Desc", NULL, NULL },
    { THUSTR_12, "Tnt12Desc", NULL, NULL },
    { THUSTR_13, "Tnt13Desc", NULL, NULL },
    { THUSTR_14, "Tnt14Desc", NULL, NULL },
    { THUSTR_15, "Tnt15Desc", NULL, NULL },
    { THUSTR_16, "Tnt16Desc", NULL, NULL },
    { THUSTR_17, "Tnt17Desc", NULL, NULL },
    { THUSTR_18, "Tnt18Desc", NULL, NULL },
    { THUSTR_19, "Tnt19Desc", NULL, NULL },
    { THUSTR_1, "Tnt01Desc", NULL, NULL },
    { THUSTR_20, "Tnt20Desc", NULL, NULL },
    { THUSTR_21, "Tnt21Desc", NULL, NULL },
    { THUSTR_22, "Tnt22Desc", NULL, NULL },
    { THUSTR_23, "Tnt23Desc", NULL, NULL },
    { THUSTR_24, "Tnt24Desc", NULL, NULL },
    { THUSTR_25, "Tnt25Desc", NULL, NULL },
    { THUSTR_26, "Tnt26Desc", NULL, NULL },
    { THUSTR_27, "Tnt27Desc", NULL, NULL },
    { THUSTR_28, "Tnt28Desc", NULL, NULL },
    { THUSTR_29, "Tnt29Desc", NULL, NULL },
    { THUSTR_2, "Tnt02Desc", NULL, NULL },
    { THUSTR_30, "Tnt30Desc", NULL, NULL },
    { THUSTR_31, "Tnt31Desc", NULL, NULL },
    { THUSTR_32, "Tnt32Desc", NULL, NULL },
    { THUSTR_3, "Tnt03Desc", NULL, NULL },
    { THUSTR_4, "Tnt04Desc", NULL, NULL },
    { THUSTR_5, "Tnt05Desc", NULL, NULL },
    { THUSTR_6, "Tnt06Desc", NULL, NULL },
    { THUSTR_7, "Tnt07Desc", NULL, NULL },
    { THUSTR_8, "Tnt08Desc", NULL, NULL },
    { THUSTR_9, "Tnt09Desc", NULL, NULL },
    { T1TEXT, "TntLevel7Text", NULL, NULL },
    { T2TEXT, "TntLevel12Text", NULL, NULL },
    { T3TEXT, "TntLevel21Text", NULL, NULL },
    { T4TEXT, "TntEndGameText", NULL, NULL },
    { T5TEXT, "TntLevel31Text", NULL, NULL },
    { T6TEXT, "TntLevel32Text", NULL, NULL },

	// PLUTONIA strings
    { PHUSTR_10, "Plut10Desc", NULL, NULL },
    { PHUSTR_11, "Plut11Desc", NULL, NULL },
    { PHUSTR_12, "Plut12Desc", NULL, NULL },
    { PHUSTR_13, "Plut13Desc", NULL, NULL },
    { PHUSTR_14, "Plut14Desc", NULL, NULL },
    { PHUSTR_15, "Plut15Desc", NULL, NULL },
    { PHUSTR_16, "Plut16Desc", NULL, NULL },
    { PHUSTR_17, "Plut17Desc", NULL, NULL },
    { PHUSTR_18, "Plut18Desc", NULL, NULL },
    { PHUSTR_19, "Plut19Desc", NULL, NULL },
    { PHUSTR_1, "Plut01Desc", NULL, NULL },
    { PHUSTR_20, "Plut20Desc", NULL, NULL },
    { PHUSTR_21, "Plut21Desc", NULL, NULL },
    { PHUSTR_22, "Plut22Desc", NULL, NULL },
    { PHUSTR_23, "Plut23Desc", NULL, NULL },
    { PHUSTR_24, "Plut24Desc", NULL, NULL },
    { PHUSTR_25, "Plut25Desc", NULL, NULL },
    { PHUSTR_26, "Plut26Desc", NULL, NULL },
    { PHUSTR_27, "Plut27Desc", NULL, NULL },
    { PHUSTR_28, "Plut28Desc", NULL, NULL },
    { PHUSTR_29, "Plut29Desc", NULL, NULL },
    { PHUSTR_2, "Plut02Desc", NULL, NULL },
    { PHUSTR_30, "Plut30Desc", NULL, NULL },
    { PHUSTR_31, "Plut31Desc", NULL, NULL },
    { PHUSTR_32, "Plut32Desc", NULL, NULL },
    { PHUSTR_3, "Plut03Desc", NULL, NULL },
    { PHUSTR_4, "Plut04Desc", NULL, NULL },
    { PHUSTR_5, "Plut05Desc", NULL, NULL },
    { PHUSTR_6, "Plut06Desc", NULL, NULL },
    { PHUSTR_7, "Plut07Desc", NULL, NULL },
    { PHUSTR_8, "Plut08Desc", NULL, NULL },
    { PHUSTR_9, "Plut09Desc", NULL, NULL },
    { P1TEXT, "PlutLevel7Text", NULL, NULL },
    { P2TEXT, "PlutLevel12Text", NULL, NULL },
    { P3TEXT, "PlutLevel21Text", NULL, NULL },
    { P4TEXT, "PlutEndGameText", NULL, NULL },
    { P5TEXT, "PlutLevel31Text", NULL, NULL },
    { P6TEXT, "PlutLevel32Text", NULL, NULL },

	// Monster cast names...
    { CC_ARACH,  "ArachnotronName",       NULL, NULL },
    { CC_ARCH,   "ArchVileName",          NULL, NULL },
    { CC_BARON,  "BaronOfHellName",       NULL, NULL },
    { CC_CACO,   "CacodemonName",         NULL, NULL },
    { CC_DEMON,  "DemonName",             NULL, NULL },
    { CC_HEAVY,  "HeavyWeaponDudeName",   NULL, NULL },
    { CC_HELL,   "HellKnightName",        NULL, NULL },
    { CC_IMP,    "ImpName",               NULL, NULL },
    { CC_MANCU,  "MancubusName",          NULL, NULL },
    { CC_PAIN,   "PainElementalName",     NULL, NULL },
    { CC_REVEN,  "RevenantName",          NULL, NULL },
    { CC_SHOTGUN,"ShotgunGuyName",        NULL, NULL },
    { CC_CYBER,  "CyberdemonName",        NULL, NULL },
    { CC_SPIDER, "SpiderMastermindName",  NULL, NULL },
    { CC_ZOMBIE, "ZombiemanName",         NULL, NULL },
	{ CC_LOST,   "LostSoulName",          NULL, NULL },
	{ CC_HERO,   "OurHeroName",           NULL, NULL },

	{ NULL, NULL, NULL, NULL }  // End sentinel
};

langinfo_t cheat_list[] =
{
	{ "idbehold",   "idbehold9",  "BEHOLD menu",  NULL },
	{ "idbeholda",  "idbehold5",  "Auto-map",     NULL },
	{ "idbeholdi",  "idbehold3",  "Invisibility", NULL },
	{ "idbeholdl",  "idbehold6",  "Lite-Amp Goggles", NULL },
	{ "idbeholdr",  "idbehold4",  "Radiation Suit",   NULL },
	{ "idbeholds",  "idbehold2",  "Berserk", NULL },
	{ "idbeholdv",  "idbehold1",  "Invincibility", NULL },
	{ "idchoppers", "idchoppers", "Chainsaw",   NULL },
	{ "idclev",     "idclev",     "Level Warp", NULL },
	{ "idclip",     "idclip",     "No Clipping 2", NULL },
	{ "iddqd",      "iddqd",      "God mode",  NULL },
	{ "iddt",       "iddt",       "Map cheat", NULL },
	{ "idfa",       "idfa",       "Ammo", NULL },
	{ "idkfa",      "idkfa",      "Ammo & Keys",  NULL },
	{ "idmus",      "idmus",      "Change music", NULL },
	{ "idmypos",    "idmypos",    "Player Position", NULL },
	{ "idspispopd", "idspispopd", "No Clipping 1",   NULL },

	{ NULL, NULL, NULL, NULL }  // End sentinel
};


//------------------------------------------------------------------------

bool TextStr::ReplaceString(const char *before, const char *after)
{
	assert(after[0]);

	for (int i = 0; lang_list[i].orig_text; i++)
	{
		langinfo_t *lang = lang_list + i;

		if (StrCaseCmp(before, lang->orig_text) != 0)
			continue;

		int len = strlen(lang->orig_text);

		if (! lang->new_text)
			lang->new_text = new char[len + 5];

		StrMaxCopy(lang->new_text, after, len + 4);

		return true;
	}

	return false;
}

bool TextStr::ReplaceCheat(const char *deh_name, const char *str)
{
	assert(str[0]);

	for (int i = 0; cheat_list[i].orig_text; i++)
	{
		langinfo_t *cht = cheat_list + i;

		if (StrCaseCmp(deh_name, cht->deh_name) != 0)
			continue;

		int len = strlen(cht->orig_text);

		if (! cht->new_text)
			cht->new_text = new char[len + 1];

		StrMaxCopy(cht->new_text, str, len);

		return true;
	}

	return false;
}


//------------------------------------------------------------------------

namespace TextStr
{
	bool got_one;

	void BeginTextLump(void)
	{
		WAD::NewLump("DDFLANG");

		WAD::Printf(GEN_BY_COMMENT);
		WAD::Printf("<LANGUAGES>\n\n");
		WAD::Printf("[ENGLISH]\n");
	}

	void FinishTextLump(void)
	{
		WAD::Printf("\n");
		WAD::FinishLump();
	}

	void WriteTextString(const langinfo_t *info)
	{
		if (! got_one)
		{
			got_one = true;
			BeginTextLump();
		}

		WAD::Printf("%s = \"", info->ldf_name);

		const char *str = info->new_text ? info->new_text : info->orig_text;

		for (; *str; str++)
		{
			if (*str == '\n')
			{
				WAD::Printf("\\n\"\n  \"");
				continue;
			}

			if (*str == '\"')
			{
				WAD::Printf("\\n\"\n  \"");
				continue;
			}

			// FIXME: handle non-english characters (e.g. french accents)

			WAD::Printf("%c", *str);
		}

		WAD::Printf("\";\n");
	}
}

void TextStr::ConvertLDF(void)
{
	got_one = false;

	for (int i = 0; lang_list[i].orig_text; i++)
	{
	    if (! all_mode && ! lang_list[i].new_text)
			continue;

		WriteTextString(lang_list + i);
	}

	// --- cheats ---

	if (got_one)
		WAD::Printf("\n");

	for (int i = 0; cheat_list[i].orig_text; i++)
	{
	    if (! all_mode && ! cheat_list[i].new_text)
			continue;

		WriteTextString(cheat_list + i);
	}

	if (got_one)
		FinishTextLump();
}


// ISSUES:
//    EDGE has no "medikit you really needed" in LDF.
//    EDGE is lacking Level names and Finale texts for TNT and PLUTONIA.
