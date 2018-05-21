//------------------------------------------------------------------------
// BOOKTEXT : Unix/FLTK Manual Text
//------------------------------------------------------------------------
//
//  GL-Friendly Node Builder (C) 2000-2007 Andrew Apted
//
//  Based on 'BSP 2.3' by Colin Reed, Lee Killough and others.
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

#include "local.h"

#define GRN  "@C61"   // selection looks bad, oh well


static const char *contents_text[] =
{
  "",
  "@c@l@b glBSPX Manual",
  "",
  "@-",
  "",
  "@r by Andrew Apted   ",
  "@r Updated: JULY 2007   ",
  "",
  "@c@m Table of Contents",
  "",
  "#L01@c" GRN " 1. Introduction",
  "#L02@c" GRN " 2. Using glBSPX",
  "#L03@c" GRN " 3. Interaction with other tools",
  "#L04@c" GRN " 4. How glBSP Works",
  "#L05@c" GRN " 5. Differences to BSP 2.3",
  "#L06@c" GRN " 6. Contact and Links",
  "#L07@c" GRN " 7. Acknowledgements",
  NULL
};


//------------------------------------------------------------------------

static const char *intro_text[] =
{
  "",
  "@c@l 1. Introduction",
  "",
  "@-",
  "",
  "#P00",
  "glBSP is a nodes builder specially designed to be used with OpenGL-based",
  "DOOM game engines. It adheres to the \"GL-Friendly Nodes\"",
  "specification, which means it adds some new special nodes to a WAD",
  "file that makes it very easy for an OpenGL DOOM engine to",
  "compute the polygons needed for drawing the levels.",
  "",
  "#P00",
  "There are many DOOM ports that understand the GL Nodes which glBSP",
  "creates, including: EDGE, the Doomsday engine (JDOOM), Doom3D, PrBoom,",
  "ZDoomGL, Vavoom and Doom3D.#- See the Contact and Links page.",
  "",
  "#P00",
  "glBSPX is the GUI (graphical user interface) version, which runs",
  "in a window. There is also a command-line version called simply",
  "'glbsp', which is designed to be run from a DOS box (Windows)",
  "or a text terminal (Linux etc). ",
  "glBSPX lets you perform all the usual node-building jobs,",
  "though the command-line version supports some more (rarely-useful)",
  "options.",
  "",
  "@m Status:",
  "",
  "#P00",
  "The current version is 2.24. It has been tested and",
  "known to work on numerous large wads, including DOOM I, DOOM II,",
  "TeamTNT's Eternal III, Fanatic's QDOOM, and many others.",
  "",
  "@m Legal stuff:",
  "",
  "#P00",
  "glBSP and glBSPX are Copyright (C) 2000-2007 Andrew Apted.  It",
  "was originally based on 'BSP 2.3' (C) Colin Reed and Lee Killough,",
  "which was created from the basic theory stated in DEU5 (OBJECTS.C)",
  "by Raphael Quinet.",
  "",
  "#P00",
  "This GUI version (glBSPX) is based in part on the work of the FLTK ",
  "project.  See http://www.fltk.org.",
  "",
  "#P00",
  "glBSP and glBSPX are free software, under the GNU General Public License",
  "(GPL).  Click on the the Help > License menu to see the full ",
  "text.  In particular, there is ABSOLUTELY NO WARRANTY.  USE AT",
  "YOUR OWN RISK.",
  "",
  " All trademarks are the propriety of their owners.",
  "",
  NULL
};


//------------------------------------------------------------------------

static const char *using_text[] =
{
  "",
  "@c@l 2. Using glBSPX",
  "",
  "@-",
  "",
  "#P00",
  "The way glBSP operates is pretty simple: it will load the input",
  "wad file, build the nodes for the levels that it finds, and then",
  "save to the output file.  All this is triggered when you press",
  "on the big 'BUILD' button in the main window. ",
  "The 'Files' box let you specify the Input and Output filenames,",
  "and the other boxes let you control various aspects about the way",
  "nodes are built.",
  "",
  "#P00",
  "On the bottom of the main window is the text output box, which",
  "shows various messages during node building (stuff that the",
  "command-line version normally prints in the terminal or DOS box). ",
  "Watch out for warning messages, which appear in a red font, they",
  "can indicate serious problems that were found in your levels.",
  "",
  
  "@m  Build Modes",
  "",
  "#P00",
  "There are five different build modes in glBSPX, which can be",
  "selected in the upper left box in the main window.",
  "Here is a description of them:",
  "",
  "@t   GWA Mode",
  "#P20",
  "Use this mode for general use, e.g you've downloaded a normal",
  "DOOM wad and wish to play it with an OpenGL DOOM engine",
  "(one which is GLBSP-aware, of course). ",
  "This mode will put all the GL Nodes into a separate file with",
  "the \".gwa\" extension.  The engine should notice the GWA file",
  "and load it automatically.",
  "",
  "#P20",
  "The rest of the modes below create a new self-contained WAD file,",
  "which will contain any GL Nodes that are built.",
  "",
  "@t   GL, Normal if missing",
  "#P20",
  "This mode builds the GL nodes, and will detect if the Normal nodes",
  "are missing and build them too when absent. Hence it will keep any",
  "pre-existing Normal nodes, so less work to do, plus the normal",
  "nodes may be better optimised for non-GLBSP-aware DOOM ports.",
  "",
  "@t   GL and Normal nodes",
  "#P20",
  "This mode builds both the GL nodes and the Normal nodes.",
  "",
  "@t   GL nodes only",
  "#P20",
  "This mode builds only the GL nodes, completely ignoring any",
  "pre-existing Normal nodes (even if absent).  This mode is",
  "probably not very useful.",
  "",

  "@m Misc Options",
  "",
  "#P00",
  "The upper right box in the main window contain a set of",
  "miscellaneous options. Some of them only appear when certain",
  "build modes are enabled, because in other modes they would",
  "be ignored.  The full set of misc options are:",
  "",
  "@t   Extra Warnings",
  "#P20",
  "Shows extra warning messages, which detail various",
  "non-serious problems that glBSP discovers while building the",
  "nodes.  Often these warnings show a real",
  "problem in the level (e.g a non-closed sector or",
  "invalid sidedef), so they are worth checking now and then.",
  "",
  "@t   Don't clobber REJECT",
  "#P20",
  "Normally glBSP will create an simple REJECT map for",
  "each level.  This options prevents any existing",
  "REJECT map from being clobbered, such as one built by a",
  "dedicated reject building tool (RMB, etc).",
  "",
  "@t   Pack Sidedefs",
  "#P20",
  "Pack sidedefs, by detecting which sidedefs are",
  "identical and removing the duplicates, producing a",
  "smaller PWAD.",
  "",
  "#P20",
  "NOTE: this may make your level a lot",
  "harder to edit!  Therefore this option is most useful",
  "when producing a final WAD for public release.",
  "",
  "@t   Fresh Partition Lines",
  "#P20",
  "Normally glBSP will cheat a bit and",
  "re-use the original node information to create the GL",
  "nodes, doing it much faster.  When this happens, the",
  "message \"Using original nodes to speed things up\" is",
  "shown.",
  "",
  "#P20",
  "This option forces glBSP to create \"fresh\" GL nodes.",
  "The downside to reusing the original nodes is that they",
  "may not be as good as the ones glBSP creates, e.g. the",
  "special checks to minimise slime-trails don't kick in,",
  "and the -factor value doesn't have any effect.",
  "",

  "@m Factor",
  "",
  "#P00",
  "The factor value is located just below the Misc options. ",
  "It specifies the cost assigned to seg splits. ",
  "Larger values make seg splits more costly (and thus glBSP tries",
  "harder to avoid them), but smaller values produce better BSP",
  "trees.  See the section 'How glBSP Works' for more info. ",
  "The default factor is known to be a good compromise.",
  "",

  NULL
};


//------------------------------------------------------------------------

static const char *note_tc_text[] =
{
  "",
  "@c@l 3. Interaction with other tools",
  "",
  "@-",
  "",
  "#P00",
  "As far as I know,",
  "none of the various WAD tools that exist (such as DSHRINK, CLEANWAD,",
  "DEUTEX, etc..) are 'glBSP aware', they will rearrange or even remove",
  "some of the special GL entries, and everything goes pear shaped.",
  "",
  "#P00",
  "When using a reject building tool (such as RMB), you need to give",
  "the -noreject to glBSP to prevent the good REJECT data from being",
  "overwritten.",
  "",
  "@b *** DO NOT: ***",
  "",
  "#P13",
  "+ Run dshrink on your map WADs at any time!",
  "",
  "#P13",
  "+ Run cleanwad on your map WADs *after* you have compiled your GL",
  "  friendly nodes!  (The GL nodes will be removed).",
  "",
  "#P13",
  "+ Use Wintex/DEUSF to merge map WADs with GL friendly nodes in them!",
  "  (The GL node entries will be rearranged, causing problems).",
  "",
  NULL
};


//------------------------------------------------------------------------

static const char *howwork_text[] =
{
  "",
  "@c@l 4. How glBSP Works",
  "",
  "@-",
  "",
  "#P00",
  "A node builder works like this: say you are looking at your level in",
  "the automap or in the level editor.  The node builder needs to pick a",
  "line (stretching to infinity) that divides the whole map in two halves",
  "(can be rough).  Now cover up one half with your hand, and repeat the",
  "process on the remaining half.  The node builder keeps doing this",
  "until the areas that remain are convex (i.e. none of the walls can",
  "block the view of any other walls when you are inside that area).",
  "",
  "#P00",
  "Those infinite lines are called 'partition lines', and when they cross",
  "a linedef, the linedef has to be split.  Each split piece is called a",
  "'seg', and every split causes more segs to be created.  Having fewer",
  "segs is good: less to draw & smaller files, so partition lines are",
  "chosen so that they minimise splits.  The 'Factor' value controls how",
  "costly these splits are.  Higher values cause the node builder to try",
  "harder to avoid splits.",
  "",
  "#P00",
  "So, each 'partition' line splits an area (or 'space') of the level",
  "into *two* smaller spaces.  This is where the term 'Binary Space",
  "Partition' (BSP) comes from.",
  "",
  "#P00",
  "Another thing: having a good BSP tree is also important, and helps for",
  "faster rendering & smaller files.  Thus the node builder also tries to",
  "make the BSP tree good (by making it balanced, and keeping it small). ",
  "If the Factor value is too high, it will care too much about the",
  "splits, and probably make a bad BSP tree.  How good the BSP tree is",
  "can be gauged by the output line that reads:",
  "",
  "@c@t Heights of left and right subtrees = (12,24)",
  "",
  "#P00",
  "Lower values are better (the BSP tree is smaller), and values that are",
  "closer together are also better (the BSP is more balanced).",
  "",
  NULL
};


//------------------------------------------------------------------------

static const char *diff_text[] =
{
  "",
  "@c@l 5. Differences to BSP 2.3",
  "",
  "@-",
  "",
  "#P00",
  "As mentioned in the readme file, glBSP was originally based on BSP 2.3.  Most of",
  "the code has been rewritten, however, and some features of BSP were",
  "changed or abandoned.  Features that are different:",
  "",
  "#P13",
  "+ This GUI version, glBSPX, is completely new !",
  "",
  "#P13",
  "+ When the output file is not specified (i.e. no -o option), then",
  "  the default output file will be a GWA file with the same name. ",
  "  Under BSP 2.3, the default output file would be \"tmp.wad\". ",
  "  (This only applies to the command-line version).",
  "",
  "#P13",
  "+ All code for doing visplane checks has been removed.  It was very",
  "  complex stuff, and for OpenGL DOOM ports this checking is not",
  "  needed.  Thus glBSP does not accept the following options that",
  "  BSP 2.3 supports: -thold, -vp, -vpmark, -vpwarn.",
  "",
  "#P13",
  "+ glBSP works on big-endian platforms (like the Mac).",
  "",
  "#P13",
  "+ The 'just for a grin' special feature where a linedef with tag",
  "  999 would cause an angle adjustment was removed.",
  "",
  "#P13",
  "+ glBSP has Hexen support.",
  "",
  "#P13",
  "+ glBSP compresses the blockmap, and can pack sidedefs.",
  "",
  "#P13",
  "+ glBSP has a much more modular architecture, and can even serve",
  "  as a plug-in for other programs.",
  "",
  NULL
};


//------------------------------------------------------------------------

static const char *contact_text[] =
{
  "",
  "@c@l 6. Contact and Links",
  "",
  "@-",
  "",
  "  The homepage for glBSP can be found here:",
  "",
  "@t     http://glbsp.sourceforge.net/",
  "",
  "  Questions, bug reports, suggestions, etc... can be made on the",
  "  web forum at:",
  "",
  "@t     https://sourceforge.net/forum/forum.php?forum_id=33133",
  "",
  "",
  "@m Compatible Engines",
  "",
  "@t   EDGE     http://edge.sourceforge.net/",
  "@t   JDOOM    http://www.doomsdayhq.com/",
  "@t   PrBOOM   http://prboom.sourceforge.net/",
  "@t   ZDoomGL  http://zdoomgl.mancubus.net/",
  "@t   Vavoom   http://www.vavoom-engine.com/",
  "@t   Doom3D   http://doomworld.com/doom3d/",
  "",
  NULL
};


//------------------------------------------------------------------------

static const char *acknow_text[] =
{
  "",
  "@c@l 7. Acknowledgements",
  "",
  "@-",
  "",
  "#P10",
  "Andy Baker, for making binaries, writing code and other help.",
  "",
  "#P10",
  "Marc A. Pullen, for testing and helping with the documentation.",
  "",
  "#P10",
  "Lee Killough and André Majorel, for giving their permission to put",
  "glBSP under the GNU GPL.",
  "",
  "#P10",
  "Janis Legzdinsh for fixing many problems with Hexen wads.",
  "",
  "#P10",
  "Darren Salt has sent in numerous patches.",
  "",
  "#P10",
  "Jaakko Keränen, who gave some useful feedback on the GL-Friendly",
  "Nodes specification.",
  "",
  "#P10",
  "The authors of FLTK (Fast Light Tool Kit), for a nice LGPL C++ GUI",
  "toolkit that even I can get working on both Linux and Win32.",
  "",
  "#P10",
  "Marc Rousseau (author of ZenNode 1.0), Robert Fenske Jr (author of",
  "Warm 1.6), L.M. Witek (author of Reject 1.1), and others, for",
  "releasing the source code to their WAD utilities, and giving me lots",
  "of ideas to \"borrow\" :), like blockmap packing.",
  "",
  "#P10",
  "Colin Reed and Lee Killough (and others), who wrote the original BSP",
  "2.3 which glBSP was based on.",
  "",
  "#P10",
  "Matt Fell, for the Doom Specs v1.666.",
  "",
  "#P10",
  "Raphael Quinet, for DEU and the original idea.",
  "",
  "#P10",
  "id Software, for not only creating such an irresistable game, but",
  "releasing the source code for so much of their stuff.",
  "",
  "#P10",
  ". . . and everyone else who deserves it ! ",
  "",
  NULL
};


//------------------------------------------------------------------------

const book_page_t book_pages[] =
{
  { contents_text },   // #00
  { intro_text },      // #01
  { using_text },      // #02
  { note_tc_text },    // #03
  { howwork_text },    // #04
  { diff_text },       // #05
  { contact_text },    // #06
  { acknow_text },     // #07
  { NULL }
};

