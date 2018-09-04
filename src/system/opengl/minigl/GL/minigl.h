#ifndef MINIGL_H
#define MINIGL_H

#ifdef __cplusplus
extern "C" {
#endif

//TODO clean this up

#include <kos.h>
#include <GL/GL.h>
#include <assert.h>
//#include "pvr_cxt.h"

void dcglInit(int fsaa);

//full size codebooks
#define GL_COMPRESSED_VQ_1555_DC		0x1DC00
#define GL_COMPRESSED_VQ_4444_DC		0x1DC01
#define GL_COMPRESSED_VQ_565_DC			0x1DC02
#define GL_COMPRESSED_VQ_YUV_DC			0x1DC03
#define GL_COMPRESSED_VQ_1555_MM_DC		0x1DC04
#define GL_COMPRESSED_VQ_4444_MM_DC		0x1DC05
#define GL_COMPRESSED_VQ_565_MM_DC		0x1DC06
#define GL_COMPRESSED_VQ_YUV_MM_DC		0x1DC07
//#define GL_COMPRESSED_VQ_PAL4_DC		0x1DC08
//#define GL_COMPRESSED_VQ_PAL8_DC		0x1DC09
//#define GL_COMPRESSED_VQ_PAL4_MM_DC		0x1DC0A
//#define GL_COMPRESSED_VQ_PAL8_MM_DC		0x1DC0B

//pretwiddled and flipped dma-able textures
#define GL_COMPRESSED_TWID_1555_DC		0x1DC80
#define GL_COMPRESSED_TWID_4444_DC		0x1DC81
#define GL_COMPRESSED_TWID_565_DC		0x1DC82
#define GL_COMPRESSED_TWID_YUV_DC		0x1DC83
#define GL_COMPRESSED_TWID_1555_MM_DC		0x1DC84
#define GL_COMPRESSED_TWID_4444_MM_DC		0x1DC85
#define GL_COMPRESSED_TWID_565_MM_DC		0x1DC86
#define GL_COMPRESSED_TWID_YUV_MM_DC		0x1DC87

#define GL_TEXTURE_FILTER_CONTROL         0x8500
#define GL_TEXTURE_LOD_BIAS               0x8501
#define GL_TEXTURE_MAX_ANISOTROPY_EXT     0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#define GL_COLOR_SUM                      0x8458
#define GL_CURRENT_SECONDARY_COLOR        0x8459
#define GL_SECONDARY_COLOR_ARRAY_SIZE     0x845A
#define GL_SECONDARY_COLOR_ARRAY_TYPE     0x845B
#define GL_SECONDARY_COLOR_ARRAY_STRIDE   0x845C
#define GL_SECONDARY_COLOR_ARRAY_POINTER  0x845D
#define GL_SECONDARY_COLOR_ARRAY          0x845E

typedef GLfloat   (*DCGLgetfloat)(int idx);
typedef GLboolean (*DCGLgetboolean)(GLenum name);
typedef void      (*DCGLenable)(GLenum name);
typedef void      (*DCGLdisable)(GLenum name);
typedef void      (*DCGLchange)();

#include "minigl_mat.h"
#include "minigl_tex.h"

#define DBOS(a...) do { printf(a); fflush(stdout); } while(0)

GLenum dcglGetError(void);
void dcglPrintError(void);	//nonstandard
const char* dcgluErrorString(GLenum errorCode);
void dcglSetError(GLenum error, const char *func);	//TODO should be private
#define dcglSetErrorF(errornum) dcglSetError(errornum, __func__)
//dcglRequire(condition_that_should_be_true, gl_error_to_set_if_false, side_effect_if_false_like_return)
#define dcglRequire(assert_condition, errornum, ...) do { if (!(assert_condition)) { dcglSetErrorF(errornum); __VA_ARGS__; } } while(0)

//assert that shouldn't crash, just report a user error (TODO make it actually do this)
#define dcglSoftAssert(a...) assert(a)
//assert that should crash, error in gl
#define dcglHardAssert(a...) assert(a)
//assert that catches unimplemented things
#define dcglUnimpAssert(a...) assert(a)

#define dcglUntested() DBOS("Call to untested \"%s\"!\n", __FUNCTION__)
#define dcglBuggy() DBOS("Call to buggy \"%s\"!\n", __FUNCTION__)

//checks that the function is not called between a Begin/End pairs
#define dcglNotInBegin(...) dcglRequire(!dcgl_prim.in_begin, GL_INVALID_OPERATION, __VA_ARGS__)

//checks that the pvr is ready to recieve polygons
//extern int dcgl_in_scene;
//static inline int dcglIsInScene() {
//	return dcgl_in_scene;
//}
#define dcglIsInScene() (dcgl_prim.in_scene)
void glBeginSceneEXT(void);
void glEndSceneEXT(void);
void dcglBlendSceneEXT(void);

//#define dcglCheckInScene(...) do { if (!dcglIsInScene()) { dcglLog("not in scene"); __VA_ARGS__; } } while(0)
#define dcglCheckInScene(...) dcglRequire(dcglIsInScene(), GL_INVALID_OPERATION, __VA_ARGS__)

//a second level of checking that the pvr is ready to recieve polygons, with notable speed hit
#if 1
	#define dcglCheckInSceneExtra()
	
	#define dcglHardAssertExtra(a...)
	//#define dcglLog(str, ...) DBOS("%s \"%s %i\": "str"\n", __func__, __FILE__, __LINE__, ##__VA_ARGS__)
	#define dcglLog(str, ...) DBOS("%s %4i: "str"\n", __func__, __LINE__, ##__VA_ARGS__)
#else
	#define dcglHardAssertExtra(a...) dcglHardAssert(a)
	#define dcglCheckInSceneExtra() dcglCheckInScene()
#endif

//#define DCGL_LOG_ALL_SUBMITS 1

#if DCGL_LOG_ALL_SUBMITS
	#define dcglLogSubmit(str, ...) DBOS("%s: "str"\n", __func__, ##__VA_ARGS__)
#else
	#define dcglLogSubmit(str, ...) 
#endif

static inline float DCGL_CLAMP01(float val) {
	if (val < 0)
		val = 0;
	if (val > 1)
		val = 1;
	return val;
}

typedef struct {
	int cmd;
	float x,y,z;
	float u,v;
	int pad[2];
	float fa,fr,fg,fb;
	float fogd,fro,fgo,fbo;
} DCGLvertex_old;

typedef struct {
	float x,y,z,w;
	float u,v,p,q;
	
	//float fa,fr,fg,fb;
	float fr,fg,fb,fa;
	float fogd,fro,fgo,fbo;
} DCGLvertex;

typedef struct {
	int cmd;
	float x,y,z;
	float u,v;
	int pad[2];
	float fa,fr,fg,fb;
	float fogd,fro,fgo,fbo;
} DCGLpvrvertex;

typedef struct {
	float x,y,z,w;
	float u,v,p,q;
	
	float fa,fr,fg,fb;
	float fro,fgo,fbo; int edge;
	
	float nx,ny,nz,fogd;
	float tx,ty,tz,tw;
} DCGLvertexbig;

typedef struct {
	float x,y,z,w;
	float u,v,p,q;
	
	float fa,fr,fg,fb;
	float fogd,fro,fgo,fbo;
} DCGLvertex_compact;

typedef struct {
	int cmd;
	float x,y,z;
	float fa,fr,fg,fb;
} DCGLvertexUntex;

typedef int (*DCGLclip)(DCGLvertex *a, DCGLvertex *b, DCGLvertex *provoking, DCGLvertex *dst);
typedef void (*DCGLcallback)(void);

int dcglIsPVRStateDirty();
void dcglSetPVRStateDirty(int i);

//#define DCGL_VERT_BUFF_SIZE (8*1024*1024/sizeof(DCGLpvrvertex))
#define DCGL_VERT_BUFF_SIZE (1000)

extern DCGLvertex *dcgl_cur_vert;

extern pvr_context dcgl_current_context;

char * dcglGetCmdbuffPos();
void dcglAddCmdbuffPosBytes(size_t bytes);
void dcglSetCmdbuffPos(char *pos);
void dcglSetCmdbuffPosSQ(char *pos);
void dcglListTarget(int list);

#include "minigl_prim.h"

void glEnable(GLenum cap);
void glDisable(GLenum cap);
GLboolean glIsEnabled(GLenum cap);
void glGetIntegerv(GLenum pname, GLint *params);
void glDepthMask(GLboolean flag);
void glDepthFunc(GLenum func);
void glBlendFunc(GLenum sfactor, GLenum dfactor);
void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void glShadeModel(GLenum mode);
void glClear(GLbitfield  mask);
GLenum dcglGetError(void);
void glClearDepth(GLclampf depth);
void glFogf(GLenum pname, GLfloat param);
void glFogi(GLenum pname, GLint param);
void glFogfv(GLenum pname, const GLfloat *params);
void glAlphaFunc(GLenum func, GLclampf ref);
void glGetTexParameteriv(GLenum target, GLenum pname, GLint * params);

#ifdef __cplusplus
}
#endif


#endif
