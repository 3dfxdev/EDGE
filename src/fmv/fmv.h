/*
 *
 * Full-Motion Video Player - Copyright 2016 by Chilly Willy <ChillyWillyGuru@gmail.com>
 *
 * This code falls under the BSD license.
 *
 */

#ifndef FMV_H
#define FMV_H

#include "../system/i_defs.h"
#include "../system/i_defs_gl.h"

#include "../../epi/file.h"
#include "../../epi/filesystem.h"
#include "../../epi/file_memory.h"


static unsigned short _fmv_word;
static unsigned int _fmv_long;

#define f_tell(F) F->GetPosition()

#define read_chars(F, buf, len) F->Read(buf, len)

#define f_getc(F)  F->GetCharacters()

#ifdef __LITTLE_ENDIAN__
#define read_word(F) (F->Read(&_fmv_word, 2), _fmv_word)
#define read_long(F) (F->Read(&_fmv_long, 4), _fmv_long)
#else
#define read_word(F) (F->Read(&_fmv_word, 2), ((_fmv_word >> 8) | (_fmv_word << 8)))
#define read_long(F) (F->Read(&_fmv_long, 4), ((_fmv_long >> 24) | ((_fmv_long & 0xFF0000) >> 8) | ((_fmv_long & 0xFF00) << 8) | (_fmv_long << 24)))
#endif


// uncomment to print info about the parsed stream
#define FMV_DEBUG

// uncomment to print unexpected error messages
#define FMV_ERROR

// uncomment to print extra junk and info chunks
//#define FMV_DUMP_EXTRA


/* Input codes to control player */
#define FMV_ICODE_NONE    0
#define FMV_ICODE_STOP    1
#define FMV_ICODE_PAUSE   2 // toggle
#define FMV_ICODE_RESTART 3

/*
 * The player calls this function to setup playback hardware.
 *
 * Return: error code
 */
typedef int (*setup_callback)(int twidth, int theight, float swidth, float sheight, int audblksz);

/*
 * The player loop calls this function to query the input handler.
 *
 * Return: input code
 */
typedef int (*input_callback)();

/*
 * The player loop calls this function when the video texture and audio
 * buffer hold a frame worth of video/audio.
 *
 * Return: error code
 */
typedef int (*sync_callback)(GLuint tex, short *audblk);

typedef struct callbacks_t 
{
	setup_callback setup;
	input_callback input;
	sync_callback  sync;
} callbacks_t;

/* Error codes from player */
#define FMV_AV_UPDATE    3 /* video and audio updated */
#define FMV_AUD_UPDATE   2 /* audio buffer updated */
#define FMV_VID_UPDATE   1 /* video texture updated */
#define FMV_ERR_NONE     0
#define FMV_ERR_ENDMOVI -1 /* no more data in movi chunk */
#define FMV_ERR_STOPPED -2 /* user stopped playback */
#define FMV_ERR_UNKNOWN -3 /* video stream codec unrecognized  */
#define FMV_ERR_CORRUPT -4 /* input video stream corrupt */
#define FMV_ERR_MEMORY  -5 /* ran out of memory while parsing */
#define FMV_ERR_NOFILE  -6 /* couldn't open file */

/*
 * Play a full-motion video file
 * Inputs: filename = name of the file to play
 *         loop = play video in a continuous loop = -1, n > 0 to loop n times
 *         cbs = callback struct for player support functions
 *
 * Return: error code
 */
//int fmv_play(char *filename);


#endif
