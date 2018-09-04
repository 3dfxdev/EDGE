#include "minigl.h"
#include "minigl_tex.h"
#include "minigl_mat.h"
#include <math.h>

typedef enum {
	DCGL_BOOLEAN,	DCGL_ENUM,	DCGL_FLOAT,	DCGL_NAME,	DCGL_INVAL,	DCGL_CONST_INT,	DCGL_CONST_FLOAT,	DCGL_CONST_STRING
} DCGLstatetype;

typedef struct {
	GLenum name;
	short type;
	short size;
	union {
		int i;
		float f;
		void *p;
		int *ip;
		GLuint *np;
		GLenum *ep;
		float *fp;
		const char *cstr;
	} val;
	
	DCGLchange changecb;
	//void (*enablecb)();
	const char *str;
	union {
		struct {
			DCGLenable  enable;
			DCGLdisable disable;
			DCGLgetboolean get;
		} bool;
	};
} DCGLstate;

static void dcglApplyDepthFunc();
static void dcglApplyDepthMask();
static void dcglApplyBlendModeAlphaTest();
static void dcglApplyCulling();
static void dcglApplyFog();
static void dcglApplyShadeModel();

extern void dcglApplyTexture();
extern DCGLtexturetarget dcgl_current_texture[3];
void dcglEnableTex(GLenum name);
void dcglDisableTex(GLenum name);
GLboolean dcglGetTexEnabled(GLenum name);

static struct {
	GLenum src;
	GLenum dst;
	int enable;
} dcgl_blend = { .src = GL_ONE, .dst = GL_ZERO, .enable = 0 };
static struct {
	GLenum func;
	GLclampf ref;
} dcgl_alphatest = { .func = GL_ALWAYS, .ref = 0 };
struct {
	GLenum compare;
	int testenable;
	int writeenable;
} dcgl_depth = { GL_LESS, 1, 1 };
struct {
	GLenum frontface;
	GLenum cullface;
	int enable;
} dcgl_cull = { GL_CCW, GL_BACK, 0 };
struct {
	int enabled;
	GLenum mode;
	float density; // must be >0
	float start;
	float end;
	//int index;	//this is really for indexed rendering, and not supported
	int table_dirty;
	int color_dirty;
	float color[4];
} dcgl_fog = {0, GL_EXP, 1, 0, 1, 1, 1, {0,0,0,0}};
GLenum dcgl_shademodel = GL_SMOOTH;

//static int dcgl_gl_context_dirty = 1;	//gl needs to regenerate state
static int dcgl_pvr_context_dirty = 1;	//hardware needs updated state
/* int dcglIsGLStateDirty() {
	return dcgl_gl_context_dirty;
}
void dcglSetGLStateDirty(int i) {
	dcgl_gl_context_dirty = i;
} */
int dcglIsPVRStateDirty() {
	return dcgl_pvr_context_dirty;
}
void dcglSetPVRStateDirty(int i) {
	//dcglLog("%i -> %i", dcgl_pvr_context_dirty, i);
	dcgl_pvr_context_dirty = i;
}
void dcglSubmitState() {
	if (dcglIsPVRStateDirty()) {
		void *cmdbuf = (void *)dcglGetCmdbuffPos();
		dcglLogSubmit("dirty, sending %i bytes to %p", sizeof(dcgl_current_context), cmdbuf);
		
		//dcglLog("%08X", dcgl_current_context.cmd);
		//dcglLog("%08X", dcgl_current_context.mode1);
		//dcglLog("%08X", dcgl_current_context.bf.m.mode2);
		//dcglLog("%08X", dcgl_current_context.bf.m.tex);
		
		dcglSetPVRStateDirty(0);
		//pc_set_anisotropic(pc_no_mod(&dcgl_current_context), 1);
		//sq_cpy((void *)PVR_TA_INPUT, &dcgl_current_context, sizeof(dcgl_current_context));
		dcglAddCmdbuffPosBytes(sizeof(dcgl_current_context));
		sq_cpy(cmdbuf, &dcgl_current_context, sizeof(dcgl_current_context));
	}
}
void dcglRegenerateState() {
	dcglApplyDepthFunc();
	dcglApplyDepthMask();
	dcglApplyBlendModeAlphaTest();
	dcglApplyTexture();
	dcglApplyCulling();
	dcglApplyShadeModel();
	dcglApplyFog();
	//extern void dcglApplyTMU(int tmu_idx);
	//dcglApplyTMU(0);
	//dcglLog("\n%08X\n%08X\n%08X\n%08X", dcgl_current_context.cmd, dcgl_current_context.mode1, dcgl_current_context.bf.m.mode2, dcgl_current_context.bf.m.tex);
}

#define DCGLS_DEF(state_name, type) \
	.name = state_name, .str = #state_name, .type = type

#define DCGLS_BOOL(state_name, state_pointer, state_size, state_enable, state_disable, state_get) \
	{ DCGLS_DEF(state_name, DCGL_BOOLEAN), .val.ip = state_pointer, \
	  .size = state_size, .bool.enable = state_enable, \
	  .bool.disable = state_disable, .bool.get = state_get },

#define DCGLS_BOOL_SIMPLE(state_name, state_pointer, state_size, state_change) \
	DCGLS_BOOL(state_name, state_pointer, state_size, state_change, state_change, NULL)

#define DCGLS_ENUM_SIMPLE(state_name, state_pointer, state_size, state_change) \
	{ DCGLS_DEF(state_name, DCGL_ENUM), .val.ep = state_pointer, \
	  .size = state_size, },

#define DCGLS_CONST_STRING(state_name, str) \
	{ DCGLS_DEF(state_name, DCGL_CONST_STRING), .cstr = str, .size = 1 },

static void dcgl_state_nop() { ; }
static DCGLstate dcgl_state[] = {
{ GL_BLEND,		DCGL_BOOLEAN,	1, { .ip = &dcgl_blend.enable},			dcglApplyBlendModeAlphaTest,	"GL_BLEND" },
{ GL_TEXTURE_2D,	DCGL_BOOLEAN,	1, { .ip = 0},	dcglApplyTexture,	"GL_TEXTURE_2D", 
	.bool.enable = dcglEnableTex, .bool.disable = dcglDisableTex, .bool.get = dcglGetTexEnabled },
{ GL_FOG,		DCGL_BOOLEAN,	1, { .ip = &dcgl_fog.enabled},			dcglApplyFog,		"GL_FOG" },
{ GL_DEPTH_FUNC,	DCGL_ENUM,	1, { .ep = &dcgl_depth.compare },		dcgl_state_nop,		"GL_DEPTH_FUNC" },
{ GL_DEPTH_TEST,	DCGL_BOOLEAN,	1, { .ip = &dcgl_depth.testenable },		dcglApplyDepthFunc,	"GL_DEPTH_TEST" },
{ GL_DEPTH_WRITEMASK,	DCGL_BOOLEAN,	1, { .ip = &dcgl_depth.writeenable},		dcglApplyDepthMask,	"GL_DEPTH_WRITEMASK" },
{ GL_BLEND_SRC,		DCGL_ENUM,	1, { .ep = &dcgl_blend.src},			dcglApplyBlendModeAlphaTest,	"GL_BLEND_SRC" },
{ GL_BLEND_DST,		DCGL_ENUM,	1, { .ep = &dcgl_blend.dst},			dcglApplyBlendModeAlphaTest,	"GL_BLEND_DST" },
{ GL_TEXTURE_1D,	DCGL_BOOLEAN,	1, { .ip = 0},	dcglApplyTexture,	"GL_TEXTURE_1D",
	.bool.enable = dcglEnableTex, .bool.disable = dcglDisableTex, .bool.get = dcglGetTexEnabled },
//{ GL_TEXTURE_BINDING_1D,DCGL_NAME,	1, { .np = &dcgl_current_texture[0].id},	dcgl_state_nop,		"GL_TEXTURE_BINDING_1D" },
//{ GL_TEXTURE_BINDING_2D,DCGL_NAME,	1, { .np = &dcgl_current_texture[1].id},	dcgl_state_nop,		"GL_TEXTURE_BINDING_2D" },
{ GL_CULL_FACE,		DCGL_BOOLEAN,	1, { .ip = &dcgl_cull.enable},			dcglApplyCulling,	"GL_CULL_FACE" },
{ GL_FRONT_FACE,	DCGL_ENUM,	1, { .ep = &dcgl_cull.frontface},		dcgl_state_nop,		"GL_FRONT_FACE" },
{ GL_CULL_FACE_MODE,	DCGL_ENUM,	1, { .ep = &dcgl_cull.cullface},		dcgl_state_nop,		"GL_CULL_FACE_MODE" },
{ GL_SHADE_MODEL,	DCGL_ENUM,	1, { .ep = &dcgl_shademodel},			dcglApplyShadeModel,	"GL_SHADE_MODEL" },
{ GL_FOG_MODE,		DCGL_ENUM,	1, { .ep = &dcgl_fog.mode},			dcgl_state_nop,		"GL_FOG_MODE" },
{ GL_FOG_DENSITY,	DCGL_FLOAT,	1, { .fp = &dcgl_fog.density},			dcgl_state_nop,		"GL_FOG_DENSITY" },
{ GL_FOG_START,		DCGL_FLOAT,	1, { .fp = &dcgl_fog.start},			dcgl_state_nop,		"GL_FOG_START" },
{ GL_FOG_END,		DCGL_FLOAT,	1, { .fp = &dcgl_fog.end},			dcgl_state_nop,		"GL_FOG_END" },
{ GL_FOG_COLOR,		DCGL_FLOAT,	4, { .fp = &dcgl_fog.color[0]},			dcgl_state_nop,		"GL_FOG_COLOR" },

{ GL_VENDOR,		DCGL_CONST_STRING,1, { .cstr = "None"},			dcgl_state_nop,		"GL_VENDOR" },
{ GL_RENDERER,		DCGL_CONST_STRING,1, { .cstr = "DCGL"},			dcgl_state_nop,		"GL_RENDERER" },
{ GL_VERSION,		DCGL_CONST_STRING,1, { .cstr = "1.0.0"},		dcgl_state_nop,		"GL_VERSION" },
{ GL_EXTENSIONS,	DCGL_CONST_STRING,1,
{ .cstr = "GL_SGIS_generate_mipmap GL_EXT_scene_marker GL_SGIS_texture_edge_clamp GL_ARB_texture_mirrored_repeat"},
										dcgl_state_nop,		"GL_EXTENSIONS" },
//{ GL_SCENE_REQUIRED_EXT,DCGL_CONST_INT,	1, { .i = GL_TRUE },			dcgl_state_nop,		"GL_SCENE_REQUIRED_EXT" },

//{ GL_VIEWPORT,		DCGL_FLOAT,	4, { .fp = ??},				dcgl_state_nop,		"GL_VIEWPORT" },
//{ GL_MATRIX_MODE,		DCGL_ENUM,	1, { .ep = ??},				dcgl_state_nop,		"GL_MATRIX_MODE" },
//{ GL_PROJECTION_MATRIX,	DCGL_FLOAT,	16, { .fp = ??},			dcgl_state_nop,		"GL_PROJECTION_MATRIX" },
//{ GL_MODELVIEW_MATRIX,	DCGL_FLOAT,	16, { .fp = ??},			dcgl_state_nop,		"GL_MODELVIEW_MATRIX" },
//{ GL_TEXTURE_MATRIX,		DCGL_FLOAT,	16, { .fp = ??},			dcgl_state_nop,		"GL_TEXTURE_MATRIX" },
//{ GL_TRANSPOSE_MODELVIEW_MATRIX,DCGL_FLOAT,	16, { .fp = ??},			dcgl_state_nop,		"GL_TRANSPOSE_MODELVIEW_MATRIX" },
//{ GL_TRANSPOSE_PROJECTION_MATRIX,DCGL_FLOAT,	16, { .fp = ??},			dcgl_state_nop,		"GL_TRANSPOSE_PROJECTION_MATRIX" },
//{ GL_TRANSPOSE_TEXTURE_MATRIX,DCGL_FLOAT,	16, { .fp = ??},			dcgl_state_nop,		"GL_TRANSPOSE_TEXTURE_MATRIX" },
//{ GL_PROJECTION_STACK_DEPTH,	DCGL_INT,	1, { .ip = ??},				dcgl_state_nop,		"GL_PROJECTION_STACK_DEPTH" },
//{ GL_MODELVIEW_STACK_DEPTH,	DCGL_INT,	1, { .ip = ??},				dcgl_state_nop,		"GL_MODELVIEW_STACK_DEPTH" },
//{ GL_TEXTURE_STACK_DEPTH,	DCGL_INT,	1, { .ip = ??},				dcgl_state_nop,		"GL_TEXTURE_STACK_DEPTH" },

{ GL_MAX_MODELVIEW_STACK_DEPTH,	DCGL_CONST_INT,	1, { .i = DCGL_MAX_MODELVIEW_STACK_DEPTH },	dcgl_state_nop,		"GL_MAX_MODELVIEW_STACK_DEPTH" },
{ GL_MAX_PROJECTION_STACK_DEPTH,DCGL_CONST_INT,	1, { .i = DCGL_MAX_PROJECTION_STACK_DEPTH},	dcgl_state_nop,		"GL_MAX_PROJECTION_STACK_DEPTH" },
{ GL_MAX_TEXTURE_STACK_DEPTH,   DCGL_CONST_INT,	1, { .i = DCGL_MAX_TEXTURE_STACK_DEPTH},	dcgl_state_nop,		"GL_MAX_TEXTURE_STACK_DEPTH" },
{ GL_MAX_TEXTURE_SIZE,		DCGL_CONST_INT,	1, { .i = 1024},				dcgl_state_nop,		"GL_MAX_TEXTURE_SIZE" },
{ GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT,DCGL_CONST_FLOAT,	1, { .f = 2},				dcgl_state_nop,		"GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT" },
//{ GL_MAX_TEXTURE_UNITS,	DCGL_CONST_INT,	1, { .i = 8},					dcgl_state_nop,		"GL_MAX_TEXTURE_UNITS" },
//{ GL_MAX_LIGHTS,		DCGL_CONST_INT,	1, { .i = 8},					dcgl_state_nop,		"GL_MAX_LIGHTS" },
{ GL_DEPTH_BITS,   	DCGL_CONST_INT,	1, { .i = 32 },					dcgl_state_nop,		"GL_DEPTH_BITS" },
//{ GL_RED_BITS,   	DCGL_CONST_INT,	1, { .i = 8 },					dcgl_state_nop,		"GL_RED_BITS" },
//{ GL_GREEN_BITS,   	DCGL_CONST_INT,	1, { .i = 8 },					dcgl_state_nop,		"GL_GREEN_BITS" },
//{ GL_BLUE_BITS,   	DCGL_CONST_INT,	1, { .i = 8 },					dcgl_state_nop,		"GL_BLUE_BITS" },
//{ GL_ALPHA_BITS,   	DCGL_CONST_INT,	1, { .i = 8 },					dcgl_state_nop,		"GL_ALPHA_BITS" },

//{ GL_CURRENT_FOG_COORD,	DCGL_FLOAT,	1, { .ip = &??? },		dcgl??,	"GL_CURRENT_FOG_COORD" },
//{ GL_CURRENT_NORMAL,		DCGL_FLOAT,	3, { .ip = &??? },		dcgl??,	"GL_CURRENT_NORMAL" },
{ GL_CURRENT_TEXTURE_COORDS,	DCGL_FLOAT,	2, { .fp = &dcgl_prim.vert_params.u },	dcgl_state_nop,		"GL_CURRENT_TEXTURE_COORDS" },
//{ GL_CURRENT_COLOR,		DCGL_FLOAT,	4, { .ip = &??? },		dcgl??,	"GL_CURRENT_COLOR" },
//{ GL_CURRENT_SECONDARY_COLOR,	DCGL_FLOAT,	4, { .ip = &??? },		dcgl??,	"GL_CURRENT_SECONDARY_COLOR" },
//{ GL_COLOR_SUM,	DCGL_BOOLEAN,	1, { .ip = &??? },		dcgl??,	"GL_COLOR_SUM" },
//{ GL_DITHER,		DCGL_BOOLEAN,	1, { .ip = &??? },		dcgl??,	"GL_DITHER" },
};

#define dcgl_state_cnt (sizeof(dcgl_state) / sizeof(dcgl_state[0]))

DCGLstate * dcglFindStateVar(GLenum name) {
	int i;
	for(i=0;i<dcgl_state_cnt;i++) {
		if (dcgl_state[i].name == name)
			return dcgl_state + i;
	}
	return NULL;
}

void glEnable(GLenum cap) {
	dcglNotInBegin(return);
	DCGLstate *state = dcglFindStateVar(cap);
	//if (state) dcglLog("%08x -> %s", cap, state->str);
	if (!state || state->type != DCGL_BOOLEAN) {
		dcglSetErrorF(GL_INVALID_ENUM);
		dcglLog("invalid cap %08x", cap);
		return;
	}
	if (state->val.ip) {
		if (!*state->val.ip) {
			*state->val.ip = 1;
			state->changecb();
		}
	} else {
		if (state->bool.enable)
			state->bool.enable(cap);
		else
			dcglUnimpAssert(0);
	}
}

void glDisable(GLenum cap) {
	dcglNotInBegin(return);
	DCGLstate *state = dcglFindStateVar(cap);
	if (!state || state->type != DCGL_BOOLEAN) {
		dcglSetErrorF(GL_INVALID_ENUM);
		return;
	}
	if (state->val.ip) {
		if (*state->val.ip) {
			*state->val.ip = 0;
			state->changecb();
		}
	} else {
		if (state->bool.disable)
			state->bool.disable(cap);
		else
			dcglUnimpAssert(0);
	}
}

GLboolean glIsEnabled(GLenum cap) {
	dcglNotInBegin(return 0);
	DCGLstate *state = dcglFindStateVar(cap);
	if (!state || state->type != DCGL_BOOLEAN) {
		dcglSetErrorF(GL_INVALID_ENUM);
		return 0;
	}
	if (state->val.ip) {
		return *state->val.ip;
	} else {
		dcglUnimpAssert(0);
		return 0;
	}
}

void glGetIntegerv(GLenum pname, GLint *params) {
	dcglNotInBegin(return);
	DCGLstate *state = dcglFindStateVar(pname);
	dcglRequire(state, GL_INVALID_ENUM, return);
	
	switch(state->type) {
	case DCGL_ENUM:
		*params = *state->val.ip; break;
	case DCGL_CONST_INT:
		*params = state->val.i; break;
	default:
		dcglSetErrorF(GL_INVALID_ENUM);
	}
}

void glGetFloatv(GLenum pname, GLfloat *params) {
	dcglNotInBegin(return);
	DCGLstate *state = dcglFindStateVar(pname);
	dcglRequire(state, GL_INVALID_ENUM, return);
	
	int i;
	switch(state->type) {
	case DCGL_FLOAT:
		for(i=0;i<state->size;i++)
			*params++ = state->val.fp[i];
		break;
	default:
		dcglSetErrorF(GL_INVALID_ENUM);
	}
}

void glGetDoublev(GLenum pname, GLdouble *params) {
	dcglHardAssert(sizeof(GLfloat) == sizeof(GLdouble));
	glGetFloatv(pname, params);
}

void glGetBooleanv(GLenum pname, GLboolean *params) {
	dcglNotInBegin(return);
	DCGLstate *state = dcglFindStateVar(pname);
	dcglRequire(state, GL_INVALID_ENUM, return);
	
	int i;
	if (state->val.ip) {
		switch(state->type) {
		case DCGL_BOOLEAN:
			for(i=0;i<state->size;i++)
				*params++ = *state->val.ip;
			break;
		default:
			dcglSetErrorF(GL_INVALID_ENUM);
		}
	} else {
		dcglUnimpAssert(0);
	}
}

const GLubyte* glGetString (GLenum name) {
	dcglNotInBegin(return 0);
	DCGLstate *state = dcglFindStateVar(name);
	dcglRequire(state, GL_INVALID_ENUM, return 0);
	
	if (state->type == DCGL_CONST_STRING)
		return (const GLubyte*)state->val.cstr;
	
	dcglSetErrorF(GL_INVALID_ENUM);
	return 0;
}

//	DEPTH
void glDepthMask(GLboolean flag) {
	dcglNotInBegin(return);
	if (dcgl_depth.writeenable == flag) return;
	
	dcgl_depth.writeenable = !!flag;
	dcglApplyDepthMask();
}

static void dcglApplyDepthMask() {
	//dcglLog("write: %i", dcgl_depth.writeenable);
	pc_set_depth_write_disable(&dcgl_current_context, !dcgl_depth.writeenable);
	dcglSetPVRStateDirty(1);
}

void glDepthFunc(GLenum func) {
	dcglNotInBegin(return);
	if (dcgl_depth.compare == func) return;
	
	dcglRequire(func == GL_LESS || func == GL_LEQUAL
		|| func == GL_GREATER || func == GL_GEQUAL
		|| func == GL_EQUAL || func == GL_NOTEQUAL
		|| func == GL_NEVER || func == GL_ALWAYS
		, GL_INVALID_ENUM, return);
	dcgl_depth.compare = func;
	dcglApplyDepthFunc();
}

static void dcglApplyDepthFunc() {
	if (dcgl_depth.testenable) {
		pc_depth_compare compare;
		//less/greater are reversed because the pvr uses a reverse depth buffer
		switch(dcgl_depth.compare) {
		case GL_LESS: 		compare = PC_GREATER;  break;
		case GL_LEQUAL: 	compare = PC_GEQUAL;   break;
		case GL_GREATER: 	compare = PC_LESS;     break;
		case GL_GEQUAL: 	compare = PC_LEQUAL;   break;
		case GL_EQUAL: 		compare = PC_EQUAL;    break;
		case GL_NOTEQUAL:	compare = PC_NOTEQUAL; break;
		case GL_NEVER: 		compare = PC_NEVER;    break;
		case GL_ALWAYS: 	compare = PC_ALWAYS;   break;
		default: return;	//TODO handle
		}
		pc_set_depth_compare(&dcgl_current_context, compare);
	} else {
		pc_set_depth_compare(&dcgl_current_context, GL_ALWAYS);
	}
	dcglSetPVRStateDirty(1);
}

//	BLENDING
void glBlendFunc(GLenum sfactor, GLenum dfactor) {
	dcglNotInBegin(return);
	if (dcgl_blend.src == sfactor && dcgl_blend.dst == dfactor) return;
	
	//TODO error check unsupported modes GL_INVALID_ENUM
	dcglRequire(sfactor == GL_ZERO || sfactor == GL_ONE
		|| sfactor == GL_DST_COLOR || sfactor == GL_ONE_MINUS_DST_COLOR
		|| sfactor == GL_SRC_ALPHA || sfactor == GL_ONE_MINUS_SRC_ALPHA
		|| sfactor == GL_DST_ALPHA || sfactor == GL_ONE_MINUS_DST_ALPHA
		, GL_INVALID_ENUM, return);
	dcglRequire(dfactor == GL_ZERO || dfactor == GL_ONE
		|| dfactor == GL_SRC_COLOR || dfactor == GL_ONE_MINUS_SRC_COLOR
		|| dfactor == GL_SRC_ALPHA || dfactor == GL_ONE_MINUS_SRC_ALPHA
		|| dfactor == GL_DST_ALPHA || dfactor == GL_ONE_MINUS_DST_ALPHA
		, GL_INVALID_ENUM, return);
	dcgl_blend.src = sfactor;
	dcgl_blend.dst = dfactor;
	dcglApplyBlendModeAlphaTest();
}

static void dcglApplyBlendModeAlphaTest() {
	//dc can't do blend and alpha test at the same time, give priority to blend
	if (dcgl_blend.enable) {
		pc_blend_mode bsrc = GL_ONE, bdst = GL_ZERO;
		switch(dcgl_blend.src) {
		case GL_ZERO:			bsrc = PC_ZERO;            break;
		case GL_ONE:			bsrc = PC_ONE;             break;
		case GL_DST_COLOR:		bsrc = PC_OTHER_COLOR;     break;
		case GL_ONE_MINUS_DST_COLOR:	bsrc = PC_INV_OTHER_COLOR; break;
		case GL_SRC_ALPHA:		bsrc = PC_SRC_ALPHA;       break;
		case GL_ONE_MINUS_SRC_ALPHA:	bsrc = PC_INV_SRC_ALPHA;   break;
		case GL_DST_ALPHA:		bsrc = PC_DST_ALPHA;       break;
		case GL_ONE_MINUS_DST_ALPHA:	bsrc = PC_INV_DST_ALPHA;   break;
		default: dcglLog("unsupported %i", dcgl_blend.src); return;
		}
		
		switch(dcgl_blend.dst) {
		case GL_ZERO:			bdst = PC_ZERO;            break;
		case GL_ONE:			bdst = PC_ONE;             break;
		case GL_SRC_COLOR:		bdst = PC_OTHER_COLOR;     break;
		case GL_ONE_MINUS_SRC_COLOR:	bdst = PC_INV_OTHER_COLOR; break;
		case GL_SRC_ALPHA:		bdst = PC_SRC_ALPHA;       break;
		case GL_ONE_MINUS_SRC_ALPHA:	bdst = PC_INV_SRC_ALPHA;   break;
		case GL_DST_ALPHA:		bdst = PC_DST_ALPHA;       break;
		case GL_ONE_MINUS_DST_ALPHA:	bdst = PC_INV_DST_ALPHA;   break;
		default: dcglLog("unsupported %i", dcgl_blend.dst); return;
		}
		//dcglLog("src: %i, dst: %i", bsrc, bdst);
		pc_set_blend_modes(pc_no_mod(&dcgl_current_context), bsrc, bdst);
		dcglListTarget(PVR_LIST_TR_POLY);
	} else {
		//dcglLog("off");
		if (dcgl_alphatest.func == GL_ALWAYS) {
			pc_set_blend_modes(pc_no_mod(&dcgl_current_context), PC_ONE, PC_ZERO);
			dcglListTarget(PVR_LIST_OP_POLY);
		} else if (dcgl_alphatest.func == GL_GREATER || dcgl_alphatest.func == GL_GEQUAL) {
			pc_set_blend_modes(pc_no_mod(&dcgl_current_context), PC_SRC_ALPHA, PC_INV_SRC_ALPHA);
			dcglListTarget(PVR_LIST_PT_POLY);
		} else {
			//unsupported alpha test, just draw it anyways as opaque
			//TODO add support for GL_NEVER
			//TODO generate warning
			pc_set_blend_modes(pc_no_mod(&dcgl_current_context), PC_ONE, PC_ZERO);
			dcglListTarget(PVR_LIST_OP_POLY);
		}
	}
	dcglSetPVRStateDirty(1);
}

//	ALPHA TEST
void glAlphaFunc(GLenum func, GLclampf ref) {
	dcglNotInBegin(return);
	
	dcgl_alphatest.func = func;
	//TODO hw only does gequal, fudge ref value so greater doesn't include equal
	dcgl_alphatest.ref = DCGL_CLAMP01(ref);
	
	//TODO hw only supports one ref value per render, check and warn if it changes
	//currently uses the last set value for test
	pvr_set_alpha_compare(dcgl_alphatest.ref);
			
	dcglApplyBlendModeAlphaTest();
}

//	CULLING
void glCullFace(GLenum mode) {
	dcglNotInBegin(return);
	if (dcgl_cull.cullface == mode) return;
	
	dcglRequire(mode == GL_FRONT || mode == GL_BACK || mode == GL_FRONT_AND_BACK, GL_INVALID_ENUM, return);
	dcgl_cull.cullface = mode;
	dcglApplyCulling();
}

void glFrontFace(GLenum mode) {
	dcglNotInBegin(return);
	if (dcgl_cull.frontface == mode) return;
	
	dcglRequire(mode == GL_CW || mode == GL_CCW, GL_INVALID_ENUM, return);
	dcgl_cull.frontface = mode;
	dcglApplyCulling();
}

void dcglApplyCulling()
{
	pc_culling cull = PC_CULL_SMALL;
	if (dcgl_cull.enable) {
		if (dcgl_cull.frontface == GL_CCW)
			cull = (dcgl_cull.cullface == GL_BACK) ? PC_CULL_CW : PC_CULL_CCW;
		else
			cull = (dcgl_cull.cullface == GL_BACK) ? PC_CULL_CCW : PC_CULL_CW;
	}
	//dcglLog("%i", cull);
	pc_set_cull_mode(&dcgl_current_context, cull);
	dcglSetPVRStateDirty(1);
}

//	SHADE MODEL
void glShadeModel(GLenum mode) {
	dcglNotInBegin(return);
	if (dcgl_shademodel == mode) return;
	
	dcglRequire(mode == GL_FLAT || mode == GL_SMOOTH, GL_INVALID_ENUM, return);
	dcgl_shademodel = mode;
	dcglApplyShadeModel();
}

static void dcglApplyShadeModel() {
	pc_set_gouraud(&dcgl_current_context, dcgl_shademodel == GL_SMOOTH);
	if (dcgl_shademodel == GL_SMOOTH)
		dcglGouraudClip();
	else
		dcglFlatClip();
	dcglSetPVRStateDirty(1);
}

//	FOG
void glFogfv(GLenum pname, const GLfloat *params) {
	dcglNotInBegin(return);
	
	switch(pname) {
	case GL_FOG_MODE:
		//TODO what's the right way to handle this?
		dcglRequire(*params == GL_LINEAR || *params == GL_EXP || *params == GL_EXP2, GL_INVALID_ENUM, return);
		if (dcgl_fog.mode != *params) {
			dcgl_fog.mode = *params;
			dcgl_fog.table_dirty = 1;
		}
		break;
	case GL_FOG_DENSITY:
		dcglRequire(*params >= 0, GL_INVALID_VALUE, return);
		if (dcgl_fog.density != *params) {
			dcgl_fog.density = *params;
			dcgl_fog.table_dirty = 1;
		}
		break;
	case GL_FOG_START:
		if (dcgl_fog.start != *params) {
			dcgl_fog.start = *params;
			dcgl_fog.table_dirty = 1;
		}
		break;
	case GL_FOG_END:
		if (dcgl_fog.end != *params) {
			dcgl_fog.end = *params;
			dcgl_fog.table_dirty = 1;
		}
		break;
	case GL_FOG_INDEX:
		dcglLog("fog index not supported\n");
		break;
	case GL_FOG_COLOR:
		if (dcgl_fog.color[0] != DCGL_CLAMP01(params[0]) || dcgl_fog.color[1] != DCGL_CLAMP01(params[1]) ||
		    dcgl_fog.color[2] != DCGL_CLAMP01(params[2]) || dcgl_fog.color[3] != DCGL_CLAMP01(params[3])) {
			dcgl_fog.color[0] = DCGL_CLAMP01(params[0]);
			dcgl_fog.color[1] = DCGL_CLAMP01(params[1]);
			dcgl_fog.color[2] = DCGL_CLAMP01(params[2]);
			dcgl_fog.color[3] = DCGL_CLAMP01(params[3]);
			dcgl_fog.color_dirty = 1;
		}
		break;
	default:
		dcglRequire(0, GL_INVALID_ENUM, return);
	}
}

void glFogi(GLenum pname, GLint param) {
	if (pname == GL_FOG_MODE) {
		dcglRequire(param == GL_LINEAR || param == GL_EXP || param == GL_EXP2, GL_INVALID_ENUM, return);
		if (dcgl_fog.mode != param) {
			dcgl_fog.mode = param;
			dcgl_fog.table_dirty = 1;
		}
	} else {
		GLfloat fparam = param * (1.0f / (1<<31));
		glFogfv(pname, &fparam);
	}
}

void glFogf(GLenum pname, GLfloat param) {
	glFogfv(pname, &param);
}

void dcglApplyFogGlobals() {
	if (dcgl_fog.table_dirty) {
		pvr_fog_far_depth(dcgl_fog.end);
		switch(dcgl_fog.mode) {
		case GL_EXP:    pvr_fog_table_exp(dcgl_fog.density); break;
		case GL_EXP2:   pvr_fog_table_exp2(dcgl_fog.density); break;
		case GL_LINEAR: pvr_fog_table_linear(dcgl_fog.start, dcgl_fog.end); break;
		};
		dcgl_fog.table_dirty = 0;
	}
	if (dcgl_fog.color_dirty) {
		pvr_fog_table_color(dcgl_fog.color[3], dcgl_fog.color[0], dcgl_fog.color[1], dcgl_fog.color[2]);
		dcgl_fog.color_dirty = 0;
	}
}

static void dcglApplyFog() {
	pc_set_fog_mode(pc_no_mod(&dcgl_current_context), dcgl_fog.enabled ? PC_FOG_TABLE : PC_FOG_DISABLE);
	dcglSetPVRStateDirty(1);
}

//	CLEAR
static float dcgl_clearcolor[4];
static float dcgl_cleardepth = 1;	//TODO figure this out
void glClearDepth(GLclampf depth) {
	dcgl_cleardepth = DCGL_CLAMP01(depth);
}

void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
	dcgl_clearcolor[0] = DCGL_CLAMP01(red);
	dcgl_clearcolor[1] = DCGL_CLAMP01(green);
	dcgl_clearcolor[2] = DCGL_CLAMP01(blue);
	dcgl_clearcolor[3] = DCGL_CLAMP01(alpha);
	//TODO do something about alpha
	pvr_set_bg_color(dcgl_clearcolor[0],dcgl_clearcolor[1],dcgl_clearcolor[2]);
}

void glClear(GLbitfield  mask) {
	//PVR always clears all buffers
}

//	MISC
void glHint(GLenum target, GLenum mode) {
	//TODO
}

void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
	//HW doesn't support
	dcglLog("Unsupported");
}

void glScissor(GLint x, GLint y, GLsizei width, GLsizei height) {
	//HW only really supports scissor boundries on mod-32
	//TODO
	//GL has origin at bottom-left, HW has origin at top-left
	dcglLog("Unimplemented");
}

void glPolygonOffset(GLfloat factor, GLfloat units) {
	//TODO
	dcglLog("Unimplemented");
}

void glPixelStorei(GLenum pname, GLint param) {
	//TODO
	dcglLog("Unimplemented");
}

void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid * data) {
	//TODO
	dcglLog("Unimplemented");
}



