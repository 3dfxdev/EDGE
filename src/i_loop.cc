//----------------------------------------------------------------------------
//  EDGE SDL System Stuff
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2005  The EDGE Team.
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
#include "i_sdlinc.h"

#include "e_main.h"

// Application active?
int app_state = APP_STATE_ACTIVE;

//
// I_Loop
//
void I_Loop(void)
{
	bool gameon = true;
	while (gameon)
	{
		// We always do this once here, although the engine may makes in own
		// calls to keep on top of the event processing
		I_ControlGetEvents(); 
		
		if (app_state & APP_STATE_ACTIVE)
			engine::Tick();
			
		if (app_state & APP_STATE_PENDING_QUIT) // Engine may have set this 
			gameon = false; 
	}

	return;
}
