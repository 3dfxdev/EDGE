//WLF_EXTENSION ADDS ALL WL6 FILES ALL AT ONCE FOR WOLFENSTEIN, JUST FOR TESTING, MAYBE MAKE THIS MORE ROBUST IN THE FUTURE...?
const char *wlf_extension[] = { "AUDIOHED", "AUDIOT", "GAMEMAPS","MAPHEAD", "VGADICT", "VGAGRAPH", "VGAHEAD", "VSWAP", NULL }; //test to load this bitch up. . .
static void IdentifyWolfenstein(void)
{
	std::string wolf_par;
	std::string wolf_file;
	std::string wolf_dir;
	
	const char *w = M_GetParm("-wolf3d"); ///Yep, the big one! 
	
	wolf_par = std::string("MAPHEAD.WL6");; //Brute force MAPHEAD, just for now.

	// Should the Wolfenstein directory not be set by now, then we
	// use our standby option of the current directory.
	if (wolf_dir.empty())
		wolf_dir = ".";
	
	// Should the Wolf Parameter not be empty then it means
	// that one was given which is not a directory. Therefore
	// we assume it to be a name or some shit! 
	else if (!wolf_par.empty())
	{
		std::string fn = wolf_par;

		// Is it missing the extension?
		std::string ext = epi::PATH_GetExtension(wolf_par.c_str());
		if (ext.empty())
		{
			fn += ("." WOLFDATEXT); //Binds us to WL6 files!
		}

		// If no directory given...
		std::string dir = epi::PATH_GetDir(fn.c_str());
		if (dir.empty())
			wolf_file = epi::PATH_Join(wolf_dir.c_str(), fn.c_str());
		else
			wolf_file = fn;

		if (!epi::FS_Access(wolf_file.c_str(), epi::file_c::ACCESS_READ))
		{
			I_Error("IdentifyVersion: Unable to add specified Wolfenstein file: '%s'", fn.c_str());
		}
	}
	else
	{
		
		const char *location2;
		int max = 1;
		
		if (stricmp(wolf_dir.c_str(), game_dir.c_str()) != 0)
		{
			// WOLF directory & game directory differ 
			// therefore do a second loop which will
			// mean we check both.
			max++;
		}
		
	bool done = false;
	
	for (int i = 0; i < max && !done; i++)
		{
			location2 = (i == 0 ? wolf_dir.c_str() : game_dir.c_str());
			
			//
			// go through the available WL6 names constructing an access
			// name for each, adding the file if they exist.
			//
			// -ACB- 2000/06/08 Quit after we found a file - don't load
			//                  more than one IWAD
			//

			//
			// FIND WL6
			//
			for (int w_idx = 0; wlf_extension[w_idx]; w_idx++)
			{
				std::string fn(epi::PATH_Join(location2, wlf_extension[w_idx]));

				fn += ("." WOLFDATEXT); //Wolfenstein Datas, maybe instead of +=, use an iterator, fn++?

				if (epi::FS_Access(fn.c_str(), epi::file_c::ACCESS_READ))
				{
					wolf_file = fn;
					done = true;
					I_Printf("wolf_file returning true!/n");
					break;
				}
			}
		}
	}
	
	if (wolf_file.empty())
		I_Error("NO WOLF3D install found, ABORTING!\n");
	
		W_AddRawFilename(wolf_file.c_str(), FLKIND_WL6); //<--- This needs defining! Done.
		
		wolf_base = epi::PATH_GetBasename(wolf_file.c_str());
		
		I_Debugf("WOLF WL6 BASE = [%s]\n", wolf_base.c_str()); //Eventually we should collect all of these. . .
		
		// Emulate this behaviour?
		
		//this will join edge2.wad with Wolf's MAPHEAD (and etc., files --)
		std::string reqwolf(epi::PATH_Join(wolf_dir.c_str(), REQUIREDWAD "." WOLFDATEXT));
		
		
	if (!epi::FS_Access(reqwolf.c_str(), epi::file_c::ACCESS_READ))
	{
		reqwolf = epi::PATH_Join(game_dir.c_str(), REQUIREDWAD "." WOLFDATEXT);

		if (!epi::FS_Access(reqwad.c_str(), epi::file_c::ACCESS_READ))
		{
			I_Error("IdentifyVersion: Could not find required Wolf3D %s.%s!\n",
				REQUIREDWAD, WOLFDATEXT);
		}
	}
	
	W_AddRawFilename(reqwolf.c_str(), FLKIND_EWad);
	I_Printf("Wolfenstein Data is loaded and joined with 3DGE -- let's keep going!/n")
}
