/*
 * png.c -- png texture loader
 * last modification: aug. 14, 2007
 *
 * Copyright (c) 2005-2007 David HENRY
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * gcc -Wall -ansi -lGL -lGLU -lglut
 *    `libpng12-config --cflags --libs` png.c -o png
 */

 #define _STDCALL_SUPPORTED
#define _M_IX86
#define GLUT_DISABLE_ATEXIT_HACK
 
#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#include <png.h>
#include "../lib_win32/libpng-1.2.12/png.h"

/* Microsoft Visual C++ */
#ifdef _MSC_VER
#pragma comment (lib, "libpng.lib")
#pragma comment (lib, "zlib.lib")
#pragma comment (linker, "/nodefaultlib:libc")
#endif	/* _MSC_VER */

/* OpenGL texture info */
struct gl_texture_t
{
  GLsizei width;
  GLsizei height;

  GLenum format;
  GLint internalFormat;
  GLuint id;

  GLubyte *texels;
};

/* Texture id for the demo */
GLuint texId;


static void
GetPNGtextureInfo (int color_type, struct gl_texture_t *texinfo)
{
  switch (color_type)
    {
    case PNG_COLOR_TYPE_GRAY:
      texinfo->format = GL_LUMINANCE;
      texinfo->internalFormat = 1;
      break;

    case PNG_COLOR_TYPE_GRAY_ALPHA:
      texinfo->format = GL_LUMINANCE_ALPHA;
      texinfo->internalFormat = 2;
      break;

    case PNG_COLOR_TYPE_RGB:
      texinfo->format = GL_RGB;
      texinfo->internalFormat = 3;
      break;

    case PNG_COLOR_TYPE_RGB_ALPHA:
      texinfo->format = GL_RGBA;
      texinfo->internalFormat = 4;
      break;

    default:
      /* Badness */
      break;
    }
}

static struct gl_texture_t *
ReadPNGFromFile (const char *filename)
{
  struct gl_texture_t *texinfo;
  png_byte magic[8];
  png_structp png_ptr;
  png_infop info_ptr;
  int bit_depth, color_type;
  FILE *fp = NULL;
  png_bytep *row_pointers = NULL;
  png_uint_32 w, h;
  int i;

  /* Open image file */
  fp = fopen (filename, "rb");
  if (!fp)
    {
      fprintf (stderr, "error: couldn't open \"%s\"!\n", filename);
      return NULL;
    }

  /* Read magic number */
  fread (magic, 1, sizeof (magic), fp);

  /* Check for valid magic number */
  if (!png_check_sig (magic, sizeof (magic)))
    {
      fprintf (stderr, "error: \"%s\" is not a valid PNG image!\n",
	       filename);
      fclose (fp);
      return NULL;
    }

  /* Create a png read struct */
  png_ptr = png_create_read_struct
    (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr)
    {
      fclose (fp);
      return NULL;
    }

  /* Create a png info struct */
  info_ptr = png_create_info_struct (png_ptr);
  if (!info_ptr)
    {
      fclose (fp);
      png_destroy_read_struct (&png_ptr, NULL, NULL);
      return NULL;
    }

  /* Create our OpenGL texture object */
  texinfo = (struct gl_texture_t *)
    malloc (sizeof (struct gl_texture_t));

  /* Initialize the setjmp for returning properly after a libpng
     error occured */
  if (setjmp (png_jmpbuf (png_ptr)))
    {
      fclose (fp);
      png_destroy_read_struct (&png_ptr, &info_ptr, NULL);

      if (row_pointers)
	free (row_pointers);

      if (texinfo)
	{
	  if (texinfo->texels)
	    free (texinfo->texels);

	  free (texinfo);
	}

      return NULL;
    }

  /* Setup libpng for using standard C fread() function
     with our FILE pointer */
  png_init_io (png_ptr, fp);

  /* Tell libpng that we have already read the magic number */
  png_set_sig_bytes (png_ptr, sizeof (magic));

  /* Read png info */
  png_read_info (png_ptr, info_ptr);

  /* Get some usefull information from header */
  bit_depth = png_get_bit_depth (png_ptr, info_ptr);
  color_type = png_get_color_type (png_ptr, info_ptr);

  /* Convert index color images to RGB images */
  if (color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_palette_to_rgb (png_ptr);

  /* Convert 1-2-4 bits grayscale images to 8 bits
     grayscale. */
  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
    png_set_expand_gray_1_2_4_to_8 (png_ptr);

  if (png_get_valid (png_ptr, info_ptr, PNG_INFO_tRNS))
    png_set_tRNS_to_alpha (png_ptr);

  if (bit_depth == 16)
    png_set_strip_16 (png_ptr);
  else if (bit_depth < 8)
    png_set_packing (png_ptr);

  /* Update info structure to apply transformations */
  png_read_update_info (png_ptr, info_ptr);

  /* Retrieve updated information */
  png_get_IHDR (png_ptr, info_ptr, &w, &h, &bit_depth,
		&color_type, NULL, NULL, NULL);
  texinfo->width = w;
  texinfo->height = h;

  /* Get image format and components per pixel */
  GetPNGtextureInfo (color_type, texinfo);

  /* We can now allocate memory for storing pixel data */
  texinfo->texels = (GLubyte *)malloc (sizeof (GLubyte) * texinfo->width
	       * texinfo->height * texinfo->internalFormat);

  /* Setup a pointer array.  Each one points at the begening of a row. */
  row_pointers = (png_bytep *)malloc (sizeof (png_bytep) * texinfo->height);

  for (i = 0; i < texinfo->height; ++i)
    {
#if 1
      row_pointers[i] = (png_bytep)(texinfo->texels +
	((texinfo->height - (i + 1)) * texinfo->width * texinfo->internalFormat));
#else
      row_pointers[i] = (png_bytep)(texinfo->texels +
	 (texinfo->width * i * texinfo->internalFormat));
#endif
    }

  /* Read pixel data using row pointers */
  png_read_image (png_ptr, row_pointers);

  /* Finish decompression and release memory */
  png_read_end (png_ptr, NULL);
  png_destroy_read_struct (&png_ptr, &info_ptr, NULL);

  /* We don't need row pointers anymore */
  free (row_pointers);

  fclose (fp);
  return texinfo;
}

GLuint loadPNGTexture (const char *filename)
{
  struct gl_texture_t *png_tex = NULL;
  GLuint tex_id = 0;
  GLint alignment;

  png_tex = ReadPNGFromFile (filename);

  if (png_tex && png_tex->texels)
    {
      /* Generate texture */
      glGenTextures (1, &png_tex->id);
      glBindTexture (GL_TEXTURE_2D, png_tex->id);

      /* Setup some parameters for texture filters and mipmapping */
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      glGetIntegerv (GL_UNPACK_ALIGNMENT, &alignment);
      glPixelStorei (GL_UNPACK_ALIGNMENT, 1);

#if 0
      glTexImage2D (GL_TEXTURE_2D, 0, png_tex->internalFormat,
		    png_tex->width, png_tex->height, 0, png_tex->format,
		    GL_UNSIGNED_BYTE, png_tex->texels);
#else
      gluBuild2DMipmaps (GL_TEXTURE_2D, png_tex->internalFormat,
			 png_tex->width, png_tex->height,
			 png_tex->format, GL_UNSIGNED_BYTE, png_tex->texels);
#endif

      glPixelStorei (GL_UNPACK_ALIGNMENT, alignment);

      tex_id = png_tex->id;

      /* OpenGL has its own copy of texture data */
      free (png_tex->texels);
      free (png_tex);
    }

  return tex_id;
}

#if 0
	static void
	cleanup ()
	{
	  glDeleteTextures (1, &texId);
	}

	static void
	init (const char *filename)
	{
	  /* Initialize OpenGL */
	  glClearColor (0.5f, 0.5f, 0.5f, 1.0f);
	  glShadeModel (GL_SMOOTH);

	  glEnable (GL_DEPTH_TEST);

	  glEnable (GL_BLEND);
	  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	  /* Load PNG texture from file */
	  texId = loadPNGTexture (filename);
	  if (!texId)
	    exit (EXIT_FAILURE);
	}

	static void
	reshape (int w, int h)
	{
	  if (h == 0)
	    h = 1;

	  glViewport (0, 0, (GLsizei)w, (GLsizei)h);

	  glMatrixMode (GL_PROJECTION);
	  glLoadIdentity ();
	  gluPerspective (45.0, w/(GLdouble)h, 0.1, 1000.0);

	  glMatrixMode (GL_MODELVIEW);
	  glLoadIdentity ();

	  glutPostRedisplay ();
	}

	static void
	display ()
	{
	  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	  glLoadIdentity ();

	  glEnable (GL_TEXTURE_2D);
	  glBindTexture (GL_TEXTURE_2D, texId);

	  /* Draw textured quad */
	  glTranslatef (0.0, 0.0, -5.0);
	  glBegin (GL_QUADS);
	    glTexCoord2f (0.0f, 0.0f);
	    glVertex3f (-1.0f, -1.0f, 0.0f);

	    glTexCoord2f (1.0f, 0.0f);
	    glVertex3f (1.0f, -1.0f, 0.0f);

	    glTexCoord2f (1.0f, 1.0f);
	    glVertex3f (1.0f, 1.0f, 0.0f);

	    glTexCoord2f (0.0f, 1.0f);
	    glVertex3f (-1.0f, 1.0f, 0.0f);
	  glEnd  ();

	  glDisable (GL_TEXTURE_2D);

	  glutSwapBuffers ();
	}

	static void
	keyboard (unsigned char key, int x, int y)
	{
	  /* Escape */
	  if (key == 27)
	    exit (0);
	}

	int
	main (int argc, char *argv[])
	{
	  if (argc < 2)
	    {
	      fprintf (stderr, "usage: %s <filename.png>\n", argv[0]);
	      return -1;
	    }

	  glutInit (&argc, argv);
	  glutInitDisplayMode (GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	  glutInitWindowSize (640, 480);
	  glutCreateWindow ("PNG Texture Demo");

	  atexit (cleanup);
	  init (argv[1]);

	  glutReshapeFunc (reshape);
	  glutDisplayFunc (display);
	  glutKeyboardFunc (keyboard);

	  glutMainLoop ();

	  return 0;
	}
#endif
