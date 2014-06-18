#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/timeb.h>
#include <C:/hyperedge/edge2/lib_win32/SDL/include/SDL.h>
#include <C:/hyperedge/edge2/lib_win32/SDL/include/SDL_audio.h>
#include "roq.h"

SDL_Surface *screen;
SDL_Surface *img;
SDL_Color *col;

#define X11_BUTTONPRESS       1
#define X11_BUTTONRELEASE     2
#define X11_KEYPRESS          3

typedef struct {
	int type;
	int x, y, button;
	int key_code;
} x11_event;

/* -------------------------------------------------------------------------- */
void init_SDL_display(int width, int height, int full_screen)
{
Uint32 vidflags = 0;

	if(SDL_Init(SDL_INIT_VIDEO) < 0 )
		{
		fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
		exit(1);
		}

	atexit(SDL_Quit);

	SDL_EventState(SDL_KEYUP, SDL_IGNORE);
	SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);

	vidflags = SDL_SWSURFACE;
	if(full_screen) vidflags |= SDL_FULLSCREEN;

	if((screen = SDL_SetVideoMode(width, height, 32, vidflags)) == NULL)
		{
		fprintf(stderr, "Couldn't set video mode: %s\n", SDL_GetError());
		exit(1);
		}

	SDL_Delay(500);
	printf("%dx%dx%d  pitch: %d  Shift RGBA: %d,%d,%d,%d\n", screen->w, screen->h,
		screen->format->BitsPerPixel, screen->pitch, screen->format->Rshift,
		screen->format->Gshift, screen->format->Bshift, screen->format->Ashift);
}


/* -------------------------------------------------------------------------- */
void display_SDL(unsigned char *pa, unsigned char *pb, unsigned char *pc,
	int width, int height)
{
SDL_Rect dest;
char *buf = screen->pixels + screen->offset;
int x, y, rs = screen->format->Rshift/8, gs = screen->format->Gshift/8,
	bs = screen->format->Bshift/8;

	if(SDL_MUSTLOCK(screen)) SDL_LockSurface(screen);

	if(screen->format->BitsPerPixel == 32)
		{
#define limit(x) ((((x) > 0xffffff) ? 0xff0000 : (((x) <= 0xffff) ? 0 : (x) & 0xff0000)) >> 16)
		for(y = 0; y < height; y++)
			{
			for(x = 0; x < width/2; x++)
				{
				int r, g, b, y1, y2, u, v;
				y1 = *(pa++); y2 = *(pa++);
				u = pb[x] - 128;
				v = pc[x] - 128;

				y1 *= 65536; y2 *= 65536;
				r = 91881 * v;
				g = -22554 * u + -46802 * v;
				b = 116130 * u;

				buf[(x*8)+rs] = limit(r + y1);
				buf[(x*8)+gs] = limit(g + y1);
				buf[(x*8)+bs] = limit(b + y1);
				buf[(x*8)+4+rs] = limit(r + y2);
				buf[(x*8)+4+gs] = limit(g + y2);
				buf[(x*8)+4+bs] = limit(b + y2);
				}
			if(y & 0x01) { pb += width/2; pc += width/2; }
			buf += screen->pitch;
			}
		}
	else return;

	if(SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);

	dest.x = 0;
	dest.y = 0;
	dest.w = width;
	dest.h = height;

	SDL_UpdateRects(screen, 1, &dest);
}


/* -------------------------------------------------------------------------- */
x11_event *sdl_check_event(int wait)
{
static x11_event ev;
SDL_Event event;
int new_event;

	if(wait) new_event = SDL_WaitEvent(&event);
	else new_event = SDL_PollEvent(&event);

	if(new_event)
		{
		ev.type = -1;
		switch(event.type)
			{
			case SDL_KEYDOWN:
				ev.type = X11_KEYPRESS; ev.key_code = event.key.keysym.sym;
				break;
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				ev.type = (event.type == SDL_MOUSEBUTTONDOWN ? X11_BUTTONPRESS : X11_BUTTONRELEASE);
				ev.button = event.button.button;
				ev.x = event.button.x; ev.y = event.button.y;
				break;
			}

		return(ev.type == -1 ? NULL : &ev);
		}
	return NULL;
}



/*USELESS USAGE*/ 
#if 0
void usage(void)
{
	printf("SRoQPlay by Tim Ferguson.\n\n"
		"sroqplay [options] <RoQ file>\n"
		"   -f  full screen.\n"
		"\n");
}
#endif

/* -------------------------------------------------------------------------- */
double mstime(void)
{
struct timeb tb;

	ftime(&tb);
	return((double)tb.time + (double)tb.millitm/1000.0);
}


/* -------------------------------------------------------------------------- */
void sdl_mixaudio(void *unused, Uint8 *stream, int len)
{
roq_info *ri = (roq_info *)unused;
static int offs, left = 0, end = 0;
int n, pos = 0;

	if(end) return;

	if(left > 0)
		{
		pos = (left > len ? len : left);
		memcpy(stream, ri->audio + offs, pos);
		len -= pos;
		left -= pos;
		offs += pos;
		}

	while(len > 0)
		{
		if(roq_read_audio(ri) > 0)
			{
			n = (ri->audio_size > len ? len : ri->audio_size);
			memcpy(stream + pos, ri->audio, n);
			len -= n;
			pos += n;
			offs = n;
			left = ri->audio_size - n;
			}
		else { end = 1; break; }
		}
}


/* -------------------------------------------------------------------------- */
int main(int argc, char **argv)
{
roq_info *ri;
int ch, full_screen = 0, play_audio = 0, ret;
x11_event *ev;
SDL_AudioSpec fmt;
Uint32 start_tm, cur_tm, display_time;

	while((ch = getopt(argc, argv, "fh")) != EOF)
		{
		switch(ch)
			{
			case 'f': full_screen = 1; break;
			case 'h':
			case '?':
			default: usage();
				return -1;
			}
		}

	if(argc - optind < 1)
		{
		usage();
		return -1;
		}

	if((ri = roq_open(argv[optind])) == NULL) return -1;

	printf("RoQ: length %ld:%02ld  video: %dx%d %ld frames  audio: %d channels.\n",
		ri->stream_length/60000, ((ri->stream_length + 500)/1000) % 60,
		ri->width, ri->height, ri->num_frames, ri->audio_channels);

	init_SDL_display(ri->width, ri->height, full_screen);
	if(ri->audio_channels > 0) play_audio = 1;

	if(play_audio)
		{
		fmt.freq = 22050;
		fmt.format = AUDIO_S16;
		fmt.channels = ri->audio_channels;
		fmt.samples = 512;
		fmt.callback = sdl_mixaudio;
		fmt.userdata = (void *)ri;

		if(SDL_OpenAudio(&fmt, NULL) < 0)
			{
			printf("Unable to open audio: %s\n", SDL_GetError());
			return -1;
			}

		SDL_PauseAudio(0);
		}

	start_tm = SDL_GetTicks();

	while(1)
		{
		if(play_audio)
			{
			SDL_LockAudio();
			ret = roq_read_frame(ri);
			SDL_UnlockAudio();
			}
		else ret = roq_read_frame(ri);
		if(ret <= 0) break;

		if((ev = sdl_check_event(0)) != NULL)
			{
			if(ev->type == X11_BUTTONPRESS || (ev->type == X11_KEYPRESS &&
				(ev->key_code == 27 || ev->key_code == 113))) break;
			}

			/* crappy video time sync  (should be done with audio dev) */
		display_time = (1000 * (Uint32)ri->frame_num)/30;
		cur_tm = SDL_GetTicks();
		while(cur_tm - start_tm < display_time)
			{
			SDL_Delay(10);
			cur_tm = SDL_GetTicks();
			}
		display_SDL(ri->y[0], ri->u[0], ri->v[0], ri->width, ri->height);
		printf("frm: %d  tm: <%d  %d>    \r", ri->frame_num, cur_tm - start_tm, display_time); fflush(stdout);
		}

	if(play_audio) SDL_CloseAudio();
	roq_close(ri);
	return 0;
}


