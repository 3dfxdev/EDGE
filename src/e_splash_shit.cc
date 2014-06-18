/* Copyright (c) Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

/* blender renders two spinning icosahedrons (red and green).
   The blending factors for the two icosahedrons vary sinusoidally
   and slightly out of phase.  blender also renders two lines of
   text in a stroke font: one line antialiased, the other not.  */

#include "i_defs.h"
#include "i_defs_gl.h"

#include "g_game.h"
#include "r_misc.h"
#include "r_gldefs.h"
// #include "hu_draw.h"
#include "r_modes.h"
#include "r_texgl.h"



GLfloat light0_ambient[] =
{0.2, 0.2, 0.2, 1.0};
GLfloat light0_diffuse[] =
{0.0, 0.0, 0.0, 1.0};
GLfloat light1_diffuse[] =
{1.0, 0.0, 0.0, 1.0};
GLfloat light1_position[] =
{1.0, 1.0, 1.0, 0.0};
GLfloat light2_diffuse[] =
{0.0, 1.0, 0.0, 1.0};
GLfloat light2_position[] =
{-1.0, -1.0, 1.0, 0.0};
float s = 0.0;
GLfloat angle1 = 0.0, angle2 = 0.0;

//void 
//output(GLfloat x, GLfloat y, char *text)
//{
 // char *p;

//  glPushMatrix();
//  glTranslatef(x, y, 0);
//  for (p = text; *p; p++)
//    glutStrokeCharacter(GLUT_STROKE_ROMAN, *p);
//  glPopMatrix();
//}

void 
display(void)
{
  static GLfloat amb[] =
  {0.4, 0.4, 0.4, 0.0};
  static GLfloat dif[] =
  {1.0, 1.0, 1.0, 0.0};

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_LIGHT1);
  glDisable(GL_LIGHT2);
  amb[3] = dif[3] = cos(s) / 2.0 + 0.5;
  glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
  glMaterialfv(GL_FRONT, GL_DIFFUSE, dif);

  glPushMatrix();
  glTranslatef(-0.3, -0.3, 0.0);
  glRotatef(angle1, 1.0, 5.0, 0.0);
  glCallList(1);        /* render ico display list */
  glPopMatrix();

  glClear(GL_DEPTH_BUFFER_BIT);
  glEnable(GL_LIGHT2);
  glDisable(GL_LIGHT1);
  amb[3] = dif[3] = 0.5 - cos(s * .95) / 2.0;
  glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
  glMaterialfv(GL_FRONT, GL_DIFFUSE, dif);

  glPushMatrix();
  glTranslatef(0.3, 0.3, 0.0);
  glRotatef(angle2, 1.0, 0.0, 5.0);
  glCallList(1);        /* render ico display list */
  glPopMatrix();

  glPushAttrib(GL_ENABLE_BIT);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
//  gluOrtho2D(0, 1500, 0, 1500);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  /* Rotate text slightly to help show jaggies. */
  glRotatef(4, 0.0, 0.0, 1.0);
//  output(200, 225, "HYPER3DGE ");
  glDisable(GL_LINE_SMOOTH);
  glDisable(GL_BLEND);
//  output(160, 100, "edge2.sf.net");
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glPopAttrib();
  glMatrixMode(GL_MODELVIEW);

//  glutSwapBuffers();
}

void 
idle(void)
{
  angle1 = (GLfloat) fmod(angle1 + 0.8, 360.0);
  angle2 = (GLfloat) fmod(angle2 + 1.1, 360.0);
  s += 0.05;
}


bool E_DrawSplash(int millies)
{
	int max_time = 2700;

	millies = millies - 400;

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glEnable(GL_BLEND);

	glDisable(GL_BLEND);

	I_FinishFrame();
	I_StartFrame();

	return (millies >= max_time);  // finished
}

void E_SplashScreen(void)
{
	//srand(time(NULL));
	srand(I_PureRandom());

//	color = rand() & 7;

	int start_millies = I_GetMillies();

	for (;;)
	{
		int millies = I_GetMillies() - start_millies;

		if (E_DrawSplash(millies))
			break;
	}

}
