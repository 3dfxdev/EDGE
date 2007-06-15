//----------------------------------------------------------------------------
//  EDGE Memory Manager
//----------------------------------------------------------------------------
//
//  Copyright (c) 1999-2002  The EDGE Team.
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
//
// This replaces the classic Z_Zone code from the original Doom Source.
//
#include "epi.h"
#include "arrays.h"
//#include "memalloc.h"
#include "memmanager.h"

#include <string.h>

//#define HOGGIE_MEM_DEBUG

namespace epi
{

// MEMORY FLUSH HANDLER ARRAY CLASS

//
// mem_flusher_array_c::Add()
//
bool mem_flusher_array_c::Add(mem_flusher_c* flusher)
{
	int i;

	// Look for a blank entry
	i=0;
	while(i<array_entries)
	{
		if (!(*(mem_flusher_c**)FetchObjectDirect(i)))
		{
			memcpy(FetchObjectDirect(i), &flusher, sizeof(mem_flusher_c*));
			return true;
		}

		i++;
	}

	// No entry found, so we'll to add one 
	return (InsertObject((void*)&flusher)>=0)?true:false;
}

//
// mem_flusher_array_c::GetNumEntries()
//
int mem_flusher_array_c::GetNumEntries()
{
	int i, entries;

	entries = 0;
	i=0;
	while(i<array_entries)
	{
		if (*(mem_flusher_c**)FetchObjectDirect(i))
		{
			entries++;
		}

		i++;
	}

	return entries;
}

//
// mem_flusher_array_c::Remove()
//
void mem_flusher_array_c::Remove(mem_flusher_c* flusher)
{
	mem_flusher_c* currflusher;
	int i;

	i=0;
	while(i<array_entries)
	{
		currflusher = *(mem_flusher_c**)FetchObjectDirect(i);
		if (currflusher && currflusher == flusher)
		{
			memset(FetchObjectDirect(i), 0, sizeof(mem_flusher_c*));
			return;
		}

		i++;
	}

	return;
}

//
// mem_flusher_array_c::operator[]
//
mem_flusher_c* mem_flusher_array_c::operator[](int pos)
{
	if (pos<0 || pos>=array_entries)
		return NULL;

	return *(mem_flusher_c**)FetchObjectDirect(pos);
}

// MEMORY MANAGER CLASS

//
// mem_manager_c Constructor
//
mem_manager_c::mem_manager_c(unsigned int _maxsize) : 
    maxsize(_maxsize)
{
	flags = FLAGS_FLUSH;
}

//
// mem_manager_c Deconstructor
//
mem_manager_c::~mem_manager_c()
{
}

//
// mem_manager_c::Flush()
//
bool mem_manager_c::Flush(int urgency)
{
	array_iterator_c it;
	mem_flusher_c *flusher;

	if (!(flags & FLAGS_FLUSH))
	{
		return false;
	}

	flags &= ~FLAGS_FLUSH;	// Lets disable this for now - it could get terribly messly

	it = flusher_list.GetBaseIterator();
	while (it.IsValid())
	{
		flusher = *(mem_flusher_c**)((void*)it);
		if (flusher)
		{
			flusher->Flush(urgency);
		}
		it++;
	}

	flags |= FLAGS_FLUSH;	// Back on
	return true;
}

//
// mem_manager_c::HookFlushHandler()
//
bool mem_manager_c::HookFlushHandler(mem_flusher_c *flush_handler)
{
	array_iterator_c it;
	mem_flusher_c **entry;

	it = flusher_list.GetBaseIterator();
	while (it.IsValid())
	{
		entry = (mem_flusher_c**)((void*)it);
		if (*entry == NULL)
		{
			memcpy(entry, &flush_handler, sizeof(mem_flusher_c**));
			return true;
		}
		else
		{
			// Already Added?
			if (*entry == flush_handler)
			{
				return true;
			}
		}
		it++;
	}

	return (flusher_list.InsertObject(&flush_handler) >= 0)?true:false;
}

//
// mem_manager_c::UnhookFlushHandler()
//
bool mem_manager_c::UnhookFlushHandler(mem_flusher_c *flush_handler)
{
	array_iterator_c it;
	mem_flusher_c **entry;

	it = flusher_list.GetBaseIterator();
	while (it.IsValid())
	{
		entry = (mem_flusher_c**)((void*)it);
		if (*entry && *entry == flush_handler)
		{
			memset(entry, 0, sizeof(mem_flusher_c**));
			return true;
		}
		it++;
	}

	return false;
}

//
// mem_manager_c::GetAllocatedSize()
//
unsigned int mem_manager_c::GetAllocatedSize()
{
	//struct mallinfo mi = dlmallinfo();
	//return (mi.uordblks+mi.hblkhd);
	return 0;
}

void mem_manager_c::SetMaxSize(unsigned int new_maxsize)
{
	if (new_maxsize < maxsize)
	{
		// FIXME: force some flushing ?  check AllocatedSize ??
	}

	maxsize = new_maxsize;
}

};	// <-- epi namespace END

//----------------------------------------------------------------------------
// NEW/DELETE OVERRIDES
//----------------------------------------------------------------------------

/*
#ifdef HOGGIE_MEM_DEBUG
void L_WriteDebug(const char *message, ...);
#endif

//
// new (Array)
//
void* operator new[](size_t size) throw (std::bad_alloc)
{
    //
	//if (epi::the_mem_manager)
	//{
	//	if (epi::the_mem_manager->GetAllocatedSize() + 8 >
	//		epi::the_mem_manager->GetMaxSize())
	//	{
	//		throw std::bad_alloc();
	//	}
	//}

#ifdef HOGGIE_MEM_DEBUG
	struct mallinfo mi;
	mi = dlmallinfo();
	L_WriteDebug("[ARRAYa] Mem size: %ld  sbrk'd: %ld\n", (mi.uordblks+mi.hblkhd), mi.arena);
#endif

	void *ptr = dlmalloc(size);

	if (! ptr)
		throw std::bad_alloc();

    memset(ptr, (rand() % 0xFF), size);
    //    memset(ptr, 0xAB, size);

#ifdef HOGGIE_MEM_DEBUG
	mi = dlmallinfo();
	L_WriteDebug("[ARRAYb] Mem size: %ld  sbrk'd: %ld\n", (mi.uordblks+mi.hblkhd), mi.arena);
#endif

	return ptr;
}

//
// new (Individual)
//
void* operator new(size_t size) throw (std::bad_alloc)
{
    //
	//if (epi::the_mem_manager)
	//{
	//	if (epi::the_mem_manager->GetAllocatedSize() + 8 >
	//		epi::the_mem_manager->GetMaxSize())
	//	{
	//		throw std::bad_alloc();
	//	}
	//}

#ifdef HOGGIE_MEM_DEBUG
	struct mallinfo mi;
	mi = dlmallinfo();
	L_WriteDebug("[INDEPa] Mem size: %ld  sbrk'd: %ld\n", (mi.uordblks+mi.hblkhd), mi.arena);
#endif

	void *ptr = dlmalloc(size);

	if (! ptr)
		throw std::bad_alloc();

    memset(ptr, (rand() % 0xFF), size);

#ifdef HOGGIE_MEM_DEBUG
	mi = dlmallinfo();
	L_WriteDebug("[INDEPb] Mem size: %ld  sbrk'd: %ld\n", (mi.uordblks+mi.hblkhd), mi.arena);
#endif

	return ptr;
}

//
// delete (Array)
//
void operator delete[] (void * ptr) throw ()
{
	if (ptr)
	{
		dlfree(ptr);

#ifdef HOGGIE_MEM_DEBUG
		struct mallinfo mi = dlmallinfo();
		L_WriteDebug("[ARRAY] Size down to %d\n", (mi.uordblks+mi.hblkhd));
#endif
	}
}

//
// delete (Individual)
//
void operator delete(void *ptr) throw ()
{
	if (ptr)
	{
		dlfree(ptr);

#ifdef HOGGIE_MEM_DEBUG
		struct mallinfo mi = dlmallinfo();
		L_WriteDebug("[INDEP] Size down to %d\n", (mi.uordblks+mi.hblkhd));
#endif
	}
}
*/

