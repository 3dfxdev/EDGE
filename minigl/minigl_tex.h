#ifndef MINIGL_TEX_H
#define MINIGL_TEX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "minigl.h"
#include "pvr_cxt.h"

#define DCGL_MIN_TEXTURE_SIZE (8)
#define DCGL_MAX_TEXTURE_SIZE (1024)
#define DCGL_MAX_COMBINED_TEXTURE_IMAGE_UNITS 1

typedef struct {
	unsigned int pcw : 3;
	unsigned int mipmap : 1;
	unsigned int vq : 1;
	unsigned int format : 3;
	unsigned int pch : 3;
	unsigned int twid : 1;
	unsigned int cbsize : 12;
} DCGLpvrformat2;

typedef struct {
	unsigned char a,r,g,b;
} DCGLargb8888;

typedef DCGLargb8888 (*dcglGetPixel) (const unsigned char * pixel);
typedef void (*dcglStoreTexel) (const DCGLargb8888 src, unsigned short * dst);

typedef struct {
	int size;	//in bytes
	dcglGetPixel getp;
	//GLint format;
	//GLint type;	
} DCGLtextureinput;

typedef struct {
	int size;	//of texel in bytes
	dcglStoreTexel putp;
	DCGLpvrformat2 pvrf;
} DCGLtextureoutput;

#define DCGL_TEXFLAG_ALLOCATED	(0x0001)	/* is texture struct has been allocated */
//#define DCGL_TEXFLAG_MIPMAPPED	(0x0002)	/* has space allocated for mipmaps, but might not be complete */
#define DCGL_TEXFLAG_AUTOMIPMAP	(0x0004)	/* will autogen mipmaps when level 0 is loaded */
#define DCGL_TEXFLAG_PVRDIRTY	(0x0008)	/* pvr context data needs to be regenerated */

typedef struct {
	short flags;
	short levels;	//bitmask indicating which mipmap levels are complete, lowest bit is level 0, second bit is level 1, etc
	short w,h;	//size of texture in texels
	unsigned short *base;	//pointer used to access texture
	//unsigned short *alloc;	//pointer used for freeing texture
	int alloc_size;
	DCGLtextureoutput out;
	GLenum swrap,twrap;
	GLenum minfilter, magfilter;
	pvr_context pvrdata;
} DCGLtexture;

typedef struct {
	int enabled;
	GLuint id;
	DCGLtexture *tex;
} DCGLtexturetarget;

#define DCGL_MAX_TEXTURE_TARGETS (3)
typedef struct {
	GLenum mode;
	float lodbias;
	pc_mipmap_bias pclodbias;
	DCGLtexturetarget targ[DCGL_MAX_TEXTURE_TARGETS];
} DCGLtexunit;

void dcglInitTextures();

void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid * data);
void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data);

void glBindTexture(GLenum target, GLuint texture);

void glGenTextures(GLsizei n, GLuint *textures);
void glDeleteTextures(GLsizei n, const GLuint *textureNames);
GLboolean glIsTexture(GLuint texture);

void glTexEnvi(GLenum target, GLenum pname, GLint param);
void glTexParameteri(GLenum target, GLenum pname, GLint param);
void glTexEnvf(GLenum target, GLenum pname, GLfloat param);
void glTexEnviv(GLenum target, GLenum pname, const GLint *params);

void glGenerateMipmap(GLenum target);

//from minigl_texconv
int dcglGetTextureOutput(GLint internalformat, DCGLtextureoutput *dst);
int dcglGetTextureInput(GLint format, GLint type, DCGLtextureinput *dst);

#ifdef __cplusplus
}
#endif


#endif
