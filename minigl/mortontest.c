#include <stdio.h>
#include "morton.h"

int main(int argc, char **argv) {
#if 0
	//8x2
	int initx = 0, inity = 0, mx = 8,my = 2;
	unsigned int xw = 2, yw = 16;
#elif 0
	//8x4
	int initx = 0, inity = 0, mx = 8,my = 4;
	unsigned int xw = 3, yw = 16;
#elif 1
	//4x8
	int initx = 0, inity = 0, mx = 4,my = 8;
	unsigned int xw = 16, yw = 4;
#elif 0
	//2x8
	int initx = 0, inity = 0, mx = 2,my = 8;
	unsigned int xw = 16, yw = 3;
#else
	//8x8 standard
	int initx = 0, inity = 0, mx = 8,my = 8;
	unsigned int xw = 16, yw = 16;
#endif

	printf("%ix%i -> %ix%i\n",mx,my,xw,yw);
	
	mortonx_t sxm = Int2MortonXWidth(initx,xw), xm;
	mortony_t sym = Int2MortonYWidth(inity,yw), ym;
	
	printf("initial %i %i\n",sxm,sym);
	int x,y;
	ym = sym;
	for(y = 0; y < my; y++) {
		xm = sxm;
		for(x = 0; x < mx; x++) {
			printf("%i %i -> %08x  ", x, y, CombineMorton(xm,ym));
			xm = IncPartialMortonX(xm,xw);
		}
		printf("\n");
		ym = IncPartialMortonY(ym,yw);
	}
	return 0;
}
