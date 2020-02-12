#ifndef __M_SWAP__
#define __M_SWAP__

//#include "wl_def.h"

static inline WORD ReadLittleShort(const BYTE * const ptr)
{
	return WORD(BYTE(*ptr)) |
		(WORD(BYTE(*(ptr+1)))<<8);
}

static inline WORD ReadBigShort(const BYTE * const ptr)
{
	return WORD(BYTE(*(ptr+1))) |
		(WORD(BYTE(*ptr))<<8);
}

static inline DWORD ReadLittle24(const BYTE * const ptr)
{
	return DWORD(BYTE(*ptr)) |
		(DWORD(BYTE(*(ptr+1)))<<8) |
		(DWORD(BYTE(*(ptr+2)))<<16);
}

static inline DWORD ReadLittleLong(const BYTE * const ptr)
{
	return DWORD(BYTE(*ptr)) |
		(DWORD(BYTE(*(ptr+1)))<<8) |
		(DWORD(BYTE(*(ptr+2)))<<16) |
		(DWORD(BYTE(*(ptr+3)))<<24);
}

static inline DWORD ReadBigLong(const BYTE * const ptr)
{
	return (DWORD(BYTE(*ptr))<<24) |
		(DWORD(BYTE(*(ptr+1)))<<16) |
		(DWORD(BYTE(*(ptr+2)))<<8) |
		DWORD(BYTE(*(ptr+3)));
}

static inline void WriteLittleShort(BYTE * const ptr, WORD value)
{
	ptr[0] = value&0xFF;
	ptr[1] = (value>>8)&0xFF;
}


// Now for some writing
// Syntax: char data[x] = {WRITEINT32_DIRECT(integer),WRITEINT32_DIRECT(integer)...}
#if 0
#define WRITEINT32_DIRECT(integer) (BYTE)(integer&0xFF),(BYTE)((integer>>8)&0xFF),(BYTE)((integer>>16)&0xFF),(BYTE)((integer>>24)&0xFF)
#define WRITEINT16_DIRECT(integer) (BYTE)(integer&0xFF),(BYTE)((integer>>8)&0xFF)
#define WRITEINT8_DIRECT(integer) (BYTE)(integer&0xFF)

#define WRITEINT32(pointer, integer) *pointer = (BYTE)(integer&0xFF);*(pointer+1) = (BYTE)((integer>>8)&0xFF);*(pointer+2) = (BYTE)((integer>>16)&0xFF);*(pointer+3) = (BYTE)((integer>>24)&0xFF);
#define WRITEINT24(pointer, integer) *pointer = (BYTE)(integer&0xFF);*(pointer+1) = (BYTE)((integer>>8)&0xFF);*(pointer+2) = (BYTE)((integer>>16)&0xFF);
#define WRITEINT16(pointer, integer) *pointer = (BYTE)(integer&0xFF);*(pointer+1) = (BYTE)((integer>>8)&0xFF);
#define WRITEINT8(pointer, integer) *pointer = (BYTE)(integer&0xFF);
#endif

// After the fact Byte Swapping ------------------------------------------------

static inline WORD SwapShort(WORD x)
{
	return ((x&0xFF)<<8) | ((x>>8)&0xFF);
}

static inline DWORD SwapLong(DWORD x)
{
	return ((x&0xFF)<<24) |
		(((x>>8)&0xFF)<<16) |
		(((x>>16)&0xFF)<<8) |
		((x>>24)&0xFF);
}

#ifdef __BIG_ENDIAN__
#define BigShort(x) (x)
#define BigLong(x) (x)
#define LittleShort SwapShort
#define LittleLong SwapLong
#else
#define BigShort SwapShort
#define BigLong SwapLong
#define LittleShort(x) (x)
#define LittleLong(x) (x)
#endif

#endif /* __M_SWAP__ */
