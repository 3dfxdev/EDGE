#ifndef MINIGLMAT_H
#define MINIGLMAT_H

#ifdef __cplusplus
extern "C" {
#endif

#define DCGL_MAX_MODELVIEW_STACK_DEPTH 32
#define DCGL_MAX_PROJECTION_STACK_DEPTH 2
#define DCGL_MAX_TEXTURE_STACK_DEPTH 2

void dcglInitMatrices(int fsaa);	//call before using any other functions in this file
void dcglLoadFullMatrix();	//loads full model->screen transformation matrix into XMTRX
matrix_t * dcglFullMatrixPtr();
void dcglPrintXMTRX();
void dcglPrintMatrix(matrix_t tempmat);
void dcglPrintMatrices();

void dcglMatrixMode(GLenum  mode);
void dcglDepthRange(GLclampf n, GLclampf f);
void dcglViewport(GLint x, GLint y, GLsizei width, GLsizei height);

void dcglLoadIdentity();
void dcglPushMatrix();
void dcglPopMatrix();
void dcglLoadMatrix(const float *m);
void dcglMultMatrix(const float *m);
void dcglLoadTransposeMatrixf(const float *m);
void dcglMultTransposeMatrixf(const float *m);

void dcglTranslatef(float x, float y, float z);
void dcglScalef(float x, float y, float z);
void dcglRotatef(float angle, float x, float y, float z);

void dcglFrustum(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat nearVal, GLfloat farVal);
void dcglOrtho(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat nearVal, GLfloat farVal);
void dcgluPerspective(GLfloat angle, GLfloat aspect, GLfloat nearVal, GLfloat farVal);	//TODO move me
void dcgluOrtho2D(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top);	//TODO move me

#ifdef __cplusplus
}
#endif

#endif
