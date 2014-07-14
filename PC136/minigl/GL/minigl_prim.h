#ifndef MINIGL_PRIM_H
#define MINIGL_PRIM_H

#ifdef __cplusplus
extern "C" {
#endif

#define dcgl_ftrv(x, y, z, w) { \
        register float __x __asm__("fr4") = (x); \
        register float __y __asm__("fr5") = (y); \
        register float __z __asm__("fr6") = (z); \
        register float __w __asm__("fr7") = (w); \
        __asm__ __volatile__( \
                              "ftrv   xmtrx,fv4\n" \
                              : "=f" (__x), "=f" (__y), "=f" (__z), "=f" (__w) \
                              : "0" (__x), "1" (__y), "2" (__z), "3" (__w) ); \
        x = __x; y = __y; z = __z; w = __w; \
    }

#define dcgl_movca(addr) { \
        register int __zero __asm__("r0") = 0; \
        __asm__ __volatile__( "movca.l   r0,@%0\n" : : "r" (__zero), "r" (addr) : "memory" ); \
    }

#undef dcgl_movca
#define dcgl_movca(addr)

#define sq_calc_address_control_register_value(dst)  (((((unsigned int)dst) >> 26) << 2) & 0x1c)
#define sq_set_address_control_registers(dst) do { QACR0 = QACR1 = sq_calc_address_control_register_value(dst); } while(0)
#define sq_get_store_queue_address(dst) ((void*)(0xe0000000 | (((unsigned long)dst) & 0x03ffffe0)))

static inline void * sq_prepare(void *dst) {
	dcglHardAssertExtra( !(((uintptr_t)dst) & 0x1f) );
	sq_set_address_control_registers(dst);
	return sq_get_store_queue_address(dst);
}

typedef struct {
	float nearclip;	//distance of near clip plane, must be >0
	DCGLclip polyclipper;
	DCGLclip lineclipper;
	DCGLcallback endcb;
	DCGLvertex *cur_vert;
	int in_begin;
	int in_scene;
	__attribute__((aligned(32))) pvr_context current_context;
	__attribute__((aligned(32))) DCGLvertex vert_params;
	__attribute__((aligned(32))) DCGLvertex vert_buff[DCGL_VERT_BUFF_SIZE];
} DCGLprimstate;
extern DCGLprimstate dcgl_prim;

void dcglSubmitState();
static inline void dcglLoadContext()
{
	dcglSubmitState();
}

static inline void dcglCopyVertexParams(DCGLvertex *src, DCGLvertex *dst)
{
	dst->u = src->u;
	dst->v = src->v;
	dcgl_movca(&dst->fa);
	dst->fa = src->fa;
	dst->fr = src->fr;
	dst->fg = src->fg;
	dst->fb = src->fb;
}

static inline void dcglResetVertBuff(void)
{
	dcgl_prim.cur_vert = dcgl_prim.vert_buff;
}


static inline void dcglFlatClip()
{
	extern int dcglClipEdgeFlat(DCGLvertex *a, DCGLvertex *b, DCGLvertex *provoking, DCGLvertex *dst);
	extern int dcglClipLineFlat(DCGLvertex *a, DCGLvertex *b, DCGLvertex *provoking, DCGLvertex *dst);
	dcgl_prim.polyclipper = dcglClipEdgeFlat;
	dcgl_prim.lineclipper = dcglClipLineFlat;
}
static inline void dcglGouraudClip()
{
	extern int dcglClipEdgeGouraud(DCGLvertex *a, DCGLvertex *b, DCGLvertex *provoking, DCGLvertex *dst);
	extern int dcglClipLineGouraud(DCGLvertex *a, DCGLvertex *b, DCGLvertex *provoking, DCGLvertex *dst);
	dcgl_prim.polyclipper = dcglClipEdgeGouraud;
	dcgl_prim.lineclipper = dcglClipLineGouraud;
}
static inline void dcglNoClip()
{
	extern int dcglClipEdgeNull(DCGLvertex *a, DCGLvertex *b, DCGLvertex *provoking, DCGLvertex *dst);
	dcgl_prim.polyclipper = dcglClipEdgeNull;
}

static inline void glBegin(GLenum type)
{
	dcglCheckInSceneExtra();
	dcglNotInBegin(return);
	
	dcgl_prim.in_begin = 1;
	dcglResetVertBuff();
	
	extern void dcglEndTriangles();
	extern void dcglEndQuads();
	extern void dcglEndQuadStrip();
	extern void dcglEndTriangleStrip();
	extern void dcglEndTriangleFan();
	extern void dcglEndLines();
	extern void dcglEndLineStrip();
	extern void dcglEndLineLoop();
	extern void dcglEndPoints();
	
	switch(type) {
	case GL_LINES:
		dcgl_prim.endcb = dcglEndLines;
		break;
	case GL_LINE_STRIP:
		dcgl_prim.endcb = dcglEndLineStrip;
		break;
	case GL_POINTS:
		dcgl_prim.endcb = dcglEndPoints;
		break;
	case GL_LINE_LOOP:
		dcgl_prim.endcb = dcglEndLineLoop;
		break;
	case GL_TRIANGLES:
		dcgl_prim.endcb = dcglEndTriangles;
		break;
	case GL_QUADS:
		dcgl_prim.endcb = dcglEndQuads;
		break;
	case GL_TRIANGLE_STRIP:
		dcgl_prim.endcb = dcglEndTriangleStrip;
		break;
	case GL_POLYGON:
	case GL_TRIANGLE_FAN:
		dcgl_prim.endcb = dcglEndTriangleFan;
		break;
	case GL_QUAD_STRIP:
		dcgl_prim.endcb = dcglEndQuadStrip;
		break;
	default:
		assert_msg(0, "unsupported prim type");
	}
}

static inline void dcglCopyVertex(const DCGLvertex *src, DCGLvertex *dst)
{
	__builtin_memcpy(dst,src,sizeof(*src));
	//fcopy32(src,dst);
	//fcopy32(((char*)src)+32,((char*)dst)+32);
}


static inline void glTexCoord2f(float u, float v)
{
	dcgl_prim.vert_params.u = u;
	dcgl_prim.vert_params.v = v;
}

#if 0
	static inline void dcglTexCoord4f(float s, float t, float r, float q)
	{
		dcgl_prim.vert_params.u = s;
		dcgl_prim.vert_params.v = t;
	}
#endif

static inline void glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
	dcgl_prim.vert_params.fr = r;
	dcgl_prim.vert_params.fg = g;
	dcgl_prim.vert_params.fb = b;
	dcgl_prim.vert_params.fa = a;
}
static inline void glColor3f(GLfloat r, GLfloat g, GLfloat b) {
	glColor4f(r,g,b,1);
}

static inline void glFogCoordf(GLfloat dist)
{
	dcgl_prim.vert_params.fogd = dcgl_prim.cur_vert->fogd = dist;
}

static inline void glSecondaryColor3f(GLfloat r, GLfloat g, GLfloat b)
{
	dcgl_prim.vert_params.fro = r;
	dcgl_prim.vert_params.fgo = g;
	dcgl_prim.vert_params.fbo = b;
}
    
static inline void glVertex4f(float x, float y, float z, float w)
{
	dcgl_movca(dcgl_prim.cur_vert);
	dcglCopyVertexParams(&dcgl_prim.vert_params, dcgl_prim.cur_vert);
	dcgl_prim.cur_vert->x = x;
	dcgl_prim.cur_vert->y = y;
	dcgl_prim.cur_vert->z = z;
	dcgl_prim.cur_vert->w = w;
	dcgl_prim.cur_vert++;
	
	//assert(dcgl_prim.cur_vert < (dcgl_prim.vert_buff + DCGL_VERT_BUFF_SIZE));
}

static inline void glVertex3f(float x, float y, float z) {
	glVertex4f(x,y,z,1);
}

static inline void glVertex2f(float x, float y) {
	glVertex4f(x,y,0,1);
}

static inline void glNormal3f(float x, float y, float z) {
	;
}
#define glNormal3i(x,y,z) glNormal3f(x,y,z)

void glEnd();

static inline void glRectf(float x1, float y1, float x2, float y2)
{
	glBegin(GL_POLYGON);
	glVertex2f(x1, y1);
	glVertex2f(x2, y1);
	glVertex2f(x2, y2);
	glVertex2f(x1, y2);
	glEnd();
}

#if 1

#define dcglColorMacro(tag, type, scale) \
static inline void glColor3 ## tag (type r, type g, type b)		{ glColor3f(r*(1.0f/scale),g*(1.0f/scale),b*(1.0f/scale)); } \
static inline void glColor4 ## tag (type r, type g, type b, type a)	{ glColor4f(r*(1.0f/scale),g*(1.0f/scale),b*(1.0f/scale),a*(1.0f/scale)); } \
static inline void glColor3 ## tag ## v (const type *c)		{ glColor3f(c[0]*(1.0f/scale), c[1]*(1.0f/scale), c[2]*(1.0f/scale)); } \
static inline void glColor4 ## tag ## v (const type *c)		{ glColor4f(c[0]*(1.0f/scale), c[1]*(1.0f/scale), c[2]*(1.0f/scale), c[3]*(1.0f/scale)); }

#define dcglTexCoord2Macro(tag, type, scale) \
static inline void glTexCoord2 ## tag (type x, type y)		{ glTexCoord2f(x*(1.0f/scale),y*(1.0f/scale)); } \
static inline void glTexCoord2 ## tag ## v (const type *c)		{ glTexCoord2f(c[0]*(1.0f/scale), c[1]*(1.0f/scale)); }

#define dcglTexCoordMacro(tag, type, scale) \
static inline void glTexCoord1 ## tag (type x)				{ glTexCoord2f(x*(1.0f/scale), 0); } \
static inline void glTexCoord3 ## tag (type x, type y, type z)		{ glTexCoord2f(x*(1.0f/scale),y*(1.0f/scale)); } \
static inline void glTexCoord4 ## tag (type x, type y, type z, type w)	{ glTexCoord2f(x*(1.0f/scale),y*(1.0f/scale)); } \
static inline void glTexCoord1 ## tag ## v (const type *c)			{ glTexCoord2f(c[0]*(1.0f/scale), 0); } \
static inline void glTexCoord3 ## tag ## v (const type *c)			{ glTexCoord2f(c[0]*(1.0f/scale), c[1]*(1.0f/scale)); } \
static inline void glTexCoord4 ## tag ## v (const type *c)			{ glTexCoord2f(c[0]*(1.0f/scale), c[1]*(1.0f/scale)); }

#define dcglVertexMacro(tag, type, scale) \
static inline void glVertex2 ## tag (type x, type y)			{ glVertex3f(x*(1.0f/scale),y*(1.0f/scale), 0); } \
static inline void glVertex3 ## tag (type x, type y, type z)		{ glVertex3f(x*(1.0f/scale),y*(1.0f/scale), z*(1.0f/scale)); } \
static inline void glVertex4 ## tag (type x, type y, type z, type w)	{ glVertex4f(x*(1.0f/scale),y*(1.0f/scale), z*(1.0f/scale), w*(1.0f/scale)); } \
static inline void glVertex2 ## tag ## v (const type *c)		{ glVertex3f(c[0]*(1.0f/scale), c[1]*(1.0f/scale), 0); } \
static inline void glVertex3 ## tag ## v (const type *c)		{ glVertex3f(c[0]*(1.0f/scale), c[1]*(1.0f/scale), c[2]*(1.0f/scale)); } \
static inline void glVertex4 ## tag ## v (const type *c)		{ glVertex4f(c[0]*(1.0f/scale), c[1]*(1.0f/scale), c[2]*(1.0f/scale), c[3]*(1.0f/scale)); }

dcglVertexMacro(s, GLshort, 32768.)
dcglVertexMacro(i, GLint, 2147483648.)
dcglVertexMacro(d, GLdouble, 1.)

dcglTexCoord2Macro(s, GLshort, 32768.)
dcglTexCoord2Macro(i, GLint, 2147483648.)
dcglTexCoord2Macro(d, GLdouble, 1.)

dcglTexCoordMacro(s, GLshort, 32768.)
dcglTexCoordMacro(us, GLushort, 65536.)
dcglTexCoordMacro(i, GLint, 2147483648.)
dcglTexCoordMacro(ui, GLint, 4294967296.)
dcglTexCoordMacro(d, GLdouble, 1.)
//glTexCoordMacro(f, GLfloat, 1.)

dcglColorMacro(b, GLbyte, 128.)
dcglColorMacro(ub, GLubyte, 256.)
dcglColorMacro(s, GLshort, 32768.)
dcglColorMacro(us, GLushort, 65536.)
dcglColorMacro(i, GLint, 2147483648.)
dcglColorMacro(ui, GLint, 4294967296.)
dcglColorMacro(d, GLdouble, 1.)

#define glVertex2f(x,y) glVertex3f(x,y,0)
#define glVertex2i(x,y) glVertex3f(x,y,0)
static inline void glColor4fv (const GLfloat *c)	{
	glColor4f(c[0], c[1], c[2], c[3]);
}

static inline void glRectd(GLdouble  x1,  GLdouble  y1,  GLdouble  x2,  GLdouble  y2) { glRectf(x1, y1, x2, y2); }
static inline void glRecti(GLint  x1,  GLint  y1,  GLint  x2,  GLint  y2) { glRectf(x1, y1, x2, y2); }
static inline void glRects(GLshort  x1,  GLshort  y1,  GLshort  x2,  GLshort  y2) { glRectf(x1, y1, x2, y2); }
static inline void glRectdv(const GLdouble *  v1,  const GLdouble *  v2) { glRectf(v1[0], v1[1], v2[0], v2[1]); }
static inline void glRectfv(const GLfloat *  v1,  const GLfloat *  v2) { glRectf(v1[0], v1[1], v2[0], v2[1]); }
static inline void glRectiv(const GLint *  v1,  const GLint *  v2) { glRectf(v1[0], v1[1], v2[0], v2[1]); }
static inline void glRectsv(const GLshort *  v1,  const GLshort *  v2) { glRectf(v1[0], v1[1], v2[0], v2[1]); }

#endif

#ifdef __cplusplus
}
#endif

#endif
