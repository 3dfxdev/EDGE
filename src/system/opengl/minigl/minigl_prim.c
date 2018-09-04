#include "minigl.h"
#include <math.h>

DCGLprimstate dcgl_prim = { .nearclip = 0.0001f, .vert_params = {0,0,0,1, 0,0,0,1, 1,1,1,1, 0,0,0,0} };

__attribute__((aligned(32))) pvr_context dcgl_current_context;

//	RENDERING
static inline int dcglVerticesInBuffer() {
	return dcgl_prim.cur_vert - dcgl_prim.vert_buff;
}
#define dcglParams(vert) &(vert)->x, &(vert)->u, &(vert)->fr

void dcglTransformVectors(float *srcp, size_t srcstride, float *dstp, size_t dststride, size_t count);
void dcglTransformVectorsMat(float *srcp, size_t srcstride, float *dstp, size_t dststride, size_t count, matrix_t *mat);
#if 0
static void dcglTransformVectorsC(float *srcp, size_t srcstride, float *dstp, size_t dststride, size_t count) {
	srcstride /= sizeof(*srcp);
	dststride /= sizeof(*dstp);
	do {
		float x = srcp[0], y = srcp[1], z = srcp[2], w = srcp[3];
		dcgl_ftrv(x,y,z,w);
		srcp += srcstride;
		dstp[0] = x; dstp[1] = y; dstp[2] = z; dstp[3] = w;
		dstp += dststride;
	} while(--count);
}

static int dcglAnyGreaterC(float valcompare, float *val, size_t stride, int count) {
	stride /= sizeof(*val);
	do {
		if (valcompare > *val)
			return 1;
		val += stride;
	} while(--count);
	return 0;
}
#endif

static inline int dcglVertBuffNeedsClipping() {
	extern int dcglAnyGreater(float valcompare, float *val, size_t stride, int count);
	return dcglAnyGreater(dcgl_prim.nearclip, &dcgl_prim.vert_buff->w, sizeof(dcgl_prim.vert_buff[0]), dcglVerticesInBuffer());
}

static DCGLpvrvertex * dcglDirectModeEngage()
{
	dcglCheckInSceneExtra();
	
	extern void *dcglSQEngage();
	
	return dcglSQEngage();
}

#define dcglPref(addr) __asm__ __volatile__("pref @%0" : : "r" (addr))
#define dcglSubmitDirectLow(addr) __asm__ __volatile__("pref @%0" : : "r" (addr))
#define dcglSubmitDirectHigh(addr) __asm__ __volatile__("pref @%0" : : "r" (((char*)(addr))+32))

static inline void dcglPerspective(DCGLvertex *vert) {
	float invz = 1.0f / vert->w;
	vert->w = invz;
	vert->w = invz;
	vert->y *= invz;
	vert->x *= invz;
}

static void dcglSubmitNoPerspReal(DCGLvertex *vert, DCGLpvrvertex *nv, int cmd) {
	dcglLogSubmit("call");
	dcglCheckInSceneExtra();
	
	nv->v = vert->v;
	nv->u = vert->u;
	nv->z = vert->w;
	nv->y = vert->y;
	nv->x = vert->x;
	nv->cmd = cmd;
	
	//pvr_dr_commit(nv);
	dcglSubmitDirectLow(nv);
	
	nv->fb = vert->fb;
	nv->fg = vert->fg;
	nv->fr = vert->fr;
	nv->fa = vert->fa;
	
	//pvr_dr_commit(&nv->fa);
	dcglSubmitDirectHigh(nv);
}

static void dcglSumbitOne(float *pos, float *uv, float *argb, int cmd, DCGLpvrvertex *dst) {
	//dcglLog("%p %p %p %08x -> %p",pos,uv,argb,cmd, dst);
	float invz = 1.0f / pos[3];
	
	dst->cmd = cmd;
	dst->v = uv[1];
	dst->u = uv[0];
	dst->z = invz;
	dst->y = pos[1] * invz;
	dst->x = pos[0] * invz;
	dcglSubmitDirectLow(dst);
	
	dst->fbo = argb[7];
	dst->fgo = argb[6];
	dst->fro = argb[5];
	dst->fogd = argb[4];
	dst->fb = argb[3];
	dst->fg = argb[2];
	dst->fr = argb[1];
	dst->fa = argb[0];
	dcglSubmitDirectHigh(dst);
}

DCGLpvrvertex * dcglSubmitPerspective(float *pos, float *uv, float *argb, int stride, int striplen, int count, DCGLpvrvertex *dst);

DCGLpvrvertex * dcglSubmitMany(float *pos, float *uv, float *argb, int stride, int striplen, int count, DCGLpvrvertex *dst) {
	int tempstriplen = striplen;
	stride /= sizeof(float);
	do {
		float invz = 1.0f / pos[3];
		//float invz = 1.0f;
		if (--tempstriplen) {
			dst->cmd = PVR_CMD_VERTEX;
		} else {
			dst->cmd = PVR_CMD_VERTEX_EOL;
			tempstriplen = striplen;
		}
		dst->v = uv[1];
		dst->u = uv[0];
		dst->z = invz;
		dst->y = pos[1] * invz;
		dst->x = pos[0] * invz;
		
		dcglSubmitDirectLow(dst);
		
		dst->fbo = argb[7];
		dst->fgo = argb[6];
		dst->fro = argb[5];
		dst->fogd = argb[4];
		dst->fb = argb[3];
		dst->fg = argb[2];
		dst->fr = argb[1];
		dst->fa = argb[0];
		
		dcglSubmitDirectHigh(dst);
		pos += stride;
		uv += stride;
		argb += stride;
		dst++;
	} while(--count);
	return dst;
}

static DCGLpvrvertex * dcglSubmitManyQuad(float *pos, float *uv, float *argb, int stride, int count, DCGLpvrvertex *dst) {
	//dcglLog("#: %i",count);
	int cstride = stride / sizeof(float);
	do {
		if (1) {
			dst = dcglSubmitMany(pos, uv, argb,                                stride, 0, 2, dst);
			dst = dcglSubmitMany(pos+cstride*3, uv+cstride*3, argb+cstride*3, -stride, 2, 2, dst);
		} else {
			dst = dcglSubmitPerspective(pos, uv, argb,                                stride, 0, 2, dst);
			dst = dcglSubmitPerspective(pos+cstride*3, uv+cstride*3, argb+cstride*3, -stride, 2, 2, dst);
		}
		pos += cstride*4;
		uv += cstride*4;
		argb += cstride*4;
		//dst += 4;
	} while(--count);
	return dst;
}

static inline float dcglLerp(float v0, float v1, float w)
{
	return v0 + w*(v1-v0);
	//return (1-w)*v0 + w*v1;
	//t*b + (a-t*a)
	//t*b + (-(t*a-a))
}

static inline float dcglInsectionPoint(float a, float b)
{
	return a / (a - b);
}

static void dcglWriteIntersectionGouraud(DCGLvertex *a, DCGLvertex *b, float weight, float w, DCGLvertex *d)
{
	d->x = dcglLerp(a->x, b->x, weight);
	d->y = dcglLerp(a->y, b->y, weight);
	d->z = w;
	d->w = w;
	d->u = dcglLerp(a->u, b->u, weight);
	d->v = dcglLerp(a->v, b->v, weight);
	d->fa = dcglLerp(a->fa, b->fa, weight);
	d->fr = dcglLerp(a->fr, b->fr, weight);
	d->fg = dcglLerp(a->fg, b->fg, weight);
	d->fb = dcglLerp(a->fb, b->fb, weight);
	if (0) {
		d->fogd = dcglLerp(a->fogd, b->fogd, weight);
		d->fro = dcglLerp(a->fro, b->fro, weight);
		d->fgo = dcglLerp(a->fgo, b->fgo, weight);
		d->fbo = dcglLerp(a->fbo, b->fbo, weight);
	}
}

static void dcglWriteIntersectionFlat(DCGLvertex *a, DCGLvertex *b, float weight, DCGLvertex *shading, float w, DCGLvertex *d)
{
	d->x = dcglLerp(a->x, b->x, weight);
	d->y = dcglLerp(a->y, b->y, weight);
	d->z = w;
	d->w = w;
	d->u = dcglLerp(a->u, b->u, weight);
	d->v = dcglLerp(a->v, b->v, weight);
	d->fa = shading->fa;
	d->fr = shading->fr;
	d->fg = shading->fg;
	d->fb = shading->fb;
	if (0) {
		d->fogd = shading->fogd;
		d->fro = shading->fro;
		d->fgo = shading->fgo;
		d->fbo = shading->fbo;
	}
}

int dcglClipLineGouraud(DCGLvertex *a, DCGLvertex *b, DCGLvertex *provoking, DCGLvertex *dst) {
	float a_dist = dcgl_prim.nearclip - a->w, b_dist = dcgl_prim.nearclip - b->w;
	int a_in = a_dist < 0, b_in = b_dist < 0;	//TODO double check this
	if (a_in && b_in) {
		//both in
		dcglCopyVertex(a,dst);
		dcglCopyVertex(b,dst+1);
		return 2;
	} else if (a_in) {
		//going out, intersection from a to b
		float insection = dcglInsectionPoint(a_dist, b_dist);
		dcglCopyVertex(a,dst);
		dcglWriteIntersectionGouraud(a,b,insection,dcgl_prim.nearclip,dst+1);
		return 2;
	} else if (b_in) {
		//going in, intersection from a to b, then b
		float insection = dcglInsectionPoint(b_dist, a_dist);
		dcglWriteIntersectionGouraud(b,a,insection,dcgl_prim.nearclip,dst);
		dcglCopyVertex(b,dst+1);
		return 2;
	} else {
		//both out
		return 0;
	}
}

int dcglClipLineFlat(DCGLvertex *a, DCGLvertex *b, DCGLvertex *provoking, DCGLvertex *dst) {
	float a_dist = dcgl_prim.nearclip - a->w, b_dist = dcgl_prim.nearclip - b->w;
	int a_in = a_dist < 0, b_in = b_dist < 0;	//TODO double check this
	if (a_in && b_in) {
		//both in
		dcglCopyVertex(a,dst);
		dcglCopyVertex(b,dst+1);
		return 2;
	} else if (a_in) {
		//going out, intersection from a to b
		float insection = dcglInsectionPoint(a_dist, b_dist);
		dcglCopyVertex(a,dst);
		dcglWriteIntersectionFlat(a,b,insection,provoking,dcgl_prim.nearclip,dst+1);
		return 2;
	} else if (b_in) {
		//going in, intersection from a to b, then b
		float insection = dcglInsectionPoint(b_dist, a_dist);
		dcglWriteIntersectionFlat(b,a,insection,provoking,dcgl_prim.nearclip,dst);
		dcglCopyVertex(b,dst+1);
		return 2;
	} else {
		//both out
		return 0;
	}
}

int dcglClipEdgeGouraud(DCGLvertex *a, DCGLvertex *b, DCGLvertex *provoking, DCGLvertex *dst) {
	float a_dist = dcgl_prim.nearclip - a->w, b_dist = dcgl_prim.nearclip - b->w;
	int a_in = a_dist < 0, b_in = b_dist < 0;
	if (a_in && b_in) {
		//both in
		dcglCopyVertex(b,dst);
		return 1;
	} else if (a_in) {
		//going out, intersection from a to b
		float insection = dcglInsectionPoint(a_dist, b_dist);
		dcglWriteIntersectionGouraud(a,b,insection,dcgl_prim.nearclip,dst);
		return 1;
	} else if (b_in) {
		//going in, intersection from a to b, then b
		float insection = dcglInsectionPoint(b_dist, a_dist);
		dcglWriteIntersectionGouraud(b,a,insection,dcgl_prim.nearclip,dst);
		dcglCopyVertex(b,dst+1);
		return 2;
	} else {
		//both out
		return 0;
	}
}

int dcglClipEdgeFlat(DCGLvertex *a, DCGLvertex *b, DCGLvertex *provoking, DCGLvertex *dst) {
	float a_dist = dcgl_prim.nearclip - a->w, b_dist = dcgl_prim.nearclip - b->w;
	int a_in = a_dist < 0, b_in = b_dist < 0;
	if (a_in && b_in) {
		//both in
		dcglCopyVertex(b,dst);
		return 1;
	} else if (a_in) {
		//going out, intersection from a to b
		float insection = dcglInsectionPoint(a_dist, b_dist);
		dcglWriteIntersectionFlat(a,b,insection,provoking,dcgl_prim.nearclip,dst);
		return 1;
	} else if (b_in) {
		//going in, intersection from a to b, then b
		float insection = dcglInsectionPoint(b_dist, a_dist);
		dcglWriteIntersectionFlat(b,a,insection,provoking,dcgl_prim.nearclip,dst);
		dcglCopyVertex(b,dst+1);
		return 2;
	} else {
		//both out
		return 0;
	}
}

int dcglClipEdgeNull(DCGLvertex *a, DCGLvertex *b, DCGLvertex *provoking, DCGLvertex *dst) {
	dcglCopyVertex(b,dst);
	return 1;
}

static void dcglSendQuad(DCGLvertex *verts) {
	dcglLogSubmit("call");
	
	DCGLpvrvertex *dmode = dcglDirectModeEngage();
	dmode = dcglSubmitManyQuad(dcglParams(verts), sizeof(*verts), 1, dmode);
	dcglSetCmdbuffPosSQ((char*)dmode);
}

static void dcglSend2TriStrip(DCGLvertex *verts) {
	DCGLpvrvertex *dmode = dcglDirectModeEngage();
	dmode = dcglSubmitMany(dcglParams(verts), sizeof(*verts), 4, 4, dmode);
	dcglSetCmdbuffPosSQ((char*)dmode);
}

static inline void dcglSend2TriStripNoPersp(DCGLvertex *verts) {
	DCGLpvrvertex *dmode = dcglDirectModeEngage();
	dcglSubmitNoPerspReal(verts  , dmode, PVR_CMD_VERTEX);
	dcglSubmitNoPerspReal(verts+1, dmode, PVR_CMD_VERTEX);
	dcglSubmitNoPerspReal(verts+2, dmode, PVR_CMD_VERTEX);
	dcglSubmitNoPerspReal(verts+3, dmode, PVR_CMD_VERTEX_EOL);
}

static inline void dcglSendTriangle(DCGLvertex *verts) {
	DCGLpvrvertex *dmode = dcglDirectModeEngage();
	dmode = dcglSubmitMany(dcglParams(verts), sizeof(*verts), 3, 3, dmode);
	dcglSetCmdbuffPosSQ((char*)dmode);
}

#define DCGL_POINT_SIZE (10.0f)
static inline void dcglSendPoint(const DCGLvertex *point) {
	dcglBuggy();
	DCGLvertex pointbuff[4];
	
	dcglCopyVertex(point, pointbuff  );
	dcglCopyVertex(point, pointbuff+1);
	dcglCopyVertex(point, pointbuff+2);
	dcglCopyVertex(point, pointbuff+3);
	dcglPerspective(pointbuff  );
	dcglPerspective(pointbuff+1);
	dcglPerspective(pointbuff+2);
	dcglPerspective(pointbuff+3);
	
	pointbuff[0].y -= DCGL_POINT_SIZE;
	pointbuff[2].y -= DCGL_POINT_SIZE;
	pointbuff[2].x += DCGL_POINT_SIZE;
	pointbuff[3].x += DCGL_POINT_SIZE;
	
	dcglSend2TriStripNoPersp(pointbuff);
}

void dcglSendTrifan(DCGLvertex *verts, int vertcnt) {
	//submits a preclipped/unclipped triangle fan
	//since the pvr doesn't support trifans natively, send as pairs of triangles
	DCGLvertex *curvert = verts+1;
	int tricnt = vertcnt - 2;
	
	if (tricnt < 0)
		return;
	//handle odd number of triangles
	DCGLpvrvertex *dmode = dcglDirectModeEngage();
	if (tricnt & 1) {
		dcglSumbitOne(&verts->x, &verts->u, &verts->fr,       PVR_CMD_VERTEX,     dmode+0);
		dcglSumbitOne(&curvert->x, &curvert->u, &curvert->fr, PVR_CMD_VERTEX,     dmode+1);
		curvert++;
		dcglSumbitOne(&curvert->x, &curvert->u, &curvert->fr, PVR_CMD_VERTEX_EOL, dmode+2);
		dmode += 3;
		tricnt--;
	}
	//submit pairs of triangles as 2 tri strips
	while(tricnt > 0) {
		dcglSumbitOne(&curvert->x, &curvert->u, &curvert->fr, PVR_CMD_VERTEX,     dmode+0);
		curvert++;
		dcglSumbitOne(&curvert->x, &curvert->u, &curvert->fr, PVR_CMD_VERTEX,     dmode+1);
		dcglSumbitOne(&verts->x, &verts->u, &verts->fr,       PVR_CMD_VERTEX,     dmode+2);
		curvert++;
		dcglSumbitOne(&curvert->x, &curvert->u, &curvert->fr, PVR_CMD_VERTEX_EOL, dmode+3);
		
		dmode += 4;
		tricnt -= 2;
	}
	dcglSetCmdbuffPosSQ((char*)dmode);
}

void dcglSendTrifan2(DCGLvertex *verts, int vertcnt) {
	//submits a preclipped/unclipped triangle fan
	//since the pvr doesn't support trifans natively, send as pairs of triangles
	DCGLvertex *curvert = verts+1;
	DCGLvertex *root = verts;
	int tricnt = vertcnt - 2;
	
	if (tricnt < 0)
		return;
	//handle odd number of triangles
	DCGLpvrvertex *dmode = dcglDirectModeEngage();
	
	//submit pairs of triangles as 2 tri strips
	while(tricnt > 0) {
		dcglSumbitOne(&curvert->x, &curvert->u, &curvert->fr, PVR_CMD_VERTEX,     dmode+0);
		curvert++;
		dcglSumbitOne(&curvert->x, &curvert->u, &curvert->fr, PVR_CMD_VERTEX,     dmode+1);
		dcglSumbitOne(&root->x, &root->u, &root->fr,       PVR_CMD_VERTEX,     dmode+2);
		curvert++;
		dcglSumbitOne(&curvert->x, &curvert->u, &curvert->fr, PVR_CMD_VERTEX_EOL, dmode+3);
		
		dmode += 4;
		tricnt -= 2;
	}
	
	if (tricnt) {
		dcglSumbitOne(&root->x, &root->u, &root->fr,       PVR_CMD_VERTEX,     dmode+0);
		dcglSumbitOne(&curvert->x, &curvert->u, &curvert->fr, PVR_CMD_VERTEX,     dmode+1);
		curvert++;
		dcglSumbitOne(&curvert->x, &curvert->u, &curvert->fr, PVR_CMD_VERTEX_EOL, dmode+2);
		dmode += 3;
		tricnt--;
	}
	dcglSetCmdbuffPosSQ((char*)dmode);
}

#define DCGL_LINE_HALFWIDTH (5.0f)
void dcglSendLine(DCGLvertex *start, DCGLvertex *end) {
	dcglBuggy();
	DCGLvertex clipbuff[4];
	
	dcglCopyVertex(start, clipbuff  );
	dcglCopyVertex(start, clipbuff+1);
	dcglCopyVertex(end,   clipbuff+2);
	dcglCopyVertex(end,   clipbuff+3);
	dcglPerspective(clipbuff  );
	dcglPerspective(clipbuff+1);
	dcglPerspective(clipbuff+2);
	dcglPerspective(clipbuff+3);
	
	float dx = fabsf(end->x - start->x);
	float dy = fabsf(end->y - start->y);
	int closer_to_horizontal = dx > dy;
	
	if (closer_to_horizontal) {
		clipbuff[0].y += -DCGL_LINE_HALFWIDTH;
		clipbuff[1].y +=  DCGL_LINE_HALFWIDTH;
		clipbuff[2].y += -DCGL_LINE_HALFWIDTH;
		clipbuff[3].y +=  DCGL_LINE_HALFWIDTH;
	} else {
		clipbuff[0].x += -DCGL_LINE_HALFWIDTH;
		clipbuff[1].x +=  DCGL_LINE_HALFWIDTH;
		clipbuff[2].x += -DCGL_LINE_HALFWIDTH;
		clipbuff[3].x +=  DCGL_LINE_HALFWIDTH;
	}
	dcglSend2TriStrip(clipbuff);
}

void dcglLineWithClip(DCGLvertex *start, DCGLvertex *end) {
	DCGLvertex clip[2];
	
	dcglHardAssert(dcgl_prim.lineclipper);
	
	if (dcgl_prim.lineclipper(start, end, end, clip))
		dcglSendLine(clip,clip+1);
}

void dcglTriangleWithClip(DCGLvertex *v0, DCGLvertex *v1, DCGLvertex *v2) {
	DCGLvertex clipbuff[8];
	int vertcnt = 0;
	
	vertcnt += dcgl_prim.polyclipper(v0, v1, v2, clipbuff+vertcnt);
	vertcnt += dcgl_prim.polyclipper(v1, v2, v2, clipbuff+vertcnt);
	vertcnt += dcgl_prim.polyclipper(v2, v0, v2, clipbuff+vertcnt);
	
	if (vertcnt == 3) {
		dcglSendTriangle(clipbuff);
	} else if (vertcnt == 4) {
		dcglSendQuad(clipbuff);
	} else {
		;
	}
}

void dcglQuadWithClip(DCGLvertex *v0, DCGLvertex *v1, DCGLvertex *v2, DCGLvertex *v3) {
	DCGLvertex clipbuff[8];
	int vertcnt = 0;
	
	vertcnt += dcgl_prim.polyclipper(v0, v1, v3, clipbuff+vertcnt);
	vertcnt += dcgl_prim.polyclipper(v1, v2, v3, clipbuff+vertcnt);
	vertcnt += dcgl_prim.polyclipper(v2, v3, v3, clipbuff+vertcnt);
	vertcnt += dcgl_prim.polyclipper(v3, v0, v3, clipbuff+vertcnt);
	
	dcglLogSubmit("call %i verts", vertcnt);
	if (vertcnt == 3) {
		dcglSendTriangle(clipbuff);
	} else if (vertcnt == 4) {
		dcglSendQuad(clipbuff);
	} else if (vertcnt > 4) {
		dcglSendTrifan(clipbuff,vertcnt);
	} else {
		
	}
}

void dcglEndTriangles() {
	if (dcglVertBuffNeedsClipping()) {
		//pointer math is so that incomplete triangles are not sent
		DCGLvertex *curvert = dcgl_prim.vert_buff+2;
		while (curvert < dcgl_prim.cur_vert) {
			dcglTriangleWithClip(curvert-2,curvert-1,curvert);
			curvert += 3;
		}
	} else {
		if (dcglVerticesInBuffer() >= 3) {
			//TODO handle incomplete triangles correctly
			DCGLpvrvertex *dmode = dcglDirectModeEngage();
			//dmode = dcglSubmitMany(dcglParams(dcgl_prim.vert_buff),
			dmode = dcglSubmitPerspective(dcglParams(dcgl_prim.vert_buff),
				sizeof(*dcgl_prim.vert_buff), 3, dcglVerticesInBuffer(), dmode);
			dcglSetCmdbuffPosSQ((char*)dmode);
		}
	}
}

void dcglEndQuads() {
	//pointer math is so that incomplete quads are not sent
	DCGLvertex *curvert = dcgl_prim.vert_buff+3;
	if (dcglVertBuffNeedsClipping()) {
		dcglLogSubmit("clip");
		while (curvert < dcgl_prim.cur_vert) {
			dcglQuadWithClip(curvert-3,curvert-2,curvert-1,curvert);
			curvert += 4;
		}
	} else {
		dcglLogSubmit("noclip");
		//dcglLog("noclip");
		if (dcglVerticesInBuffer() >= 4) {
			DCGLpvrvertex *dmode = dcglDirectModeEngage();
			dmode = dcglSubmitManyQuad(dcglParams(dcgl_prim.vert_buff),
				sizeof(*dcgl_prim.vert_buff), dcglVerticesInBuffer()/4, dmode);
			dcglSetCmdbuffPosSQ((char*)dmode);
		}
	}
}

void dcglEndQuadStrip() {
	//pointer math is so that incomplete quads are not sent
	DCGLvertex *curvert = dcgl_prim.vert_buff+3;
	if (dcglVertBuffNeedsClipping()) {
		while (curvert < dcgl_prim.cur_vert) {
			dcglQuadWithClip(curvert-3,curvert-2,curvert-1,curvert);
			curvert += 2;
		}
	} else {
		//wonderful, a nonclipped strip! dreamcast is happy
		if (dcglVerticesInBuffer() >= 4) {
			DCGLpvrvertex *dmode = dcglDirectModeEngage();
			dmode = dcglSubmitPerspective(dcglParams(dcgl_prim.vert_buff),
				sizeof(*dcgl_prim.vert_buff), dcglVerticesInBuffer(), dcglVerticesInBuffer(), dmode);
			dcglSetCmdbuffPosSQ((char*)dmode);
		}
	}
}

void dcglEndTriangleStrip() {
	if (dcglVertBuffNeedsClipping()) {
		//pointer math is so that incomplete triangles are not sent
		DCGLvertex *curvert = dcgl_prim.vert_buff+2;
		while (curvert < dcgl_prim.cur_vert) {
			dcglTriangleWithClip(curvert-2,curvert-1,curvert);
			curvert++;
		}
	} else {
		//wonderful, a nonclipped strip! dreamcast is happy
		if (dcglVerticesInBuffer() >= 3) {
			DCGLpvrvertex *dmode = dcglDirectModeEngage();
			//dmode = dcglSubmitMany(dcglParams(dcgl_prim.vert_buff),
			dmode = dcglSubmitPerspective(dcglParams(dcgl_prim.vert_buff),
				sizeof(*dcgl_prim.vert_buff), dcglVerticesInBuffer(), dcglVerticesInBuffer(), dmode);
			dcglSetCmdbuffPosSQ((char*)dmode);
		}
	}
}

void dcglEndTriangleFan() {
	if (dcglVertBuffNeedsClipping()) {
		//pointer math is so that incomplete triangles are not sent
		DCGLvertex *curvert = dcgl_prim.vert_buff+2;
		while (curvert < dcgl_prim.cur_vert) {
			dcglTriangleWithClip(dcgl_prim.vert_buff,curvert-1,curvert);
			curvert++;
		}
	} else {
		dcglSendTrifan(dcgl_prim.vert_buff,dcglVerticesInBuffer());
	}
}

void dcglEndLines() {
	//pointer math is so that incomplete lines are not sent
	DCGLvertex *curvert = dcgl_prim.vert_buff+1;
	if (dcglVertBuffNeedsClipping()) {
		while (curvert < dcgl_prim.cur_vert) {
			dcglLineWithClip(curvert-1,curvert);
			curvert += 2;
		}
	} else {
		while (curvert < dcgl_prim.cur_vert) {
			dcglSendLine(curvert-1,curvert);
			curvert += 2;
		}
	}
}

void dcglEndLineStrip() {
	if (dcglVertBuffNeedsClipping()) {
		DCGLvertex *curvert = dcgl_prim.vert_buff+1;
		while (curvert < dcgl_prim.cur_vert) {
			dcglLineWithClip(curvert-1,curvert);
			curvert++;
		}
	} else {
		DCGLvertex *curvert = dcgl_prim.vert_buff+1;
		while (curvert < dcgl_prim.cur_vert) {
			dcglSendLine(curvert-1,curvert);
			curvert++;
		}
	}
}

void dcglEndLineLoop() {
	if (dcglVertBuffNeedsClipping()) {
		DCGLvertex *curvert = dcgl_prim.vert_buff+1;
		while (curvert < dcgl_prim.cur_vert) {
			dcglLineWithClip(curvert-1,curvert);
			curvert++;
		}
		dcglLineWithClip(curvert-1,dcgl_prim.vert_buff);
	} else {
		DCGLvertex *curvert = dcgl_prim.vert_buff+1;
		while (curvert < dcgl_prim.cur_vert) {
			dcglSendLine(curvert-1,curvert);
			curvert++;
		}
		dcglSendLine(curvert-1,dcgl_prim.vert_buff);
	}
}

void dcglEndPoints() {
	if (dcglVertBuffNeedsClipping()) {
		DCGLvertex *curvert = dcgl_prim.vert_buff;
		while (curvert < dcgl_prim.cur_vert) {
			if (dcgl_prim.cur_vert->w < dcgl_prim.nearclip)
				dcglSendPoint(curvert);
			curvert++;
		}
	} else {
		DCGLvertex *curvert = dcgl_prim.vert_buff;
		while (curvert < dcgl_prim.cur_vert) {
			dcglSendPoint(curvert);
			curvert++;
		}
	}
}

void glEnd() {
	static __attribute__((aligned(32))) matrix_t color_swizzle = {
		{0,1,0,0},
		{0,0,1,0},
		{0,0,0,1},
		{1,0,0,0}};
	
	dcglLogSubmit("call");
	dcglCheckInScene(return);
	if (0) {
		int vert_cnt = dcglVerticesInBuffer();
		dcglTransformVectorsMat(&dcgl_prim.vert_buff[0].x, sizeof(*dcgl_prim.vert_buff),
				      &dcgl_prim.vert_buff[0].x, sizeof(*dcgl_prim.vert_buff),
				      vert_cnt, dcglFullMatrixPtr());
		dcglTransformVectorsMat(&dcgl_prim.vert_buff[0].fr, sizeof(*dcgl_prim.vert_buff),
				      &dcgl_prim.vert_buff[0].fr, sizeof(*dcgl_prim.vert_buff),
				      vert_cnt, &color_swizzle);
	} else {
		dcglLoadFullMatrix();
		int vert_cnt = dcglVerticesInBuffer();
		dcglTransformVectors(&dcgl_prim.vert_buff[0].x, sizeof(*dcgl_prim.vert_buff),
				      &dcgl_prim.vert_buff[0].x, sizeof(*dcgl_prim.vert_buff),
				      vert_cnt);
		dcglTransformVectorsMat(&dcgl_prim.vert_buff[0].fr, sizeof(*dcgl_prim.vert_buff),
				      &dcgl_prim.vert_buff[0].fr, sizeof(*dcgl_prim.vert_buff),
				      vert_cnt, &color_swizzle);
	}
	dcglLoadContext();
	
	//TODO check dcgl_cull.cullface for front_and_back
	dcgl_prim.endcb();
	dcgl_prim.in_begin = 0;
}

void glLineWidth(GLfloat width) {
	//TODO
	dcglLog("Unimplemented");
}

#if 0
#include <stdlib.h>

void dcglWriteTest(DCGLvertex *testverts, int cnt) {
	do {
		testverts->x = cnt+1;
		testverts->y = cnt*1.25+1;
		testverts->z = cnt*2;
		testverts->w = cnt*4;
		testverts->u = 20+cnt;
		testverts->v = 40+cnt;
		testverts->p = -30+cnt;
		testverts->q = -50+cnt;
		testverts->fa = 60+cnt;
		testverts->fr = 80+cnt;
		testverts->fg = 100+cnt;
		testverts->fb = 120+cnt;
		testverts->fogd =140+cnt;
		testverts->fro = 160+cnt;
		testverts->fgo = 180+cnt;
		testverts->fbo = 200+cnt;
		testverts++;
	} while(--cnt);
}

void dcglTest() {
	__attribute__((aligned(32))) DCGLvertex src[6];
	__attribute__((aligned(32))) DCGLpvrvertex result[6];
	DCGLpvrvertex *curvert;
	int i;
	
	dcglWriteTest(src,6);
	memset(result, 0, sizeof(result));
	curvert = dcglSubmitMany(dcglParams(src), sizeof(DCGLvertex), 2, 4, result);
	i = 6;
	dcglLog("C: %p %p %i", result, curvert, curvert-result);
	curvert = result;
	do {
		printf("%08x\n(%6.4f, %6.4f, %6.4f) (%3.2f %3.2f)\n", curvert->cmd, curvert->x, curvert->y, curvert->z, curvert->u, curvert->v);
		printf("(%f %f %f %f) ",curvert->fa,curvert->fr,curvert->fg,curvert->fb);
		printf("(%f %f %f %f)\n",curvert->fogd,curvert->fro,curvert->fgo,curvert->fbo);
		curvert++;
	} while(--i);
	
	dcglWriteTest(src,6);
	memset(result, 0, sizeof(result));
	curvert = dcglSubmitPerspective(dcglParams(src), sizeof(DCGLvertex), 2, 4, result);
	dcglLog("ASM: %p %p %i", result, curvert, curvert-result);
	curvert = result;
	i = 6;
	do {
		printf("%08x\n(%6.4f, %6.4f, %6.4f) (%3.2f %3.2f)\n", curvert->cmd, curvert->x, curvert->y, curvert->z, curvert->u, curvert->v);
		printf("(%f %f %f %f) ",curvert->fa,curvert->fr,curvert->fg,curvert->fb);
		printf("(%f %f %f %f)\n",curvert->fogd,curvert->fro,curvert->fgo,curvert->fbo);
		curvert++;
	} while(--i);
	
	exit(0);
}
#endif
