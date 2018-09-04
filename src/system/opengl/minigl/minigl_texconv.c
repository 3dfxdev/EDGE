#include "minigl.h"
#include "minigl_tex.h"
#include <math.h>

typedef struct { unsigned char r,g,b; } DCGLrgb888;

#if 1
	typedef struct { unsigned short b:5, g:6, r:5;      } DCGLrgb565;
	typedef struct { unsigned short b:5, g:6, r:5, a:1; } DCGLargb1555;
	typedef struct { unsigned short b:4, g:4, r:4, a:4; } DCGLargb4444;
#else
	typedef struct { unsigned short      r:5, g:6, b:5; } DCGLrgb565;
	typedef struct { unsigned short a:1, r:5, g:6, b:5; } DCGLargb1555;
	typedef struct { unsigned short a:4, r:4, g:4, b:4; } DCGLargb4444;
#endif

DCGLargb8888 dcglGetPixelR8 (const unsigned char * pixel) {
	DCGLargb8888 pix = {255, pixel[0], 0, 0};
	return pix;
}
DCGLargb8888 dcglGetPixelRG88 (const unsigned char * pixel) {
	DCGLargb8888 pix = {255, pixel[0], pixel[1], 0};
	return pix;
}
DCGLargb8888 dcglGetPixelRGB888 (const unsigned char * pixel) {
	DCGLargb8888 pix = {255, pixel[0], pixel[1], pixel[2]};
	return pix;
}
DCGLargb8888 dcglGetPixelARGB8888 (const unsigned char * pixel) {
	DCGLargb8888 pix = {pixel[0], pixel[1], pixel[2], pixel[3]};
	return pix;
}

DCGLargb8888 dcglGetPixelRGBA8888 (const unsigned char * pixel) {
	DCGLargb8888 pix = {pixel[3], pixel[0], pixel[1], pixel[2]};
	return pix;
}

DCGLargb8888 dcglGetPixelBGR888 (const unsigned char * pixel) {
	DCGLargb8888 pix = {255, pixel[2], pixel[1], pixel[0]};
	return pix;
}
DCGLargb8888 dcglGetPixelBGRA888 (const unsigned char * pixel) {
	DCGLargb8888 pix = {pixel[3], pixel[2], pixel[1], pixel[0]};
	return pix;
}

static inline unsigned int ScaleWithRound(unsigned int value, unsigned int srcwidth, unsigned int dstwidth) {
	if (1) {
		return value >> (srcwidth-dstwidth);
	} else {
		//int roundup = (1 << (srcwidth-dstwidth)) >> 1;
		//int roundup = (1 << (dstwidth)) >> 1;
		int roundup = (1 << (dstwidth)) >> 2;
		int srcmax = (1<<srcwidth)-1;
		//int roundup = 0;
		value += roundup;
		if (value > srcmax)
			value = srcmax;
		return value >> (srcwidth-dstwidth);
	}
}
//static inline unsigned int Scale(unsigned int value, unsigned int srcwidth, unsigned int dstwidth) { return value >> (srcwidth-dstwidth); }
#define SWR8(value, dstwidth)  ScaleWithRound(value, 8, dstwidth)

void dcglStoreTexelRGB565(DCGLargb8888 src, unsigned short * dst) {
	union { DCGLrgb565 srccolor; unsigned short raw; } conv
		= { {.r=SWR8(src.r,5), .g=SWR8(src.g,6), .b=SWR8(src.b,5)} };
	*dst = conv.raw;
}

void dcglStoreTexelARGB1555(DCGLargb8888 src, unsigned short * dst) {
	union { DCGLargb1555 srccolor; unsigned short raw; } conv
		= { {.a=SWR8(src.a,1), .r=SWR8(src.r,5), .g=SWR8(src.g,5), .b=SWR8(src.b,5)} };
	*dst = conv.raw;
}

void dcglStoreTexelARGB4444(DCGLargb8888 src, unsigned short * dst) {
	union { DCGLargb4444 srccolor; unsigned short raw; } conv
		= { {.a=SWR8(src.a,4), .r=SWR8(src.r,4), .g=SWR8(src.g,4), .b=SWR8(src.b,4)} };
	*dst = conv.raw;
}

//returns 0 on success and 1 on error
int dcglGetTextureInput(GLint format, GLint type, DCGLtextureinput *dst) {
	static const struct {
		GLint format;
		GLint type;
		int size;
		dcglGetPixel cb;
	} table[] = {
		{GL_RGB,  GL_UNSIGNED_BYTE, 3, dcglGetPixelRGB888},
		{GL_RGBA, GL_UNSIGNED_BYTE, 4, dcglGetPixelRGBA8888},
		{GL_BGR,  GL_UNSIGNED_BYTE, 3, dcglGetPixelBGR888},
		{GL_BGRA, GL_UNSIGNED_BYTE, 4, dcglGetPixelBGRA888},
		{GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, 4, dcglGetPixelARGB8888},
	};
	
	dcglHardAssert(dst);
	int i, tablesize = sizeof(table) / sizeof(table[0]);
	for(i = 0; i < tablesize; i++) {
		if (table[i].format == format && table[i].type == type) {
			//dst->format = format;
			//dst->type = type;
			dst->size = table[i].size;
			dst->getp = table[i].cb;
			return 0;
		}
	}
	dcglLog("unsupported format/type combination. format: %i. type: %i", format, type);
	dcglHardAssert(0);
	return 1;
}

static dcglStoreTexel dcglInternalStore(pc_texture_format format) {
	switch(format) {
	case PC_RGB565:		return dcglStoreTexelRGB565;
	case PC_ARGB1555:	return dcglStoreTexelARGB1555;
	case PC_ARGB4444:	return dcglStoreTexelARGB4444;
	default: ;
	};
	dcglHardAssert(0);
	return NULL;
}

int dcglGetTextureOutput(GLint internalformat, DCGLtextureoutput *dst) {
	static const struct {
		GLint glintform;
		pc_texture_format dcglintform;
	} table[] = {
/*		{GL_ALPHA4,          DCGL_ARGB4444},
		{GL_ALPHA8,          DCGL_ARGB4444},
		{GL_ALPHA12,         DCGL_ARGB4444},
		{GL_ALPHA16,         DCGL_ARGB4444},
		
		{GL_LUMINANCE4,      DCGL_ARGB4444},
		{GL_LUMINANCE8,      DCGL_ARGB1555},
		
		{GL_R3_G3_B2,        DCGL_RGB565},
		{GL_RGB4,            DCGL_ARGB4444},
		{GL_RGB5,            DCGL_ARGB1555},
		{GL_RGB8,            DCGL_RGB565},
		{GL_RGB10,           DCGL_RGB565},
		{GL_RGB12,           DCGL_RGB565},
		{GL_RGB16,           DCGL_RGB565},
		{GL_RGBA4,           DCGL_ARGB4444},
		{GL_RGB5_A1,         DCGL_ARGB1555}, */
		
		{GL_ALPHA,           PC_ARGB4444},
		{GL_RGB,             PC_RGB565},
		{GL_RGBA,            PC_ARGB4444},
		{GL_LUMINANCE,       PC_ARGB1555},
		
//		{DCGL_YUV422,        DCGL_YUV422},
	};
	
	int i, tablesize = sizeof(table) / sizeof(table[0]);
	for(i = 0; i < tablesize; i++) {
		if (table[i].glintform == internalformat) {
			//dst->pvrformat = table[i].dcglintform;
			dst->pvrf.format = table[i].dcglintform;
			dst->size = 2;
			dst->putp = dcglInternalStore(dst->pvrf.format);
			return 0;
		}
	}
	
	dcglLog("unsupported internal format: %i", internalformat);
	dcglHardAssert(0);
	return 1;
}

