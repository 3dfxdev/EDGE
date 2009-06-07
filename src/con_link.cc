//----------------------------------------------------------------------------
//  LIST OF ALL CVARS
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2007-2008  The EDGE Team.
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

#include "i_defs.h"

#include "con_var.h"


extern cvar_c r_width, r_height, r_depth, r_fullscreen;
extern cvar_c r_colormaterial, r_colorlighting;
extern cvar_c r_dumbsky, r_dumbmulti, r_dumbcombine, r_dumbclamp;
extern cvar_c r_nearclip, r_farclip, r_fadepower;
extern cvar_c r_fov, r_zoomedfov;
extern cvar_c r_mipmapping, r_smoothing;
extern cvar_c r_dithering, r_hq2x;
extern cvar_c r_dynamiclight;

extern cvar_c am_smoothmap;
extern cvar_c m_diskicon, m_busywait, m_screenhud;
extern cvar_c m_obituaries;
extern cvar_c debug_subsector;



// TEMP
cvar_c s_volume, s_musicvol;


// Flag letters:
// =============
//
//   r : read only, user cannot change it
//   c : config file (saved and loaded)
//


cvar_link_t  all_cvars[] =
{
	{ "r_width",      &r_width,      "rc",  "640"   },
	{ "r_height",     &r_height,     "rc",  "480"   },
    { "r_depth",      &r_depth,      "rc",  "32"    },
    { "r_fullscreen", &r_fullscreen, "rc",  "1"     },

	{ "r_nearclip",   &r_nearclip,   "c",   "4"     },
	{ "r_farclip",    &r_farclip,    "c",   "64000" },
	{ "r_fadepower",  &r_fadepower,  "c",   "1",    },
	{ "r_fov",        &r_fov,        "c",   "90",   },
	{ "r_zoomedfov",  &r_zoomedfov,  "c",   "10",   },

	{ "r_mipmapping", &r_mipmapping, "c",   "0"  },
	{ "r_smoothing",  &r_smoothing,  "c",   "0"  },
	{ "r_dithering",  &r_dithering,  "c",   "0"  },
	{ "r_hq2x",       &r_hq2x,       "c",   "1"  },

	{ "r_dynamiclight",  &r_dynamiclight,  "c",  "1"  },

	{ "r_colormaterial", &r_colormaterial, "",   "1"  },
	{ "r_colorlighting", &r_colorlighting, "",   "1"  },
	{ "r_dumbsky",       &r_dumbsky,       "",   "0"  },
	{ "r_dumbmulti",     &r_dumbmulti,     "",   "0"  },
	{ "r_dumbcombine",   &r_dumbcombine,   "",   "0"  },
	{ "r_dumbclamp",     &r_dumbclamp,     "",   "0"  },

	{ "am_smoothmap", &am_smoothmap, "c",   "1",    },
	{ "m_diskicon",   &m_diskicon,   "c",   "1",    },
	{ "m_busywait",   &m_busywait,   "c",   "1",    },
	{ "m_screenhud",  &m_screenhud,  "c",   "0",    },
	{ "m_obituaries", &m_obituaries, "c",   "1",    },

	{ "s_volume",     &s_volume,     "c",   "10",   },
	{ "s_musicvol",   &s_musicvol,   "c",   "10",   },

	{ "debug_subsector", &debug_subsector, "", "0" },

//---- END OF LIST -----------------------------------------------------------

	{ NULL, NULL, NULL, NULL }
};

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
