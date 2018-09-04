/* KallistiOS ##version##

   nehe16.c
   (c)2001 Paul Boese

   Parts (c)2000 Tom Stanis/Jeff Molofee/Benoit Miller
*/

#include <kos.h>
#include <fcntl.h>
#include <stdlib.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <pcx/pcx.h>
#include "watchdog.h"
#include "profile.h"
#include "minigl_override.h"
/* Simple GL example to demonstrate fog (PVR table fog).

   Essentially the same thing as NeHe's lesson16 code.
   To learn more, go to http://nehe.gamedev.net/.

   There is no lighting yet in GL, so it has not been included.

   DPAD controls the cube rotation, button A & B control the depth
   of the cube, button X toggles fog on/off, and button Y toggles fog type.
*/
#define BENCH 0
#define TRILIN 0
#define TRILIN2 0
enum { fsaa = 0 };

//static GLfloat yrot;        /* Y Rotation */
static GLfloat xspeed;      /* X Rotation Speed */
static GLfloat yspeed;      /* Y Rotation Speed */

#if 1
static GLfloat xrot;        /* X Rotation */
static GLfloat yrot;        /* Y Rotation */
static GLfloat z = -5.0f;   /* Depth Into The Screen */
#endif

//static GLfloat xrot=100;        /* X Rotation */
//static GLfloat z = 0.659996f;   /* Depth Into The Screen */

//static GLfloat xrot=189;        /* X Rotation */
//static GLfloat z = 0.379996;   /* Depth Into The Screen */

#if 0
static GLfloat xrot=161;        /* X Rotation */
static GLfloat yrot=17.6;        /* Y Rotation */
static GLfloat z = -2.32;   /* Depth Into The Screen */
#endif

static GLuint  texture;         /* Storage For Texture */

/* Storage For Three Types Of Fog */
GLuint fogType = 0; /* use GL_EXP initially */
GLuint fogMode[] = { GL_EXP, GL_EXP2, GL_LINEAR };
char cfogMode[3][10] = {"GL_EXP   ", "GL_EXP2  ", "GL_LINEAR" };
GLfloat fogColor[4] = { 0.5f, 0.5f, 0.5f, 1.0f }; /* Fog Color */
//GLfloat fogColor[4] = { 0.5f, 1.0f, 0.5f, 1.0f }; /* Fog Color */
int fog = GL_TRUE;


/* Load a texture using pcx_load_texture and glKosTex2D */
/*void loadtxr(const char *fn, GLuint *txr) {
    kos_img_t img;
    pvr_ptr_t txaddr;

    if(pcx_to_img(fn, &img) < 0) {
        printf("can't load %s\n", fn);
        return;
    }

    txaddr = pvr_mem_malloc(img.w * img.h * 2);
    pvr_txr_load_kimg(&img, txaddr, PVR_TXRLOAD_INVERT_Y);
    kos_img_free(&img, 0);

    glGenTextures(1, txr);
    dcglLog("%i", GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, *txr);
    glKosTex2D(GL_RGB565_TWID, img.w, img.h, txaddr);
    dcglLog("tex %p %p", txr, txaddr);
} */

void loadtxr_vq(const char *fn, GLuint *txr) {
    glGenTextures(1, txr);
    dcglLog("%i", GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, *txr);
    
    char buff[1024*384];
    int f = open(fn, O_RDONLY);
    assert(f != -1);
    int amtread = read(f, buff, sizeof(buff));
    close(f);
    dcglLog("%i", amtread);
    
    glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_VQ_565_MM_DC, 1024, 1024, 0, amtread, buff);
}

void loadtxrxor(const char *fn, GLuint *txr) {
    glGenTextures(1, txr);
    dcglLog("%i", GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, *txr);
    int hw = 512;
    unsigned char pattern[hw][hw][4];
    int x,y;
    for(y = 0; y < hw; y++) {
	for(x = 0; x < hw; x++) {
		int c = x ^ y;
		if (hw < 256) {
			c *= 256/hw;
		}
		pattern[y][x][0] = 0;
		pattern[y][x][1] = c;
		pattern[y][x][2] = 0;
		pattern[y][x][0] = pattern[y][x][1] = pattern[y][x][2] = c;
		pattern[y][x][3] = (c >> 4) << 4;
	}
    }
    //glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, hw, hw, 0, GL_RGBA, GL_UNSIGNED_BYTE, &pattern);
    //dcglGenerateMipmap(GL_TEXTURE_2D);
    
}

void draw_gl(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, z);

    glRotatef(xrot, 1.0f, 0.0f, 0.0f);
    glRotatef(yrot, 0.0f, 1.0f, 0.0f);

    glBindTexture(GL_TEXTURE_2D, texture);

    //dcglPrintMatricesKGL();
    
    //dcglLoadContextKGL();
    extern void dcglRegenerateState();
    //dcglRegenerateState();
//    dcglLoadContextKGL();
//    dcglLoadFullMatrixKGL();
    
    //dcglPrintMatrices();
    //dcglLoadFullMatrix();
 //   dcglFlatClip();
//    dcglGouraudClip();
if(1) {
//dcglLog("begin");
    glBegin(GL_QUADS);
    //glColor4f(1,1,1,0.2);
    glColor4f(1,1,1,0.8);
    glColor4f(1,1,1,1);
    //glBegin(GL_LINE_STRIP);
    /* Front Face */
//    glNormal3f(0.0f, 0.0f, 1.0f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f,  1.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(1.0f, -1.0f,  1.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(1.0f,  1.0f,  1.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-1.0f,  1.0f,  1.0f);
    /* Back Face */
//    glNormal3f(0.0f, 0.0f, -1.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(-1.0f,  1.0f, -1.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(1.0f,  1.0f, -1.0f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(1.0f, -1.0f, -1.0f);
    /* Top Face */
//    glNormal3f(0.0f, 1.0f, 0.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-1.0f,  1.0f, -1.0f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-1.0f,  1.0f,  1.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(1.0f,  1.0f,  1.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(1.0f,  1.0f, -1.0f);
    /* Bottom Face */
//    glNormal3f(0.0f, -1.0f, 0.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(1.0f, -1.0f, -1.0f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(1.0f, -1.0f,  1.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f,  1.0f);
    /* Right face */
    glNormal3f(1.0f, 0.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(1.0f, -1.0f, -1.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(1.0f,  1.0f, -1.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(1.0f,  1.0f,  1.0f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(1.0f, -1.0f,  1.0f);
    /* Left Face */
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f,  1.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(-1.0f,  1.0f,  1.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-1.0f,  1.0f, -1.0f);
//dcglLog("end1");
    glEnd();
//dcglLog("end2");
} else if (0) {
//glBegin(GL_TRIANGLES);
glBegin(GL_LINES);
    /* Front Face */
//    glNormal3f(0.0f, 0.0f, 1.0f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f,  1.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(1.0f, -1.0f,  1.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(1.0f,  1.0f,  1.0f);
    /* Back Face */
//    glNormal3f(0.0f, 0.0f, -1.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(-1.0f,  1.0f, -1.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(1.0f,  1.0f, -1.0f);
    /* Top Face */
//    glNormal3f(0.0f, 1.0f, 0.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-1.0f,  1.0f, -1.0f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-1.0f,  1.0f,  1.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(1.0f,  1.0f,  1.0f);
    /* Bottom Face */
//    glNormal3f(0.0f, -1.0f, 0.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(1.0f, -1.0f, -1.0f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(1.0f, -1.0f,  1.0f);
    /* Right face */
//    glNormal3f(1.0f, 0.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(1.0f, -1.0f, -1.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(1.0f,  1.0f, -1.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(1.0f,  1.0f,  1.0f);
    /* Left Face */
//    glNormal3f(-1.0f, 0.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f,  1.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(-1.0f,  1.0f,  1.0f);
    glEnd();
} else if (0) {
	glBegin(GL_TRIANGLE_STRIP);
	//glBegin(GL_QUAD_STRIP);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(-1.0f, -1.0f,  1.0f);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(1.0f, -1.0f,  1.0f);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(-1.0f,  1.0f,  1.0f);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(1.0f,  1.0f,  1.0f);

	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(1.0f, 1.0f, -1.0f);
	
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
} else if (0) {
    /* Back Face */
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(-1.0f,  1.0f, -1.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(1.0f,  1.0f, -1.0f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(1.0f, -1.0f, -1.0f);
    /* Top Face */
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-1.0f,  1.0f, -1.0f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-1.0f,  1.0f,  1.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(1.0f,  1.0f,  1.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(1.0f,  1.0f, -1.0f);
    /* Bottom Face */
//    glNormal3f(0.0f, -1.0f, 0.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(1.0f, -1.0f, -1.0f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(1.0f, -1.0f,  1.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f,  1.0f);
    /* Right face */
    glNormal3f(1.0f, 0.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(1.0f, -1.0f, -1.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(1.0f,  1.0f, -1.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(1.0f,  1.0f,  1.0f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(1.0f, -1.0f,  1.0f);
    /* Left Face */
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f, -1.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f,  1.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(-1.0f,  1.0f,  1.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-1.0f,  1.0f, -1.0f);
//dcglLog("end1");
    glEnd();
} else {
    glBegin(GL_TRIANGLES);
    //glBegin(GL_LINE_LOOP);
    //glBegin(GL_LINE_STRIP);
    //glBegin(GL_POINTS);
    glTexCoord2f(0.0f, 0.0f);
    glColor4f(1,0,0,0);
    glVertex3f(-1.0f, -1.0f,  1.0f);
    glTexCoord2f(1.0f, 0.0f);
    glColor4f(1,1,0,1);
    glVertex3f(1.0f, -1.0f,  1.0f);
    glTexCoord2f(1.0f, 1.0f);
    glColor4f(1,1,1,1);
    glVertex3f(1.0f,  1.0f,  1.0f);
    glEnd();
}

}

pvr_init_params_t params = {
    /* Enable opaque and translucent polygons with size 16 */
    { PVR_BINSIZE_32, PVR_BINSIZE_0, PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_8 },

    /* Vertex buffer size 512K */
    1024 * 1024 * 1.5,
    
    0, fsaa
};

extern uint8 romdisk[];
KOS_INIT_ROMDISK(romdisk);

#if BENCH
#include "bench.c"
#endif

int main(int argc, char **argv) {
    maple_device_t *cont;
    cont_state_t *state;
    GLboolean xp = GL_FALSE;
    GLboolean yp = GL_FALSE;
watchdog_init();
	//dcglTest();
profile_init(512*1024/3);
    /* Initialize KOS */
    pvr_init(&params);
    printf("nehe16 beginning %i\n", PVR_VGET(PVR_PIXEL_CENTER));
    
    //pvr_rt_set_scaler(pvr_rt_get_framebuffer(), pvr_scaler_ratio(0.5) + PVR_SCALER_HSHRINK);
    //pvr_rt_set_clip(pvr_rt_get_framebuffer(), 100, 100, 320, 240);
    //pvr_rt_set_clip(pvr_rt_get_framebuffer(), 220, 130, 460, 350);
    //pvr_rt_set_scaler(pvr_rt_get_framebuffer(), pvr_scaler_ratio(0.5) + PVR_SCALER_HSHRINK);
    //pvr_rt_set_scaler(pvr_rt_get_framebuffer(), pvr_scaler_ratio(0.999) + PVR_SCALER_HSHRINK);
    //pvr_rt_set_scaler(pvr_rt_get_framebuffer(), pvr_scaler_ratio(1) + PVR_SCALER_HSHRINK);
    //pvr_rt_set_scaler(pvr_rt_get_framebuffer(), pvr_scaler_ratio(0.9) );
    //pvr_set_downsample_weights(1.0f/3, 1.0f/3);
    //pvr_set_downsample_weights(0.5, 0);
    //PVR_VSET(PVR_PIXEL_CENTER,0);
    


    /* Get basic stuff initialized */
    //glKosInit();
    dcglInit(fsaa);
#if BENCH
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glOrtho(0, 640, 0, 480, -100, 100);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    //glEnable(GL_TEXTURE_2D);
    //glBindTexture(GL_TEXTURE_2D, 0);

    profile_go();
    quadmark();
    profile_stop();
    profile_save("/pc/profilegl.txt",0);
    return 0;
#endif

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, 640.0f / 480.0f, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_TEXTURE_2D);
    glShadeModel(GL_SMOOTH);
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClearDepth(1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glColor4f(1.0f, 1.0f, 1.0f, 0.5);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    /* Set up the fog */
    glFogi(GL_FOG_MODE, fogMode[fogType]);     /* Fog Mode */
    glFogfv(GL_FOG_COLOR, fogColor);           /* Set Fog Color */
    glFogf(GL_FOG_DENSITY, 0.35f);             /* How Dense The Fog is */
//    glHint(GL_FOG_HINT, GL_DONT_CARE);         /* Fog Hint Value */
    glFogf(GL_FOG_START, 0.0f);                /* Fog Start Depth */
    glFogf(GL_FOG_END, 5.0f);                  /* Fog End Depth */
    glEnable(GL_FOG);                          /* Enables GL_FOG */

    /* Set up the textures */
    //loadtxr("/rd/crate.pcx", &texture);
    loadtxrxor("/rd/crate.pcx", &texture);
    //loadtxr_vq("/rd/fruit.vq", &texture);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_FILTER, GL_FILTER_BILINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    while(1) {
	watchdog_pet();
        cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

        /* Check key status */
        state = (cont_state_t *)maple_dev_status(cont);

        if(!state) {
            printf("Error reading controller\n");
            break;
        }

        if(state->buttons & CONT_START)
            break;

        if(state->buttons & CONT_A) {
            if(z >= -15.0f) z -= 0.02f;
        }

        if(state->buttons & CONT_B) {
            //if(z <= 0.0f) z += 0.02f;
            z += 0.02f;
        }

	if((state->buttons & CONT_X) && !xp) {
		xspeed = 0;
		yspeed = 0;
		DBOS("xrot: %f  yrot: %f  z: %f  clip: %f\n",xrot,yrot,z, dcgl_prim.nearclip);
	}
	
        //*
	if((state->buttons & CONT_X) && !xp) {
            xp = GL_TRUE;
            fogType = (fogType + 1) % 3;
            glFogi(GL_FOG_MODE, fogMode[fogType]);
            DBOS("%s\n", cfogMode[fogType]);
        }
//*/
        
	
        if(!(state->buttons & CONT_X))
            xp = GL_FALSE;
	if (state->ltrig > 10) {
		float shift = (state->ltrig - 10) / 8000.0f;
		dcgl_prim.nearclip += shift;
	}
	if (state->rtrig > 10) {
		float shift = (state->rtrig - 10) / 8000.0f;
		dcgl_prim.nearclip -= shift;
	}

        if((state->buttons & CONT_Y) && !yp) {
            yp = GL_TRUE;
            fog = !fog;
        }

        if(!(state->buttons & CONT_Y))
            yp = GL_FALSE;

        if(state->buttons & CONT_DPAD_UP)
            xspeed -= 0.05f;

        if(state->buttons & CONT_DPAD_DOWN)
            xspeed += 0.05f;

        if(state->buttons & CONT_DPAD_LEFT)
            yspeed -= 0.05f;

        if(state->buttons & CONT_DPAD_RIGHT)
            yspeed += 0.05f;

        /* Begin frame */
        glKosBeginFrame();
	//dcglBeginSceneEXT();
	
        /* Switch fog off/on */
//*        
	if(fog) {
            glEnable(GL_FOG);
        }
        else {
            glDisable(GL_FOG);
        }
//*/
        /* Draw the GL "scene" */
	extern int dcgl_trilinear_pass;
	extern void dcglApplyTexture();
	extern void dcglTextureSetPVRDirty(DCGLtexture *tex, int dirty);
if (TRILIN2) {	
	dcgl_trilinear_pass = 0;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	//dcglApplyTexture();
	//dcglSetPVRStateDirty(1);
	glDisable(GL_BLEND);
	glBlendFunc(GL_ONE,GL_ZERO);
}
	//glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	//glAlphaFunc(GL_GREATER, 0.5);
	//glBlendFunc(GL_ONE,GL_ONE);
        draw_gl();
if (TRILIN2) {
	dcgl_trilinear_pass = 1;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	//dcglApplyTexture();
	//dcglSetPVRStateDirty(1);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE,GL_ONE);
        draw_gl();
}
	xrot += xspeed;
	yrot += yspeed;
	
        /* Finish the frame */
	//dcglEndSceneEXT();
        glKosFinishFrame();
    }

    return 0;
}


