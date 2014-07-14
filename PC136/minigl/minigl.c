#include "minigl.h"
#include <math.h>


typedef struct {
	//short pass, listtype;
	int qacr_val;
	char *start;
	char *end;
	char *curpos;
} DCGLcommandbuffer;

#define DCGL_CMDBUFFS (PVR_LIST_PT_POLY+1)
struct {
	char *datadst;
	DCGLcommandbuffer *curbuff;
	int mainlist;
	DCGLcommandbuffer buff[DCGL_CMDBUFFS];	//each buffer represents one pvr list type
} dcgl_cmd = { (char*)PVR_TA_INPUT, &dcgl_cmd.buff[0], PVR_LIST_OP_POLY };

void dcglCommandTarget(DCGLcommandbuffer *targ) {
	if (dcgl_cmd.curbuff != targ) {
		dcglHardAssertExtra(targ);
		dcglHardAssertExtra(targ->curpos);
		dcgl_cmd.curbuff->curpos = dcgl_cmd.datadst;
		dcgl_cmd.curbuff = targ;
		dcgl_cmd.datadst = targ->curpos;
	}
}
void dcglListTarget(int list) {
	dcglHardAssertExtra(0 >= list && list < DCGL_CMDBUFFS);
	dcglCommandTarget(dcgl_cmd.buff + list);
}
char * dcglGetCmdbuffPos() {
	return dcgl_cmd.datadst;
}
static inline void dcglCheckCmdbuffSize() {
	dcglHardAssertExtra(dcgl_cmd.datadst < dcgl_cmd.curbuff->end);
}
void dcglSetCmdbuffPos(char *pos) {
	dcgl_cmd.datadst = pos;
	dcglCheckCmdbuffSize();
}
void dcglAddCmdbuffPosBytes(size_t bytes) {
	dcglSetCmdbuffPos(dcglGetCmdbuffPos() + bytes);
}
void dcglSetCmdbuffPosSQ(char *pos) {
	ptrdiff_t diff = pos - (char*)sq_get_store_queue_address(dcgl_cmd.datadst);
	dcglAddCmdbuffPosBytes(diff);
}
void dcglAddCmdbuffPosLines(unsigned int lines) {
	dcglAddCmdbuffPosBytes(lines * 32);
}

//clears command buffer
void dcglClearCmdbuff() {
	//verify there is no dma (potentially using the buffers)
	dcglHardAssert(pvr_dma_ready());
	int i;
	for(i = 0; i < DCGL_CMDBUFFS; i++) {
		dcgl_cmd.buff[i].curpos = dcgl_cmd.buff[i].start;
	}
	dcgl_cmd.datadst = dcgl_cmd.curbuff->curpos;
}

static void dcglInitCmdbuff() {
	//static const int bufsizes[DCGL_CMDBUFFS] = {256*1024, 0, 256*1024, 0, 0};
	static const int bufsizes[DCGL_CMDBUFFS] = {-1, 0, 512*1024, 0, 256*1024};
	int i, directlistset = 0;
	
	dcgl_cmd.mainlist = 6;
	for(i = 0; i < DCGL_CMDBUFFS; i++) {
		//non-direct list sizes should be a multiple of 32
		if (bufsizes[i] != -1) {
			dcglHardAssert((bufsizes[i] % 64) == 0);
		}
		switch(bufsizes[i]) {
		case -1:	//direct list
			dcglHardAssert(!directlistset);
			dcgl_cmd.mainlist = i;
			dcgl_cmd.buff[i].start = (char*)PVR_TA_INPUT;
			dcgl_cmd.buff[i].end = dcgl_cmd.buff[i].start + 0x1000000;	//TODO verify
			directlistset = 1;
			break;
		case 0:		//no list
			dcgl_cmd.buff[i].end = dcgl_cmd.buff[i].start = NULL;
			break;
		default:	//buffered list
			dcgl_cmd.buff[i].start = memalign(32,bufsizes[i]);
			dcgl_cmd.buff[i].end = dcgl_cmd.buff[i].start + bufsizes[i];
		}
	}
	dcgl_cmd.curbuff = &dcgl_cmd.buff[0];
	dcglClearCmdbuff();
}

void dcglBeginList(int list) {
	int pvr_result = pvr_list_begin(list);
	if (pvr_result) dcglLog("list %i begin error %i",dcgl_cmd.mainlist,pvr_result);
	
	//send context to lock TA into target list
	pvr_context cxt = {(list << PVR_TA_CMD_TYPE_SHIFT) | 0x80840012};
	sq_cpy((void *)PVR_TA_INPUT, &cxt, sizeof(cxt));
}

//sends any buffered commands to the pvr
void dcglFlushCmdbuffs() {
	int i;
	
	dcgl_cmd.curbuff->curpos = dcgl_cmd.datadst;	//flush cached position
	for(i = 0; i < DCGL_CMDBUFFS; i++) {
		//main list is sent directly to hw, and not buffered
		if (i == dcgl_cmd.mainlist) {
			dcglLogSubmit("list %i is direct",i);
			continue;
		}
		//does this list have something in it?
		if (dcgl_cmd.buff[i].curpos > dcgl_cmd.buff[i].start) {
			//make sure list isn't null
			dcglHardAssert(dcgl_cmd.buff[i].start);
			
			dcglBeginList(i);
			
			//check list length is multiple of 32
			int blocks = (dcgl_cmd.buff[i].curpos - dcgl_cmd.buff[i].start) / 32;
			dcglLogSubmit("list %i -- bytes: %i, blocks %i", i, dcgl_cmd.buff[i].curpos - dcgl_cmd.buff[i].start, blocks);
			dcglHardAssert(((dcgl_cmd.buff[i].curpos - dcgl_cmd.buff[i].start) % 32) == 0);
			dcglHardAssert(((unsigned int)dcgl_cmd.buff[i].start % 32) == 0);
			dcglHardAssert(blocks > 0);
			dcglHardAssert(blocks < (6*1024*1024/32));	//sanity check
			int pvr_result = pvr_dma_load_ta(dcgl_cmd.buff[i].start, blocks*32, 1, NULL, 0);
			if (pvr_result) dcglLog("list %i dma error %i",i,pvr_result);
			
			pvr_result = pvr_list_finish();
			if (pvr_result) dcglLog("list finish error %i",pvr_result);
		} else {
			dcglLogSubmit("list %i is empty",i);
		}
	}
}

void *dcglSQEngage() {
	//dcglLog("%p", dcglGetCmdbuffPos());
	return sq_prepare(dcglGetCmdbuffPos());
}

void dcglInit(int fsaa)
{
	extern void dcglRegenerateState();
	
	dcglLog("Initializing");
	
	//dcglTargetHW();
	dcglInitCmdbuff();
	dcglInitMatrices(fsaa);
	dcglInitTextures();
	pc_set_default_polygon(&dcgl_current_context);
	dcglRegenerateState(); 
}

//	SCENE MARKER
void dcglSetInScene(int i) {
	//dcglLog("%i -> %i", dcgl_in_scene, i);
	dcgl_prim.in_scene = i;
}

void glBeginSceneEXT(void) {
	dcglHardAssert(!dcglIsInScene());
	
	int pvr_result;
	pvr_result = pvr_wait_ready();
	if (pvr_result) dcglLog("pvr wait error %i",pvr_result);
	
	dcglClearCmdbuff();
	pvr_scene_begin();
	
	if (dcgl_cmd.mainlist < DCGL_CMDBUFFS)
		dcglBeginList(dcgl_cmd.mainlist);
	
	dcglSetInScene(1);
	dcglSetPVRStateDirty(1);
}

void glEndSceneEXT(void) {
	dcglHardAssert(dcglIsInScene());
	
	extern void dcglApplyFogGlobals();
	dcglApplyFogGlobals();

	int pvr_result;
	if (dcgl_cmd.mainlist < DCGL_CMDBUFFS) {
		pvr_result = pvr_list_finish();
		if (pvr_result) dcglLog("list finish error %i",pvr_result);
	}
	
	dcglFlushCmdbuffs();
	
	pvr_result = pvr_scene_finish();
	if (pvr_result) dcglLog("scene finish error %i",pvr_result);
	dcglSetInScene(0);
}

//	ERROR HANDLING
static GLenum dcgl_current_error = GL_NO_ERROR;
static const char *dcgl_error_func = "";
//official: const GLubyte* dcgluErrorString(GLenum errorCode) {
const char* dcgluErrorString(GLenum errorCode) {
	const char *errstr = "";
	switch(errorCode) {
	case GL_NO_ERROR:		errstr = "GL_NO_ERROR"; break;
	case GL_INVALID_ENUM:		errstr = "GL_INVALID_ENUM"; break;
	case GL_INVALID_VALUE:		errstr = "GL_INVALID_VALUE"; break;
	case GL_INVALID_OPERATION:	errstr = "GL_INVALID_OPERATION"; break;
	case GL_STACK_OVERFLOW:		errstr = "GL_STACK_OVERFLOW"; break;
	case GL_STACK_UNDERFLOW:	errstr = "GL_STACK_UNDERFLOW"; break;
	case GL_OUT_OF_MEMORY:		errstr = "GL_OUT_OF_MEMORY"; break;
	//case GL_TABLE_TOO_LARGE:	errstr = "GL_TABLE_TOO_LARGE"; break;
	default:	errstr = "***UNKNOWN ERROR*** (should not happen)";
	}
	return errstr;
}
void dcglSetError(GLenum error, const char *func) {
	if (dcgl_current_error == GL_NO_ERROR) {
		dcgl_error_func = func;
		dcgl_current_error = error;
		dcglLog("set: %s from %s", dcgluErrorString(error), func);
	} else {
		dcglLog("dropped: %s from %s", dcgluErrorString(error), func);
	}
}
GLenum glGetError(void) {
	GLenum reterror = dcgl_current_error;
	dcgl_current_error = GL_NO_ERROR;
	dcgl_error_func = "";
	return reterror;
}

void dcglPrintError(void) {
	if (dcgl_current_error == GL_NO_ERROR) {
		dcglLog("No error");
	} else {
		dcglLog("%s at %s", dcgluErrorString(dcgl_current_error), dcgl_error_func);
		dcglGetError();	//call to clear error
	}
}
