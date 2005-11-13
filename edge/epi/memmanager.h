//----------------------------------------------------------------------------
//  EDGE Memory Manager Header
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
//
// This replaces the classic Z_Zone code from the original Doom Source.
//
#ifndef __EPI_MEMORY_MANAGER_HEADER__
#define __EPI_MEMORY_MANAGER_HEADER__

#include "epi.h"
#include "arrays.h"	
#include "errors.h"	

#include <new>

namespace epi
{

// CLASSES

// Flush handler
class mem_flusher_c
{
public:
    mem_flusher_c() {};
    virtual ~mem_flusher_c() {};
	
private:
		
public:
	virtual void Flush(int urgency) = 0;
};

// The array for handling mem_flusher_c
class mem_flusher_array_c : public array_c
{
public:
	mem_flusher_array_c() : array_c(sizeof(mem_flusher_c*)) {};
    ~mem_flusher_array_c() { Clear(); };
	
private:
	void CleanupObject(void *obj)			{ /* Does nothing */ }
		
public:
	bool Add(mem_flusher_c* flusher);
	int GetNumEntries(void);
	int GetSize(void)						{ return array_entries; }
	void Remove(mem_flusher_c* flusher);

	mem_flusher_c* operator[](int pos); 		
};

// The memory manager class itself
class mem_manager_c
{
public:
	mem_manager_c(unsigned int _maxsize);
	~mem_manager_c();

	enum flags_e
	{
		FLAGS_FLUSH = 0x01
	};

	enum urgency_e
	{
		URGENCY_NONE,
		URGENCY_LOW,
		URGENCY_MEDIUM,
		URGENCY_HIGH,
		URGENCY_EXTREME
	};

private:
	int flags;
	unsigned int maxsize;

	mem_flusher_array_c flusher_list;
	
public:
	// Flush Handling
	bool Flush(int urgency);

	bool HookFlushHandler(mem_flusher_c *flush_handler);
	bool UnhookFlushHandler(mem_flusher_c *flush_handler);

	// Size
	unsigned int GetAllocatedSize();
	unsigned int GetMaxSize() { return maxsize; }

	void SetMaxSize(unsigned int new_maxsize);
};

/*
#define ZONEID  0x1d4a11f1

// A cache flusher is a function that can find and free unused memory.
typedef void cache_flusher_f(z_urgency_e urge);

typedef struct stack_array_s stack_array_t;

struct stack_array_s
{
	// points to the actual array pointer.
	// the stack_array is defined to be uninitialised if this one is NULL.
	void ***ptr;

	// the size of each element in ptr.
	int elem_size;

	// The number of currently used objects.
	// Elements above this limit will be freed automatically if memory gets
	// tight. Otherwise it will stay there, saving some future Z_ReMalloc calls.
	int num;

	// The number of allocated objects.
	int max;

	// alloc_bunch elements will be allocated at a time, saving some memory block
	// overheads.
	// If this is negative, the array will work in a different way:
	// *ptr will be an array of elements rather than an array of pointers.
	// This means that no pointer except *ptr may point to an element
	// of the array, since it can be reallocated any time.
	int alloc_bunch;

	// Shows the number of users who have locked this grow_array.
	int locked;

	stack_array_t *next;
};

// Stack array functions.
void Z_LockStackArray(stack_array_t *a);
void Z_UnlockStackArray(stack_array_t *a);
void Z_ClearStackArray(stack_array_t *a);
void Z_DeleteStackArray(stack_array_t *a);
void Z_InitStackArray(stack_array_t *a, void ***ptr, int elem_size, int alloc_bunch);
stack_array_t *Z_CreateStackArray(void ***ptr, int elem_size, int alloc_bunch);
void Z_SetArraySize(stack_array_t *a, int num);

// Generic helper functions.
char *Z_StrDup(const char *s);

// Memory handling functions.
void Z_RegisterCacheFlusher(cache_flusher_f *f);
void Z_Init(void);
void *Z_Calloc2(int size);
void *Z_Malloc2(int size);
void *Z_ReMalloc2(void *ptr, int size);
void Z_Free(void *ptr);


// -ES- 1999/12/16 Leak Hunt
#ifdef LEAK_HUNT
void *Z_RegisterMalloc(void *ptr, const char *file, int row);
void *Z_UnRegisterTmpMalloc(void *ptr, const char *func);
void Z_DumpLeakInfo(int level);
#define Z_Malloc(size) Z_RegisterMalloc(Z_Malloc2(size),__FILE__,__LINE__)
#define Z_Calloc(size) Z_RegisterMalloc(Z_Calloc2(size),__FILE__,__LINE__)
#define Z_ReMalloc(ptr,size) Z_RegisterMalloc(Z_ReMalloc2(ptr,size),__FILE__,__LINE__)
#else
#define Z_Malloc Z_Malloc2
#define Z_Calloc Z_Calloc2
#define Z_ReMalloc Z_ReMalloc2
#endif

//
// Z_New
//
// Allocates num elements of type. Use this instead of Z_Malloc whenever
// possible.
//
#define Z_New(type, num) ((type *) Z_Malloc((num) * sizeof(type)))

//
// Z_Resize
//
// Reallocates a block. Use instead of Z_ReMalloc wherever possible.
// Unlike normal Z_ReMalloc, the pointer parameter is assigned the new
// value, and there is no return value.
//
#define Z_Resize(ptr,type,n)  \
	(void)((ptr) = (type *) Z_ReMalloc((void *)(ptr), (n) * sizeof(type)))

//
// Z_ClearNew
//
// The Z_Calloc version of Z_New. Allocates mem and clears it to zero.
//
#define Z_ClearNew(type, num) ((type *) Z_Calloc((num) * sizeof(type)))

//
// Z_Clear
//
// Clears memory to zero.
//
#define Z_Clear(ptr, type, num)  \
	memset((void *)(ptr), ((ptr) - ((type *)(ptr))), (num) * sizeof(type))

//
// Z_MoveData
//
// moves data from src to dest.
//
#define Z_MoveData(dest, src, type, num)  \
	I_MoveData((void *)(dest), (void *)(src), (num) * sizeof(type) + ((src) - (type *)(src)) + ((dest) - (type *)(dest)))

//
// StrNCpy
//
// Copies up to max characters of src into dest, and then applies a
// terminating zero (so dest must hold at least max+1 characters).
// The terminating zero is always applied (there is no reason not to)
//
#define Z_StrNCpy(dest, src, max) \
	(void)(strncpy((dest), (src), (max)), (dest)[(max)] = 0)

// -AJA- 2001/07/24: New lightweight "Bunches"

#define Z_Bunch(type)  \
	struct { type *arr; int max; int num; }

#define Z_BunchNewSize(var, type)  \
do \
{  \
	if ((var).num > (var).max)  \
	{								\
		(var).max = (var).num + 16;				\
		Z_Resize((var).arr, type, (var).max);	\
	} \
} \
while(0) \

*/

};

/*
//----------------------------------------------------------------------------
// NEW/DELETE OVERRIDES
//----------------------------------------------------------------------------

void* operator new[] (size_t size) throw (std::bad_alloc);
void* operator new(size_t size) throw (std::bad_alloc);

void operator delete[] (void * lpPtr) throw ();
void operator delete(void * lpPtr) throw ();
*/

#endif /* __EPI_MEMORY_MANAGER__ */

