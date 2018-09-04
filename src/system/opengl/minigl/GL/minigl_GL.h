#include "minigl.h"

#if 0
	#define glBegin(type) dcglBegin(type)
	#define glEnd(type) dcglEnd()
	#define glTexCoord2f(u,v) dcglTexCoord2f(u,v)
	#define glColor4f(r,g,b,a) dcglColor4f(r,g,b,a)
	#define glColor3f(r,g,b) dcglColor3f(r,g,b)
	#define glVertex3f(x,y,z) dcglVertex3f(x,y,z)
	#define glNormal3f(x,y,z) dcglNormal3f(x,y,z)
	#define glNormal3i(x,y,z) dcglNormal3f(x,y,z)
#endif

#if 0
	#define dcglLoadFullMatrixKGL() dcglLoadFullMatrix()
	
	#define glMatrixMode(mode) dcglMatrixMode(mode)
	#define glDepthRange(n, f) dcglDepthRange(n, f)
	#define glViewport(x, y, width, height) dcglViewport(x, y, width, height)

	#define glLoadIdentity() dcglLoadIdentity()
	#define glPushMatrix() dcglPushMatrix()
	#define glPopMatrix() dcglPopMatrix()
	#define glLoadMatrix(m) dcglLoadMatrix(m)
	#define glMultMatrix(m) dcglMultMatrix(m)
	#define glLoadTransposeMatrixf(m) dcglLoadTransposeMatrixf(m)
	#define glMultTransposeMatrixf(m) dcglMultTransposeMatrixf(m)

	#define glTranslatef(x, y, z) dcglTranslatef(x, y, z)
	#define glScalef(x, y, z) dcglScalef(x, y, z)
	#define glRotatef(angle, x, y, z) dcglRotatef(angle, x, y, z)

	#define glFrustum(left, right, bottom, top, nearVal, farVal) dcglFrustum(left, right, bottom, top, nearVal, farVal)
	#define glOrtho(left, right, bottom, top, nearVal, farVal) dcglOrtho(left, right, bottom, top, nearVal, farVal)
	#define gluPerspective(angle, aspect, nearVal, farVal) dcgluPerspective(angle, aspect, nearVal, farVal)
	#define gluOrtho2D(left, right, bottom, top) dcgluOrtho2D(left, right, bottom, top)
#endif

#if 0
	#define glBindTexture(target, texture) dcglBindTexture(target, texture)
	#define glGenTextures(n, textures) dcglGenTextures(n, textures)
	#define glTexImage2D(target, level, internalformat, width, height, border, format, type, data) \
		dcglTexImage2D(target, level, internalformat, width, height, border, format, type, data)
	
	//#define glEnable(cap) dcglEnable(cap)
	//#define glDisable(cap) dcglDisable(cap)
	//#define glIsEnabled(cap) dcglIsEnabled(cap)
	//#define glDepthMask(flag) dcglDepthMask(flag)
	//#define glDepthFunc(func) dcglDepthFunc(func)
	//#define glBlendFunc(sfactor, dfactor) dcglBlendFunc(sfactor, dfactor)
	//#define glClearColor(red, green, blue, alpha) dcglClearColor(red, green, blue, alpha)
	
	#define dcglLoadContextKGL() dcglLoadContext()
	
	//#define glShadeModel(mode) dcglShadeModel(mode)
	//#define glClear(mode) dcglClear(mode)
#endif
#if 1
	#define glKosBeginFrame() glBeginSceneEXT()
	#define glKosFinishFrame() glEndSceneEXT()
#endif
#if 0
	#define glGetError() dcglGetError()
	#define glClearDepth(depth) dcglClearDepth(depth)
	//#define glFogf(pname, param) dcglFogf(pname, param)
	//#define glFogi(pname, param) dcglFogi(pname, param)
	//#define glFogfv(pname, params) dcglFogfv(pname, params)
	
	#define glTexParameteri(target, pname, param) dcglTexParameteri(target, pname, param)
	#define glTexImage2D(target, level, internalformat, width, height, border, format, type, data) \
		dcglTexImage2D(target, level, internalformat, width, height, border, format, type, data)
	
	#define glGenerateMipmap(target) dcglGenerateMipmap(target)
	#define glCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data) \
		dcglCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);
	#define glTexEnvf(target, pname, param) dcglTexEnvf(target, pname, param)
	#define glTexEnvi(target, pname, param) dcglTexEnvi(target, pname, param)
	#define glTexEnviv(target, pname, params) dcglTexEnviv(target, pname, params)
	
	#define glDeleteTextures(n, textureNames) dcglDeleteTextures(n, textureNames)
	#define glIsTexture(texture) dcglIsTexture(texture)
	
	//#define glGetTexParameteriv(target, pname, params) dcglGetTexParameteriv(target, pname, params)
	//#define glAlphaFunc(func, ref) dcglAlphaFunc(func, ref)
#endif

#if 0
#define glColorMacro(tag, type, scale) \
static inline void glColor3 ## tag (type r, type g, type b)		{ dcglColor3f(r*(1.0f/scale),g*(1.0f/scale),b*(1.0f/scale)); } \
static inline void glColor4 ## tag (type r, type g, type b, type a)	{ dcglColor4f(r*(1.0f/scale),g*(1.0f/scale),b*(1.0f/scale),a*(1.0f/scale)); } \
static inline void glColor3 ## tag ## v (const type *c)			{ dcglColor3f(c[0]*(1.0f/scale), c[1]*(1.0f/scale), c[2]*(1.0f/scale)); } \
static inline void glColor4 ## tag ## v (const type *c)			{ dcglColor4f(c[0]*(1.0f/scale), c[1]*(1.0f/scale), c[2]*(1.0f/scale), c[3]*(1.0f/scale)); }

#define glTexCoord2Macro(tag, type, scale) \
static inline void glTexCoord2 ## tag (type x, type y)			{ dcglTexCoord2f(x*(1.0f/scale),y*(1.0f/scale)); } \
static inline void glTexCoord2 ## tag ## v (const type *c)		{ dcglTexCoord2f(c[0]*(1.0f/scale), c[1]*(1.0f/scale)); }

#define glTexCoordMacro(tag, type, scale) \
static inline void glTexCoord1 ## tag (type x)				{ dcglTexCoord2f(x*(1.0f/scale), 0); } \
static inline void glTexCoord3 ## tag (type x, type y, type z)		{ dcglTexCoord2f(x*(1.0f/scale),y*(1.0f/scale)); } \
static inline void glTexCoord4 ## tag (type x, type y, type z, type w)	{ dcglTexCoord2f(x*(1.0f/scale),y*(1.0f/scale)); } \
static inline void glTexCoord1 ## tag ## v (const type *c)		{ dcglTexCoord2f(c[0]*(1.0f/scale), 0); } \
static inline void glTexCoord3 ## tag ## v (const type *c)		{ dcglTexCoord2f(c[0]*(1.0f/scale), c[1]*(1.0f/scale)); } \
static inline void glTexCoord4 ## tag ## v (const type *c)		{ dcglTexCoord2f(c[0]*(1.0f/scale), c[1]*(1.0f/scale)); }

#define glVertexMacro(tag, type, scale) \
static inline void glVertex2 ## tag (type x, type y)			{ dcglVertex3f(x*(1.0f/scale),y*(1.0f/scale), 0); } \
static inline void glVertex3 ## tag (type x, type y, type z)		{ dcglVertex3f(x*(1.0f/scale),y*(1.0f/scale), z*(1.0f/scale)); } \
static inline void glVertex4 ## tag (type x, type y, type z, type w)	{ dcglVertex4f(x*(1.0f/scale),y*(1.0f/scale), z*(1.0f/scale), w*(1.0f/scale)); } \
static inline void glVertex2 ## tag ## v (const type *c)		{ dcglVertex3f(c[0]*(1.0f/scale), c[1]*(1.0f/scale), 0); } \
static inline void glVertex3 ## tag ## v (const type *c)		{ dcglVertex3f(c[0]*(1.0f/scale), c[1]*(1.0f/scale), c[2]*(1.0f/scale)); } \
static inline void glVertex4 ## tag ## v (const type *c)		{ dcglVertex4f(c[0]*(1.0f/scale), c[1]*(1.0f/scale), c[2]*(1.0f/scale), c[3]*(1.0f/scale)); }

glVertexMacro(s, GLshort, 32768.)
glVertexMacro(i, GLint, 2147483648.)
glVertexMacro(d, GLdouble, 1.)

glTexCoord2Macro(s, GLshort, 32768.)
glTexCoord2Macro(i, GLint, 2147483648.)
glTexCoord2Macro(d, GLdouble, 1.)

glTexCoordMacro(s, GLshort, 32768.)
glTexCoordMacro(us, GLushort, 65536.)
glTexCoordMacro(i, GLint, 2147483648.)
glTexCoordMacro(ui, GLint, 4294967296.)
glTexCoordMacro(d, GLdouble, 1.)
//glTexCoordMacro(f, GLfloat, 1.)

glColorMacro(b, GLbyte, 128.)
glColorMacro(ub, GLubyte, 256.)
glColorMacro(s, GLshort, 32768.)
glColorMacro(us, GLushort, 65536.)
glColorMacro(i, GLint, 2147483648.)
glColorMacro(ui, GLint, 4294967296.)
glColorMacro(d, GLdouble, 1.)

#define glVertex2f(x,y) glVertex3f(x,y,0)

static inline void glRectd(GLdouble  x1,  GLdouble  y1,  GLdouble  x2,  GLdouble  y2) { dcglRectf(x1, y1, x2, y2); }
static inline void glRecti(GLint  x1,  GLint  y1,  GLint  x2,  GLint  y2) { dcglRectf(x1, y1, x2, y2); }
static inline void glRects(GLshort  x1,  GLshort  y1,  GLshort  x2,  GLshort  y2) { dcglRectf(x1, y1, x2, y2); }
static inline void glRectdv(const GLdouble *  v1,  const GLdouble *  v2) { dcglRectf(v1[0], v1[1], v2[0], v2[1]); }
static inline void glRectfv(const GLfloat *  v1,  const GLfloat *  v2) { dcglRectf(v1[0], v1[1], v2[0], v2[1]); }
static inline void glRectiv(const GLint *  v1,  const GLint *  v2) { dcglRectf(v1[0], v1[1], v2[0], v2[1]); }
static inline void glRectsv(const GLshort *  v1,  const GLshort *  v2) { dcglRectf(v1[0], v1[1], v2[0], v2[1]); }
#endif

