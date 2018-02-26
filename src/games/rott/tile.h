/* tile.h */

typedef struct
{
	unsigned red, green, blue;
} 
rgb_t;

typedef struct
{
	double	hue, saturation, value;
} 
hsv_t;
 
/* The following two macros allow rand() to be used instead of drand48().
 * This was done because Microsoft Visual C++ doesn't have drand48(), and
 * apparently even some UNIX systems don't have it.  The rand() function
 * isn't quite as random as drand48(), but it is good enough for this program.
 */
#define drand48()	((double)(rand() & 0x7fff) / 32768.0)
#define srand48(seed)	srand(seed)

/* This macro gives the dimension of a vector */
#define QTY(array)	(sizeof(array) / sizeof(*(array)))

/* This is the entire color table, defined in "util.c" */
extern rgb_t	palette[256];
extern int	palettesize;

/* functions from "util.c" */
extern rgb_t name_to_RGB(char *name);
extern hsv_t RGB_to_HSV(rgb_t rgb);
extern rgb_t HSV_to_RGB(hsv_t hsv);
extern void genpalette(rgb_t firstrgb, rgb_t lastrgb, rgb_t otherrgb, int maxpalette, int cw, int ccw, int bounce);
extern void reduce(unsigned char *imagedata, int width, int height);

/* output functions */
extern void WriteGif(char *filename, unsigned char *img, unsigned width, unsigned height, rgb_t palette[], unsigned ncolors);
extern void WriteBmp(char *filename, unsigned char *img, unsigned width, unsigned height, rgb_t palette[], unsigned ncolors);
#ifdef LINUX
extern void WriteX11(char *filename, unsigned char *img, unsigned width, unsigned height, rgb_t palette[], unsigned ncolors);
#endif

