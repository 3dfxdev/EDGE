#include <kos.h>
#include "minigl.h"

void setup()
{
    pvr_init(&pvr_params);
    glKosInit();
    pvr_set_bg_color(0, 0, 0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluPerspective(45.0f, 640.0f / 480.0f, 0.1f, 100.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
}

int main(int argc, char **argv)
{
	setup();
}
