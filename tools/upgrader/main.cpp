//----------------------------------------------------------------------------
//  EDGE Upgrader Tool
//----------------------------------------------------------------------------
//
//  Copyright (c) 2004  The EDGE Team.
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

#include "epi.h"

#include "epiarray.h"
#include "epicrc.h"
#include "epiendian.h"
#include "epierror.h"
#include "epifile.h"
#include "epifilesystem.h"
#include "epistring.h"
#include "epitype.h"

#include <iostream>

// Versioning
#define VERSION_FILE		"version.dat"
#define VERSION_ID			0x00000001

// Debugging stuff...
#define DEBUG 

// Parameter keys...
#define KEY_CFGDIR			"--config-dir"
#define KEY_EDGEDIR			"--edge-dir"
#define KEY_HELP			"--help"

/*	--> For future use...
	#define KEY_EXISTINGVERSION "--existing-version"
	#define KEY_MODE			"--mode"
*/

// Edge settings
#define EDGE_BUG_ADDR		"<http://edge.sourceforge.net>"

#ifdef WIN32
#define EDGE_CFG_SUBDIR		"Application Data\\Edge"
#else
#define EDGE_CFG_SUBDIR		".edge"
#endif

#define EDGE_CFG_NAME		"edge.cfg"

#define EDGE_SG_MASK		"*.esg"
#define EDGE_SGSUBDIR		"savegame"

// Error Codes
enum error_e
{
	ERROR_GENERICUSER,
	ERROR_INTERNAL,
	ERROR_NEEDHELP,
	ERROR_NOCONFIGDIR,
	ERROR_NOEDGEDIR,
	ERROR_UNABLETOCREATEDIR,
	ERROR_UPGRADEDALREADY,
	ERROR_NUMTYPES
};

// Upgrade Modes (THIS IS SAVED OFF - PLEASE DO NOT MODIFY THE ORDER OF THE ENUM!)
enum upgrademode_e
{
	UPGRADEMODE_UNKNOWN,		
	UPGRADEMODE_127_128,		// Version 1.27 to 1.28
	UPGRADEMODE_NUMMODES		
};

// Helper class for managing the parameter list
struct param_s
{
    param_s()
    {
        name = NULL;
        value = NULL;
    }

    epi::string_c *name;
    epi::string_c *value;
};

class paramlist_c : public epi::array_c
{
public:
    paramlist_c() : epi::array_c(sizeof(param_s))
    {
    }
    
    ~paramlist_c()
    {
        Clear();
    }
    
private:
	void CleanupObject(void *obj)
	{
		param_s *p = (param_s*)obj;
		if (p->name)  { delete p->name;  }
		if (p->value) {	delete p->value; }
	}

public:
	//
	// Find the parameter by given key
	//
	int FindParamByKey(char *k)
	{
		if (!k)
			return -1;

		epi::array_iterator_c it;
		int i;
		param_s* p;

		i = 0;
		it = GetBaseIterator();
		while (it.IsValid())
		{
			p = (param_s*)((void*)it);

			if (!p->name->Compare(k))
			{
				return i;	// Got a matching key - return the index
			}

			i++;
			it++;
		}

		return -1;
	}

	//
	// GetEntry
	//
	param_s* GetEntry(int idx)
	{
		return (param_s*)FetchObject(idx);
	}

	//
	// GetSize()
	//
	int GetSize()
	{
		return array_entries;
	}

	//
	// Load parameter list for std main() arguments
	//
    bool Load(int argc, char **argv)
    {
    	bool read;
    	param_s tmp_param;
    	epi::string_c s;
    	int i;
    
        i=0;
        while(i<argc)
        {
        	read = false;
        	tmp_param.name = NULL;
        	tmp_param.value = NULL;
       		
			// Check for key value in the form : --<key>=<value>
  	        s = argv[i];
        	if (s.GetLength() > 2 && s.GetAt(0) == '-' && s.GetAt(1) == '-')
        	{
        		int pos = s.Find('=');
        		if (pos>3)
          		{
          			epi::string_c s2;
          			
          			s2 = s;
          			
          			s.RemoveRight(s.GetLength()-pos);
          			s2.RemoveLeft(pos+1);

					tmp_param.name = new epi::string_c(s);
					tmp_param.value = new epi::string_c(s2);
					if (tmp_param.name == NULL || tmp_param.value == NULL)
					{
						if (tmp_param.name)
							delete tmp_param.name;

						return false;
					}

					read = true;
            	}	
        	}
        	
			// if we haven't read in the value yet, take the whole thing as a parameter
        	if (!read)
        	{
        		tmp_param.name = new epi::string_c(s);
        		if (!tmp_param.name)
        			return false;
        	}
        	
			// Add to self..
        	if (InsertObject(&tmp_param)<0)
        		return false;
        	 
        	i++;
        }

		return true;
    }
};

class version_file_c
{
public:
	version_file_c() {};
	~version_file_c() {};

private:
	u32_t version;
	u32_t umode;

public:
	u32_t GetVersion()					{ return version; }
	u32_t GetUpgradeMode()				{ return umode; }

	void SetVersion(u32_t _version)		{ version = _version; }
	void SetUpgradeMode(u32_t _umode)	{ umode = _umode; }

	bool Read(const char *name)
	{
		epi::crc32_c crc1;
		epi::file_c *f;
		u32_t tmp_version;
		u32_t tmp_umode;
		u32_t tmp_crcval;

		f = epi::the_filesystem->Open(name, 
				epi::file_c::ACCESS_READ | epi::file_c::ACCESS_BINARY);

		if (!f)
			return false;

		f->Read32BitInt(&tmp_version);
		tmp_version = EPI_LE_U32(tmp_version);
		
		f->Read32BitInt(&tmp_umode);
		tmp_umode = EPI_LE_U32(tmp_umode);
		
		f->Read32BitInt(&tmp_crcval);
		tmp_crcval = EPI_LE_U32(tmp_crcval);

		crc1 += tmp_version;
		crc1 += tmp_umode;
		if (crc1.crc != tmp_crcval)
			return false;

		version = tmp_version;
		umode = tmp_umode;
		return true;
	};

	bool Write(const char *name)
	{
		epi::file_c *f;
		bool ok;

		f = epi::the_filesystem->Open(name, 
				epi::file_c::ACCESS_WRITE | epi::file_c::ACCESS_BINARY);

		if (!f)
			return false;

		ok = true;
		try
		{
			epi::crc32_c crc1;
			u32_t tmp;
			
			crc1 += version;
			crc1 += umode;

			tmp = EPI_LE_U32(version);
			if (!f->Write32BitInt(&tmp))
			{
				epi::error_c err(ERROR_INTERNAL);
				throw err;
			}
			
			tmp = EPI_LE_U32(umode);
			if (!f->Write32BitInt(&tmp))
			{
				epi::error_c err(ERROR_INTERNAL);
				throw err;
			}

			tmp = EPI_LE_U32(crc1.crc);
			if (!f->Write32BitInt(&tmp))
			{
				epi::error_c err(ERROR_INTERNAL);
				throw err;
			}


			epi::the_filesystem->Close(f);
		}
		catch (epi::error_c)
		{
			epi::the_filesystem->Close(f);
			epi::the_filesystem->Delete(name);
			ok = false;
		}

		return ok;
	};

};

// ============================= PROTOTYPES =============================

void Pause(void); 

// =========================== INIT/SHUTDOWN ============================ 

#ifdef WIN32
epi::string_c *oldworkingdir = NULL;
#endif 

//
// Init
//
bool Init(char **argv)
{
	if (!epi::Init())
		return false;

#ifdef WIN32
	// Windows users will most likely double click, so we assume that the 
	// tool is located in the edge directory and that this is our current 
	// directory. We do restore the directory afterwards since our win32 
	// user may be a commandline junkie.
	epi::string_c *workingdir = NULL;
	char p[MAX_PATH];
	int idx;

	try
	{
		if (!epi::the_filesystem->GetCurrDir(p, MAX_PATH))
			return false;
		
		oldworkingdir = new epi::string_c(p);
		if (!oldworkingdir)
		{
			epi::error_c err(ERROR_INTERNAL);
			throw err;
		}

		workingdir = new epi::string_c(argv[0]);
		if (!workingdir)
		{
			epi::error_c err(ERROR_INTERNAL);
			throw err;
		}

		idx = workingdir->ReverseFind(DIRSEPARATOR);
		if (idx<0)
		{
			epi::error_c err(ERROR_INTERNAL);
			throw err;
		}

		workingdir->RemoveRight(workingdir->GetLength()-idx);
		
		if (!epi::the_filesystem->SetCurrDir(*workingdir))
		{
			epi::error_c err(ERROR_INTERNAL);
			throw err;
		}

		delete workingdir;
		workingdir = NULL;
	}
	catch (epi::error_c)
	{
		delete oldworkingdir;
		oldworkingdir = NULL;

		delete workingdir;
		workingdir = NULL;

		return false;
	}
#endif

	return true;
}

//
// Shutdown
//
void Shutdown(void)
{
#ifdef WIN32
	if (oldworkingdir)
		epi::the_filesystem->SetCurrDir(*oldworkingdir);
    
	Pause();
#endif

    epi::Shutdown();
    return;
}

// ============================= MISC UTIL ============================== 

//
// ReportInternalError
//
void ReportInternalError(const char *err)
{
    std::cerr << "===========================================" << '\n';
    std::cerr << " Upgrader tool has failed with an internal " << '\n';
    std::cerr << " error.  Should the  problem continue than " << '\n';
    std::cerr << " please post a bug  on the EDGE website at " << '\n';
    std::cerr << " http://edge.sourceforge.net.              " << '\n';
    
    if (err != NULL)
    {
        std::cerr << '\n';
        std::cerr << " Error Description:" << '\n';
        std::cerr << " " << err << '\n';
    }
    
    std::cerr << "===========================================";
}

//
// ReportNothingTodo
//
void ReportNothingToDo(void)
{
	std::cout << "Nothing to do!" << "\n";
}

//
// ReportUserError
//
void ReportUserError(const char *err)
{
	// No error specified here is an internal error
	if (!err)
	{
		ReportInternalError(NULL);
		return;
	}

	std::cerr << "Unable to complete upgrade process due to: " << '\n';
    std::cerr << " " << err << "\n";
	return;
}

//
// Usage
//
void Usage(void)
{
	std::cout << "upgrader [OPTION]" << '\n';
	std::cout << "Upgrade tool for the Enhanced Doom Gaming Engine (EDGE)." << '\n';
	std::cout << '\n';
	std::cout << KEY_CFGDIR	 << "=CFGPATH\twhere CFGPATH is a path to the EDGE config files" << '\n';
	std::cout << KEY_EDGEDIR << "=BINPATH\twhere BINPATH is a path to the EDGE binaries" << '\n';
	std::cout << KEY_HELP    << "\t\t\tdisplay this help and exit" << '\n';
	std::cout << '\n';
	std::cout << "CFGPATH will default to the home directory should one exist. BINPATH will" << '\n';
	std::cout << "default to ";
#ifndef WIN32
	std::cout << "the current directory." << "\n";
#else
	std::cout << "the directory in which the upgrade tool is located." << "\n";
#endif
	std::cout << '\n';
	std::cout << "Report bugs at " << EDGE_BUG_ADDR << '\n';
}

//
// VersionStream
//
bool VersionStream(const char *cfgdir, bool update, version_file_c& vf)
{
	epi::string_c s;

	s = cfgdir;
	if (s.GetLastChar() != DIRSEPARATOR)
		s += DIRSEPARATOR;

	s += VERSION_FILE;

	// Are we reading one in...?
	if (!update)
		return vf.Read(s);

	// Must be writing one out...
	return vf.Write(s);
}

//
// Pause
//
// Emulates the DOS pause functionality
//
void Pause(void)
{
    char c;
    
    std::cout << '\n' << "Press return to continue." << '\n';
    
    std::cin.sync();
    std::cin.read(&c,1);
}
 
// ========================= PARAMETER HANDLING ========================= 

//
// CopyConfigDirectory
//
void CopyConfigDirectory(epi::string_c *destdir, epi::string_c *srcdir) throw (epi::error_c)
{
	epi::filesystem_dir_c fsd;
	epi::string_c dest, src;
	version_file_c vf;
	int len;

	if (!destdir || !srcdir)
	{
		epi::error_c err(ERROR_INTERNAL, "Bad Params in CopyConfigDirectory()");
		throw err;
	}

	if (!epi::the_filesystem->IsDir(*srcdir))
	{
		epi::error_c err(ERROR_NOEDGEDIR, *srcdir);
		throw err;
	}

	// Add the directory seperator if we need one
	src = *srcdir;
	dest = *destdir;

	if (!epi::the_filesystem->IsDir(dest))
	{
		if (!epi::the_filesystem->MakeDir(dest))
		{
			epi::error_c err(ERROR_UNABLETOCREATEDIR, dest);
			throw err;
		}
	}
	else
	{
		//
		// Attempt to load a version file; if it succeeds than
		// we've done a upgrade already. At this point we know
		// this the only version and therefore no action is
		// needed.
		//
		if (VersionStream(*destdir, false, vf))
		{
			epi::error_c err(ERROR_UPGRADEDALREADY, dest);
			throw err;
		}
	}
	
	if (src.GetLastChar() != DIRSEPARATOR)
		src += DIRSEPARATOR;
	
	if (dest.GetLastChar() != DIRSEPARATOR)
		dest += DIRSEPARATOR;

	src += EDGE_CFG_NAME;
	dest += EDGE_CFG_NAME;

	if (!epi::the_filesystem->Copy(dest, src))
	{
		epi::error_c err(ERROR_GENERICUSER, "Config file copy failed");
		throw err;
	}

	// Strip the config name from the src & dest strings
	len = (int)strlen(EDGE_CFG_NAME);
	src.RemoveRight(len);
	dest.RemoveRight(len);

	// Add the savegame sub dir
	src += EDGE_SGSUBDIR;  
	dest += EDGE_SGSUBDIR;

	if (epi::the_filesystem->ReadDir(&fsd, src, EDGE_SG_MASK))
	{
		epi::string_c dest_file, src_file;
		int i, max;

		if (!epi::the_filesystem->IsDir(dest))
		{
			if (!epi::the_filesystem->MakeDir(dest))
			{
				epi::error_c err(ERROR_UNABLETOCREATEDIR, dest);
				throw err;
			}
		}

		dest_file = dest;
		dest_file += DIRSEPARATOR;

		src_file = src;
		src_file += DIRSEPARATOR;

		max = fsd.GetSize();
		for (i=0; i<max; i++)
		{
			// Add the filename
			dest_file += fsd[i]->name->GetString();
			src_file += fsd[i]->name->GetString();

			// Attempt the copy
			if (!epi::the_filesystem->Copy(dest_file, src_file))
			{
				epi::error_c err(ERROR_GENERICUSER, "Savegame file copy failed");
				throw err;
			}

			// Strip out the filename
			len = fsd[i]->name->GetLength();
			dest_file.RemoveRight(len);
			src_file.RemoveRight(len);
		}
	}

	// Update the version data
	vf.SetUpgradeMode(UPGRADEMODE_127_128);
	vf.SetVersion(VERSION_ID);
	if (!VersionStream(*destdir, true, vf))
	{
		epi::error_c err(ERROR_GENERICUSER, "Write the version data file failed");
		throw err;
	}
	
	return;
}

//
// GetConfigDirectory
//
// Returns:
//  >0 - Internal Error
//  0  - OK
//  <0 - Config directory could not be determined
//
int GetConfigDirectory(paramlist_c *pl, epi::string_c &cfgdir)
{
    const char *s;
    int idx;
    
    if (!pl)
        return 1;      // Internal Error
    
	cfgdir.Empty();

    // Check for a parameter setting the value
    idx = pl->FindParamByKey(KEY_CFGDIR);
    if (idx >= 0)
	{
		if (pl->GetEntry(idx)->value)
			cfgdir = pl->GetEntry(idx)->value->GetString();
	}
	
	if (!cfgdir.GetLength())
	{
		s = getenv("HOME");
		if (!s)
			return -1;

		cfgdir = s;
		
		if (cfgdir.GetLastChar() != DIRSEPARATOR)
			cfgdir += DIRSEPARATOR;

		cfgdir += EDGE_CFG_SUBDIR;
	}

    return 0;
}

//
// GetEdgeDirectory
//
// Returns:
//  >0 - Internal Error
//  0  - OK
//
int GetEdgeDirectory(paramlist_c *pl, epi::string_c &edgedir)
{
    const char *s;
    int idx;
    
    if (!pl)
        return 1;        // Internal Error
        
    // Check for a parameter setting the value
    idx = pl->FindParamByKey(KEY_EDGEDIR);
    if (idx >= 0)
        s = pl->GetEntry(idx)->value->GetString();
    else
        s = NULL;
        
    if (s)
    {
        edgedir = s;
    }
    else
    {
        edgedir = ".";
        edgedir += DIRSEPARATOR;
    }
    
    return 0;
}

//
// GetUpgradeMode
//
// Returns:
//  >0 - Internal Error
//  0  - OK
//
int GetUpgradeMode(paramlist_c *pl, int &mode)
{
	if (!pl)
		return 1;

	// TODO: Logic behind this!
	mode = UPGRADEMODE_UNKNOWN;
	return 0;
}

// =============================== MAIN =================================  

//
// DogsBollox
//
void DogsBollox(paramlist_c *pl) throw (epi::error_c)
{
	int mode;

	if (pl->FindParamByKey(KEY_HELP) > 0)
	{
		epi::error_c err(ERROR_NEEDHELP, NULL);
		throw err;
	}

	if (GetUpgradeMode(pl, mode))
	{
		epi::error_c err(ERROR_INTERNAL, "Blank parameter list given for GetUpgradeMode()");
		throw err;
	}

	if (mode == UPGRADEMODE_UNKNOWN) { mode = UPGRADEMODE_127_128; }	// default mode

	switch (mode)
	{
		case UPGRADEMODE_127_128:
		{
			epi::string_c cfgdir;
			epi::string_c edgedir;
			int error;
			
			// Get the config directory
			error = GetConfigDirectory(pl, cfgdir); 
			if (error != 0)
			{
				if (error < 0)
				{
					epi::error_c err(ERROR_NOCONFIGDIR);
					throw err;
				}
				else
				{
					epi::error_c err(ERROR_INTERNAL);
					throw err;
				}
			}	

		    // Get the edge directory
		    if (GetEdgeDirectory(pl, edgedir) != 0)
			{
				epi::error_c err(ERROR_NOEDGEDIR);
				throw err;
			}	

			CopyConfigDirectory(&cfgdir, &edgedir);
			break;
		}
	
		default:
			break;
	}

	return;
}

//
// main
//
// Life starts here
//
int main(int argc, char *argv[])
{
    paramlist_c paramlist;

	try
	{
		if (!Init(argv))
		{
			epi::error_c err(ERROR_INTERNAL, "Failed to initialise");
			throw err;
		}

		if (!paramlist.Load(argc, argv))
		{
			epi::error_c err(ERROR_INTERNAL, "Couldn't load the parameter list");
			throw err;
		}

		DogsBollox(&paramlist);

		std::cout << "Upgrade completed OK!" << "\n";
	}
	catch (epi::error_c err)
	{
		if (!err.IsEPIGenerated())
		{
			switch(err.GetCode())
			{
				case ERROR_GENERICUSER:
					ReportUserError(err.GetInfo());
					break;

				case ERROR_INTERNAL:
					ReportInternalError(err.GetInfo());
					break;
			
				case ERROR_NEEDHELP:
					Usage();
					break;

				case ERROR_NOCONFIGDIR:
					ReportNothingToDo();
					break;

				case ERROR_NOEDGEDIR:
				{
					epi::string_c s;

					s = "Unable to find the specified edge directory";
					if (err.GetInfo())
					{
						s += ": ";
						s += err.GetInfo();
					}

					ReportUserError(s);
					break;
				}

				case ERROR_UNABLETOCREATEDIR:
				{
					epi::string_c s;

					s = "Unable to create directory";
					if (err.GetInfo())
					{
						s += ": ";
						s += err.GetInfo();
					}

					ReportUserError(s);
					break;
				}

				case ERROR_UPGRADEDALREADY:
					ReportNothingToDo();
					break;

				default:
					ReportInternalError(NULL);
					break;
			}
		}

	}

    Shutdown();
    return 0;
}
