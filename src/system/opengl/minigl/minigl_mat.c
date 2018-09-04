#include "minigl.h"
#include <math.h>
#include <dc/matrix3d.h>

#define dcglCopyMatrix(src,dst) memcpy((dst),(src),sizeof(matrix_t))
#define dcglSetIdentity(dst) memcpy((dst),dcgl_identity,sizeof(matrix_t))

typedef enum {
	DCGL_MODELVIEW,
	DCGL_PROJECTION,
	DCGL_TEXTURE,
	DCGL_MATRIX_COUNT,	//not a real matrix type, just the number of valid matrix types
} DCGLmatrixtype;

typedef struct {
	matrix_t *stackpos;
	matrix_t *base;
	int stackmax;
	int padding;
	matrix_t mat;
} DCGLmatrixstack;

__attribute__((aligned(32))) matrix_t dcgl_modelview_matrix[DCGL_MAX_MODELVIEW_STACK_DEPTH];
__attribute__((aligned(32))) matrix_t dcgl_projection_matrix[DCGL_MAX_PROJECTION_STACK_DEPTH];
__attribute__((aligned(32))) matrix_t dcgl_texture_matrix[DCGL_MAX_TEXTURE_STACK_DEPTH];
__attribute__((aligned(32))) matrix_t dcgl_viewport;
__attribute__((aligned(32))) matrix_t static dcgl_saved;
__attribute__((aligned(32))) matrix_t const dcgl_identity = {{1,0,0,0},{0,1,0,0},{0,0,1,0},{0,0,0,1}};

int dcgl_screen_matrix_dirty = 1;
int dcgl_2x_horizontal_fsaa;
DCGLmatrixstack *dcgl_current_matrix;
DCGLmatrixtype dcgl_current_matrix_type;
__attribute__((aligned(32))) matrix_t dcgl_screen_matrix;

struct {
	int screen_matrix_dirty;
	int horizontal_2x_fsaa;
	DCGLmatrixstack *current_matrix;
	DCGLmatrixtype current_matrix_type;
	__attribute__((aligned(32))) matrix_t screen_matrix;
} dcgl_mat;

__attribute__((aligned(16))) DCGLmatrixstack dcgl_matrix_stacks[DCGL_MATRIX_COUNT];

inline int dcglIsScreenMatrixDirty() {
	return dcgl_screen_matrix_dirty;
}
inline void dcglSetScreenMatrixDirty(int dirty) {
	dcgl_screen_matrix_dirty = dirty;
}

void dcglPrintMatrices() {
	DBOS("Viewport:\n");
	dcglPrintMatrix(dcgl_viewport);
	
	DBOS("Projection:\n");
	dcglPrintMatrix(dcgl_matrix_stacks[DCGL_PROJECTION].mat);
	
	DBOS("Modelview:\n");
	dcglPrintMatrix(dcgl_matrix_stacks[DCGL_MODELVIEW].mat);
	
	DBOS("\n");
}

void dcglPrintXMTRX() {
	matrix_t __attribute__((aligned(32))) tempmat;
	mat_store(&tempmat);
	DBOS("[ %f %f %f %f ]\n",tempmat[0][0],tempmat[1][0],tempmat[2][0],tempmat[3][0]); 
	DBOS("[ %f %f %f %f ]\n",tempmat[0][1],tempmat[1][1],tempmat[2][1],tempmat[3][1]); 
	DBOS("[ %f %f %f %f ]\n",tempmat[0][2],tempmat[1][2],tempmat[2][2],tempmat[3][2]); 
	DBOS("[ %f %f %f %f ]\n",tempmat[0][3],tempmat[1][3],tempmat[2][3],tempmat[3][3]); 
}

void dcglPrintMatrix(matrix_t tempmat) {
	DBOS("[ %f %f %f %f ]\n",tempmat[0][0],tempmat[1][0],tempmat[2][0],tempmat[3][0]); 
	DBOS("[ %f %f %f %f ]\n",tempmat[0][1],tempmat[1][1],tempmat[2][1],tempmat[3][1]); 
	DBOS("[ %f %f %f %f ]\n",tempmat[0][2],tempmat[1][2],tempmat[2][2],tempmat[3][2]); 
	DBOS("[ %f %f %f %f ]\n",tempmat[0][3],tempmat[1][3],tempmat[2][3],tempmat[3][3]); 
}

void glMatrixMode(GLenum  mode) {
	dcglNotInBegin();
	switch(mode) {
	case GL_MODELVIEW:	dcgl_current_matrix_type = DCGL_MODELVIEW;  break;
	case GL_PROJECTION:	dcgl_current_matrix_type = DCGL_PROJECTION; break;
	case GL_TEXTURE:	dcgl_current_matrix_type = DCGL_TEXTURE;    break;
	default:		dcglSoftAssert(0); return;
	}
	dcgl_current_matrix = dcgl_matrix_stacks + dcgl_current_matrix_type;
}

void glDepthRange(GLclampf n, GLclampf f) {
	dcglNotInBegin();
	n = DCGL_CLAMP01(n);
	f = DCGL_CLAMP01(f);
	
	dcglUnimpAssert(0);
}

void glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
	dcglNotInBegin();
	//TODO not really implemented!
	//dcglUnimpAssert(0);
	//TODO are x and y relative to topleft or bottomleft?
	//TODO read in the screen dimentions from the video driver?
/*	dcgl_viewport[0][0] = width/640.0f;
	dcgl_viewport[3][0] = x + width/2;
	dcgl_viewport[1][1] = -height/480.0f;
	dcgl_viewport[3][1] = height/2/480.0f-y;
	
	dcgl_viewport[0][0] = 240.0f;
	dcgl_viewport[1][1] = 240.0f;
	dcgl_viewport[3][1] = 240.0f;
	dcgl_viewport[3][1] = 320.0f; */
	
	dcgl_viewport[0][0] = width * 0.5f * (dcgl_2x_horizontal_fsaa ? 2 : 1);
	dcgl_viewport[3][0] = width * 0.5f * (dcgl_2x_horizontal_fsaa ? 2 : 1);
	dcgl_viewport[1][1] = -height * 0.5f;
	dcgl_viewport[3][1] = height * 0.5f;
	
	dcglSetScreenMatrixDirty(1);
}

void dcglInitMatrices(int fsaa) {
	dcgl_matrix_stacks[DCGL_MODELVIEW].base     = dcgl_modelview_matrix;
	dcgl_matrix_stacks[DCGL_MODELVIEW].stackpos = dcgl_modelview_matrix;
	dcgl_matrix_stacks[DCGL_MODELVIEW].stackmax = GL_MAX_MODELVIEW_STACK_DEPTH;
	
	dcgl_matrix_stacks[DCGL_PROJECTION].base     = dcgl_projection_matrix;
	dcgl_matrix_stacks[DCGL_PROJECTION].stackpos = dcgl_projection_matrix;
	dcgl_matrix_stacks[DCGL_PROJECTION].stackmax = GL_MAX_PROJECTION_STACK_DEPTH;
	
	dcgl_matrix_stacks[DCGL_TEXTURE].base     = dcgl_texture_matrix;
	dcgl_matrix_stacks[DCGL_TEXTURE].stackpos = dcgl_texture_matrix;
	dcgl_matrix_stacks[DCGL_TEXTURE].stackmax = GL_MAX_TEXTURE_STACK_DEPTH;
	
	dcglSetIdentity(&dcgl_matrix_stacks[DCGL_MODELVIEW ].mat);
	dcglSetIdentity(&dcgl_matrix_stacks[DCGL_PROJECTION].mat);
	dcglSetIdentity(&dcgl_matrix_stacks[DCGL_TEXTURE   ].mat);
	dcglSetIdentity(&dcgl_viewport);
	
	dcgl_2x_horizontal_fsaa = fsaa;
	//dcglViewport(0,0,640,480);
	dcgl_viewport[0][0] = 320.0f * (dcgl_2x_horizontal_fsaa ? 2 : 1);
	dcgl_viewport[3][0] = 320.0f * (dcgl_2x_horizontal_fsaa ? 2 : 1);;
	dcgl_viewport[1][1] = -240.0f;
	dcgl_viewport[3][1] = 240.0f;
	dcglSetScreenMatrixDirty(1);
}

static inline void dcglSaveXMTRX() {
	mat_store(&dcgl_saved);
}

static inline void dcglRestoreXMTRX() {
	mat_load(&dcgl_saved);
}

static inline void dcglLoadCurrentMatrix() {
	mat_load(&dcgl_current_matrix->mat);
}

static inline void dcglStoreCurrentMatrix() {
	mat_store(&dcgl_current_matrix->mat);
}

matrix_t * dcglFullMatrixPtr() {
	if (dcgl_screen_matrix_dirty) {
		mat_load(&dcgl_viewport);
		mat_apply(&dcgl_matrix_stacks[DCGL_PROJECTION].mat);
		mat_apply(&dcgl_matrix_stacks[DCGL_MODELVIEW].mat);
		mat_store(&dcgl_screen_matrix);
		dcgl_screen_matrix_dirty = 0;
	}
	return &dcgl_screen_matrix;
}

void dcglLoadFullMatrix() {
	if (dcgl_screen_matrix_dirty) {
		mat_load(&dcgl_viewport);
		mat_apply(&dcgl_matrix_stacks[DCGL_PROJECTION].mat);
		mat_apply(&dcgl_matrix_stacks[DCGL_MODELVIEW].mat);
		mat_store(&dcgl_screen_matrix);
		dcgl_screen_matrix_dirty = 0;
	} else {
		mat_load(&dcgl_screen_matrix);
	}
}

void dcglSwapInGLMatrix() {
	dcglSaveXMTRX();
	dcglLoadCurrentMatrix();
}

void dcglSwapOutGLMatrix() {
	dcglStoreCurrentMatrix();
	dcglRestoreXMTRX();
}

void glLoadIdentity() {
	dcglNotInBegin();
	dcglSetIdentity(&dcgl_current_matrix->mat);
}

void glPushMatrix() {
	dcglNotInBegin();
	//check for stack overflow
	int stackpos = dcgl_current_matrix->stackpos - dcgl_current_matrix->base;
	dcglSoftAssert(stackpos < dcgl_current_matrix->stackmax);
	
	dcglCopyMatrix(&dcgl_current_matrix->mat, dcgl_current_matrix->stackpos++);
	dcglSetScreenMatrixDirty(1);
}

void glPopMatrix() {
	dcglNotInBegin();
	//check for stack underflow
	dcglSoftAssert(dcgl_current_matrix->stackpos > dcgl_current_matrix->base);
	
	dcglCopyMatrix(--dcgl_current_matrix->stackpos, &dcgl_current_matrix->mat);
	dcglSetScreenMatrixDirty(1);
}

void glLoadMatrix(const float *m) {
	dcglNotInBegin();
	dcglCopyMatrix(m, &dcgl_current_matrix->mat);
	dcglSetScreenMatrixDirty(1);
}

void glMultMatrix(const float *m) {
	dcglNotInBegin();
	//TODO optimize
	static __attribute__((aligned(32))) matrix_t temp;
	dcglCopyMatrix(m, temp);	//make sure the matrix is aligned
	dcglSwapInGLMatrix();
	mat_apply(&temp);
	dcglSwapOutGLMatrix();
	dcglSetScreenMatrixDirty(1);
}

void glLoadTransposeMatrixf(const float *m) {
	dcglNotInBegin();
	dcglUnimpAssert(0);
}

void glMultTransposeMatrixf(const float *m) {
	dcglNotInBegin();
	dcglUnimpAssert(0);
}

void glTranslatef(float x, float y, float z) {
	dcglNotInBegin();
	dcglSwapInGLMatrix();
	mat_translate(x,y,z);
	dcglSwapOutGLMatrix();
	dcglSetScreenMatrixDirty(1);
}

void glScalef(float x, float y, float z) {
	dcglNotInBegin();
	dcglSwapInGLMatrix();
	mat_scale(x,y,z);
	dcglSwapOutGLMatrix();
	dcglSetScreenMatrixDirty(1);
}
#define Deg2Rad(deg) ((deg)*0.0174532925)
void glRotatef(float angle, float x, float y, float z) {
	dcglNotInBegin();
	float s,c;
	static __attribute__((aligned(32))) matrix_t r = {
		{0, 0, 0, 0},
		{0, 0, 0, 0},
		{0, 0, 0, 0},
		{0, 0, 0, 1}};
	angle = Deg2Rad(angle);
	sincosf(angle,&s,&c);
	
	float vector_length = x*x+y*y+z*z;
	if (vector_length) {
		float normfactor = frsqrt(vector_length);
		x *= normfactor;
		y *= normfactor;
		z *= normfactor;
	} else {
		dcglSoftAssert(vector_length != 0);
	}
	
	r[0][0] = x*x*(1-c)+  c;	r[1][0] = x*y*(1-c)-z*s;	r[2][0] = x*z*(1-c)+y*s;
	r[0][1] = y*x*(1-c)+z*s;	r[1][1] = y*y*(1-c)+  c;	r[2][1] = y*z*(1-c)-x*s;
	r[0][2] = x*z*(1-c)-y*s;	r[1][2] = y*z*(1-c)+x*s;	r[2][2] = z*z*(1-c)+  c;
	
	dcglSwapInGLMatrix();
	mat_apply(&r);
	dcglSwapOutGLMatrix();
	dcglSetScreenMatrixDirty(1);
}

void glFrustum(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat nearVal, GLfloat farVal) {
	dcglNotInBegin();
	static __attribute__((aligned(32))) matrix_t fru = {
		{0, 0, 0, 0},
		{0, 0, 0, 0},
		{0, 0, 0,-1},
		{0, 0, 0, 0}};
	
	dcglSoftAssert(nearVal > 0);
	dcglSoftAssert(farVal > 0);
	dcglSoftAssert(left != right);
	dcglSoftAssert(top != bottom);
	dcglSoftAssert(nearVal != farVal);
	
	fru[0][0] = 2*nearVal/(right-left);
	fru[2][0] = (right+left)/(right-left);
	fru[1][1] = 2*nearVal/(top-bottom);
	fru[2][1] = (top+bottom)/(top-bottom);
	//fru[2][2] = -(farVal+nearVal)/(farVal-nearVal);
	//fru[3][2] = -(2*farVal*nearVal)/(farVal-nearVal);
	fru[2][2] = farVal/(farVal-nearVal);
	fru[3][2] = -(farVal*nearVal)/(farVal-nearVal);
	
	dcglSwapInGLMatrix();
	mat_apply(&fru);
	dcglSwapOutGLMatrix();
	dcglSetScreenMatrixDirty(1);
}

void glOrtho(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat nearVal, GLfloat farVal) {
	dcglUntested();
	
	dcglNotInBegin();
	static __attribute__((aligned(32))) matrix_t fru = {
		{0, 0, 0, 0},
		{0, 0, 0, 0},
		{0, 0, 0, 0},
		{0, 0, 0, 1}};
	
	//dcglSoftAssert(nearVal > 0);
	//dcglSoftAssert(farVal > 0);
	dcglSoftAssert(left != right);
	dcglSoftAssert(top != bottom);
	dcglSoftAssert(nearVal != farVal);
	
	fru[0][0] = 2/(right-left);
	fru[3][0] = -(right+left)/(right-left);
	fru[1][1] = 2/(top-bottom);
	fru[3][1] = -(top+bottom)/(top-bottom);
	fru[2][2] = -2/(farVal-nearVal);
	fru[2][3] = -(farVal+nearVal)/(farVal-nearVal);
	
	dcglSwapInGLMatrix();
	mat_apply(&fru);
	dcglSwapOutGLMatrix();
	dcglSetScreenMatrixDirty(1);
}

void gluPerspective(GLfloat angle, GLfloat aspect, GLfloat nearVal, GLfloat farVal) {
	dcglNotInBegin();
	//copied from kglx
	GLfloat xmin, xmax, ymin, ymax;
	ymax = nearVal * ftan(angle * F_PI / 360.0f);
	ymin = -ymax;
	xmin = ymin * aspect;
	xmax = ymax * aspect;

	glFrustum(xmin, xmax, ymin, ymax, nearVal, farVal);
}

void gluOrtho2D(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top) {
	dcglUntested();
	
	dcglNotInBegin();
	glFrustum(left, right, bottom, top, -1, 1);
}
