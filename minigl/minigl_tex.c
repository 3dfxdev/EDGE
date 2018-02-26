#include "minigl.h"
#include "minigl_tex.h"
#include <math.h>

/*
	TODO notes
	if a texture has a minifing filter that requires mipmaps, treat texture as invalid?
*/
static void dcglInitTexture(DCGLtexture *tex);
static int dcglResizeTexture(DCGLtexture *tex, GLsizei width, GLsizei height, int mipmaps);

#include "morton.h"

//	TEXTURE FLAGS
static void dcglSetTextureHasMipmaps(DCGLtexture *tex, int hasmip) {
	tex->out.pvrf.mipmap = hasmip != 0;
}
static int dcglTextureHasMipmaps(const DCGLtexture *tex) {
	return tex->out.pvrf.mipmap;
}
static void dcglSetTextureAutoMipmap(DCGLtexture *tex, int automip) {
	if (automip)
		tex->flags |= DCGL_TEXFLAG_AUTOMIPMAP;
	else
		tex->flags &= ~DCGL_TEXFLAG_AUTOMIPMAP;
}
static int dcglTextureHasAutoMipmap(const DCGLtexture *tex) {
	return (tex->flags & DCGL_TEXFLAG_AUTOMIPMAP) != 0;
}
static void dcglSetTextureAllocated(DCGLtexture *tex, int allocated) {
	if (allocated)
		tex->flags |= DCGL_TEXFLAG_ALLOCATED;
	else
		tex->flags &= ~DCGL_TEXFLAG_ALLOCATED;
}
static int dcglTextureIsAllocated(const DCGLtexture *tex) {
	return (tex->flags & DCGL_TEXFLAG_ALLOCATED) != 0;
}
static void dcglTextureSetPVRDirty(DCGLtexture *tex, int dirty) {
	if (dirty)
		tex->flags |= DCGL_TEXFLAG_PVRDIRTY;
	else
		tex->flags &= ~DCGL_TEXFLAG_PVRDIRTY;
}
static int dcglTextureIsPVRDirty(const DCGLtexture *tex) {
	return (tex->flags & DCGL_TEXFLAG_PVRDIRTY) != 0;
}

static int dcglTextureCompressed(const DCGLtexture *tex) {
	return tex->out.pvrf.vq;
}
static void dcglTextureSetPCWidth(DCGLtexture *tex, pc_texture_size w) {
	tex->out.pvrf.pcw = w;
}
static pc_texture_size dcglTexturePCWidth(const DCGLtexture *tex) {
	return tex->out.pvrf.pcw;
}
static void dcglTextureSetWidth(DCGLtexture *tex, GLsizei w) {
	tex->w = w;
	dcglTextureSetPCWidth(tex, pc_convert_size(w));
}
static GLsizei dcglTextureWidth(const DCGLtexture *tex) {
	return tex->w;
}
static void dcglTextureSetPCHeight(DCGLtexture *tex, pc_texture_size h) {
	tex->out.pvrf.pch = h;
}
static void dcglTextureSetHeight(DCGLtexture *tex, GLsizei h) {
	tex->h = h;
	dcglTextureSetPCHeight(tex, pc_convert_size(h));
}
static pc_texture_size dcglTexturePCHeight(const DCGLtexture *tex) {
	return tex->out.pvrf.pch;
}
static GLsizei dcglTextureHeight(const DCGLtexture *tex) {
	return tex->h;
}
static int dcglTextureSquare(const DCGLtexture *tex) {
	return dcglTextureHeight(tex) == dcglTextureWidth(tex);
}
static void dcglSetTextureFormat(DCGLtexture *tex, GLint internalformat) {
	//tex->format = dcglTextureInternalFormat(internalformat);
	//tex->dstwrite = dcglInternalStore(tex->format);
	dcglGetTextureOutput(internalformat, &tex->out);
}

typedef struct {
	int current_tmu_idx;
	DCGLtexunit *current_tmu;
	DCGLtexturetarget *current_tmu_targets;
	DCGLtexunit tmus[DCGL_MAX_COMBINED_TEXTURE_IMAGE_UNITS];
} dcgl_tmu_state;
dcgl_tmu_state dcgl_tmu = {0, dcgl_tmu.tmus, dcgl_tmu.tmus->targ, { {GL_MODULATE,0,PC_MIPMAP_BIAS_1_00} } };

static inline void dcglSetCurTMU(int idx) {
	dcgl_tmu.current_tmu_idx = idx;
	dcgl_tmu.current_tmu = dcgl_tmu.tmus + idx;
}
static inline DCGLtexunit * dcglGetTMU(int idx) {
	return dcgl_tmu.tmus + idx;
}
static inline DCGLtexunit * dcglCurTMU() {
	return dcgl_tmu.current_tmu;
}
static inline DCGLtexturetarget * dcglCurTexTargs() {
	return dcgl_tmu.current_tmu_targets;
}
static inline DCGLtexturetarget * dcglCurTex2D() {
	return dcglCurTexTargs() + 1;
}
static inline DCGLtexturetarget * dcglCurTex1D() {
	return dcglCurTexTargs() + 0;
}
static inline int dcglTargetIdx(GLenum target) {
	if (target == GL_TEXTURE_1D)
		return 0;
	else if (target == GL_TEXTURE_2D)
		return 1;
	else
		return 2;
	
}
static inline DCGLtexturetarget * dcglGetTarget(GLenum target) {
	return dcglCurTexTargs() + dcglTargetIdx(target);
}
DCGLtexture * dcglGetTargetTexture(GLenum target) {
	return dcglGetTarget(target)->tex;
}
int dcglGetTargetTextureId(GLenum target) {
	return dcglGetTarget(target)->id;
}
int dcglGetTopTargetIdx() {
	if (dcgl_tmu.current_tmu_targets[1].enabled)
		return 1;
	else if (dcgl_tmu.current_tmu_targets[0].enabled)
		return 0;
	else
		return -1;
}
void dcglEnableTex(GLenum name) {
	dcglHardAssert(name == GL_TEXTURE_1D || name == GL_TEXTURE_2D);
	dcglGetTarget(name)->enabled = 1;
}
void dcglDisableTex(GLenum name) {
	dcglHardAssert(name == GL_TEXTURE_1D || name == GL_TEXTURE_2D);
	dcglGetTarget(name)->enabled = 0;
}
GLboolean dcglGetTexEnabled(GLenum name) {
	dcglHardAssert(name == GL_TEXTURE_1D || name == GL_TEXTURE_2D);
	return dcglGetTarget(name)->enabled;
}
void glActiveTexture(GLenum texture) {
	dcglUnimpAssert(0);

	dcglNotInBegin(return);
	dcglRequire(GL_TEXTURE0 <= texture  && texture < (GL_TEXTURE0 + DCGL_MAX_COMBINED_TEXTURE_IMAGE_UNITS), GL_INVALID_ENUM, return);
	dcglSetCurTMU(texture - GL_TEXTURE0);
}
void dcglApplyTMU(int tmu_idx) {
	switch(dcglGetTMU(tmu_idx)->mode) {
	case GL_MODULATE: pc_set_texenv(pc_no_mod(&dcgl_current_context), PC_TEXENV_MODULATE); break;
	//case GL_BLEND:    pc_set_texenv(pc_no_mod(&dcgl_current_context), PC_TEXENV_BLEND);    break;
	case GL_DECAL:    pc_set_texenv(pc_no_mod(&dcgl_current_context), PC_TEXENV_DECAL);    break;
	case GL_REPLACE:  pc_set_texenv(pc_no_mod(&dcgl_current_context), PC_TEXENV_REPLACE);  break;
	default: dcglHardAssert(0);
	}
	pc_set_mipmap_bias(pc_no_mod(&dcgl_current_context), dcglGetTMU(tmu_idx)->pclodbias);
	dcglSetPVRStateDirty(1);
}


void dcglSubTexImage2D(const void *src, DCGLtextureinput *srcread, int level, int x, int y, int w, int h, DCGLtexture *dsttex);
void dcglApplyTexture();

typedef struct {
	GLuint idx;
	DCGLtexture *tex;
} DCGLnamemap;

#define DCGL_MAX_TEXTURES (200)
#define DCGL_MAX_TEXTURE_NAMES (1000)
DCGLtexture dcgl_default_texture;
DCGLtexture dcgl_textures[DCGL_MAX_TEXTURES];
short dcgl_unused_textures[DCGL_MAX_TEXTURES];
short dcgl_unused_count;
DCGLnamemap dcgl_namemap[DCGL_MAX_TEXTURE_NAMES];	//entires with tex == NULL are unused

void dcglInitTextures() {
	//TODO at the moment, this should only be called once at startup, since it won't free existing textures if called
	assert(DCGL_MAX_TEXTURES < 0x7fff);	//make sure texture count doesn't overflow pool ref size
	int i;
	for(i = 0; i < DCGL_MAX_TEXTURES; i++) {
		dcgl_unused_textures[i] = DCGL_MAX_TEXTURES-i-1;
	}
	dcgl_unused_count = DCGL_MAX_TEXTURES;
	memset(dcgl_namemap, 0, sizeof(dcgl_namemap));
	
	dcglLog("initing default tex...");
	dcglInitTexture(&dcgl_default_texture);
	
	dcglLog("initing 2d tex...");
	dcglCurTex2D()->enabled = 1;
	glBindTexture(GL_TEXTURE_2D, 0);
	dcglCurTex2D()->enabled = 0;

	dcglLog("initing 1d tex...");
	dcglCurTex1D()->enabled = 1;
	glBindTexture(GL_TEXTURE_1D, 0);
	dcglCurTex1D()->enabled = 0;
}

static DCGLtexture * dcglAllocTextureInfo() {
	dcglHardAssert(dcgl_unused_count > 0);
	DCGLtexture *potential =  dcgl_textures + dcgl_unused_textures[dcgl_unused_count-1];
	//dcglLog("allocing texture %p", potential);
	dcglHardAssert(!dcglTextureIsAllocated(potential));
	dcglSetTextureAllocated(potential,1);
	--dcgl_unused_count;
	return potential;
}

static void dcglFreeTextureInfo(DCGLtexture *texture)
{
	ptrdiff_t idx = texture - dcgl_textures;
	dcglHardAssert(idx >= 0);
	dcglHardAssert(idx < DCGL_MAX_TEXTURES);
	
	dcglHardAssert(texture != &dcgl_default_texture);
	dcglHardAssert(dcglTextureIsAllocated(texture));
	
	//check that it's not already freed
	int i,j;
	for(i=0;i<dcgl_unused_count;i++) {
		assert(dcgl_unused_textures[i] != idx);
	}
	
	//unbind texture
	int any_unbound = 0;
	for(j = 0;j < DCGL_MAX_COMBINED_TEXTURE_IMAGE_UNITS; j++) {
		for(i = 0;i < DCGL_MAX_TEXTURE_TARGETS; i++) {
			if (dcgl_tmu.tmus[j].targ[i].tex == texture) {
				dcgl_tmu.tmus[j].targ[i].id = 0;
				dcgl_tmu.tmus[j].targ[i].tex = &dcgl_default_texture;
				any_unbound = 1;
			}
		}
	}
	
	if (any_unbound) {
		dcglApplyTexture();
		dcglApplyTMU(0);
	}
	
	//add to unused list
	dcgl_unused_textures[dcgl_unused_count++] = idx;
	dcglSetTextureAllocated(texture,0);
}

static int dcglMipmappable(pc_texture_size width, pc_texture_size height) {
	return width == height;
}

static int dcglMipmapOffset(DCGLpvrformat2 texformat, GLsizei level) {
				 //      0   1    2     3      4      5      6       7       8       9        10
				 //      1   2    4     8     16     32     64     128     256     512      1024
	const static int uncomp16[] = {0x6, 0x8, 0x10, 0x30, 0xB0, 0x2B0, 0xAB0, 0x2AB0, 0xAAB0, 0x2AAB0, 0xAAAB0};
	const static int     vq16[] = {0x0, 0x1, 0x2,  0x6,  0x16, 0x56,  0x156, 0x556,  0x1556, 0x5556,  0x15556};
	
	dcglHardAssert(texformat.mipmap);
	int idx = (texformat.pcw+3) - level;
	dcglHardAssert(idx >= 0);
	dcglHardAssert(idx <= (sizeof(uncomp16) / sizeof(uncomp16[0])));
	
	if (texformat.vq) {
		switch(texformat.format) {
		case PC_PALETTE_8B: dcglUnimpAssert(0); break;
		case PC_PALETTE_4B: dcglUnimpAssert(0); break;
		default:            return vq16[idx] + texformat.cbsize;
		}
	} else {
		switch(texformat.format) {
		case PC_PALETTE_8B: return uncomp16[idx]/2; break;
		case PC_PALETTE_4B: return uncomp16[idx]/4; break;
		default:            return uncomp16[idx];
		}
	}
	
	dcglHardAssert(0);
	//NOT REACHED
	return -1;
}

//returns size of texture in bytes, or -1 on invalid texture
static int dcglCalcTextureSize(DCGLpvrformat2 texformat) {
	unsigned int texels = pc_unconvert_size(texformat.pcw) * pc_unconvert_size(texformat.pch), size;
	
	switch(texformat.format) {
	case PC_PALETTE_8B: size = texels;   break;
	case PC_PALETTE_4B: size = texels/2; break;
	default:            size = texels*2;
	}
	
	if (texformat.vq)
		size /= 8;
	
	//dcglMipmapOffset already adds codebook size; don't count it twice on a mipmapped vq texture
	if (texformat.mipmap)
		size += dcglMipmapOffset(texformat, 0);
	else if (texformat.vq)
			size += texformat.cbsize;
	
	return size;
}

static unsigned short * dcglTextureMipmapPointer(DCGLtexture *tex, GLsizei level) {
	if (!dcglTextureHasMipmaps(tex)) {
		dcglHardAssert(level == 0);
		return tex->base;
	}
	
	dcglHardAssert(dcglMipmappable(dcglTexturePCWidth(tex), dcglTexturePCHeight(tex)));
	return tex->base + dcglMipmapOffset(tex->out.pvrf,level)/sizeof(*tex->base);
}

static int dcglMipmapLevelCount(pc_texture_size dimension) {
	return pc_size_to_log2(dimension)+1;
}
static int dcglTextureMipmapLevelCount(const DCGLtexture *tex) {
	dcglHardAssert(tex);
	//dcglHardAssert(dcglTextureSquare(tex));
	return dcglTextureSquare(tex) ? dcglMipmapLevelCount(dcglTexturePCWidth(tex)) : 1;
}
static int dcglMipmapLevelMask(pc_texture_size dimension) {
	return 0x7ff >> (dcglMipmapLevelCount(PC_SIZE_1024) - dcglMipmapLevelCount(dimension));
}

static int dcglTextureMipmapLevelMask(const DCGLtexture *tex) {
	dcglHardAssert(tex);
	//dcglLog("tex \"%p\" %ix%i - %ix%i", tex, tex->w,tex->h, dcglTexturePCWidth(tex), dcglTexturePCHeight(tex));
	dcglHardAssert(dcglTexturePCWidth(tex) == dcglTexturePCHeight(tex));
	return dcglMipmapLevelMask(dcglTexturePCWidth(tex));
}
static int dcglTextureHasCompleteMipmaps(const DCGLtexture *tex) {
	//if this has mipmaps allocated, return true if all levels complete; always return 0 on unmipmapped textures
	//dcglLog("%i", dcglTextureHasMipmaps(tex) ? tex->levels == dcglTextureMipmapLevelMask(tex) : 0);
	dcglHardAssert(tex);
	return dcglTextureHasMipmaps(tex) ? tex->levels == dcglTextureMipmapLevelMask(tex) : 0;
}
static void dcglAddMipmapLevel(DCGLtexture *tex, int level) {
	dcglHardAssert(tex);
	dcglHardAssert(level <= dcglTextureMipmapLevelCount(tex));
	tex->levels |= 1 << level;
}

static unsigned short * dcglTexturePVRPointer(DCGLtexture *tex) {
	dcglHardAssert(tex);
	if (!dcglTextureHasMipmaps(tex)) {
		return tex->base;
	} else {
		if (dcglTextureHasCompleteMipmaps(tex))
			return tex->base;
		else
			return dcglTextureMipmapPointer(tex, 0);
	}
}

//resizes the area allocated for a texture (does not resample texture)
//returns 0 if texture is (re)allocated at proper size, or 1 if unable to allocate
static int dcglResizeTexture(DCGLtexture *tex, GLsizei width, GLsizei height, int mipmaps) {
	extern void* pvr_int_realloc(void* m, size_t bytes);
	
	assert(tex);
	dcglHardAssert(tex->out.size > 0);
	dcglHardAssert(tex->out.size <= 4);
	dcglHardAssert(tex->out.putp);
	
	DCGLpvrformat2 pvrf = {
		.pcw = pc_convert_size(width),
		.pch = pc_convert_size(height),
		.mipmap = mipmaps,
		.format = tex->out.pvrf.format,
		.vq = tex->out.pvrf.vq,
		.twid = tex->out.pvrf.twid,
		.cbsize = tex->out.pvrf.cbsize };
	
	int texsize = dcglCalcTextureSize(pvrf);
	dcglHardAssert(texsize > 0);
	//dcglLog("resizing %p to %ix%i mip %i. size %i -> %i", tex, width, height, mipmaps, tex->alloc_size, texsize);
	if (texsize == tex->alloc_size) {
		//requested size is already allocated, nothing to do
		return 0;
	}
	
	if (tex->base) {
		//texture has already been allocated, must be resizing it
		
		void *newtex = pvr_int_realloc(tex->base, texsize);
		//dcglLog("from %p to %p", tex->base, newtex);
		dcglHardAssert(newtex != NULL);
		//dcglRequire(newtex != NULL, GL_OUT_OF_MEMORY, return 1);
		
		tex->base = newtex;
		
		//is it adding mipmaps to a mip-less texture of the right size?
		if (dcglTextureWidth(tex) == width && dcglTextureHeight(tex) == height && !dcglTextureHasMipmaps(tex) && mipmaps && (tex->levels&1)) {
			//if there is already a top level mipmap, move it into correct position,
			//TODO handle having something other than a top level mipmap
			//since the old and new positions probably overlap, we can't use memcpy
			//memmove isn't used because there's no official guarantee that it won't use byte access, which fails when accessing video ram
			//so the copy is done by hand
			//dcglLog("Adding mipmaps to %p", tex);
			
			short *copysrc = (short*)( ((char*)newtex)+tex->alloc_size );
			short *copydst = (short*)( ((char*)newtex)+texsize );
			int copywords = tex->alloc_size / sizeof(*copysrc);
			do {
				*--copydst = *--copysrc;
			} while(--copywords);
		} else {
			//no new mipmaps needed, must be replacing it with a different size
			tex->levels = 0;
		}
		
		tex->alloc_size = texsize;
	} else {
		//texture doesn't have video ram allocated to it
		dcglHardAssert(texsize > 0);
		
		//dcglHardAssert(texsize < pvr_mem_available());
		tex->base = (unsigned short*)pvr_mem_malloc(texsize);
		dcglHardAssert(tex->base);
		//dcglRequire(tex->base, GL_OUT_OF_MEMORY, return 1);
		
		//dcglLog("alloc %i bytes to %p", texsize, tex->base);
		
		tex->alloc_size = texsize;
		tex->levels = 0;
	}
	
	dcglTextureSetWidth(tex, width);
	dcglTextureSetHeight(tex, height);
	dcglTextureSetPCWidth(tex, pc_convert_size(width));
	dcglTextureSetPCHeight(tex, pc_convert_size(height));
	dcglSetTextureHasMipmaps(tex, mipmaps);
	
	dcglTextureSetPVRDirty(tex, 1);
	
	return 0;
}

static void dcglInitTexture(DCGLtexture *tex)
{
	tex->swrap = tex->twrap = GL_REPEAT;
	tex->minfilter = GL_NEAREST_MIPMAP_LINEAR;
	tex->magfilter = GL_LINEAR;
	tex->flags = 0;
	tex->alloc_size = 0;
	dcglSetTextureHasMipmaps(tex, 0);
	dcglSetTextureAutoMipmap(tex, 0);
	
	//TODO don't allocate blank textures on init, use default texture instead
	dcglSetTextureFormat(tex, GL_RGB);
	dcglResizeTexture(tex, 8, 8, 0);
	
	int i;
	for(i = 0; i < 8*8; i++) {
		tex->base[i] = -1;
	}
}

static DCGLtexture * dcglAllocTexture()
{
	DCGLtexture *tex = dcglAllocTextureInfo();
	dcglHardAssert(tex);
	dcglInitTexture(tex);
	
	return tex;
}

//returns a spot on the name array for the texture
//TODO will target be needed in the future?
static int dcglFindTextureName(GLenum target, GLuint textureid)
{
	//check default position
	if (textureid < DCGL_MAX_TEXTURE_NAMES) {
		if (dcgl_namemap[textureid].idx == textureid && dcgl_namemap[textureid].tex) {
			//found it
			return textureid;
		} else if (dcgl_namemap[textureid].tex == NULL) {
			//free spot
			return textureid;
		}
		//this spot belongs to another texture, drop down to search
	}
	
	//got large numbered texture, search from the top for it
	//remember first free spot found
	int freespot = 0, i;
	for(i = DCGL_MAX_TEXTURE_NAMES-1; i > 0; i--) {
		if (dcgl_namemap[i].idx == textureid && dcgl_namemap[i].tex)
			return i;	//found it, return it
		else if (freespot == 0 && dcgl_namemap[i].tex == NULL)
			freespot = i;	//remember first free spot
	}
	
	dcglHardAssert(freespot);
	//not found, allocate it in first free spot
	return freespot;
}

//converts ogl texture handle to pointer, allocates a default texture if not found
DCGLtexture * dcglGetTexture(GLenum target, GLuint textureid)
{
	int idx = dcglFindTextureName(target, textureid);
	if (dcgl_namemap[idx].tex == NULL) {
		dcgl_namemap[idx].idx = textureid;
		dcgl_namemap[idx].tex = dcglAllocTexture();
	}
	return dcgl_namemap[idx].tex;
	
}

void dcglSubTexImage2D(const void *src, DCGLtextureinput *srcread, int level, int x, int y, int w, int h, DCGLtexture *dsttex)
{
	//dcglUnimpAssert(target == GL_TEXTURE_2D);
	dcglHardAssert(src);
	dcglHardAssert(srcread);
	dcglHardAssert(dsttex);
	dcglHardAssert(level >= 0);
	dcglHardAssert(x >= 0);
	dcglHardAssert(y >= 0);
	dcglHardAssert(w >= 0);
	dcglHardAssert(h >= 0);
	dcglHardAssert(dcglTextureWidth(dsttex) <= w);
	dcglHardAssert(dcglTextureHeight(dsttex) <= h);
	if (level > 0) {
		dcglHardAssert(dcglMipmappable(dcglTexturePCWidth(dsttex), dcglTexturePCHeight(dsttex)));
		dcglHardAssert(dcglTextureHasMipmaps(dsttex));
		dcglHardAssert(level <= dcglTextureMipmapLevelCount(dsttex));
	}
	
	//TODO add support for nonsquare textures
	int i,j;
	mortonx_t sxm = Int2MortonX(x), xm;
	mortony_t sym = Int2MortonY(y), ym;
	int wstride = w * srcread->size;
	const unsigned char *srcs = ((unsigned char *)src) + (h-1) * wstride;
	unsigned short *base = dcglTextureMipmapPointer(dsttex, level);
	
	ym = sym;
	for(j = 0; j < h; j++) {
		xm = sxm;
		for(i = 0; i < w; i++) {
			dsttex->out.putp(srcread->getp(srcs), base + CombineMorton(xm,ym)/2);
			srcs += srcread->size;
			xm = IncMortonX(xm);
		}
		srcs -= wstride*2;
		ym = IncMortonY(ym);
	}
}

void dcglSubTexImage2DRect(const void *src, DCGLtextureinput *srcread, int level, int x, int y, int w, int h, DCGLtexture *dsttex)
{
	//dcglUnimpAssert(target == GL_TEXTURE_2D);
	dcglHardAssert(src);
	dcglHardAssert(srcread);
	dcglHardAssert(dsttex);
	dcglHardAssert(level >= 0);
	dcglHardAssert(x >= 0);
	dcglHardAssert(y >= 0);
	dcglHardAssert(w >= 0);
	dcglHardAssert(h >= 0);
	dcglHardAssert(dcglTextureWidth(dsttex) <= w);
	dcglHardAssert(dcglTextureHeight(dsttex) <= h);
	if (level > 0) {
		dcglHardAssert(dcglMipmappable(dcglTexturePCWidth(dsttex), dcglTexturePCHeight(dsttex)));
		dcglHardAssert(dcglTextureHasMipmaps(dsttex));
		dcglHardAssert(level <= dcglTextureMipmapLevelCount(dsttex));
	}
	
	//TODO add support for nonsquare textures
	int i,j, xlog, ylog;
	unsigned int xw = 16, yw = 16;
	xlog = pc_size_to_log2(pc_convert_size(x));
	ylog = pc_size_to_log2(pc_convert_size(y));
	if (w > h) {
		xw = ylog;
	} else {
		yw = xlog+1;	//TODO hack, fix morton.h
	}
	mortonx_t sxm = Int2MortonXWidth(x,xw), xm;
	mortony_t sym = Int2MortonYWidth(y,yw), ym;
	int wstride = w * srcread->size;
	const unsigned char *srcs = ((unsigned char *)src) + (h-1) * wstride;
	unsigned short *base = dcglTextureMipmapPointer(dsttex, level);
	
	ym = sym;
	for(j = 0; j < h; j++) {
		xm = sxm;
		for(i = 0; i < w; i++) {
			dsttex->out.putp(srcread->getp(srcs), base + CombineMorton(xm,ym)/2);
			srcs += srcread->size;
			xm = IncPartialMortonX(xm,xw);
		}
		srcs -= wstride*2;
		ym = IncPartialMortonY(ym,yw);
	}
}

static int is_pow2(int x) {
	return (x & (x-1)) == 0;
}

void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid * data)
{
	//dcglUnimpAssert(width == height);
	
	dcglRequire(border == 0, GL_INVALID_VALUE, return);
	dcglRequire(width <= DCGL_MAX_TEXTURE_SIZE, GL_INVALID_VALUE, return);
	dcglRequire(height <= DCGL_MAX_TEXTURE_SIZE, GL_INVALID_VALUE, return);
	dcglRequire(width >= DCGL_MIN_TEXTURE_SIZE, GL_INVALID_VALUE, return);
	dcglRequire(height >= DCGL_MIN_TEXTURE_SIZE, GL_INVALID_VALUE, return);
	dcglRequire(is_pow2(width), GL_INVALID_VALUE, return);
	dcglRequire(is_pow2(height), GL_INVALID_VALUE, return);
	dcglUnimpAssert(target == GL_TEXTURE_2D);	//only works on 2d texture
	dcglSoftAssert(dcglGetTargetTextureId(target) != 0);	//can't change texture 0
	
	GLsizei basewidth = width << level, baseheight = height << level;
	
	DCGLtexture *tex = dcglGetTargetTexture(target);
	dcglHardAssert(tex);
	dcglSoftAssert(!tex->out.pvrf.vq);
	dcglSetTextureFormat(tex, internalformat);
	dcglResizeTexture(tex, basewidth, baseheight, level != 0 || dcglTextureHasMipmaps(tex) || dcglTextureHasAutoMipmap(tex));
	
	DCGLtextureinput texin;
	dcglGetTextureInput(format, type, &texin);
	dcglSubTexImage2D(data, &texin, level, 0, 0, width, height, tex);
	dcglAddMipmapLevel(tex, level);
	
	if (dcglTextureHasAutoMipmap(tex))
		glGenerateMipmap(target);
	
	dcglApplyTexture();
}

void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data) {
	dcglRequire(border == 0, GL_INVALID_VALUE, return);
	dcglRequire(imageSize > 0, GL_INVALID_VALUE, return);
	dcglRequire(GL_COMPRESSED_VQ_1555_DC <= internalformat && internalformat <= GL_COMPRESSED_VQ_YUV_MM_DC, GL_INVALID_ENUM, return);
	
	DCGLtexture *tex = dcglGetTargetTexture(target);
	dcglHardAssert(tex);
	
	DCGLpvrformat2 pvrf = { .vq = 1, .twid = 1, .pcw = pc_convert_size(width), .pch = pc_convert_size(height) };
	switch(internalformat) {
	case GL_COMPRESSED_VQ_1555_DC:    pvrf.format = PC_ARGB1555; pvrf.mipmap = 0; pvrf.cbsize = 2048; break;
	case GL_COMPRESSED_VQ_1555_MM_DC: pvrf.format = PC_ARGB1555; pvrf.mipmap = 1; pvrf.cbsize = 2048; break;
	case GL_COMPRESSED_VQ_565_DC:     pvrf.format = PC_RGB565;   pvrf.mipmap = 0; pvrf.cbsize = 2048; break;
	case GL_COMPRESSED_VQ_565_MM_DC:  pvrf.format = PC_RGB565;   pvrf.mipmap = 1; pvrf.cbsize = 2048; break;
	case GL_COMPRESSED_VQ_4444_DC:    pvrf.format = PC_ARGB4444; pvrf.mipmap = 0; pvrf.cbsize = 2048; break;
	case GL_COMPRESSED_VQ_4444_MM_DC: pvrf.format = PC_ARGB4444; pvrf.mipmap = 1; pvrf.cbsize = 2048; break;
	case GL_COMPRESSED_TWID_1555_DC:    pvrf.format = PC_ARGB1555; pvrf.mipmap = 0; pvrf.cbsize = 0; pvrf.vq = 0; break;
	case GL_COMPRESSED_TWID_1555_MM_DC: pvrf.format = PC_ARGB1555; pvrf.mipmap = 1; pvrf.cbsize = 0; pvrf.vq = 0; break;
	case GL_COMPRESSED_TWID_565_DC:     pvrf.format = PC_RGB565;   pvrf.mipmap = 0; pvrf.cbsize = 0; pvrf.vq = 0; break;
	case GL_COMPRESSED_TWID_565_MM_DC:  pvrf.format = PC_RGB565;   pvrf.mipmap = 1; pvrf.cbsize = 0; pvrf.vq = 0; break;
	case GL_COMPRESSED_TWID_4444_DC:    pvrf.format = PC_ARGB4444; pvrf.mipmap = 0; pvrf.cbsize = 0; pvrf.vq = 0; break;
	case GL_COMPRESSED_TWID_4444_MM_DC: pvrf.format = PC_ARGB4444; pvrf.mipmap = 1; pvrf.cbsize = 0; pvrf.vq = 0; break;
	default: dcglSoftAssert(0);
	}
	
	//check texture is correct size
	dcglRequire(dcglCalcTextureSize(pvrf) == imageSize, GL_INVALID_VALUE, return);
	
	memcpy(&tex->out.pvrf, &pvrf, sizeof(pvrf));
	dcglResizeTexture(tex, width, height, pvrf.mipmap);
	if (pvrf.mipmap)
		tex->levels = dcglTextureMipmapLevelMask(tex);
	
	//copy by hand with shorts to make sure pvr doesn't get upset TODO use dma
	short *vqdst = (short*)tex->base;
	const short *vqsrc = data;
	imageSize /= 2;
	do {
		*vqdst++ = *vqsrc++;
	} while(--imageSize);
	
	dcglApplyTexture();
}

int dcgl_trilinear_pass = 0;
pc_texture_filter dcglCalcFilter(DCGLtexture *tex) {
	if (tex->levels && tex->minfilter == GL_LINEAR_MIPMAP_LINEAR)
		return dcgl_trilinear_pass ? PC_TRILINEAR_2ND : PC_TRILINEAR_1ST;
	else if (tex->magfilter == GL_NEAREST)
		return PC_POINT;
	else
		return PC_BILINEAR;
}

void dcglTextureUpdateContext(DCGLtexture *tex) {
	dcglHardAssert(tex);
	if (dcglTextureIsPVRDirty(tex)) {
		pvr_context_submodes *pvrc = pc_no_mod(&tex->pvrdata);
		pc_set_uv_size(pvrc, dcglTexturePCWidth(tex), dcglTexturePCHeight(tex));
		pc_set_mipmapped(pvrc, dcglTextureHasCompleteMipmaps(tex));
		pc_set_texture_address(pvrc, dcglTexturePVRPointer(tex));
		pc_set_compressed(pvrc, tex->out.pvrf.vq);
		pc_set_texture_format(pvrc, tex->out.pvrf.format);
		pc_set_twiddled(pvrc, 1);
		pc_set_filter(pvrc,dcglCalcFilter(tex));
		pc_set_textured(&tex->pvrdata, 1);
		
		pc_uv_control clamp = 0, mirror = 0;
		switch(tex->swrap) {
		case GL_REPEAT:          break;
		case GL_MIRRORED_REPEAT: mirror |= PC_UV_X; break;
		case GL_CLAMP_TO_EDGE:   //fallthrough
		case GL_CLAMP:           clamp  |= PC_UV_X; break;
		}
		switch(tex->twrap) {
		case GL_REPEAT:          break;
		case GL_MIRRORED_REPEAT: mirror |= PC_UV_Y; break;
		case GL_CLAMP_TO_EDGE:   //fallthrough
		case GL_CLAMP:           clamp  |= PC_UV_Y; break;
		}
		pc_set_uv_flip(pvrc, mirror);
		pc_set_uv_clamp(pvrc, clamp);
		
		dcglTextureSetPVRDirty(tex, 0);
	}
}

static inline uint32 dcglBitSelect(uint32 src, uint32 mask, uint32 value) {
	//return (src & ~mask) | (value & mask);
	return ((src ^ value) & mask) ^ src;
}

void dcglApplyTextureToContext(DCGLtexture *tex, pvr_context *dst) {
	dcglHardAssert(tex);
	dcglTextureUpdateContext(tex);
	
	pvr_context cxtmask;
	pvr_context_submodes *nm = pc_no_mod(&cxtmask);
	
	cxtmask.cmd = 0;
	cxtmask.mode1 = 0;
	nm->mode2 = 0;
	nm->tex = 0;
	
	pc_set_mode_texture(nm, (void*)-1, -1, -1, -1, -1, -1, -1);
	pc_set_uv_flip(nm, -1);
	pc_set_uv_clamp(nm, -1);
	pc_set_filter(nm, -1);
	pc_set_textured(&cxtmask, 1);
	
	dst->cmd = dcglBitSelect(dst->cmd, cxtmask.cmd, tex->pvrdata.cmd);
	dst->mode1 = dcglBitSelect(dst->mode1, cxtmask.mode1, tex->pvrdata.mode1);
	dst->bf.m.mode2 = dcglBitSelect(dst->bf.m.mode2, cxtmask.bf.m.mode2, tex->pvrdata.bf.m.mode2);
	dst->bf.m.tex = dcglBitSelect(dst->bf.m.tex, cxtmask.bf.m.tex, tex->pvrdata.bf.m.tex);
}

void dcglApplyTexture() {
	int targ = dcglGetTopTargetIdx();
	if (targ == 1) {
		DCGLtexture *tex = dcglCurTex2D()->tex;
		dcglHardAssert(tex);
		dcglApplyTextureToContext(tex, &dcgl_current_context);
		//pc_set_anisotropic(pc_no_mod(&dcgl_current_context), 1);
		//pc_set_mipmap_bias(pc_no_mod(&dcgl_current_context), PC_MIPMAP_BIAS_0_75);
		//pc_set_mipmap_bias(pc_no_mod(&dcgl_current_context), PC_MIPMAP_BIAS_1_25);
		//pc_set_exact_mipmap(pc_no_mod(&dcgl_current_context), 1);
	} else {
		dcglApplyTextureToContext(&dcgl_default_texture, &dcgl_current_context);
	}
	dcglSetPVRStateDirty(1);
}

void glBindTexture(GLenum target, GLuint texture) {
	int targ = dcglTargetIdx(target);
	if (targ >= 0) {
		dcglCurTexTargs()[targ].id = texture;
		dcglCurTexTargs()[targ].tex = dcglGetTexture(target, texture);
		dcglHardAssert(dcglCurTexTargs()[targ].tex);
	} else {
		dcglSoftAssert(0);	
	}
	dcglApplyTexture();
}

void glGenTextures(GLsizei n, GLuint *textures) {
	if (textures == NULL || n < 0)
		return;
	int curname = 1;
	while(n--) {
		while (curname < DCGL_MAX_TEXTURE_NAMES && dcgl_namemap[curname].tex != NULL)
			curname++;
		dcglSoftAssert(curname < DCGL_MAX_TEXTURE_NAMES);
		*textures++ = curname++;
	}
}

void glDeleteTextures(GLsizei n, const GLuint *textureNames) {
	/*
		TODO need to make sure textures that are being used by the rendering frame (i.e. last frame from ogl view)
		and the currently collecting frame (i.e. current frame from ogl view) are not deleted until their
		usage completes
	*/
	if (textureNames == NULL || n < 0)
		return;
	
	while(n--) {
		int texidx = dcglFindTextureName(GL_TEXTURE_2D, *textureNames);
		if (texidx && dcgl_namemap[texidx].tex)
			dcglFreeTextureInfo(dcgl_namemap[texidx].tex);
		textureNames++;
	}
}

GLboolean glIsTexture(GLuint texture) {
	int texidx = dcglFindTextureName(GL_TEXTURE_2D, texture);
	return (texidx == 0 || dcgl_namemap[texidx].tex == NULL) ? GL_FALSE : GL_TRUE;
}

void glTexParameteri(GLenum target, GLenum pname, GLint param) {
	dcglRequire(target == GL_TEXTURE_1D || target == GL_TEXTURE_2D, GL_INVALID_ENUM, return);
	
	DCGLtexture *tex = dcglGetTargetTexture(target);
	dcglHardAssert(tex);
	
	switch(pname) {
	case GL_TEXTURE_MIN_FILTER:
		switch(param) {
		case GL_NEAREST:
		case GL_LINEAR:
		case GL_NEAREST_MIPMAP_NEAREST:
		case GL_LINEAR_MIPMAP_NEAREST:
		case GL_NEAREST_MIPMAP_LINEAR:
		case GL_LINEAR_MIPMAP_LINEAR:
			tex->minfilter = param;
			dcglTextureSetPVRDirty(tex,1);
			break;
		default:
			dcglSetErrorF(GL_INVALID_ENUM);
			return;
		}
		break;
	case GL_TEXTURE_MAG_FILTER:
		dcglRequire(param == GL_NEAREST || param == GL_LINEAR, GL_INVALID_ENUM, return);
		tex->magfilter = param;
		break;
	case GL_TEXTURE_WRAP_S:
	case GL_TEXTURE_WRAP_T:
		switch(param) {
		case GL_REPEAT:
		case GL_CLAMP:	//pvr doesn't support borders, so this isn't really supported...
		case GL_CLAMP_TO_EDGE:
		case GL_MIRRORED_REPEAT:
			if (pname == GL_TEXTURE_WRAP_S)
				tex->swrap = param;
			else
				tex->twrap = param;
			break;
		default:
			dcglSetErrorF(GL_INVALID_ENUM);
			return;
		}
		break;
	case GL_TEXTURE_MAX_ANISOTROPY_EXT:
		dcglUnimpAssert(0);
		break;
	case GL_GENERATE_MIPMAP:
		dcglSetTextureAutoMipmap(tex,param);
		break;	//TODO check param is valid
	default:
		dcglSetErrorF(GL_INVALID_ENUM);
		return;
	}
	dcglApplyTexture();
}

void glGetTexParameteriv(GLenum target, GLenum pname, GLint * params) {
	dcglRequire(target == GL_TEXTURE_1D || target == GL_TEXTURE_2D, GL_INVALID_ENUM, return);
	
	DCGLtexture *tex = dcglGetTargetTexture(target);
	dcglHardAssert(tex);
	
	switch(pname) {
	case GL_TEXTURE_MIN_FILTER:	*params = tex->minfilter; break;
	case GL_TEXTURE_MAG_FILTER:	*params = tex->magfilter; break;
	case GL_TEXTURE_WRAP_S:		*params = tex->swrap; break;
	case GL_TEXTURE_WRAP_T:		*params = tex->twrap; break;
	default:
		dcglSetErrorF(GL_INVALID_ENUM);
		return;
	}
}

void glTexEnvi(GLenum target, GLenum pname, GLint param) {
	switch(target) {
	case GL_TEXTURE_ENV_MODE:
		switch(pname) {
		//case GL_REPLACE_EXT: param = GL_REPLACE;
		case GL_REPLACE:
		case GL_DECAL:
		case GL_MODULATE:
			dcglCurTMU()->mode = param;
			break;
		//case GL_ADD:
		case GL_BLEND:
		default:
			dcglRequire(0, GL_INVALID_ENUM, return);
		}
		break;
	case GL_TEXTURE_FILTER_CONTROL:
		dcglRequire(pname == GL_TEXTURE_LOD_BIAS, GL_INVALID_ENUM, return);
		dcglUnimpAssert(0);
		break;
	default:
		dcglRequire(0, GL_INVALID_ENUM, return);
	}
	dcglApplyTMU(0);	//TODO needs to be changed if multitexture finished
}

void glTexEnvf(GLenum target, GLenum pname, GLfloat param) {
	dcglUnimpAssert(0);
}
void glTexEnviv(GLenum target, GLenum pname, const GLint *params) {
	glTexEnvi(target, pname, *params);
}

void dcglDownsampleMap(DCGLtexture *tex, int level) {
	//reads mipmap from level and generates level+1 with a box filter
	//only works on twiddled textures
	dcglHardAssert(dcglTextureHasMipmaps(tex));
	dcglHardAssert(!dcglTextureCompressed(tex));
	dcglHardAssert(level >= 0);
	dcglHardAssert(level < dcglTextureMipmapLevelCount(tex));
	dcglHardAssert(tex->levels & (1 << level));
	
	short *src = (short*)dcglTextureMipmapPointer(tex, level);
	short *dst = (short*)dcglTextureMipmapPointer(tex, level+1);
	int i,pixels = (dcglTextureWidth(tex) >> level) * (dcglTextureHeight(tex) >> level) / 4;
	unsigned int mixer[2], mask[2] = {0,0};
	
	switch(tex->out.pvrf.format) {
	case PC_ARGB1555: mask[0] = 0x83E0; mask[1] = 0x7C1F; break;
	case PC_RGB565:   mask[0] = 0x07E0; mask[1] = 0xF81F; break;
	case PC_ARGB4444: mask[0] = 0xF0F0; mask[1] = 0x0F0F; break;
	default: dcglHardAssert(0);
	}
	
	for(i = 0; i < pixels; i++) {
		int j,k;
		for(j = 0; j < 2; j++) {
			mixer[j] = 0;
			for(k = 0; k < 4; k++)
				mixer[j] += src[k] & mask[j];
			mixer[j] = (mixer[j] >> 2) & mask[j];
		}
		*dst++ = mixer[0] | mixer[1];
		src += 4;
	}
	dcglAddMipmapLevel(tex, level+1);
}

void glGenerateMipmap(GLenum target) {
	dcglUnimpAssert(target == GL_TEXTURE_2D);
	//int targ = dcglGetTarget(target);
	DCGLtexture *tex = dcglGetTargetTexture(target);
	dcglHardAssert(tex);
	dcglRequire(dcglTextureSquare(tex), GL_INVALID_OPERATION, return);
	
	int i, levels = dcglTextureMipmapLevelCount(tex)-1;
	
	dcglResizeTexture(tex, dcglTextureWidth(tex), dcglTextureHeight(tex), 1);
	for(i = 0; i < levels; i++) {
		dcglDownsampleMap(tex, i);
	}
	dcglApplyTexture();
}

