
#define TRISTRIP 1
#if TRISTRIP == 1
#define NUMTRIS	6
#else
#define NUMTRIS 2
#endif

enum { PHASE_HALVE, PHASE_INCR, PHASE_DECR, PHASE_FINAL };

int polycnt;
int phase = PHASE_HALVE;
float avgfps = -1;
float milli = 0;

void running_stats() {
    pvr_stats_t stats;
    pvr_get_stats(&stats);

    if(avgfps == -1)
	avgfps = stats.frame_rate;
    else
	avgfps = (avgfps + stats.frame_rate) / 2.0f;
}

void stats() {
    pvr_stats_t stats;

    pvr_get_stats(&stats);
    dbglog(DBG_DEBUG, "3D Stats: %d VBLs, frame rate ~%f fps\n",
	   stats.vbl_count, stats.frame_rate);
}


int check_start() {
    maple_device_t *cont;
    cont_state_t *state;

    cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

    if(cont) {
	state = (cont_state_t *)maple_dev_status(cont);

	if(state)
	    return state->buttons & CONT_START;
    }

    return 0;
}
/*
pvr_poly_hdr_t hdr;

void setup() {
    pvr_init(&pvr_params);
    glKosInit();
    pvr_set_bg_color(0, 0, 0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glOrtho(0, 640, 0, 480, -100, 100);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
} */

int fastrand() {
	static unsigned long x=123456789, y=362436069, z=521288629;
	
	unsigned long t;
	x ^= x << 16;
	x ^= x >> 5;
	x ^= x << 1;

	t = x;
	x = y;
	y = z;
	z = t ^ x ^ y;

	return z;
}

void do_frame() {
    int x, y, z;
    int size;
    int i;
    float col;

    glKosBeginFrame();
    profile_t frametime = PfStart();
    vid_border_color(0, 255, 0);

    for(i = 0; i < polycnt; i++) {
	    if (!TRISTRIP) {
		    glBegin(GL_QUADS);
			x = (fastrand() % 512) * (640.0f/512.0f);
			y = fastrand() % 512 - 32;
			z = fastrand() % 128 + 1;
			size = fastrand() % 64 + 1;
			col = (fastrand() % 256) * 0.00391f;

			glColor3f(col, col, col);
			glVertex3f(x - size, y - size, z);
			glVertex3f(x + size, y - size, z);
			glVertex3f(x + size, y + size, z);
			glVertex3f(x - size, y + size, z);
		    glEnd();
	    } else {
		glBegin(GL_TRIANGLE_STRIP);
		//glBegin(GL_TRIANGLES);
			x = (fastrand() % 512) * (640.0f/512.0f);
			y = fastrand() % 512 - 32;
			z = fastrand() % 128 + 1;
			//size = fastrand() % 64 + 1;
			size = (fastrand() & 63) + 1;
			col = (fastrand() & 255) * 0.00391f;
			//col = (fastrand() % 256) * 0.00391f;
			if (NUMTRIS >= 7) {
				glColor3f(col/2, col/2, col/2);
				glVertex3f(x - size*2, y + size/2, z);
			}
			if (NUMTRIS >= 8)
				glVertex3f(x - size*2, y - size/2, z);
			
			glColor3f(col/2, col, col);
			glVertex3f(x - size, y + size/2, z);
			glVertex3f(x - size, y - size/2, z);
			glColor3f(col, col/2, col);
			glVertex3f(x       , y + size/2, z);
			if (NUMTRIS >= 2)
				glVertex3f(x       , y - size/2, z);
			if (NUMTRIS >= 3) {
				glColor3f(col, col, col/2);
				glVertex3f(x + size, y + size/2, z);
			}
			if (NUMTRIS >= 4)
				glVertex3f(x + size, y - size/2, z);
			if (NUMTRIS >= 5) {
				glColor3f(col/2, col/2, col/2);
				glVertex3f(x + size*2, y + size/2, z);
			}
			if (NUMTRIS >= 6)
				glVertex3f(x + size*2, y - size/2, z);
		    glEnd();
	    }
    }
    milli = milli * 0.05 + PfMillisecs(PfEnd(frametime)) * 0.95;

    vid_border_color(255, 0, 0);
    glKosFinishFrame();
    vid_border_color(0, 0, 0);
}

time_t start;
void switch_tests(int ppf) {
    printf("Beginning new test: %d polys per frame (%d per second at 60fps)\n",
	   ppf  * NUMTRIS, ppf  * NUMTRIS * 60);
    avgfps = -1;
    polycnt = ppf;
}

void check_switch() {
    time_t now;

    now = time(NULL);

    if(now >= (start + 5)) {
	start = time(NULL);
	avgfps = 60 * (16.666 / milli);
	float proc = polycnt * NUMTRIS / milli * 1000;
	int tris = polycnt * avgfps * NUMTRIS;
	printf("Milli: %f, T/s: %f, Tri: %i", milli, proc, polycnt * NUMTRIS);
	printf("  Average Frame Rate: ~%f fps (%d pps)\n", avgfps, tris);
/*
	switch(phase) {
	    case PHASE_HALVE:

		if(avgfps < 55) {
		    switch_tests(polycnt / 1.2f);
		}
		else {
		    printf("  Entering PHASE_INCR\n");
		    phase = PHASE_INCR;
		}

		break;
	    case PHASE_INCR:

		if(avgfps >= 55) {
		    switch_tests(polycnt + 60);
		}
		else {
		    printf("  Entering PHASE_DECR\n");
		    phase = PHASE_DECR;
		}

		break;
	    case PHASE_DECR:

		if(avgfps < 55) {
		    switch_tests(polycnt - 15);
		}
		else {
		    printf("  Entering PHASE_FINAL\n");
		    phase = PHASE_FINAL;
		}

		break;
	    case PHASE_FINAL:
		break;
	} // */
    }
}


void quadmark() {
    switch_tests(2400);
    start = time(NULL);

    for(;;) {
    watchdog_pet();
//	    dcglLog("hit");
	if(check_start())
	    break;

//	    dcglLog("hit");
	printf(" \r");
	
	do_frame();
	
//	    dcglLog("hit");
	running_stats();
//	    dcglLog("hit");
	check_switch();
//	    dcglLog("hit");
    }

    stats();
}

