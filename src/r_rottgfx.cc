#if 0
//----------------------------------------------------------------------------
//  EDGE2 Rise of the Triad Graphics Conversion Routines
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2017  The EDGE2 Team.
//  Copyright  ©  2007, 2008  Birger N. Andreasen  
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

#include "system/i_defs.h"
#include "system/i_defs_gl.h"

#include <limits.h>

#include "../epi/endianess.h"
#include "../epi/file.h"
#include "../epi/filesystem.h"
#include "../epi/types.h"

#include "../epi/image_data.h"
#include "../epi/image_hq2x.h"
#include "../epi/image_png.h"
#include "../epi/image_jpeg.h"
#include "../epi/image_tga.h"

#include "dm_state.h"
#include "e_search.h"
#include "e_main.h"
#include "m_argv.h"
#include "m_misc.h"
#include "p_local.h"
#include "r_defs.h"
#include "r_image.h"
#include "r_gldefs.h"
#include "r_sky.h"
#include "r_colormap.h"
#include "r_texgl.h"
#include "w_texture.h"
#include "w_wad.h"
#include "z_zone.h"

#define GAMMAENTRIES (64*8)
#include "games/rott/rott_local.h"

// EDGE: Changes from WinROTT:
// EDGE: Rename of collumofs -> columnofs (EDGE) for rottpatch_t
// EDGE; patch_t renamed to rottpatch_t 

extern int gammaindex;
extern u8_t gammatable[GAMMAENTRIES];

// EPI-fied BYTE -> u8_t!
//#define BOOL int
#define BGbyte 255
#define TRANSbyte 254
#define TRUE 1
#define FALSE 0
#define ALPHA_SHIFT 24
byte *origpal;


int ispalloaded = 0;
rgb_t colrs[256];

// The rest of these have moved into src/games/rott/rott_local.h!
//Leave this here, as it uses HANDLE (win-yuck)
int Convert24bitBmpTo8bitExt(u8_t * tgt, HANDLE hImage);


int ConvertRottSpriteIntoPCX(u8_t  *RottSpriteMemptr, u8_t  *RottPCXptr)
{
	u8_t *ptr, *old, *src, *wrksrc;
	u32_t count = 0, x, y, EmthyPixCnt, NbOfPix;
	rottpatch_t *p; // ROTT Patch format (patch_t in EDGE is reserved for DOOM patches)

	//on entry RottPCXptr must point to a me area of correct pcx size
	//RottSpriteMemptr contains ptr to sprite

	p = (rottpatch_t*)RottSpriteMemptr;
	src = RottSpriteMemptr;

	if ((p->width <= 1) || (p->height <= 1))
		return FALSE;
	if ((p->width > 320) || (p->height >= 256))
		return FALSE;

	//set the background color in pcx
	if ((p->width*p->height) >= 0xFFFF)
	{
		// ("Error in Sprite header size");
		return FALSE;
	}

	memset(RottPCXptr, BGbyte, (p->width*p->height) + 512);

	ptr = RottPCXptr;


	for (x = 0; x < (unsigned)(p->width); x++)
	{
		wrksrc = src + (p->columnofs[x]);//wrksrc points to data in src
		old = ptr;//ptr = where to store our new pcx pic data

		while (*(wrksrc++) != 0xFF)
		{
			EmthyPixCnt = *(wrksrc - 1);//get nb of pixel to skip in collum
			NbOfPix = *(wrksrc++);//get nb of pixels to draw
			ptr = old + (EmthyPixCnt*p->width);//caculate start point of target

			for (y = 0; y < NbOfPix; y++)
			{//copy pixels to target mem 
				(*ptr) = *(wrksrc++);
				if (*ptr == BGbyte)//if there is a color like BGbyte
					*ptr = 16;//change it to 16 (allmost white)
				ptr += p->width;
			}
		}
		ptr = old + 1;//increase to next col
	}
	return TRUE;
}



int ConvertMaskedRottSpriteIntoPCX(u8_t  *RottSpriteMemptr, u8_t  *RottPCXptr)
{
	transpatch_t *ppat = (transpatch_t *)RottSpriteMemptr;
	int i, j, ofs, rlen;
	u8_t *idpos = RottPCXptr;
	u8_t *spos;
	u8_t *dpos;
	int tmask = (63 - ppat->translevel) << (ALPHA_SHIFT + 2);
	u16_t *ccolofs = (u16_t*)ppat->columnofs;

	if ((ppat->width*ppat->height) >= 0xFFFF)
	{
		// ("Error in Sprite header size");
		//EDGE: This should be 
		I_Error("ROTT: Error in Sprite Header Size\n");
		return FALSE;
	}
	memset(RottPCXptr, BGbyte, ppat->width * ppat->height * 4);

	for (i = 0; i < ppat->width; i++, ccolofs++)
	{
		spos = (((u8_t *)ppat) + (*ccolofs));

		while (1)
		{
			if ((ofs = *(spos++)) == 0xFF)

			{
				break;
			}

			else

			{
				rlen = *(spos++);
				dpos = (idpos + (i + (ppat->width * ofs)));

				if (*spos == 254)
				{
					for (j = 0; j < rlen; j++, dpos += ppat->width)
						*dpos = TRANSbyte;
					spos++;
				}

				else
				{
					for (j = 0; j < rlen; j++, spos++, dpos += ppat->width)
					{
						*dpos = *spos;

						if (*dpos == BGbyte)//if there is a color like BGbyte
							*dpos = 16;//change it to 16 (allmost white)

					}
				}
			}
		}
	}
	return TRUE;
}


// EDGE: Figure out how to read IntelShort (if it's endianess conversion, look in epi/endianess.h)
// EDGE: return FALSE; does not match function type (ugh)

void GetConvWidthHeight(u8_t  *RottSpriteMemptr, int *w, int *h);
void GetConvWidthHeight(u8_t  *RottSpriteMemptr, int *w, int *h)
{
	int cofs, width;
	rottpatch_t *p;
	transpatch_t *ppat = (transpatch_t *)RottSpriteMemptr;

	if ((ppat->width <= 1) || (ppat->height <= 1))
		I_Error("ROTT: conversion height less than 1!\n");
	return;
	if ((ppat->width > 320) || (ppat->height >= 256))
		I_Error("ROTT: conversion width greater than 320x256!\n");
	return;

	cofs = EPI_LE_U16(ppat->columnofs[0]);
	width = EPI_LE_U16(ppat->width);

	if (cofs != (12 + width * 2))
	{
		p = (rottpatch_t*)RottSpriteMemptr;
		*w = p->width;
		*h = p->height;
	}
	else
	{
		*w = ppat->width;
		*h = ppat->height;
	}


}

// EDGE: Figure out how to read IntelShort (if it's endianess conversion, look in epi/endianess.h)

void GetConv(u8_t  *RottSpriteMemptr, u8_t  *RottPCXptr);
void GetConv(u8_t  *RottSpriteMemptr, u8_t  *RottPCXptr)
{
	int cofs, width;
	u8_t *mem, *src;
	int h, w;
	transpatch_t *ppat = (transpatch_t *)RottSpriteMemptr;
	//if (len <= 12) 
	//	return 0; // Too short for a transpatch

	//CA: Let's try renaming this IntelShort to EPI_LE_U16 instead.
	cofs = EPI_LE_U16(ppat->columnofs[0]);
	width = EPI_LE_U16(ppat->width);

	if (cofs != (12 + width * 2)) {
		ConvertRottSpriteIntoPCX(RottSpriteMemptr, RottPCXptr);
	}
	else {
		ConvertMaskedRottSpriteIntoPCX(RottSpriteMemptr, RottPCXptr);
	}
	/*
		src = RottPCXptr;
		mem = GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT,64*64*14);
		for (h=0;h<64;h++){
			for (w=0;w<64;w++){
				*(mem+(64*w)+h) = *(src++);
			}
		}
		memcpy (RottPCXptr, mem, 64*64);
		GlobalFree (mem);*/

}





int CntNbOfBGbyteu8_ts(u8_t *src, int w, int h)
{
	int cnt = 0;
	//count nb not visible (1's BGbytes)of pixels down to first pixels
	while ((*src == BGbyte) && (cnt < h))
	{
		cnt++;
		src += w;
	}
	//not 1 reached, exit now
	return cnt;
}

int CntNbOfNotBGbyteu8_ts(u8_t *src, int w, int h)
{
	int cnt = 0;
	//count nb of pixels down to first (1's BGbytes) pixels
	while ((*src != BGbyte) && (cnt < h))
	{
		cnt++;
		src += w;
	}
	//1 reached, exit
	return cnt;
}

//extern  unsigned bufferofs;
void ShrinkMemPictureExt(int orgw, int orgh, int neww, int newy, u8_t* src, u8_t * target)
{
	//shrink mem picure and plce it in tmpPICbuf 
	u8_t *tmp2, *s;
	u8_t x, y, tmp;
	int cnt;
	float Yratio, Xratio, old, oldY, Ycnt;

	s = (u8_t*)(src);

	Xratio = orgw * 10 / neww;
	Xratio = (Xratio / 10);
	cnt = (int)Xratio; //2
	Xratio = (Xratio - cnt) / 2; //.2
	old = 0;
	Yratio = orgh * 10 / newy;//we shall schrink 480/600 lines to 200  
	Yratio = (Yratio / 10);
	Ycnt = (int)Yratio; //2
	Yratio = (Yratio - Ycnt); //.2
	oldY = 0;

	*(target) = *(s);

	// redundant pixels
	for (y = 0; y < orgh; y++)
	{
		//EDGE: u8_t cannot be assigned to an entity of type int
		tmp = reinterpret_cast<u8_t>(s);
		tmp2 = reinterpret_cast<u8_t*>(target);

		for (x = 0; x < neww; x++)
		{
			//copy 1 pixel

			*(target += neww) = *(s);
			s += cnt;
			old += Xratio;

			if (old >= 1)
			{
				s++;
				old -= 1;
			}

		}

		oldY += Yratio;
		y += Ycnt;

		if (oldY >= 1)
		{
			y++;
			oldY -= 1;
		}
		//EDGE: a value of type "int" cannot be assigned to an entity of type "u8_t*"
		s = reinterpret_cast<u8_t*>(tmp) + orgw;

		target = tmp2 + neww;

		if (y > 198)
		{
			goto xx;
		}
	}

xx:;

}



u8_t BigMem3[0x1FFFF];
u8_t BigMem4[0x1FFFF];
u8_t BigMem5[0x1FFFF];


int ConvertPCXIntoRottSprite(u8_t  *RottSpriteMemptr, u8_t*srcdata, int lump, int lumplength, int origsize, int h, int w, int leftoffset, int topoffset)
{
	rottpatch_t *p, *org;
	int newspritelen;
	int widthalign, c;
	int h1;
	u8_t *tempbuffer, *ptr, *patchmem, *ptm, *old, *oldptr;
	unsigned int ynb, x, y, xcnt = 0, cnt = 0, picsize;//,picsize = 4096;
	unsigned int totalHeightCount, oldynb;
	unsigned char *tgt, *src;
	u16_t tabelptr;

	memset(BigMem3, BGbyte, sizeof(BigMem3));
	tempbuffer = BigMem3;//(u8_t *)GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT,lumplength+32767);



	//
//memset(tempbuffer,BGbyte,lumplength+32767);
	// remove all the DWORD-aligned u8_ts
	// and flip picture
	// calculate width DWORD-alignment in scanline
	widthalign = 0;
	c = (w / 4);
	if ((w - (c * 4)) > 0)
		widthalign = (4 - (w - (c * 4)));


	// remove all the DWORD-aligned u8_ts
	// and flip picture
	src = (unsigned char*)srcdata;
	tgt = tempbuffer;
	//	memcpy(tempbuffer,srcdata,lumplength);
	w += widthalign;
	memcpy(tgt, (src + (h*w) - w), w - widthalign);

	for (h1 = h - 1; h1 > 0; h1--)
	{
		memcpy(tgt, (src + (h1*w)), w - widthalign);
		tgt += w;
	}

	//memset(&pcxHDR,0,sizeof(PCX_HEADER));
	//tempbuffer = srcdata;//ReadExpandPCX(szFileName, &pcxHDR);
	if (tempbuffer <= 0)
		return 0;
	//picture is now in mem

	//create a patch_t to put data into
	picsize = (w + 1)*(h + 1);

	memset(BigMem4, 255, sizeof(BigMem4));
	patchmem = BigMem4;//(u16_t *)GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT,picsize+32767);

	//memset(patchmem,255,picsize+32767);
	if (patchmem <= 0)
		return 0;

	//patchmem = RottSpriteMemptr;
	p = (rottpatch_t*)patchmem;
	org = (rottpatch_t*)RottSpriteMemptr;
	ptr = tempbuffer;

	p->origsize = origsize;
	p->height = h;
	p->width = w;
	p->leftoffset = leftoffset;
	p->topoffset = topoffset;
	p->columnofs[0] = (sizeof(short) * 5) + (2 * p->width);//=2*sizeof char)
/*
	patchmem = RottSpriteMemptr;
	p = (patch_t*)patchmem;
	ptr = tempbuffer;
*/
	tabelptr = p->columnofs[0];
	for (x = 0; x < (u16_t)p->width; x++)
	{
		p->columnofs[x] = tabelptr;//=2*sizeof char)
		//ptr = tempbuffer+(x);
		oldptr = ptr;

		if (x == 10)
			x = x;
		//now count height of collum yofs,ynb
		totalHeightCount = 0; oldynb = 0;
		ptm = patchmem + p->columnofs[x];

		for (y = 0; y <= (u16_t)p->height; y++)
		{//(not visible)
			cnt = 0;
			//count nb not visible (1's)of pixels down to first pixels
			while (((*ptr) == BGbyte) && (cnt < (u16_t)p->height))
			{
				cnt++; ptr += p->width; totalHeightCount++;
			}

			//not 1 reached, count nb of them
			if (totalHeightCount >= (u16_t)p->height)
			{
				*(ptm) = 0xFF;
				break;
			}

			ynb = 0;
			old = ptm + 2;
			oldynb = totalHeightCount;

			while (((*ptr) != BGbyte) && (totalHeightCount < (u16_t)p->height))
			{
				(*old++) = (*ptr);
				ynb++; totalHeightCount++;
				ptr += p->width;
			}
			//write these 2 nbs (points to from tabel, who points to len and start)
			(*ptm++) = oldynb;
			(*ptm++) = ynb;

			//calculate next table pointer
			tabelptr += ynb + 2; //char cnt + ynb + 0xFF = 3 u8_ts
			ptm = old;
			//those 2 nbs found, begin copying data to target

		}
		tabelptr += 1; //char cnt + ynb + 0xFF = 3 u8_ts
		ptr = oldptr + 1;

	}

	//calc new sprite size
	newspritelen = (ptm - patchmem) + (sizeof(short) * 5) + 1;

	//adjust mem to our new size
	if (newspritelen > lumplength)
	{
		//Z_Realloc (&RottSpriteMemptr, newspritelen);
		//W_AdjustLumpsize(lump, newspritelen);
		//lumpcache[lump] = RottSpriteMemptr;
	}
	//copy new sprite to target mem
	//memset(RottSpriteMemptr,0xFF,newspritelen);
	memcpy(RottSpriteMemptr, patchmem, newspritelen);

	//	ZmemSize = Z_GetSize (RottSpriteMemptr);
	//	memcpy(RottSpriteMemptr,patchmem,ZmemSize);
	//	GlobalFree (patchmem);//release mem
	//	GlobalFree (tempbuffer);//release mem

	return newspritelen;

}


int ConvertPCXIntoRottTransSprite(u8_t  *RottSpriteMemptr, u8_t*srcdata, int lump, int lumplength, int origsize, int h, int w, int leftoffset, int topoffset, int translevel)
{
	transpatch_t *p, *org;
	int newspritelen;
	int widthalign, c;
	int h1;
	u8_t *tempbuffer, *ptr, *patchmem, *ptm, *old, *oldptr;
	unsigned int ynb, x, y, xcnt = 0, cnt = 0, picsize;//,picsize = 4096;
	unsigned int totalHeightCount, oldynb;
	unsigned char *tgt, *src;
	u16_t tabelptr;

	memset(BigMem3, BGbyte, sizeof(BigMem3));
	tempbuffer = BigMem3;//(u8_t *)GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT,lumplength+32767);
	// remove all the DWORD-aligned u8_ts
	// and flip picture
	// calculate width DWORD-alignment in scanline
	widthalign = 0;
	c = (w / 4);
	if ((w - (c * 4)) > 0)
		widthalign = (4 - (w - (c * 4)));


	// remove all the DWORD-aligned u8_ts
	// and flip picture
	src = (unsigned char*)srcdata;
	tgt = tempbuffer;
	//	memcpy(tempbuffer,srcdata,lumplength);
	w += widthalign;
	for (h1 = h - 1; h1 > 0; h1--)
	{
		memcpy(tgt, (src + (h1*w)), w - widthalign);
		tgt += w;
	}

	if (tempbuffer <= 0)
		return 0;

	//picture is now in mem
	//create a patch_t to put data into
	picsize = (w + 1)*(h + 1);

	memset(BigMem4, 255, sizeof(BigMem4));
	patchmem = BigMem4;//(u16_t *)GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT,picsize+32767);

	if (patchmem <= 0)
		return 0;

	//patchmem = RottSpriteMemptr;
	//EDGE: ERRORS:
	p = (rottpatch_t*)patchmem; //rottpatch_t *" cannot be assigned to an entity of type "transpatch_t *"
	org = (rottpatch_t*)RottSpriteMemptr; //rottpatch_t *" cannot be assigned to an entity of type "transpatch_t *"
	ptr = tempbuffer;

	p->origsize = origsize;
	p->height = h;
	p->width = w;
	p->leftoffset = leftoffset;
	p->topoffset = topoffset;
	p->translevel = translevel;
	p->columnofs[0] = (sizeof(short) * 6) + (2 * p->width);//=2*sizeof char)

	tabelptr = p->columnofs[0];

	for (x = 0; x < (u16_t)p->width; x++)
	{
		p->columnofs[x] = tabelptr;//=2*sizeof char)
		oldptr = ptr;
		//now count height of collum yofs,ynb
		totalHeightCount = 0;
		oldynb = 0;
		ptm = patchmem + p->columnofs[x];

		for (y = 0; y <= (u16_t)p->height; y++)
		{//(not visible)
			cnt = 0;

			//count nb not visible (1's)of pixels down to first pixels
			while (((*ptr) == BGbyte) && (cnt < (u16_t)p->height))
			{
				cnt++; ptr += p->width; totalHeightCount++;
			}

			//not 1 reached, count nb of them
			if (totalHeightCount >= (u16_t)p->height)
			{
				*(ptm) = 0xFF;
				break;
			}

			ynb = 0;
			old = ptm + 2;
			oldynb = totalHeightCount;

			if (*ptr == TRANSbyte)
			{
				cnt = 0;
				while (((*ptr) == TRANSbyte) && (totalHeightCount < (u16_t)p->height))
				{
					//(*old++) = (*ptr);
					ynb++; totalHeightCount++;
					ptr += p->width;
				}
				//write these 2 nbs (points to from tabel, who points to len and start)
				(*ptm++) = oldynb;//ofs
				(*ptm++) = ynb;//rlen	
				(*old++) = 254;//flag	
				ynb = 1;
			}
			else
			{
				while (((*ptr) != TRANSbyte) && ((*ptr) != BGbyte) && (totalHeightCount < (u16_t)p->height))
				{
					(*old++) = (*ptr);
					ynb++; totalHeightCount++;
					ptr += p->width;
				}
				//write these 2 nbs (points to from tabel, who points to len and start)
				(*ptm++) = oldynb;//ofs
				(*ptm++) = ynb;//rlen
			}

			//calculate next table pointer
			tabelptr += ynb + 2; //char cnt + ynb + 0xFF = 3 u8_ts
			ptm = old;
			//those 2 nbs found, begin copying data to target

		}
		tabelptr += 1; //char cnt + ynb + 0xFF = 3 u8_ts
		ptr = oldptr + 1;

	}

	//calc new sprite size
	newspritelen = (ptm - patchmem) + (sizeof(short) * 6) + 1;

	//copy new sprite to target mem
	memcpy(RottSpriteMemptr, patchmem, newspritelen);

	return newspritelen;

}

// EDGE: Cannot convert "HGLOBAL" to type byte (?)
void RotatePicture(u8_t *src, int iWidth, int  iHeight)
{
	u8_t *mem, *tmpsrc, *m;
	int w, h;

	tmpsrc = src;
	mem = GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, (iWidth*iHeight) + 4000); //"HGLOBAL" cannot be assigned to an entity of type "u8_t *"
	m = mem;
	for (w = 0; w < iWidth; w++)
	{
		tmpsrc = src + w;
		for (h = 0; h < iHeight; h++)
		{
			*(m++) = *(tmpsrc + (h*iWidth));
		}
	}

	memcpy(src, mem, iWidth*iHeight);

	GlobalFree(mem);//release mem
}

/*
	//create a patch_t to put data into
	picsize = (w+1)*(h+1);
	patchmem =  (u16_t *)GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT,picsize+26000);
memset(patchmem,0xFF,picsize+26000);
	if (patchmem <= 0)
		return 0;

	//patchmem = RottSpriteMemptr;
	p = (patch_t*)patchmem;
	org = (patch_t*)RottSpriteMemptr;
	ptr = tempbuffer;

	src = tempbuffer;

	p->origsize = origsize;
	p->height = h;
	p->width  = w;
	p->leftoffset  = leftoffset;
	p->topoffset  = topoffset;
	p->columnofs[0] = (sizeof(short)*5)+(2*p->width );//=2*sizeof char)


	tabelptr = p->columnofs[0];
	startofdata = patchmem + (p->width*2) + (sizeof(short)*5) ;

	for (x=0;x<(u16_t)p->width;x++){
		bgtrans = CntNbOfBGbyteu8_ts((src+x),p->width,p->height);

		rlen  = CntNbOfNotBGbyteu8_ts((src+x+(bgtrans*p->width)),p->width,p->height);

		p->columnofs[x] = startofdata-patchmem;

		*startofdata++ = bgtrans;
		*startofdata++ = rlen;

		for (i=0;i<rlen;i++)
			*startofdata++ = *(src+x+(bgtrans*p->width)+(i*p->width));
		*startofdata++ = 0xFF;
		//*startofdata++ = 0x00;
	}
	*startofdata++ = 0xFF;
	*startofdata++ = 0xFF;
	*startofdata++ = 0xFF;
	newspritelen = (startofdata - patchmem);
	memcpy(RottSpriteMemptr,patchmem,newspritelen);


//ConvertRottSpriteIntoPCX (RottSpriteMemptr, tempbuffer);

	GlobalFree (patchmem);//release mem
	GlobalFree (tempbuffer);//release mem
	return  newspritelen;
*/


// EDGE: HGLOBAL CANNOT BE ASSIGNED TO type u8_t*
int ConvertPCXIntoRott_pic_t(u8_t *RottSpriteMemptr, u8_t*srcdata, int lump, int lumplength, int h, int w)
{
	int widthalign, c, h1, h2, wx1 = w, hx1 = h;
	unsigned char *tgt, *src, *mem;
	pic_t *Win1;	byte test[1000];
	mem = GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, (w*h) + 4000); //a value of type "HGLOBAL" cannot be assigned to an entity of type "unsigned char *"

	memset(mem, 45, 4000);

	*RottSpriteMemptr = (u8_t)w;
	*(RottSpriteMemptr + 1) = (u8_t)h;

	widthalign = 0;
	c = (w / 4);
	if ((w - (c * 4)) > 0)
		widthalign = (4 - (w - (c * 4)));
	// remove all the DWORD-aligned u8_ts
	// and flip picture
	src = (unsigned char*)srcdata;
	tgt = mem;
	memcpy(test, srcdata, 1000);
	w += widthalign;
	for (h1 = h - 1; h1 >= 0; h1--)
	{
		memcpy(tgt, (src + (h1*w)), w - widthalign);
		tgt += w;
	}

	memcpy(test, mem, 1000);
	src = mem;
	tgt = RottSpriteMemptr + 2;//we reuse this src ptr

	for (h2 = 0; h2 < hx1; h2++)
	{
		for (h1 = 0; h1 < wx1; h1++)
		{
			*tgt++ = *(src + h1 + (h2*w));;
		}
	}

	Win1 = (pic_t*)RottSpriteMemptr;

	memcpy(test, RottSpriteMemptr, 1000);

	return (w*h) + 12;
}


int Convert24bitBmpTo8bitExt(u8_t * tgt, HANDLE hImage)
{
	DIBSECTION ds;
	int i, j, nPad;
	u8_t* pbBits;
	u16_t* pwBits;
	DWORD* pdwBits;
	DWORD rmask, gmask, bmask;
	int rright, gright, bright;
	int rleft, gleft, bleft;
	u8_t c, r, g, b;
	u16_t wColor;
	DWORD dwColor;

	if (ispalloaded == 0)
		loadconvpal();

	// Scan the DIB and build the octree
	GetObject(hImage, sizeof(ds), &ds);
	nPad = ds.dsBm.bmWidthBytes - (((ds.dsBmih.biWidth *  ds.dsBmih.biBitCount) + 7) / 8);

	switch (ds.dsBmih.biBitCount)
	{

	case 16: // One case for 16-bit DIBs
		if (ds.dsBmih.biCompression == BI_BITFIELDS)
		{
			rmask = ds.dsBitfields[0];
			gmask = ds.dsBitfields[1];
			bmask = ds.dsBitfields[2];
		}
		else
		{
			rmask = 0x7C00;
			gmask = 0x03E0;
			bmask = 0x001F;
		}

		rright = GetRightShiftCount(rmask);
		gright = GetRightShiftCount(gmask);
		bright = GetRightShiftCount(bmask);

		rleft = GetLeftShiftCount(rmask);
		gleft = GetLeftShiftCount(gmask);
		bleft = GetLeftShiftCount(bmask);

		pwBits = (u16_t*)ds.dsBm.bmBits;

		for (i = 0; i < ds.dsBmih.biHeight; i++)
		{
			for (j = 0; j < ds.dsBmih.biWidth; j++)
			{
				wColor = *pwBits++;
				b = (u8_t)(((wColor & (u16_t)bmask) >> bright) << bleft);
				g = (u8_t)(((wColor & (u16_t)gmask) >> gright) << gleft);
				r = (u8_t)(((wColor & (u16_t)rmask) >> rright) << rleft);
				c = makecol(r, g, b);
				*tgt++ = c;
			}
			pwBits = (u16_t*)(((u8_t*)pwBits) + nPad);
		}
		break;

	case 24: // Another for 24-bit DIBs
		pbBits = (u8_t*)ds.dsBm.bmBits;

		for (i = 0; i < ds.dsBmih.biHeight; i++)
		{
			for (j = 0; j < ds.dsBmih.biWidth; j++)
			{
				b = *pbBits++;
				g = *pbBits++;
				r = *pbBits++;
				c = makecol(r, g, b);
				*tgt++ = c;
			}
			pbBits += nPad;
		}
		break;

	case 32: // And another for 32-bit DIBs
		if (ds.dsBmih.biCompression == BI_BITFIELDS)
		{
			rmask = ds.dsBitfields[0];
			gmask = ds.dsBitfields[1];
			bmask = ds.dsBitfields[2];
		}
		else
		{
			rmask = 0x00FF0000;
			gmask = 0x0000FF00;
			bmask = 0x000000FF;
		}

		rright = GetRightShiftCount(rmask);
		gright = GetRightShiftCount(gmask);
		bright = GetRightShiftCount(bmask);

		pdwBits = (DWORD*)ds.dsBm.bmBits;

		for (i = 0; i < ds.dsBmih.biHeight; i++)
		{
			for (j = 0; j < ds.dsBmih.biWidth; j++)
			{
				dwColor = *pdwBits++;
				b = (u8_t)((dwColor & bmask) >> bright);
				g = (u8_t)((dwColor & gmask) >> gright);
				r = (u8_t)((dwColor & rmask) >> rright);
				c = makecol(r, g, b);
				*tgt++ = c;
			}
			pdwBits = (DWORD*)(((u8_t*)pdwBits) + nPad);
		}
		break;

	default: // DIB must be 16, 24, or 32-bit!
		return 0;
	}

	return 0;
}


int GetLeftShiftCount(DWORD dwVal)
{
	int i, nCount = 0;
	for (i = 0; i < sizeof(DWORD) * 8; i++)
	{
		if (dwVal & 1)
			nCount++;
		dwVal >>= 1;
	}

	return (8 - nCount);
}


int GetRightShiftCount(DWORD dwVal)
{
	int i;

	for (i = 0; i < sizeof(DWORD) * 8; i++)
	{
		if (dwVal & 1)
			return i;
		dwVal >>= 1;
	}
	return -1;
}



void ConvertPicToPPic(int width, int height, u8_t *source, u8_t *target)
{
	u8_t *src, *tgt, pixel;
	int x, y, planes;

	tgt = target;
	for (planes = 0; planes < 4; planes++)
	{
		src = source;
		src += planes;

		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width / 4; x++)
			{
				pixel = *src;
				*tgt++ = pixel;
				src += 4;
			}
		}
	}


}



void FlipPictureUpsideDown(int width, int height, u8_t *src)
{
	int w, h;
	u8_t c, *b, *t, *bot, *top;

	//flips mem without use of any extra mem

	bot = src;
	top = src + (width*height) - width;

	for (h = 0; h < height / 2; h++)
	{
		//flip button line and top line
		b = bot;
		t = top;

		for (w = 0; w < width; w++)
		{
			c = *b;
			*b++ = *t;
			*t++ = c;
		}

		bot += width;
		top -= width;
	}
	/*
	src = (unsigned char*)bm.bmBits;
	tgt = BigMem1+(bm.bmWidth*bm.bmHeight)-bm.bmWidth;
	//memcpy(tgt ,(src+(bm.bmHeight*bm.bmWidth)),bm.bmWidth);
	for (i = 0; i < bm.bmHeight ; i++ ){
		memcpy(tgt ,src, bm.bmWidth);
		tgt -= bm.bmWidth;
		src += bm.bmWidth;
	}*/
}


// This should be replaced with the routines in r_playpal.cc for ROTT:

void loadconvpal()
{
	int i, j = 0;
	for (i = 0; i < 256; ++i, j += 3)
	{
		colrs[i].red = (unsigned char)origpal[j];//colrs[i].red;
		colrs[i].green = (unsigned char)origpal[j + 1];//colrs[i].green;
		colrs[i].blue = (unsigned char)origpal[j + 2];//colrs[i].blue;
		colrs[i].red = gammatable[(gammaindex << 6) + (colrs[i].red)] << 2;
		colrs[i].green = gammatable[(gammaindex << 6) + (colrs[i].green)] << 2;
		colrs[i].blue = gammatable[(gammaindex << 6) + (colrs[i].blue)] << 2;
	}

	ispalloaded = 1;
}

unsigned char makecol(u8_t r, u8_t g, u8_t b)
{
	int i, range;
	int r1 = r, b1 = b, g1 = g;
	//this func finds the closest apprx in rott pal table
	for (range = 0; range < 256; range++)
	{
		for (i = 0; i < 256; i++)
		{
			if (((int)colrs[i].red >= (r1 - range)) && ((int)colrs[i].red <= (r1 + range)))
			{
				if (((int)colrs[i].green >= g1 - range) && ((int)colrs[i].green <= g1 + range))
				{
					if (((int)colrs[i].blue >= b1 - range) && ((int)colrs[i].blue <= b1 + range))
					{
						return (unsigned char)i;
					}
				}
			}

			/*			if (((colrs[i].red >= r1-range)&&(colrs[i].red <= r1+range))&&
							((colrs[i].green >= g1-range)&&(colrs[i].green <= g1+range))&&
							((colrs[i].blue >= b1-range)&&(colrs[i].blue <= b1+range)))
							return (unsigned char )i;*/
		}
	}

	return 0;
}


void ConvertPPicToPic(int width, int height, u8_t *src, u8_t *tgt)
{
	u8_t *dest, pixel;
	int x, y, planes;

	for (planes = 0; planes < 4; planes++)
	{
		dest = tgt;
		dest += planes;

		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++)
			{
				pixel = *src++;
				*(dest) = pixel;
				dest += 4;
			}
		}
	}


}




#endif // 0
