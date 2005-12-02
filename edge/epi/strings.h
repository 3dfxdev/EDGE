//----------------------------------------------------------------------------
//  EDGE String Class
//----------------------------------------------------------------------------
//
//  Copyright (c) 2002-2005  The EDGE Team.
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
#ifndef __EPI_STRING_CLASS__
#define __EPI_STRING_CLASS__

#include "epi.h"

#include <stdarg.h>

namespace epi 
{

// The structure below is implementation specific, do not attempt to use it directly!
struct string_desc_s
{
	// Data zone
	unsigned int usage;		// Usage counter, 0 in Empty slots, always 2 in m_null
	char* text;				// 0-terminated string, or next free string_desc_s* if usage==0
	unsigned int length;	// Actual string length, excl. 0
	unsigned int alloc;		// Allocated length, excl. 0
};

#define EPISTR_DITEMS 320		/* Max bytes to be kept in blocks pool */

class string_c
{
public:
	string_c();
	string_c(const string_c& source);
	string_c(const char* s, unsigned int prealloc = 0);
	string_c(unsigned int prealloc);
	~string_c();

private:
	string_desc_s* data;
	static void freestr(string_desc_s* tofree);
	static void freestr_nMT(string_desc_s* tofree);

public:
	const string_c& operator=(const string_c& source);
	const string_c& operator=(const char* s);

    // Get attributes, get data, Compare
	bool IsEmpty() const;
	unsigned int GetLength() const;

	// FIXME: -AJA- not sure about this auto-conversion
	operator const char* () const;
	const char* GetString() const;														// Same as above
	char GetFirstChar() const;
	char GetLastChar() const;
	char operator[](unsigned int idx) const;
	char GetAt(unsigned int idx) const;												// Same as above
	void GetLeft(unsigned int chars, string_c& result);
	void GetRight(unsigned int chars, string_c& result);
	void GetMiddle(unsigned int start, unsigned int chars, string_c& result);
	int Find(char ch, unsigned int startat = 0) const;
	int ReverseFind(char ch, unsigned int startat = 0) const;
	int Compare(const char* match) const;												// -1, 0 or 1
	int CompareNoCase(const char* match) const;										// -1, 0 or 1

	// Operators == and != are also predefined

    // Global modifications
	void Empty();																		// Sets length to 0, but keeps Buffer around
	void Reset();																		// This also releases the Buffer
	void GrowTo(unsigned int size);
	void Compact(unsigned int only_above = 0);
	static void CompactFree();
	void Format(const char* fmt, ...);
	void FormatWithArgList(const char* fmt, va_list& marker);

    // Catenation, truncation
	void operator += (const string_c& obj);
	void operator += (const char* s);
	void operator += (const char ch);													// Same as AddChar
	void AddString(const string_c& obj);											// Same as +=
	void AddString(const char* s);														// Same as +=
	void AddChar(char ch);
	void AddChars(const char* s, unsigned int startat, unsigned int howmany);
	void AddStringAtLeft(const string_c& obj);
	void AddStringAtLeft(const char* s);
	void AddInt(int value);
	void AddDouble(double value, unsigned int after_dot);
	void RemoveLeft(unsigned int count);
	void RemoveMiddle(unsigned int start, unsigned int count);
	void RemoveRight(unsigned int count);
    void Set(const char* s);
    void Set(const string_c& obj);
	void ToLower(void);
	void ToUpper(void);
	void TruncateAt(unsigned int idx);
	friend string_c operator+(const string_c& s1, const string_c& s2);
	friend string_c operator+(const string_c& s, const char* lpsz);
	friend string_c operator+(const char* lpsz, const string_c& s);
	friend string_c operator+(const string_c& s, const char ch);
	friend string_c operator+(const char ch, const string_c& s);
	void Trim(const char* charset = NULL);												// Remove all trailing
	void LeftTrim(const char* charset = NULL);											// Remove all leading
	void AllTrim(const char* charset = NULL);											// Remove both

    // Miscellaneous implementation methods
protected:
	void NewFromString(const char* s, unsigned int slen, unsigned int prealloc);
	void Buffer(unsigned int newlength);
	void CoreAddChars(const char* s, unsigned int howmany);
	void FormatCore(const char* x, va_list& marker);
	bool FmtOneValue(const char*& x, va_list& marker);

public:
	// These are public to avoid friend function definitions...
	static unsigned char* m_cache[];													// Of size CSTR_DITEMS >> 2
	static string_desc_s m_null;

	// These may be reimplemented, see below
	static void ThrowIfNull(void* p);
	static void ThrowPGMError();
	static void ThrowNoUnicode();
	static void ThrowBadIndex();
};

bool operator ==(const string_c& s1, const string_c& s2);
bool operator ==(const string_c& s1, char* s2);
bool operator ==(char* s1, const string_c& s2);
bool operator !=(const string_c& s1, const string_c& s2);
bool operator !=(const string_c& s1, char* s2);
bool operator !=(char* s1, const string_c& s2);

// Inline string_c method implementations
void EPIStringAbortApplication(int fatal_type);
inline void string_c::ThrowBadIndex()			{ EPIStringAbortApplication(2); }
inline void string_c::ThrowPGMError()			{ EPIStringAbortApplication(3); }
inline void string_c::ThrowNoUnicode()		{ EPIStringAbortApplication(4); }
inline void string_c::ThrowIfNull(void* p)	{ if (p == NULL) EPIStringAbortApplication(5); }

// Round up to next value divisible by 4, minus one.  Usually compiles to 'or bx,3' :-))
inline void up_alloc (unsigned int& value)
{
	value = (unsigned int)(value | 3);
}

#define SET_DATA_EMPTY()		\
	data = &string_c::m_null;		\
	data->usage++;


/*********************************************************************
* Proc:		string_c::string_c()
* Purpose:	Constructs an Empty instance
*********************************************************************/
inline string_c::string_c()
{
	SET_DATA_EMPTY();
}

/*********************************************************************
* Proc:		string_c::Set
*********************************************************************/
inline void string_c::Set(const char* s)
{
    if (s)
        *this = s;
    else
        Empty();
}

inline void string_c::Set(const string_c& obj)
{
    *this = obj;
}

/*********************************************************************
* Proc:		string_c::GetFirstChar
*********************************************************************/
inline char string_c::GetFirstChar() const
{
	return data->text[0];
}


/*********************************************************************
* Proc:		string_c::IsEmpty and GetLength
*********************************************************************/
inline bool string_c::IsEmpty() const
{
	return data->length == 0;
}

inline unsigned int string_c::GetLength() const
{
	return data->length;
}


/*********************************************************************
* Proc:		string_c::operator []
*********************************************************************/

inline char string_c::operator[](unsigned int idx) const
{
#ifdef DEBUG
	if (idx >= GetLength())
		ThrowBadIndex();
#endif
	return data->text[idx];
}

inline char string_c::GetAt(unsigned int idx) const
{
	return this->operator[](idx);
//	return *(const char*)this[idx];
}


/*********************************************************************
* Proc:		string_c::operator 'cast to const char*'
*********************************************************************/

inline string_c::operator const char* () const
{
	return data->text;
}

inline const char* string_c::GetString() const
{
	return data->text;
}


/*********************************************************************
* Proc:		string_c::[operator ==] and [operator !=] inlined forms
*********************************************************************/

inline bool operator ==(char* s1, const string_c& s2)
{
    return (s2 == s1);
}

inline bool operator !=(const string_c& s1, const string_c& s2)
{
    return !(s1 == s2);
}

inline bool operator !=(const string_c& s1, char* s2)
{
    return !(s1 == s2);
}

inline bool operator !=(char* s1, const string_c& s2)
{
    return !(s2 == s1);
}


/*********************************************************************
* Proc:		string_c::FormatWithArgList - synonyms for FormatCore
*********************************************************************/

inline void string_c::FormatWithArgList(const char *fmt, va_list& marker)
{
    Empty();
    FormatCore(fmt, marker);
}

/*********************************************************************
* Proc:		string_c::AddString - synonyms for operators +=
*********************************************************************/

inline void string_c::AddString(const string_c& obj)
{
    *this += obj;
}

inline void string_c::AddString(const char* s)
{
    *this += s;
}

inline void string_c::AddStringAtLeft(const string_c& obj)
{
	AddStringAtLeft(obj.GetString());
}

inline void string_c::operator += (const char ch)
{
	AddChar (ch);
}

inline void string_c::AllTrim(const char* charset /*= NULL*/)
{
	Trim(charset);
	LeftTrim(charset);
}

};

#endif // __EPI_STRING_CLASS__
