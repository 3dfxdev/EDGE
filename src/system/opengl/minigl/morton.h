#ifndef MORTON_H
#define MORTON_H

#include <limits.h>

typedef unsigned int mortonx_t;
typedef unsigned int mortony_t;

static __attribute__ ((noinline)) unsigned int Part1By1(unsigned int x)
{
	// from http://fgiesen.wordpress.com/2009/12/13/decoding-morton-codes/
	x &= 0x0000ffff;                  // x = ---- ---- ---- ---- fedc ba98 7654 3210
	x = (x ^ (x <<  8)) & 0x00ff00ff; // x = ---- ---- fedc ba98 ---- ---- 7654 3210
	x = (x ^ (x <<  4)) & 0x0f0f0f0f; // x = ---- fedc ---- ba98 ---- 7654 ---- 3210
	x = (x ^ (x <<  2)) & 0x33333333; // x = --fe --dc --ba --98 --76 --54 --32 --10
	x = (x ^ (x <<  1)) & 0x55555555; // x = -f-e -d-c -b-a -9-8 -7-6 -5-4 -3-2 -1-0
	return x;
}
static unsigned int BitMask(unsigned int length) {
	if (length >= 32)
		return UINT_MAX;
	else
		return ~(UINT_MAX << length);
}
/*
	source:       abcd efgh
	width 2:   ab cdef 0g0h
	width 4: abcd 0e0f 0g0h
*/
static unsigned int PartialMorton(unsigned int y, unsigned int mortonwidth) {
	unsigned int mortonpart = BitMask(mortonwidth << 1) & Part1By1(y);
	unsigned int linearpart = (y >> mortonwidth) << (mortonwidth << 1);
	return mortonpart | linearpart;
}
static unsigned int PartialMortonMask(unsigned int mortonwidth) {
	return 0x55555555u & BitMask(mortonwidth);
}
static mortonx_t Int2MortonXWidth(unsigned int x, unsigned int width) {
	return PartialMorton(x,width) << 1;
}
static mortonx_t Int2MortonX(unsigned int x) {
	return Int2MortonXWidth(x,16) << 1;
}
static mortonx_t AddMortonX(mortonx_t a, mortonx_t b) {
	return  (a + b + 0xaaaaaaaa) & 0x55555555;
}
static mortonx_t AddPartialMortonX(mortonx_t a, mortonx_t b, unsigned int mortonwidth) {
	return  (a + b + (PartialMortonMask(mortonwidth)<<1)) & ~(PartialMortonMask(mortonwidth)<<1);
}
static mortonx_t IncMortonX(mortonx_t a) {
	return AddMortonX(a, Int2MortonX(1));
}
static mortonx_t IncPartialMortonX(mortonx_t a, unsigned int mortonwidth) {
	return AddPartialMortonX(a, Int2MortonXWidth(1,mortonwidth), mortonwidth);
}

static mortony_t Int2MortonYWidth(unsigned int y, unsigned int width) {
	return PartialMorton(y,width);
}
static mortony_t Int2MortonY(unsigned int y) {
	return Int2MortonYWidth(y,16);
}
static mortony_t AddMortonY(mortony_t a, mortony_t b) {
	return  (a + b + 0x55555555) & 0xaaaaaaaa;
}
static mortony_t AddPartianMortonY(mortony_t a, mortony_t b, unsigned int mortonwidth) {
	return  (a + b + PartialMortonMask(mortonwidth)) & ~PartialMortonMask(mortonwidth);
}
static mortony_t IncMortonY(mortony_t a) {
	return AddMortonY(a, Int2MortonY(1));
}
static mortony_t IncPartialMortonY(mortony_t a, unsigned int mortonwidth) {
	return AddPartianMortonY(a, Int2MortonYWidth(1,mortonwidth), mortonwidth);
}

static unsigned int CombineMorton(mortonx_t x, mortony_t y) {
	return x | y;
}

#endif
