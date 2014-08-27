//from quake 2
#include "byteorder.h"
/*
============================================================================

					BYTE ORDER FUNCTIONS

============================================================================
*/

int	bigendien;

// can't just use function pointers, or dll linkage can
// mess up when qcommon is included in multiple places
// Knightmare- made these static
static short	(*_BigShort) (short l);
static short	(*_LittleShort) (short l);
static int		(*_BigLong) (int l);
static int		(*_LittleLong) (int l);
static qint64	(*_BigLong64) (qint64 l);
static qint64	(*_LittleLong64) (qint64 l);
static float	(*_BigFloat) (float l);
static float	(*_LittleFloat) (float l);

short	BigShort(short l){return _BigShort(l);}
short	LittleShort(short l) {return _LittleShort(l);}
int		BigLong (int l) {return _BigLong(l);}
int		LittleLong (int l) {return _LittleLong(l);}
qint64	BigLong64 (qint64 l) {return _BigLong64(l);}
qint64	LittleLong64 (qint64 l) {return _LittleLong64(l);}
float	BigFloat (float l) {return _BigFloat(l);}
float	LittleFloat (float l) {return _LittleFloat(l);}

short ShortSwap (short l)
{
	byte    b1,b2;

	b1 = l&255;
	b2 = (l>>8)&255;

	return (b1<<8) + b2;
}

short ShortNoSwap (short l)
{
	return l;
}

int LongSwap (int l)
{
	byte    b1,b2,b3,b4;

	b1 = l&255;
	b2 = (l>>8)&255;
	b3 = (l>>16)&255;
	b4 = (l>>24)&255;

	return ((int)b1<<24) + ((int)b2<<16) + ((int)b3<<8) + b4;
}

int	LongNoSwap (int l)
{
	return l;
}

qint64 Long64Swap (qint64 ll)
{
	byte    b1,b2,b3,b4,b5,b6,b7,b8;

	b1 = ll&255;
	b2 = (ll>>8)&255;
	b3 = (ll>>16)&255;
	b4 = (ll>>24)&255;
	b5 = (ll>>32)&255;
	b6 = (ll>>40)&255;
	b7 = (ll>>48)&255;
	b8 = (ll>>56)&255;

	return ((qint64)b1<<56) + ((qint64)b2<<48) + ((qint64)b3<<40) + ((qint64)b4<<32)
			+ ((qint64)b5<<24) + ((qint64)b6<<16) + ((qint64)b7<<8) + (qint64)b8;
}

qint64	Long64NoSwap (qint64 ll)
{
	return ll;
}

float FloatSwap (float f)
{
	union
	{
		float	f;
		byte	b[4];
	} dat1, dat2;
	
	
	dat1.f = f;
	dat2.b[0] = dat1.b[3];
	dat2.b[1] = dat1.b[2];
	dat2.b[2] = dat1.b[1];
	dat2.b[3] = dat1.b[0];
	return dat2.f;
}

float FloatNoSwap (float f)
{
	return f;
}

/*
================
Swap_Init
================
*/
void Swap_Init (void)
{
	//byte	swaptest[2] = {1,0};
	union { char a[2]; short b; } swaptest = {{1,0}};

	// set the byte swapping variables in a portable manner	
	if ( swaptest.b == 1)
	{
		bigendien = 0;
		_BigShort = ShortSwap;
		_LittleShort = ShortNoSwap;
		_BigLong = LongSwap;
		_LittleLong = LongNoSwap;
		_BigLong64 = Long64Swap;
		_LittleLong64 = Long64NoSwap;
		_BigFloat = FloatSwap;
		_LittleFloat = FloatNoSwap;
	}
	else
	{
		bigendien = 1;
		_BigShort = ShortNoSwap;
		_LittleShort = ShortSwap;
		_BigLong = LongNoSwap;
		_LittleLong = LongSwap;
		_BigLong64 = Long64NoSwap;
		_LittleLong64 = Long64Swap;
		_BigFloat = FloatNoSwap;
		_LittleFloat = FloatSwap;
	}
}
