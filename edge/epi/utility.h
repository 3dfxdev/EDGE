//----------------------------------------------------------------------------
//  EDGE Platform Interface Utility Header
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
#ifndef __EPI_UTIL__
#define __EPI_UTIL__

#include "epi.h"
#include "arrays.h"
#include "types.h"

namespace epi
{
	// Forward declaration
	class strlist_c;
	
    // Primitive array for base types
    class s32array_c : public array_c
    {
    public:
        s32array_c() : array_c(sizeof(s32_t)) {};
        ~s32array_c() { Clear(); }
        
    private:
    	void CleanupObject(void *obj) { /* Do Nothing */ } 
    
    public:
    	int GetSize() const { return array_entries; }
	    int Insert(s32_t num) { return InsertObject((void*)&num); }     	
	    s32_t& operator[](int idx) const { return *(s32_t*)FetchObject(idx); } 
    };
    
    class u32array_c : public array_c
    {
    public:
        u32array_c() : array_c(sizeof(u32_t)) {};
        ~u32array_c() { Clear(); }
        
    private:
    	void CleanupObject(void *obj) { /* Do Nothing */ } 
    
    public:
    	int GetSize() const { return array_entries; }
	    int Insert(u32_t num) { return InsertObject((void*)&num); }     	
	    u32_t& operator[](int idx) const { return *(u32_t*)FetchObject(idx); } 
    };
        
    // String Box
    class strbox_c
    {
    public:
    	strbox_c();
    	strbox_c(strbox_c &rhs);
    	~strbox_c();
    	
    private:
    	char **strs;
    	int numstrs;
    	
    	char *data;
    	int datasize;
    	
    	void Copy(strbox_c &src);
    	
    public:
    	void Clear();
    	int GetSize() const { return numstrs; }
    	void Set(strlist_c &src);

    	strbox_c& operator=(strbox_c &rhs);
    	char* operator[](int idx) const { return strs[idx]; }
    };
    
    // String entry
    class strent_c
    {
	public:
		strent_c() : data(NULL) { }
		strent_c(const char *s) : data(NULL) { Set(s); }
		strent_c(const strent_c &rhs) : data(NULL) { Set(rhs.data); }
		~strent_c() { Clear(); };

	private:
		char *data;

	public:
		void Clear() 
		{ 
			if (data) 
				delete [] data;
				
			data = NULL; 
		}
	
		const char* GetString(void) const { return data; }
	
		bool IsEmpty() { return data?(data[0] == '\0'?true:false):true; }
	
		void Set(const char *s);
		void Set(const char *s, int max);

		strent_c& operator= (const strent_c &rhs) 
		{ 
			if(&rhs != this)
				Set(rhs.data);
				 
			return *this; 
		}
		
		char operator[](int idx) const { return data[idx]; }

		// FIXME: -AJA- not sure about this auto-conversion
		operator const char* () const { return data; }
    };

    // String list
    class strlist_c : public array_c
    {
    public:
        strlist_c();
        strlist_c(strlist_c &rhs);
        ~strlist_c();
        
    private:
        void CleanupObject(void *obj);
		void Copy(strlist_c &src);
        
    public:
        void Delete(int idx) { RemoveObject(idx); }
	    int Find(const char* refname);
	    int GetSize() const { return array_entries; }
	    int Insert(const char *s);
    	void Set(strbox_c &src);
    	
		strlist_c& operator=(strlist_c &rhs);
	    char* operator[](int idx) const { return *(char**)FetchObject(idx); } 
    };
    
    // String table
    class strtable_c
    {
    public:
    	strtable_c() {};
    	strtable_c(strtable_c &rhs);
    	~strtable_c() {};
    	
    private:
    	strbox_c refs;
    	strbox_c values;
    	
    	void Copy(strtable_c &src);
    	
    public:
    	void Clear();
    	void Set(strbox_c &_refs, strbox_c &_values);
    	strtable_c& operator=(strtable_c &rhs);
    };
};

#endif /* __EPI_UTIL__ */

