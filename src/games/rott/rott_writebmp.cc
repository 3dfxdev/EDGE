/* writebmp.c */
#include <stdio.h>
//#include "tile.h"

#define put2(field, val)	((field)[0] = (uchar_t)(val),\
				 (field)[1] = (uchar_t)((val) >> 8))
#define put4(field, val)	((field)[0] = (uchar_t)(val),\
				 (field)[1] = (uchar_t)((val) >> 8),\
				 (field)[2] = (uchar_t)((val) >> 16),\
				 (field)[3] = (uchar_t)((val) >> 24))

#define BI_RGB	0	/* no compression */
#define BI_RLE4	2	/* compression for 4-bit values */
#define BI_RLE8	1	/* compression for 8-bit values */

typedef unsigned char uchar_t;
typedef enum {False, True} bool_t;

static struct
{
	uchar_t	type[2];	/* file type, must be "BM" */
	uchar_t	size[4];	/* size of file, little-endian */
	uchar_t	reserved[4];	/* reserved bytes, must be 0 */
	uchar_t	offset[4];	/* offset to data, little-endian */
} filehdr;

static struct
{
	uchar_t	size[4];	/* offset from this struct to palette */
	uchar_t	width[4];	/* width of image */
	uchar_t	height[4];	/* height of image */
	uchar_t	planes[2];	/* must be 1 */
	uchar_t	bpp[2];		/* bits per pixel - 1, 4, 8, or 24 */
	uchar_t	compress[4];	/* compression method */
	uchar_t	bytes[4];	/* number of bytes of image data */
	uchar_t	scalex[4];	/* pixels per meter - may be 0 */
	uchar_t	scaley[4];	/* pixels per meter - may be 0 */
	uchar_t	colors[4];	/* number of colors used */
	uchar_t	important[4];	/* number of important colors - may be 0 */
} bmhdr;

static struct
{
	uchar_t	blue;
	uchar_t	green;
	uchar_t	red;
	uchar_t	reserved;
} bmpcolors[256];


/* this variable is used for counting the bytes of image data */
static long imagebytes = 0;

static void raster8(uchar_t *pix, unsigned width, FILE *fp, bool_t last)
{
	unsigned run;	/* length of a run */
	unsigned x;	/* position in the raster */
	unsigned i;

	/* for each position in the raster line... */
	for (x = 0; x < width; x += run)
	{
		/* count the number of identical pixel values */
		for (run = 1; x + run < width && pix[x + run] == pix[x]; run++)
		{
			if (run == 255)
				break;
		}

		/* if only one pixel, and the following pixels aren't runs
		 * either, then don't use runs.
		 */
		if (run == 1 && x + 3 < width && pix[x + 2] != pix[x + 1] && pix[x + 3] != pix[x + 2])
		{
			/* count the number of mixed pixels */
			for (run = 3; x + run + 2 < width && pix[x + run + 1] != pix[x + run + 2]; run++)
			{
				if (run == 255)
					break;
			}

			/* write the pixel values in absolute mode */
			putc(0, fp);
			putc(run, fp);
			for (i = 0; i < run; i++)
			{
				putc(pix[x + i], fp);
			}
			imagebytes += 2 + run;

			/* align to word boundary */
			if (run & 1)
			{
				putc(0, fp);
				imagebytes++;
			}
		}
		else /* encode as a run */
		{
			putc(run, fp);
			putc(pix[x], fp);
			imagebytes += 2;
		}
	}

	/* end the raster or whole bitmap */
	putc(0, fp);
	putc(last ? 1 : 0, fp);
	imagebytes += 2;
}

static void raster4(uchar_t *pix, unsigned width, FILE *fp, bool_t last)
{
	unsigned run;	/* length of a run */
	unsigned x;	/* position in the raster */
	unsigned i;

	/* for each position in the raster line... */
	for (x = 0; x < width; x += run)
	{
		/* count the number of identical pixel values */
		for (run = 1; x + run < width && pix[x + run] == pix[x]; run++)
		{
			if (run == 255)
				break;
		}

		/* if only one pixel, and the following pixels aren't runs
		 * either, then don't use runs.
		 */
		if (run == 1 && x + 3 < width && pix[x + 2] != pix[x + 1] && pix[x + 3] != pix[x + 2])
		{
			/* count the number of mixed pixels */
			for (run = 3; x + run + 2 < width && pix[x + run + 1] != pix[x + run + 2]; run++)
			{
				if (run == 255)
					break;
			}

			/* write the pixel values in absolute mode */
			putc(0, fp);
			putc(run, fp);
			for (i = 0; i < run; i += 2)
			{
				putc((pix[x + i] << 4) | pix[x + i + 1], fp);
			}
			imagebytes += 2 + i / 2;

			/* align to word boundary */
			if (i & 2)
			{
				putc(0, fp);
				imagebytes++;
			}
		}
		else /* encode as a run */
		{
			putc(run, fp);
			putc(pix[x] * 0x11, fp);
			imagebytes += 2;
		}
	}

	/* end the raster or whole bitmap */
	putc(0, fp);
	putc(last ? 1 : 0, fp);
	imagebytes += 2;
}

static void raster1(uchar_t *pix, unsigned width, FILE *fp, bool_t last)
{
	int	bit;
	int	ch;
	unsigned x;

	/* pack the bits into a byte.  When the byte is full, write it */
	for (x = 0, bit = 128, ch = 0; x < width; x++, bit >>= 1)
	{
		if (bit == 0)
		{
			putc(ch, fp);
			imagebytes++;
			bit = 128;
			ch = 0;
		}

		if (pix[x])
			ch |= bit;
	}

	/* write the final byte now */
	putc(ch, fp);
	imagebytes++;
}

void WriteBmp(char *filename, uchar_t *data, unsigned width, unsigned height, rgb_t colrs[], unsigned ncolors)
{
	FILE	*fp;
	int	i;
	unsigned palsize;
	unsigned long	len;

	imagebytes = 0; //bna added if not 0 then writing to many data

	/* Create the output file */
	fp = fopen(filename, "wb");
	if (!fp)
	{
		perror(filename);
		return;
	}

	/* Write the headers.  We'll rewrite them again later, when we
	 * know the sizes; we're writing then here just as place-holders.
	 */
	fwrite(&filehdr, sizeof filehdr, 1, fp);
	fwrite(&bmhdr, sizeof bmhdr, 1, fp);

	/* choose a palette size */
	if (ncolors > 16)
		palsize = 256;
	else if (ncolors > 2)
		palsize = 16;
	else
		palsize = 2;

	/* Convert the palette and write it out */
	for (i = 0; i < (int)palsize; i++)
	{
		bmpcolors[i].blue = colrs[i].blue;
		bmpcolors[i].green = colrs[i].green;
		bmpcolors[i].red = colrs[i].red;
		bmpcolors[i].reserved = 0;
	}
	fwrite(bmpcolors, sizeof bmpcolors[0], palsize, fp);

	/* write each raster line, starting at the bottom */
	for (i = height - 1; i >= 0; i--)
	{
		/* write the raster line */
		if (palsize == 256)
			raster8(data + i * width, width, fp, (bool_t)(i == 0));
		else if (palsize == 16)
			raster4(data + i * width, width, fp, (bool_t)(i == 0));
		else
			raster1(data + i * width, width, fp, (bool_t)(i == 0));

#if 0
		/* pad the raster line to end on a 32-bit boundary */
		switch (imagebytes & 3)
		{
		  case 1: putc(0, fp);	imagebytes++;	/* fall thru... */
		  case 2: putc(0, fp);	imagebytes++;	/* fall thru... */
		  case 3: putc(0, fp);	imagebytes++;
		}
#endif
	}

	/* Construct the file header, and write it at the top of the file */
	filehdr.type[0] = 'B';
	filehdr.type[1] = 'M';
	len = ftell(fp);
	put4(filehdr.size, len);
	put4(filehdr.reserved, 0);
	put4(filehdr.offset, sizeof filehdr + sizeof bmhdr + palsize * sizeof bmpcolors[0]);
	rewind(fp);
	fwrite(&filehdr, 1, sizeof filehdr, fp);
	
	/* Construct the bitmap header, and write it */
	put4(bmhdr.size, sizeof(bmhdr));
	put4(bmhdr.width, width);
	put4(bmhdr.height, height);
	put2(bmhdr.planes, 1);
	if (palsize == 256)
	{
		put2(bmhdr.bpp, 8);
		put4(bmhdr.compress, BI_RLE8);
	}
	else if (palsize == 16)
	{
		put2(bmhdr.bpp, 4);
		put4(bmhdr.compress, BI_RLE4);
	}
	else
	{
		put2(bmhdr.bpp, 1);
		put4(bmhdr.compress, BI_RGB);
	}
	put4(bmhdr.bytes, imagebytes);
	put4(bmhdr.scalex, 0);
	put4(bmhdr.scaley, 0);
	put4(bmhdr.colors, ncolors);
	put4(bmhdr.important, ncolors);
	fwrite(&bmhdr, sizeof bmhdr, 1, fp);

	/* close the file */
	fclose(fp);
}

