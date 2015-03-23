#define _STDCALL_SUPPORTED
#define _M_IX86
#define GLUT_DISABLE_ATEXIT_HACK

#define COMMON_NO_LOADING

#include <windows.h>
#include <GL/glut.h>
//#include "glut.h"
#include <GL/gl.h>
#include <GL/glu.h>
#include <unistd.h>

#include <stdio.h>
#include <assert.h>
#include <math.h>
extern "C" {
#include "png.h"
}
#include "kmq2/byteorder.h"
#include "md5.h"
#include "md5_draw.h"
#include "md5_anim.h"

#define PREMULTIPLY 0

#define ESCAPE 27 /* ASCII code for the escape key. */
int window; /* The number of our GLUT window */

epi::vec3_c camrot(0,0,0);
epi::vec3_c campos(0,0,-5);
epi::vec3_c lightpos(-500,0,-500);

GLuint textureid;
int lighting = 0;
int show_normals = 0;
int show_bones = 0;
int points = 0;

const char * modelfilename;
MD5umodel * umodel;
int animfilenamecnt;
const char ** animfilenames;

void (*render)(void);
void render_md5_direct();

epi::mat4_c posemats[MD5_MAX_JOINTS+1];
MD5model *md5;
MD5animation **md5anims;

extern "C" int FS_LoadFile (const char *path, void **buffer);

void ErrorsGL(char *str)
{
	GLenum error = glGetError();
	while(error) {
		printf("Error %s: %i -> %s\n",str,error, gluErrorString(error));
		error = glGetError();
	}
}


void SharedInit(int Width, int Height)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0f,(GLfloat)Width/(GLfloat)Height,0.1f,1800.0f);
	
	glMatrixMode(GL_TEXTURE);
	
	glMatrixMode(GL_MODELVIEW);

	glFrontFace(GL_CW);
	glEnable(GL_CULL_FACE);
}

void InitGL(int Width, int Height)
{
	glClearDepth(1.0);
	glDepthFunc(GL_LESS);
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.0f, 0.0f, 0.1f, 0.0f);
	glShadeModel(GL_SMOOTH);
	//glShadeModel(GL_FLAT);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	
	SharedInit(Width,Height);
}

void ReSizeGLScene(int Width, int Height)
{
	if (Height==0)
		Height=1;

	glViewport(0, 0, Width, Height);
	
	
	SharedInit(Width,Height);
}

void render_md5_direct_triangle_normal(MD5mesh *msh);
void render_md5_direct_triangle_fullbright_wrap(MD5mesh *msh);
void render_md5_direct_triangle_weightcnt(MD5mesh *msh);
void render_md5_direct_triangle_lighting_wrap(MD5mesh *msh);
static struct {
	const char *name;
	void (*func)(MD5mesh *msh);
	//void (*func)(md5_triangle *t, md5_vertex *v, float *n);
} visualizations[] = {
	{"lighting", render_md5_direct_triangle_lighting_wrap},
	{"vertex normal", render_md5_direct_triangle_normal},
	{"fullbright", render_md5_direct_triangle_fullbright_wrap},
	{"weight count", render_md5_direct_triangle_weightcnt},
	//{"joint control", NULL},
};
int vis_cnt = sizeof(visualizations) / sizeof(visualizations[0]);
int cur_vis = 0;
//*/
int lmpolys = 0, polys = 0, verts_xform = 0;
int force_vertex_light = 0;
int vertex_light = 1;
int show_norms = 0, show_texture = 1, show_wire = 0, show_back = 0, show_bind = 0;
int quant_uv = 0;
int dvert = 0;
int lm = 0;
int cluster = 0;
int pose0 = 0, anim0 = 0;
int pose1 = 1, anim1 = 0;
float lerpweight = 0;

void DrawAxis()
{
	const float r = 25;
	const float p = 1;
	const float n = p/3;
	glDisable(GL_TEXTURE_2D);
	glBegin(GL_LINES);
		glColor3f(p,0,0);
		glVertex3f(0,0,0);
		glVertex3f(r,0,0);
		
		glColor3f(0,p,0);
		glVertex3f(0,0,0);
		glVertex3f(0,r,0);
		
		glColor3f(0,0,p);
		glVertex3f(0,0,0);
		glVertex3f(0,0,r);
		
		glColor3f(n,0,0);
		glVertex3f(0,0,0);
		glVertex3f(-r,0,0);
		
		glColor3f(0,n,0);
		glVertex3f(0,0,0);
		glVertex3f(0,-r,0);
		
		glColor3f(0,0,n);
		glVertex3f(0,0,0);
		glVertex3f(0,0,-r);
	glEnd();
}

static MD5jointposebuff jp0;
static basevert vbuff[10000];
	
void md5_bindpose_matrices(MD5model *md5, epi::mat4_c *dst) {
	md5_pose_load_identity(md5->joints, md5->jointcnt, jp0);
	md5_pose_to_matrix(jp0, md5->jointcnt, dst);
}

void md5_draw_unified(MD5umodel *umd5, epi::mat4_c *jointmats) {
	int i;
	MD5model *md5 = &umd5->model;
	
	for(i = 0; i < md5->meshcnt; i++) {
		MD5mesh *msh = md5->meshes + i;
		glBindTexture(GL_TEXTURE_2D, msh->gltex);
		
		md5_transform_vertices(msh, jointmats, vbuff);
		//md5_transform_vertices(msh, jointmats, vbuff);
		render_md5_direct_triangle_lighting(msh, vbuff);
		
	}
}

/* The main drawing function. */
void DrawGLScene()
{
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glLoadIdentity();
	
	GLfloat light_ambient[] = { 1,1,1,1 };
	GLfloat light_position[] = { 1.0, 0.0, 0.0, 0.0 };
	glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	
	
	glRotatef(90,-1.0f,0.0f,0.0f);
	glTranslatef(0,13,-4);
	glTranslatef(-campos.x,-campos.z,-campos.y);
	glRotatef(camrot.x,1.0f,0.0f,0.0f);
	glRotatef(camrot.y,0.0f,0.0f,1.0f);
	
	
	DrawAxis();
	
	render_md5_direct();

	GLenum error = glGetError();
	while(error) {
		printf("Error: %i -> %s\n",error, gluErrorString(error));
		error = glGetError();
	}
	
	glutSwapBuffers();
}

/* The function called whenever a key is pressed. */
void keyPressed(unsigned char key, int x, int y) 
{
	float stepsize = 0.2, largestep = stepsize * 16;
	int new_vis = cur_vis;
	usleep(100);
	
	epi::vec3_c step(0,0,0);
	
	switch(key) {
	case '1':	new_vis = 0; break;
	case '2':	new_vis = 1; break;
	case '3':	new_vis = 2; break;
	case '4':	new_vis = 3; break;
	
	case ESCAPE:	glutDestroyWindow(window); exit(0); break;
	case 'r':	step.z -= stepsize; break;
	case 'R':	step.z -= largestep; break;
	case 'f':	step.z += stepsize; break;
	case 'F':	step.z += largestep; break;
	case 'a':	step.x -= stepsize; break;
	case 'A':	step.x -= largestep; break;
	case 'd':	step.x += stepsize; break;
	case 'D':	step.x += largestep; break;
	case 's':	step.y -= stepsize; break;
	case 'S':	step.y -= largestep; break;
	case 'w':	step.y += stepsize; break;
	case 'W':	step.y += largestep; break;
	
	case 'q':	camrot.y -= 8; break;
	case 'e':	camrot.y += 8; break;
	case 't':	camrot.x -= 8; break;
	case 'g':	camrot.x += 8; break;
	
	case '0':	lerpweight = 0; break;
	case '[':	pose0--; break;
	case '{':	anim0--; break;
	case ']':	pose0++; break;
	case '}':	anim0++; break;
	case ';':	pose1--; break;
	case ':':	anim1--; break;
	case '\'':	pose1++; break;
	case '\"':	anim1++; break;
	case '-':	lerpweight -= 0.05; break;
	case '=':	lerpweight += 0.05; break;
	//case 'm':	pose1 = pose0 + 1; anim1 = anim0; lerpweight = 0; break;
	case ',':	lerpweight -= 0.05; anim1 = anim0; if (lerpweight < 0) { pose0--; lerpweight = 1; } pose1 = pose0 + 1; break;
	case '.':	lerpweight += 0.05; anim1 = anim0;  if (lerpweight > 1) { pose0++; lerpweight = 0; } pose1 = pose0 + 1; break;
	case '\\':	printf("%i %s: %i  --  %i %s: %i  --  w: %f\n",anim0,animfilenames[anim0],pose0,anim1,animfilenames[anim1],pose1,lerpweight); break;
	
	case 'p':	show_bind = !show_bind; break;
	case 'b':	show_wire = !show_wire; break;
	case 'n':	show_texture = !show_texture; break;
	case 'v':	show_back = !show_back; break;
	case '`':	printf("(%f, %f, %f)\n",campos.x,campos.y,campos.z); break;
	}
	
	if (pose0 < 0)
		pose0 = md5anims[anim0]->framecnt-1;
	if (pose1 < 0)
		pose1 = md5anims[anim1]->framecnt-1;
	if (anim0 < 0)
		anim0 = 0;
	if (anim1 < 0)
		anim1 = 0;
	if (anim0 >= animfilenamecnt)
		anim0 = animfilenamecnt-1;
	if (anim1 >= animfilenamecnt)
		anim1 = animfilenamecnt-1;
	
	new_vis = new_vis < vis_cnt ? new_vis : 0;
	if (new_vis != cur_vis) {
		printf("Visual: %s\n", visualizations[new_vis].name);
		cur_vis = new_vis;
		//render = visualizations[new_vis].func;
	}
	campos += step;
}

int jointlist[100];
int jointlistcnt = 0;
void add_joint(int jointidx) {
	int i;
	for(i = 0; i < jointlistcnt; i++) {
		if (jointlist[i] == jointidx)
			return;
	}
	jointlist[jointlistcnt++] = jointidx;
}

int md5_triangles_weight_cnt(MD5mesh *mesh, int triangleidx) {
	int i;
	md5_vert_idx *vertidxs = mesh->tris[triangleidx].vidx;
	jointlistcnt = 0;
	for(i = 0; i < 3; i++) {
		int j;
		MD5vertex *vertex = mesh->verts + vertidxs[i];
		for(j = 0; j < vertex->weightcnt; j++)
			add_joint(mesh->weights[j + vertex->firstweight].jointidx);
	}
	return jointlistcnt;
}

void render_md5_direct_triangle_lighting_wrap(MD5mesh *msh) {
	render_md5_direct_triangle_lighting(msh, vbuff);
}

void render_md5_direct_triangle_fullbright_wrap(MD5mesh *msh) {
	render_md5_direct_triangle_fullbright(msh, vbuff);
}

void render_md5_direct_triangle_weightcnt(MD5mesh *msh) {
	MD5triangle *ts = msh->tris;
	int j;
	
	glBegin(GL_TRIANGLES);
	
	for(j = 0; j < msh->tricnt; j++) {
		md5_vert_idx *t = ts[j].vidx;
		
		epi::vec3_c *p,*n;
		
		float paint = md5_triangles_weight_cnt(msh, j);
		if (paint == 1)
			glColor3f(0,1,0);
		else if (paint == 2)
			glColor3f(0,0,1);
		else if (paint == 3)
			glColor3f(1,1,0);
		else if (paint <= 4)
			glColor3f(1,0,1);
		else
			glColor3f(1,0,0);
		
		p = &vbuff[t[0]].pos;
		n = &vbuff[t[0]].norm;
		glTexCoord2f(vbuff[t[0]].uv.x, vbuff[t[0]].uv.y);
		glVertex3f(p->x, p->y, p->z);
		
		p = &vbuff[t[1]].pos;
		n = &vbuff[t[1]].norm;
		glTexCoord2f(vbuff[t[1]].uv.x, vbuff[t[1]].uv.y);
		glVertex3f(p->x, p->y, p->z);
		
		p = &vbuff[t[2]].pos;
		n = &vbuff[t[2]].norm;
		glTexCoord2f(vbuff[t[2]].uv.x, vbuff[t[2]].uv.y);
		glVertex3f(p->x, p->y, p->z);
		(void)n;
	}
	glEnd();
}

void render_md5_direct_triangle_normal(MD5mesh *msh) {
	MD5triangle *ts = msh->tris;
	int j;
	glBegin(GL_TRIANGLES);
	for(j = 0; j < msh->tricnt; j++) {
		
		md5_vert_idx *t = ts[j].vidx;
		epi::vec3_c *p,*n;
		
		p = &vbuff[t[0]].pos;
		n = &vbuff[t[0]].norm;
		glColor3f(n->x*0.5+0.5,n->y*0.5+0.5,n->z*0.5+0.5);
		glTexCoord2f(vbuff[t[0]].uv.x, vbuff[t[0]].uv.y);
		glVertex3f(p->x, p->y, p->z);
		p = &vbuff[t[1]].pos;
		n = &vbuff[t[1]].norm;
		glColor3f(n->x*0.5+0.5,n->y*0.5+0.5,n->z*0.5+0.5);
		glTexCoord2f(vbuff[t[1]].uv.x, vbuff[t[1]].uv.y);
		glVertex3f(p->x, p->y, p->z);
		p = &vbuff[t[2]].pos;
		n = &vbuff[t[2]].norm;
		glTexCoord2f(vbuff[t[2]].uv.x, vbuff[t[2]].uv.y);
		glColor3f(n->x*0.5+0.5,n->y*0.5+0.5,n->z*0.5+0.5);
		glVertex3f(p->x, p->y, p->z);
	}
	glEnd();
}

void render_md5_direct() {
	if (!md5)
		return;
	
	MD5jointposebuff jp0,jp1;
	
	if (!show_bind) {
		pose0 = pose0 % md5anims[anim0]->framecnt;
		pose1 = pose1 % md5anims[anim1]->framecnt;
		md5_pose_load(md5anims[anim0], pose0, jp0);
		md5_pose_load(md5anims[anim1], pose1, jp1);
		
		md5_pose_lerp(jp0,jp1,md5->jointcnt,lerpweight,jp0);
	} else {
		md5_pose_load_identity(md5->joints, md5->jointcnt, jp0);
	}
	
	md5_pose_to_matrix(jp0, md5->jointcnt, posemats);
	
	
	if (show_texture)
		glEnable(GL_TEXTURE_2D);
	else
		glDisable(GL_TEXTURE_2D);
	
	glPolygonMode(GL_FRONT_AND_BACK, show_wire ? GL_LINE : GL_FILL);
	
	if (show_back)
		glDisable(GL_CULL_FACE);
	else
		glEnable(GL_CULL_FACE);
	
	int i;
	for(i = 0; i < md5->meshcnt; i++) {
		MD5mesh *msh = md5->meshes + i;
		glBindTexture(GL_TEXTURE_2D, msh->gltex);
		
		md5_transform_vertices(msh, posemats, vbuff);
		
		visualizations[cur_vis].func(msh);
		
	}
	//md5_draw_unified(umodel,posemats);
	glEnable(GL_CULL_FACE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

extern "C" {
	int Q_strcasecmp (const char *s1, const char *s2);
}

static const char *defaultmodelfilename = "snout.md5mesh";
static const char *defaultanimfilename = "swim.md5anim";

void process_args(int argc, char **argv) {
	int i,modelfilenamecnt = 0;
	animfilenames = new const char*[argc];
	
	for(i = 1; i < argc; i++) {
		int filetypeoffset = strlen(argv[i]) - sizeof(".md5mesh") + 1;
		if (filetypeoffset > 1) {
			printf("arg: %s\n",argv[i]);
			if (Q_strcasecmp(".md5mesh", argv[i] + filetypeoffset) == 0) {
				modelfilename = argv[i];
				modelfilenamecnt++;
				if (modelfilenamecnt > 1) {
					printf("Too many meshes specified");
					exit(1);
				}
			} else if (Q_strcasecmp(".md5anim", argv[i] + filetypeoffset) == 0) {
				animfilenames[animfilenamecnt++] = argv[i];
			}
		}
	}
	printf("Meshes: %i, Anims: %i\n",modelfilenamecnt,animfilenamecnt);
	if (modelfilenamecnt == 0) {
		modelfilename = defaultmodelfilename;
		printf("*****Model not specified, trying to use \"%s\" instead\n",modelfilename);
	}
	if (animfilenamecnt == 0) {
		animfilenames[0] = defaultanimfilename;
		printf("*****Animation not specified, trying to use \"%s\" instead\n",animfilenames[0]);
		animfilenamecnt = 1;
	}
}

void load_model() {
	int size;
	char *buff = NULL;
	
	size = FS_LoadFile(modelfilename, reinterpret_cast<void**>(&buff));
	if (size == 0) {
		printf("Error loading model\n");
		exit(2);
	}
	
	MD5model *md5m = md5_load(buff);
	umodel = md5_normalize_model(md5m);
	if (PREMULTIPLY)
		md5_premultiply_unified_model(umodel);
	md5 = &umodel->model;
	
	md5_free(md5m);
	free(buff);
}

void load_anims() {
	int i,size;
	char *animbuff = NULL;
	
	md5anims = new MD5animation*[animfilenamecnt]();
	
	for(i = 0; i < animfilenamecnt; i++) {
		size = FS_LoadFile(animfilenames[i], reinterpret_cast<void**>(&animbuff));
		if (size == 0) {
			printf("Error loading anim %s\n", animfilenames[i]);
			exit(3);
		}
		md5anims[i] = md5_load_anim(animbuff);
	}
}

void load_tex() {
	int i;
	for(i = 0; i < md5->meshcnt; i++) {
		//TODO should really check for room
		strcat(md5->meshes[i].shader, ".png");
		printf("Shader: %s\n",md5->meshes[i].shader);
		md5->meshes[i].gltex = loadPNGTexture(md5->meshes[i].shader);
	}
}

int main(int argc, char **argv) 
{
	setbuf(stdout,NULL);
	Swap_Init();
	
	process_args(argc,argv);
	load_model();
	load_anims();
	
	glutInit(&argc, argv);  
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_STENCIL);  
	glutInitWindowSize(800, 600);  
	glutInitWindowPosition(400, 100);  

	window = glutCreateWindow("MD5 preview");  
	glutDisplayFunc(&DrawGLScene);  
	//glutFullScreen();
	glutIdleFunc(&DrawGLScene);
	glutReshapeFunc(&ReSizeGLScene);
	glutKeyboardFunc(&keyPressed);
	
	InitGL(640, 480);
	
	load_tex();
	
	render = render_md5_direct;
	
	
	glutMainLoop();  

	return 1;
}




