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
#include <iostream>

// Debugging stuff...
#define DEBUG 

// Parameter keys...
#define KEY_CFGDIR			"--cfg-dir"
#define KEY_EDGEDIR			"--edge-dir"
#define KEY_EXISTINGVERSION "--existing-version"
#define KEY_MODE			"--mode"

// edge settings
#define EDGE_SGDIR			"savegame"

// Upgrade Modes
typedef enum 
{
	UPGRADEMODE_UNKNOWN,		
	UPGRADEMODE_127_128,		// Version 1.27 to 1.28
	UPGRADEMODE_NUMMODES		
}
mode_e;

// Helper class for managing the parameter list
struct param_s
{
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
						return false;

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

//
// Init
//
bool Init(void)
{
    return epi::Init();
}

//
// ReportInternalError
//
void ReportInternalError(char *err)
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

//
// Shutdown
//
void Shutdown(void)
{
    epi::Shutdown();
    
#ifdef DEBUG
    Pause();
#endif

    return;
}

//
// GetUpgradeMode
//
int GetUpgradeMode(paramlist_c *pl)
{
	if (!pl)
		return UPGRADEMODE_UNKNOWN;

	// TODO: Logic behind this!
	return UPGRADEMODE_UNKNOWN;
}

//
// main
//
// Life starts here
//
int main(int argc, char *argv[])
{
    paramlist_c paramlist;
	int mode;

	if (!Init())
    {
        ReportInternalError("Failed to initialise");
        Pause();
        return 1;
    }

	if (!paramlist.Load(argc, argv))
	{
        ReportInternalError("Couldn't load the parameter list");
        Pause();
        return 2;
	}

	mode = GetUpgradeMode(&paramlist);
	if (mode == UPGRADEMODE_UNKNOWN) { mode = UPGRADEMODE_127_128; }	// default mode

	switch (mode)
	{
		case UPGRADEMODE_127_128:
		{
			epi::string_c edgedir;
			epi::string_c homedir;
			


			// Get edge directory (else assume current)
			// Get a home directory
			// Copy from edge->home directory including savegmaes
			break;
		}

		default:
			break;
	}

    Shutdown();
    return 0;
}
