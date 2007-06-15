//----------------------------------------------------------------------------
//  EDGE Platform Interface Header
//----------------------------------------------------------------------------
//
//  Copyright (c) 2004-2005  The EDGE Team.
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

#ifndef __EPI_ERROR_CLASS__
#define __EPI_ERROR_CLASS__

#define EPI_ERRINF_MAXSIZE 96

namespace epi
{
	enum error_type_e
	{
		EPI_ERRGEN_FOOBAR,
		EPI_ERRGEN_FILENOTFOUND,
		EPI_ERRGEN_MEMORYALLOCFAILED,
		EPI_ERRGEN_BADARGUMENTS,
		EPI_ERRGEN_ASSERTION,
		EPI_ERRGEN_FILEERROR,  // read/write/seek problem
		EPI_ERRGEN_NUMTYPES	
	};
	
    class error_c
    {
    public:
        error_c(int _code, const char *_info = (const char*)NULL, bool _epi = false);
        error_c(const error_c &rhs);
        ~error_c();
        
    private:
		bool epi;
    	int code;
    	char info[EPI_ERRINF_MAXSIZE];
    
		void InternalSet(int _code, const char* _info, bool _epi);

	public:
		int GetCode(void) const { return code; }
		const char* GetInfo(void) const { return info; }
		bool IsEPIGenerated(void) const { return epi; } 

		error_c& operator=(const error_c &rhs);
    };

};

#endif 
