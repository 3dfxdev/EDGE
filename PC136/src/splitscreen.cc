// EDGE2 SPLIT SCREEN SUPPORt
// 
// THIS CODE WAS CAPTURED FROM REMOOD (DOOM LEGACY), SINCE BOTH ARE IN C++ FORMATS.

// THIS WILL PIGGYBACK ON EDGE2'S CURRENT NETCODE SUPPORT (WHAT LITTLE IS LEFT)
// THIS WILL BE FOR LOCAL PLAY (ASSUME - REPLACING A BOT WILL BE "PLAYER_2" (splitscreen)

Directory Structer of Remood:
command.h - 	CV_NOTINNET = 32,			// some varaiable can't be changed in network but is not netvar (ex: splitscreen)
//description - Console Variable 

console.c - int con_lineowner[CON_MAXHUDLINES];	//In splitscreen, which player gets this line of text
										 //0 or 1 is player 1, 2 is player 2
//description - Console text output for player 2

doomdef.h - #define SPLITSCREEN
//description - internal data structure

doomstat.h - // Player taking events, and displaying.
extern int consoleplayer[MAXSPLITSCREENPLAYERS];
extern int displayplayer[MAXSPLITSCREENPLAYERS];
//description - global vars of internal state

d_clisrv.c - Line 142: 	for (i = 0; i < cv_splitscreen.value+1; i++)
//description - network game and protocol

d_main.c - lines 331 - 368, line 787, and 1738-1739
//description - main program and game loop

d_netcmd.c - 	Line 775: 	if (!cv_splitscreen.value)
	Line 786: 	for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
	Line 794: 	for (i = 0; i < cv_splitscreen.value + 1; i++)
//description - host/client network commands

d_netcmd.h - // added 16-6-98 : splitscreen
extern consvar_t cv_splitscreen;
//description - d_netcmd HEADER file

d_prof.c - PLAYER AND SPLITSCREEN PLAYER PROFILES...WILL NOT SUPPORT!!

g_game.c - 
int consoleplayer[MAXSPLITSCREENPLAYERS];				// player taking events and displaying
int displayplayer[MAXSPLITSCREENPLAYERS];				// view being displayed
int secondarydisplayplayer;		// for splitscreen
int statusbarplayer;			// player who's statusbar is displayed
										// (for spying with F12)
										
										
	//BOT_InitLevelBots ();  !!!!!!!!!!!!!!!!!!!! EDGE2 BOTS?!!!!!!!!!!!!!!!

	displayplayer[0] = consoleplayer[0];	// view the guy you are playing
	if (!cv_splitscreen.value)
		displayplayer[1] = consoleplayer[0];
	else
		for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
			displayplayer[i] = consoleplayer[i];

	gameaction = ga_nothing;
	
	--1717
	multiplayer = playeringame[1];
	if (playeringame[1] && !netgame)
		CV_SetValue(&cv_splitscreen, 1);

	if (setsizeneeded)
		R_ExecuteSetViewSize();

		
g_game. h - 
extern angle_t localangle[MAXSPLITSCREENPLAYERS];
extern int localaiming[MAXSPLITSCREENPLAYERS];	// should be a angle_t but signed
//description - game loop and event handling

g_input.c/.H - ALLOW SPLITSCREEN PLAYER CONTROL STUFF!

HU_STUFF.C - DISPLAY SECOND PLAYER MESSAGES (LOOK AT THIS LATER?)

I_JOY.C - SHARE JOYSTICK STUFF WITH SPLITSCREEN PLAYER?

M_CHEAT.C - SPLITSCREEN CRAP

M_MENU.C - NUMBER OF PLAYERS, CV_NG_SPLITSCREEN! LINE 795
M_MENU.H LINE 67 - SHOW OR HIDE THE SETUP FOR PLAYER 2 (CALLED AT SPLITSCREEN CHANGE, IN EDGE2 THERE WILL BE NO SETUP! (...MAYBE??)

M_MENUFN.C - void M_NGOptionChange(void);

CV_PossibleValue_t NGSplitScreenValue[] =
{
	{1, "Two"},
	{2, "Three"},
	{3, "Four"},
	
	{0, NULL},
};

CV_PossibleValue_t NGSplitScreenValue2[] =
{
	{0, "One"},
	{1, "Two"},
	{2, "Three"},
	{3, "Four"},
	
	{0, NULL},
};

consvar_t cv_ng_splitscreen = {"ng_splitscreen", "1", CV_SAVE, NGSplitScreenValue};
consvar_t cv_ng_splitscreen2 = {"ng_splitscreen2", "1", CV_SAVE, NGSplitScreenValue2};
LINE 896, 898 - 
	Line 896: 	// Unsetb splitscreen, we play alone here
	Line 898: 		split = cv_ng_splitscreen.value;
	Line 1059: 	CV_RegisterVar(&cv_ng_splitscreen);
	Line 1060: 	CV_RegisterVar(&cv_ng_splitscreen2);
	
P_INTER.C - HANDLE COLLISION DETECTION

		//added:16-01-98:changed consoleplayer to displayplayer
		//               (hear the sounds from the viewpoint)
		for (i = 0; i < cv_splitscreen.value + 1; i++)
			if (player == &players[displayplayer[i]])
				S_StartSound(NULL, sfx_wpnup);
		return false;
	}
	1245
	
	//added:16-01-98:consoleplayer -> displayplayer (hear sounds from viewpoint)
	for (i = 0; i < cv_splitscreen.value + 1; i++)
		if (player == &players[displayplayer[i]])
			S_StartSound(NULL, sound);
}
KILLMOBJ

		for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
			if (playeringame[consoleplayer[i]] && target->player == &players[consoleplayer[i]])
				localaiming[i] = 0;
	}
	
P_MOBJ.C Line 1493: 	for (i = 0; i < MAXSPLITSCREENPLAYERS; i++)
//DESCRIPTION - EDGE2 HAS THIS! SPLICE AND DICE?

	



