#include <assert.h>
#include "profile.h"
//#include "debug.h"
#include "watchdog.h"

#define CLOCKS_PER_SECOND	(200000000.0f)

typedef struct {
	unsigned char addr[3];
} small_address_t;

static kthread_t *profile_thread = NULL;
static volatile int profile_inited = 0;
static volatile int profiling = 0;

static small_address_t *profile_store = NULL;
static int profile_pos = 0;
static int profile_max = 0;

float PfSecs(profile_t time)
{
	return time / 1000000.0f;
}

float PfMillisecs(profile_t time)
{
	return time / 1000.0f;
}

float PfCycles(profile_t time)
{
	return CLOCKS_PER_SECOND * PfSecs(time);
}

float PfCycLoop(profile_t time, int loops)
{
	return PfCycles(time) / loops;
}

void ProfileColorPoint(int red, int green, int blue)
{
#ifdef _arch_dreamcast
	vid_border_color(red, green, blue);
#endif
}




static struct {
	uint32_t color;
	int running;
	profile_t start;
	profile_t duration;
	int highwater_age;
	profile_t highwater_duration;
	char name[12];
	char abbr[4];
} profdat[MAX_PROFILE_POINTS] = {
	{0xffff00ff, 0,	0, 0, 0,0, "SUBMIT",   "S"},
	{0xff00a0a0, 0,	0, 0, 0,0, "COLLISON", "C"},
} ;

static void profile_point_set_color(uint32_t color)
{
	#ifdef _arch_dreamcast
		vid_border_color((color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff);
	#endif
}

void profile_point_end(profile_point prof_idx)
{
	profdat[prof_idx].highwater_age++;
	if (profdat[prof_idx].highwater_age > 60) {
		if (profdat[prof_idx].highwater_duration < 80)
			profdat[prof_idx].highwater_duration = 0;
		else
			profdat[prof_idx].highwater_duration -= 100;
	}
		
	profdat[prof_idx].duration = PfEnd(profdat[prof_idx].start);
	profdat[prof_idx].running = 0;
	if (profdat[prof_idx].duration > profdat[prof_idx].highwater_duration) {
		profdat[prof_idx].highwater_duration = profdat[prof_idx].duration;
		profdat[prof_idx].highwater_age = 0;
	}
}

void profile_point_start(profile_point prof_idx)
{
	profdat[prof_idx].running = 1;
	profile_point_set_color(profdat[prof_idx].color);
	profdat[prof_idx].start = PfStart();
}

uint32_t profile_point_get_color(profile_point prof_idx)
{
	return profdat[prof_idx].color;
}

void profile_point_start_colored(profile_point prof_idx)
{
	profile_point_set_color(profile_point_get_color(prof_idx));
	profile_point_start(prof_idx);
}

profile_t profile_point_get_highwater(profile_point prof_idx)
{
	return profdat[prof_idx].highwater_duration;
}

profile_t profile_point_get_last_time(profile_point prof_idx)
{
	return profdat[prof_idx].duration;
}

void profile_point_string(profile_point *pfp_idxs, int points, char *dst)
{
	int buffused = 0;
	
	buffused += sprintf(dst, "%s: %03.2f ms", 
		profdat[*pfp_idxs].abbr,
		PfMillisecs(profdat[*pfp_idxs].duration));
	
	points--;
	while(points--) {
		pfp_idxs++;
		buffused += sprintf(dst + buffused, "  %s: %03.2f ms", 
			profdat[*pfp_idxs].abbr,
			PfMillisecs(profdat[*pfp_idxs].duration));
	}
	assert(buffused < 80);
}

void profile_init(size_t size)
{
	assert(watchdog_running());
	assert(!profile_inited);
	
	profile_pos = 0;
	profiling = 0;
	profile_thread = thd_current;
	profile_max = size;
	if (profile_store == NULL)
		profile_store = malloc(sizeof(small_address_t) * profile_max);
	
	profile_inited = 1;
}

void profile_reset(void)
{
	assert(profile_inited);
	profile_pos = 0;
	profiling = 0;
}

void profile_shutdown(void)
{
	assert(profile_inited);
	
	profile_thread = NULL;
	profile_inited = 0;
	profiling = 0;
	if (profile_store)
		free(profile_store);
	profile_store = NULL;
	profile_pos = 0;
	profile_max = 0;
}

void profile_go(void)
{
	assert(profile_inited);
	assert(!profiling);
	profiling = 1;
}

void profile_stop(void)
{
	assert(profile_inited);
	profiling = 0;
}

int profile_running(void)
{
	return profiling;
}

void profile_record_pc(void)
{
#if 1
	static profile_t nextprofile = 0;
	
	if (nextprofile >= PfStart()) {
		return;
	}
#endif
	if (profile_pos >= profile_max) {
		profiling = 0;
		printf("Profile records filled!\n");
		fflush(stdout);
	} else {
		unsigned int pc = (unsigned int)profile_thread->context.pc;
		
		profile_store[profile_pos].addr[0] = pc & 0xff;
		profile_store[profile_pos].addr[1] = (pc>>8) & 0xff;
		profile_store[profile_pos].addr[2] = (pc>>16) & 0xff;
		profile_pos++;
	}
#if 1
	nextprofile = PfStart() + 2000;
#endif
}

int profile_save(const char *fname, int append)
{
	assert(profile_inited);
	assert(!profiling);
	
	printf("Writing %i profile samples to %s... ", profile_pos,fname);
	fflush(stdout);
	
	FILE *of;
	int result = 1;
	size_t written = 0;
	
	of = fopen(fname, append ? "a" : "w");
	if (of != NULL) {
		//written = fwrite(&profile_store[0], sizeof(small_address_t), profile_pos, of);
		int i;
		for(i = 0; i < profile_pos; i++) {
			uint32 addr = (profile_store[i].addr[2] << 16) | (profile_store[i].addr[1] << 8) | profile_store[i].addr[0];
			written += fprintf(of,"8C%06X\n",addr);
		}
		//written = fwrite(&profile_store[0], sizeof(small_address_t), profile_pos, of);
		fclose(of);
		//if (written == profile_pos)
			result = 0;
	}
	
	printf("Done! %i bytes (%i, %i)\n",written,result,ferror(of));
	fflush(stdout);
	
	return result;
}

