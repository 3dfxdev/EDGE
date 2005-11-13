//----------------------------------------------------------------------------
//  EDGE String Class
//----------------------------------------------------------------------------
//
//  Copyright (c) 2003-2005  The EDGE Team.
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
// Based on work by Kamen Lilov (kamen@kami.com). Many thanks to him for 
// allowing us to use an early version of his work. Kamen has worked to 
// produce a commerical version of the CStr class which can be found 
// here (http://www.utilitycode.com/str/).
//
#include "strings.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace epi
{

/*********************************************************************
 * Proc:		EPIStringAbortApplication
 * Purpose:	Used only if the caller does not supply his own version
 *			of the ThrowXXX methods.  Displays a system-modal message
 *			box, then aborts the application.
 *********************************************************************/

void EPIStringAbortApplication(int fatal_type)
{
	// Display message and abort
	char msg[64];
	sprintf (msg, "string_c fatal %d", fatal_type);
	abort();
}


// Define specific type of ASSERT
#ifdef DEBUG
#define DEV_ASSERT(x)  { if ((x) == 0)  EPIStringAbortApplication(1111); }
#else
#define DEV_ASSERT(x)  { }
#endif


/*********************************************************************
 * Proc:		i_blkalloc / i_blkverify / i_blkrealfree
 * Purpose:	Helpers
 *********************************************************************/

#ifdef DEBUG

// Debug alloc: prepend byte length, append protective mask
unsigned char* i_blkalloc (unsigned int bytes)
{
	unsigned char* x = (unsigned char*) malloc (bytes + 8);
	((unsigned int*)x)[0] = bytes;
	x += 4;
	memset (x+bytes, 0x78, 4);
	return x;
}

// Debug verify: make sure byte length matches and protective mask is OK
void i_blkverify (unsigned char* block, unsigned int bytes)
{
	unsigned char* realblock = block-4;
	DEV_ASSERT (((unsigned int*)realblock)[0] == bytes);
	DEV_ASSERT (block[bytes+0] == 0x78  &&  block[bytes+1] == 0x78);
	DEV_ASSERT (block[bytes+2] == 0x78  &&  block[bytes+3] == 0x78);
}

// Debug real free: call free() 4 bytes earlier
void i_blkrealfree(unsigned char* block, unsigned int bytes)
{
	i_blkverify(block, bytes);
	unsigned char* realblock = block-4;
	free (realblock);
}

#else

// Release alloc: just do malloc
inline unsigned char* i_blkalloc (unsigned int bytes)
{
	return (unsigned char*) malloc (bytes);
}

// Release verify: do nothing
inline void i_blkverify (unsigned char*, unsigned int)
{
}

// Release real free: just call free()
void i_blkrealfree(unsigned char* block, unsigned int)
{
	free (block);
}

#endif


/*********************************************************************
 * Proc:		blkalloc
 * Purpose:	Allocates a memory block for the string manager.  If the
 *			block falls below the max bytes threshold (and almost all
 *			do), the function attempts to return a block from the
 *			cache.  Otherwise, it acts as a regular malloc.
 * In:		bytes - how many bytes to be allocated (MUST be divisible
 *					by four, and must be >0!)
 * Out:		Ptr to allocated (or obtained) block, NULL if no memory
 * Rems:		In debug build, 8 additional bytes are allocated before
 *			and after the block, to facilitate memory overwrite
 *			detection.
 *********************************************************************/

static unsigned char* blkalloc(unsigned int bytes)
{
	DEV_ASSERT (bytes != 0);
	DEV_ASSERT ((bytes & 3) == 0);
	unsigned char* x;

	// Large block?
	if (bytes >= EPISTR_DITEMS)
		return i_blkalloc (bytes);

	// No block in cache?
	unsigned int cindex = bytes >> 2;

	x = string_c::m_cache[cindex];
	if (x == NULL)
		return i_blkalloc (bytes);

	// Return block from cache, update it
	unsigned char** x_fp = (unsigned char**) x;
	string_c::m_cache[cindex] = *x_fp;
	return x;
}


/*********************************************************************
 * Proc:		blkfree
 * Purpose:	Deallocates a memory block used by the string manager.
 *			Does not release the block by free(), but instead forms
 *			a linked list of blocks of a given size, storing a ptr
 *			to the list head in m_cache.
 * In:		blk   - a block previously allocated with blkalloc
 *			bytes - the exact same value passed to blkalloc
 *********************************************************************/

static void blkfree (void* blk, unsigned int bytes)
{
	// Check the block; translates to nothing in release builds
	DEV_ASSERT (blk != NULL  &&  bytes != 0  &&  (bytes & 3) == 0);
	i_blkverify ((unsigned char*) blk, bytes);

	// Large blocks are returned directly to the memory manager
	if (bytes >= EPISTR_DITEMS)
    {
		i_blkrealfree ((unsigned char*) blk, bytes);
		return;
	}

	// Put reference to next free block in first 4 bytes
	unsigned int cindex = bytes >> 2;
	unsigned char** blk_fp = (unsigned char**) blk;

	*blk_fp = string_c::m_cache[cindex];

	// And add block as head of cache
	string_c::m_cache[cindex] = (unsigned char*) blk;
}


unsigned char* string_c::m_cache[EPISTR_DITEMS / 4] = { NULL };
string_desc_s string_c::m_null = { 2, "", 0, 1 };


/*********************************************************************
 * Proc:		Find_desc
 * Purpose:	Helper.  Allocates (from the cache or memory) a string_desc_s block
 *********************************************************************/

inline string_desc_s* Find_desc()
{
	unsigned char* x = blkalloc (sizeof(string_desc_s));
	string_c::ThrowIfNull(x);
	return (string_desc_s*) x;
}


/*********************************************************************
 * Proc:		string_c::freestr (static method)
 * Purpose:	Decrements the usage count of the specified string.  When
 *			the count reaches zero, blkfrees the data bytes and moves
 *			the string_desc_s block to the pool of available blocks.
 *********************************************************************/

void string_c::freestr_nMT(string_desc_s* tofree)
{

#ifdef DEBUG
	// Check validity
	if (tofree == NULL  ||  tofree->usage == 0)
		ThrowPGMError();
#endif

	// Decrement counter, don't deallocate anything if we're using m_null
	tofree->usage--;
	if (tofree == &string_c::m_null)
	{
#ifdef DEBUG
		if (tofree->usage < 2)
			ThrowPGMError();
#endif
		return;
	}

	// When the usage count reaches 0, blkfree the block
	if (tofree->usage == 0)
	{
		// Deallocate associated data block
		blkfree (tofree->text, tofree->alloc+1);
		// Deallocate the descriptor itself
		blkfree (tofree, sizeof(string_desc_s));
	}
}

void string_c::freestr(string_desc_s* tofree)
{
	freestr_nMT (tofree);
}


/*********************************************************************
 * Proc:		string_c::destructor
 *********************************************************************/

string_c::~string_c()
{
	// Note that we don't have to check for m_null here, because
	//   the usage count there will always be 2 or more.
	// MT note: InterlockedDecrement() is used instead of --

	if (--data->usage == 0)
	{
#ifdef DEBUG
		if (data == &string_c::m_null)
			string_c::ThrowPGMError();
#endif
		// blkfree associated text block and descriptor
		blkfree (data->text, data->alloc+1);
		blkfree (data, sizeof(string_desc_s));
	}
}


/*********************************************************************
 * Proc:		string_c::NewFromString
 * Purpose:	Core code for 'copy char* constructor' and '= operator'.
 *			Assumes our 'data' field is garbage on entry.
 *********************************************************************/

void string_c::NewFromString(const char* s, unsigned int slen, unsigned int prealloc)
{
	// Determine actual size of Buffer that needs to be allocated
	if (slen > prealloc)
		prealloc = slen;

	// Empty string?
	if (slen == 0  &&  prealloc == 0)
	{
		SET_DATA_EMPTY();
		return;
	}

	// Round up to blkalloc() granularity
	up_alloc (prealloc);

	// Get memory for string
	char* target = (char*) blkalloc (prealloc+1);
	ThrowIfNull(target);

	// Locate blkfree descriptor, fill it in
	data = Find_desc();
	data->text = target;
	data->usage = 1;
	data->length = slen;
	data->alloc = prealloc;

	// Copy the string bytes, including the null
	memcpy (target, s, slen+1);
}


/*********************************************************************
 * Proc:	string_c::string_c (unsigned int prealloc)
 * Purpose:	Instantiates an Empty string, but with the specified
 *			number of bytes in the preallocated Buffer.
 * In:		prealloc - number of bytes to reserve
 *********************************************************************/

string_c::string_c(unsigned int prealloc)
{
	up_alloc (prealloc);

	// Get memory for string
	char* target = (char*) blkalloc (prealloc+1);
	ThrowIfNull(target);
	target[0] = 0;

	// Locate blkfree descriptor, fill it in
	data = Find_desc();
	data->text = target;
	data->usage = 1;
	data->length = 0;
	data->alloc = prealloc;
}


/*********************************************************************
 * Proc:		string_c::string_c(const char*)
 * Purpose:	Construct an instance that exactly copies the specified
 *			string.  An optional second parameter specifies the Buffer
 *			size (will be ignored if it is less than what's needed)
 *********************************************************************/

string_c::string_c(const char* s, unsigned int prealloc /*= 0*/)
{
	size_t slen = strlen(s);
	NewFromString(s, (unsigned int) slen, prealloc);
}


/*********************************************************************
 * Proc:		string_c::copy constructor
 *********************************************************************/

string_c::string_c(const string_c& source)
{
	if (this == &source)
		return;
	data = source.data;
	data->usage++;
}

/*********************************************************************
 * Proc:		string_c::operator = string_c
 * Purpose:	Copies a string into another string, destroying the
 *			previous content.
 *********************************************************************/

const string_c& string_c::operator=(const string_c& source)
{
	if (this != &source)
	{
		freestr(data);
		data = source.data;
		data->usage++;
	}

	return *this;
}


/*********************************************************************
 * Proc:		string_c::Buffer
 * Purpose:	Helper.  Makes sure that our internal Buffer has
 *			the specified number of bytes available, and that
 *			we can overwrite it (i.e. usage is 1).  If this
 *			isn't so, prepares a copy of our internal data.
 *********************************************************************/

void string_c::Buffer (unsigned int newlength)
{
	up_alloc(newlength);

	// Reallocate. First handle case where we cannot just
	//   touch the Buffer.  We don't need to test for m_null
	//   here because it's usage field is always >1

	if (data->usage == 1)
	{
		// Buffer already big enough?
		if (data->alloc >= newlength)
			return;
		// Nope. We need to reallocate here.
	}

	string_desc_s* prevdata = data;
	if (newlength < prevdata->length)
	{
		newlength = prevdata->length;
	}

	NewFromString(prevdata->text, prevdata->length, newlength);
	freestr_nMT(prevdata);
}


/*********************************************************************
 * Proc:		string_c::Compact
 * Purpose:	If alloc is bigger than length, shrinks the
 *			Buffer to hold only as much memory as necessary.
 * In:		only_above - will touch the object only if the difference
 *				is greater than this value.  By default, 0 is passed,
 *				but other good values might be are 1 to 7.
 * Rems:		Will Compact even the Buffer for a string shared between
 *			several string_c instances.
 *********************************************************************/

void string_c::Compact(unsigned int only_above /*= 0*/)
{
	// Nothing to do?
	if (data == &string_c::m_null)
		return;

	unsigned int diff = (unsigned int) int(data->alloc - data->length);
	if (diff <= only_above)
		return;

	// Shrink Buffer.  The up_alloc() call is because it is no good
	//   to allocate 14 or 15 bytes when we can get 16; the memory
	//   manager will waste the few remaining bytes anyway.
	unsigned int to_shrink = data->length;
	up_alloc (to_shrink);

	// Using realloc to shrink a memory block might cause heavy
	//   fragmentation.  Therefore, a preferred method is to
	//   allocate a new block and copy the data there.
	char* xnew = (char*) blkalloc (to_shrink+1);
	if (xnew == NULL)
		return;				// No need to throw 'no mem' here

	// Copy data, substitute fields
	char* xold = data->text;
	unsigned int  xoal = data->alloc;
	data->text = xnew;
	memcpy (xnew, xold, data->length+1);
	blkfree ((unsigned char*) xold, xoal+1);
	data->alloc = to_shrink;
}


/*********************************************************************
 * Proc:		string_c::CompactFree (static)
 * Purpose:	Releases any unused blocks we might cache.
 *********************************************************************/

void string_c::CompactFree()
{
	for (unsigned int i=0; i < (EPISTR_DITEMS / 4); i++)
	{
		if (m_cache[i] != NULL)
		{
			while (m_cache[i] != NULL)
			{
				unsigned char** x = (unsigned char**) m_cache[i];
				m_cache[i] = *x;
				i_blkrealfree ((unsigned char*) x, i << 2);
			}
		}
	}
}


/*********************************************************************
 * Proc:		string_c::operator = const char*
 * Purpose:	Copies a string into our internal Buffer, if it is big
 *			enough and is used only by us; otherwise, blkfrees the
 *			current instance and allocates a new one.
 *********************************************************************/

const string_c& string_c::operator=(const char* s)
{
	// Check for zero length string.
	size_t slen = strlen(s);
	if (slen == 0)
	{
		if (data == &string_c::m_null)
			return *this;

		freestr(data);
		SET_DATA_EMPTY();
		return *this;
	}

	// Can we handle this without reallocations?  NOTE: we do
	//   not use Buffer() here because if the string needs to
	//   be expanded, the old one will be copied, and we don't
	//   care about it anyway.

	if (data->usage == 1  &&  data->alloc >= slen)
	{
		// Yes, copy bytes and get out
		memcpy (data->text, s, slen+1);
		data->length = (unsigned int) slen;
		return *this;
	}

	// No. blkfree old string, allocate new one.
	freestr_nMT(data);
	NewFromString(s, (unsigned int) slen, 0);

	return *this;
}

/*********************************************************************
 * Proc:		string_c::Empty
 * Purpose:	Sets the string to NULL.  However, the allocated Buffer
 *			is not released.
 *********************************************************************/

void string_c::Empty()
{
	// Already Empty, and with Buffer zero?
	if (data == &string_c::m_null)
		return;

	// More than one instance served?
	if (data->usage != 1)
	{
		// Get a copy of the Buffer size, release shared instance
		unsigned int to_alloc = data->alloc;
		freestr_nMT(data);

		// Get memory for string
		char* target = (char*) blkalloc (to_alloc+1);
		ThrowIfNull(target);

		// Locate blkfree descriptor, fill it in
		data = Find_desc();
		data->text = target;
		data->usage = 1;
		data->length = 0;
		data->alloc = (unsigned int) to_alloc;
		target[0] = 0;
		return;
	}

	// Only one instance served, so just set the charcount to zero.
	data->text[0] = 0;
	data->length = 0;
}


/*********************************************************************
 * Proc:		string_c::Reset
 * Purpose:	Sets the string to NULL, deallocates Buffer
 *********************************************************************/
void string_c::Reset()
{
	if (data != &string_c::m_null)
	{
		freestr(data);
		SET_DATA_EMPTY();
	}
}


/*********************************************************************
 * Proc:		string_c::AddChar
 * Purpose:	Appends a single character to the end of the string
 *********************************************************************/

void string_c::AddChar(char ch)
{
	// Get a copy if usage>1, expand Buffer if necessary
	unsigned int cur_len = data->length;
	Buffer(cur_len+1);

	data->text[cur_len] = ch;
	data->text[cur_len+1] = 0;
	data->length = cur_len+1;
}


/*********************************************************************
 * Proc:		string_c::AddInt
 * Purpose:	Appends a decimal signed integer, possibly with - sign
 *********************************************************************/

void string_c::AddInt(int value)
{
	char buf[32];
	sprintf(buf, "%d", value);
	AddString (buf);
}


/*********************************************************************
 * Proc:		string_c::AddDouble
 * Purpose:	Appends a signed double value, uses specified # of digits
 *********************************************************************/

void string_c::AddDouble(double value, unsigned int after_dot)
{
	char fmt[16];	
	char buf[256];

	if (after_dot > 48)
	{
		after_dot = 48;
	}

	sprintf (fmt, "%%.%df", (int) after_dot);
	sprintf (buf, fmt, value);

	AddString(buf);
}


/*********************************************************************
 * Proc:		string_c::CoreAddChars
 * Purpose:	Core code for AddChars() and operators +=; assumes
 *			howmany is bigger than 0
 *********************************************************************/

void string_c::CoreAddChars(const char* s, unsigned int howmany)
{
	// Prepare big enough Buffer
	Buffer (data->length + howmany);

	// And copy the bytes
	char* dest = data->text + data->length;
	memcpy (dest, s, howmany);
	dest[howmany] = 0;
	data->length = data->length + howmany;
}


/*********************************************************************
 * Proc:		string_c::operator += (both from const char* and from string_c)
 * Purpose:	Append a string to what we already contain.
 *********************************************************************/

void string_c::operator += (const string_c& obj)
{
	if (obj.data->length != 0)
		CoreAddChars (obj.data->text, obj.data->length);
}

void string_c::operator += (const char* s)
{
	size_t slen = strlen(s);

	if (slen != 0)
	{
		CoreAddChars (s, (unsigned int) slen);
	}
}

/*********************************************************************
 * Proc:		string_c::AddChars
 * Purpose:	Catenates a number of characters to our internal data.
 *********************************************************************/
void string_c::AddChars(const char* s, unsigned int startat, unsigned int howmany)
{
	if (howmany != 0)
	{
		CoreAddChars (s+startat, howmany);
	}
}

/*********************************************************************
 * Proc:		string_c::AddStringAtLeft
 * Purpose:	Prepend a string before us
 *********************************************************************/
void string_c::AddStringAtLeft(const char* s)
{
	size_t slen = strlen(s);

	if (slen == 0)
		return;

	// Make Buffer big enough
	Buffer (GetLength() + slen);

	// Move existing data -- do NOT use memcpy!!
	memmove (data->text+slen, data->text, GetLength()+1);

	// And copy bytes to be prepended
	memcpy (data->text, s, slen);

	data->length = data->length + slen;
}

/*********************************************************************
 * Proc:		operator +LPCSTR for string_c
 *********************************************************************/
string_c operator+(const char* lpsz, const string_c& s)
{
	string_c temp (s.GetLength() + strlen(lpsz));
	temp = lpsz;
	temp += s;
	return temp;
}

/*********************************************************************
 * Proc:		operator +char for string_c
 *********************************************************************/
string_c operator+(const char ch, const string_c& s)
{
	string_c temp (s.GetLength() + 1);
	temp  = ch;
	temp += s;
	return temp;
}

/*********************************************************************
 * Proc:		string_c::GetLastChar
 *********************************************************************/
char string_c::GetLastChar() const
{
	unsigned int l = GetLength();

	if (l < 1)
		ThrowBadIndex();

	return data->text[l-1];
}

/*********************************************************************
 * Proc:		string_c::GetMiddle
 *********************************************************************/
void string_c::GetMiddle(unsigned int start,
                                unsigned int chars,
                                string_c& result)
{
	result.Empty();

	// Nothing to return?
	unsigned int l = GetLength();

	if (l == 0 || (start+chars) == 0)
		return;

	// Do not return data beyond the end of the string
	if (start >= l)
    	return;

	if ((start+chars) >= l)
		chars = l - start;

	// Copy bytes
	result.CoreAddChars(GetString()+start, chars);
}
/*********************************************************************
 * Proc:		string_c::GetLeft
 *********************************************************************/
void string_c::GetLeft(unsigned int chars, string_c& result)
{
	GetMiddle(0, chars, result);
}

/*********************************************************************
 * Proc:		string_c::GetRight
 *********************************************************************/
void string_c::GetRight(unsigned int chars, string_c& result)
{
	if (chars >= GetLength())
	{
		result = *this;
		return;
	}

	GetMiddle(GetLength()-chars, chars, result);
}

/*********************************************************************
 * Proc:		string_c::TruncateAt
 * Purpose:	Cuts off the string at the character with the specified
 *			index.  The allocated Buffer remains the same.
 *********************************************************************/
void string_c::TruncateAt(unsigned int idx)
{
	if (idx >= GetLength())
		return;

	// Spawn a copy if necessary
	Buffer (data->alloc);		// Preserve Buffer size

	// And do the truncation
	data->text[idx] = 0;
	data->length = idx;
}

/*********************************************************************
 * Proc:		string_c::Find and ReverseFind
 * Purpose:	Scan the string for a particular character (must not
 *			be 0); return the index where the character is found
 *			first, or -1 if cannot be met
 *********************************************************************/
int string_c::Find (char ch, unsigned int startat /*= 0*/) const
{
	char* scan;

	// Start from middle of string?
	if (startat > 0)
	{
		if (startat >= GetLength())
			ThrowBadIndex();
	}

	scan = strchr(data->text+startat, ch);

	if (scan == NULL)
		return -1;
	else
		return (int)(scan - data->text);
}

int string_c::ReverseFind(char ch, unsigned int startat /*= 0*/) const
{
	if (startat == 0)
	{
		// Scan entire string
		char* scan = strrchr (data->text, ch);

		if (scan)
			return (int)(scan - data->text);
	}
	else
	{
		// Make sure the index is OK
		if (startat >= GetLength())
			ThrowBadIndex();

		for (int Findex = (int) startat-1; Findex >= 0; Findex--)
		{
			if (data->text[Findex] == ch)
				return Findex;
		}
	}

	return -1;
}


/*********************************************************************
 * Proc:		string_c::Compare and CompareNoCase
 *********************************************************************/
int string_c::Compare(const char* match) const
{
	int i = strcmp (data->text, match);

	if (i == 0)
		return 0;
	else if (i < 0)
		return -1;
	else
		return 1;
}

int string_c::CompareNoCase(const char* match) const
{
	int i = strcasecmp (data->text, match);

	if (i == 0)
		return 0;
	else if (i < 0)
		return -1;
	else
		return 1;
}


/*********************************************************************
 * Proc:		string_c::GrowTo
 * Purpose:	If the Buffer is smaller than the amount of characters
 *			specified, reallocates the Buffer.  This function cannot
 *			reallocate to a Buffer smaller than the existing one.
 *********************************************************************/
void string_c::GrowTo(unsigned int size)
{
	Buffer (size);
}


/*********************************************************************
 * Proc:		string_c::operator == (basic forms, the rest are inline)
 *********************************************************************/

bool operator ==(const string_c& s1, const string_c& s2)
{
	unsigned int slen = s2.GetLength();

	if (s1.GetLength() != slen)
		return false;

	return memcmp(s1.GetString(), s2, slen) == 0;
}

bool operator ==(const string_c& s1, char* s2)
{
	unsigned int slen = (unsigned int)strlen(s2);

	if (s1.GetLength() != slen)
		return false;

	return memcmp(s1.GetString(), s2, slen) == 0;
}


/*********************************************************************
 * Proc:		string_c::RemoveLeft
 *********************************************************************/
void string_c::RemoveLeft(unsigned int count)
{
	if (GetLength() <= count)
	{
		Empty();
		return;
	}

	if (count == 0)
		return;

	Buffer (data->alloc);		// Preserve Buffer size
	memmove (data->text, data->text+count, GetLength()-count+1);
	data->length = data->length - count;
}

void string_c::RemoveMiddle(unsigned int start, unsigned int count)
{
	if (GetLength() <= start)
	{
		Empty();
		return;
	}

	Buffer (data->alloc);		// Preserve Buffer size

	char* pstart = data->text + start;
	if (GetLength() <= (start+count))
	{
		pstart[0] = 0;
		data->length = start;
		return;
	}

	memmove (pstart, pstart+count, GetLength()-(start+count)+1);
	data->length = data->length - count;
}

void string_c::RemoveRight(unsigned int count)
{
	if (GetLength() <= count)
		Empty();
	else
		TruncateAt (GetLength() - count);
}

/*********************************************************************
 * Proc:		string_c::FmtOneValue
 * Purpose:	Helper for string_c::Format, Formats one %??? item
 * In:		x - ptr to the '%' sign in the specification; on exit,
 *				will point to the first char after the spec.
 * Out:		True if OK, False if should end Formatting (but also copy
 *			the remaining characters at x)
 *********************************************************************/
bool string_c::FmtOneValue (const char*& x, va_list& marker)
{
	// Start copying Format specifier to a local Buffer
	char fsbuf[64];
	fsbuf[0] = '%';
	int fsp = 1;

/*	
 GetMoreSpecifiers:
	// Get one character
#ifdef DEBUG
	if (fsp >= sizeof(fsbuf))
	{
		string_c::ThrowPGMError();
		return false;
	}
#endif
	char ch = x[0];
	if (ch == 0)
		return false;		// unexpected end of Format string
	x++;

	// Chars that may exist in the Format prefix
	const char fprefix[] = "-+0 #*.123456789hlL";
	if (strchr (fprefix, ch) != NULL)
	{
		fsbuf[fsp] = ch;
		fsp++;
		goto GetMoreSpecifiers;
	}
*/
	bool bGetMore;
	char ch;

	bGetMore = true;
	do
	{
#ifdef DEBUG
		if (fsp >= (int)sizeof(fsbuf))
		{
			string_c::ThrowPGMError();
			return false;
		}
#endif
		ch = x[0];
		if (ch == 0)
			return false;		// unexpected end of Format string
		x++;

		// Chars that may exist in the Format prefix
		const char fprefix[] = "-+0 #*.123456789hlL";
		if (strchr (fprefix, ch) != NULL) 
		{
			fsbuf[fsp] = ch;
			fsp++;
		}
		else
		{
			bGetMore = false;
		}
	}
	while (bGetMore);

	// 's' is the most important parameter specifier type
	if (ch == 's') 
	{
		// Find out how many characters should we actually print.
		//   To do this, get the string length, but also try to
		//   detect a .precision field in the Format specifier prefix.
		const char* value = va_arg (marker, const char*);
		unsigned int slen = (unsigned int)strlen(value);
		fsbuf[fsp] = 0;
		char* precis = strchr (fsbuf, '.');

		if (precis != NULL  &&  precis[1] != 0) 
		{
			// Convert value after dot, put within 0 and slen
			char* endptr;
			int result = (int) strtol (precis+1, &endptr, 10);
			if (result >= 0  &&  result < int(slen))
				slen = (unsigned int) result;
		}

		// Copy the appropriate number of characters
		if (slen > 0)
			CoreAddChars (value, (unsigned int) slen);

		return true;
	}

	// '!' is our private extension, allows direct passing of string_c*
	if (ch == '!') 
	{
		// No precision characters taken into account here.
		const string_c* value = va_arg (marker, const string_c*);
		*this += *value;
		return true;
	}

	// Chars that Format an integer value
	const char intletters[] = "cCdiouxX";
	if (strchr (intletters, ch) != NULL) 
	{
		fsbuf[fsp] = ch;
		fsbuf[fsp+1] = 0;
		char valbuf[64];
		int value = va_arg (marker, int);
		sprintf (valbuf, fsbuf, value);
		*this += valbuf;
		return true;
	};

	// Chars that Format a double value
	const char dblletters[] = "eEfgG";

	if (strchr (dblletters, ch) != NULL) 
	{
		fsbuf[fsp] = ch;
		fsbuf[fsp+1] = 0;
		char valbuf[256];
		double value = va_arg (marker, double);
		sprintf (valbuf, fsbuf, value);
		*this += valbuf;
		return true;
	}

	// 'Print pointer' is supported
	if (ch == 'p') 
	{
		fsbuf[fsp] = ch;
		fsbuf[fsp+1] = 0;
		char valbuf[64];
		void* value = va_arg (marker, void*);
		sprintf (valbuf, fsbuf, value);
		*this += valbuf;
		return true;
	};

	// 'store # written so far' is obscure and unsupported
	if (ch == 'n') 
	{
		ThrowPGMError();
		return false;
	}

	// 'Print unicode string' is not supported also
	if (ch == 'S') 
	{
		ThrowNoUnicode();
		return false;
	}

	// If we fall here, the character does not represent an item
	AddChar(ch);
	return true;
}


/*********************************************************************
 * Proc:		string_c::Format
 * Purpose:	sprintf-like method
 *********************************************************************/
void string_c::FormatCore (const char* x, va_list& marker)
{
	for (;;) 
	{
		// Locate next % sign, copy chunk, and exit if no more
		char* next_p = strchr (x, '%');

		if (next_p == NULL)
			break;

		if (next_p > x)
			CoreAddChars (x, (int)(next_p-x));

		x = next_p+1;
		// We're at a parameter
		if (!FmtOneValue(x, marker))
			break;		// Copy rest of string and return
	}

	if (x[0] != 0)
		*this += x;
}

void string_c::Format(const char* fmt, ...)
{
	Empty();

	// Walk the string
	va_list marker;
	va_start(marker, fmt);
	FormatCore(fmt, marker);
	va_end(marker);
}

/*********************************************************************
 * Proc:		operator + (string_c and string_c, string_c and LPCSTR)
 *********************************************************************/
string_c operator+(const string_c& s1, const string_c& s2)
{
	string_c out (s1.GetLength() + s2.GetLength());
	out  = s1;
	out += s2;
	return out;
}

string_c operator+(const string_c& s, const char* lpsz)
{
	unsigned int slen = (unsigned int)strlen(lpsz);
	string_c out (s.GetLength() + slen);
	out.CoreAddChars(s.data->text, s.GetLength());
	out += lpsz;
	return out;
}

string_c operator+(const string_c& s, const char ch)
{
	string_c out (s.GetLength() + 1);
	out.CoreAddChars(s.data->text, s.GetLength());
	out += ch;
	return out;
}

/*********************************************************************
 * Proc:		string_c::LeftTrim
 * Purpose:	Remove leading characters from a string.  All characters
 *			to be excluded are passed as a parameter; NULL means
 *			'truncate spaces'
 *********************************************************************/
void string_c::LeftTrim(const char* charset /*= NULL*/)
{
	unsigned int good = 0;

	if (charset) 
	{
		while (strchr (charset, data->text[good]) != NULL)
			good++;
	}
	else 
	{
		while (data->text[good] == ' ')
			good++;
	}

	if (good > 0)
		RemoveLeft(good);
}


/*********************************************************************
 * Proc:		string_c::Trim
 * Purpose:	Remove trailing characters; see LeftTrim
 *********************************************************************/
void string_c::Trim(const char* charset /*= NULL*/)
{
	unsigned int good = data->length;

	if (good == 0)
		return;

	if (charset) 
	{
		while (good > 0  &&  strchr (charset, data->text[good-1]) != NULL)
			--good;
	}
	else 
	{
		while (good > 0  &&  data->text[good-1] == ' ')
			--good;
	}

	TruncateAt (good);		// Also works well with good == 0
}

/*********************************************************************
 * Proc:		string_c::ToLower
 * Purpose:	Converts string characters to lower case
 *********************************************************************/
void string_c::ToLower(void)
{
	if (!data->length)
		return;
	
	char *s = data->text;
	do
	{
		*s = tolower(*s);
	}
	while(*s++);
}

/*********************************************************************
 * Proc:		string_c::ToUpper
 * Purpose:	Converts string characters to lower case
 *********************************************************************/
void string_c::ToUpper(void)
{
	if (!data->length)
		return;
	
	char *s = data->text;
	do
	{
		*s = toupper(*s);
	}
	while(*s++);
}

};
