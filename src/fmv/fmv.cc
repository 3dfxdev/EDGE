/*
 *
 * Full-Motion Video Player - Copyright 2016 by Chilly Willy <ChillyWillyGuru@gmail.com>
 *
 * This code falls under the BSD license.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fmv.h"
#include "avi.h"
#include "roq/roq.h"

#include "../system/i_system.h"

static int movie_init = 0;

extern avi_header_t avi_header;
extern stream_header_t stream_header;
extern stream_format_t stream_format;
extern stream_header_t astream_header;
extern stream_format_auds_t astream_format;

extern GLuint tex[NUM_GL_TEXTURES];
extern short *pcm_buffer;
extern unsigned short *frame[2];
extern unsigned char *read_buffer;

extern int texture_width, texture_height;
extern float scale_width, scale_height;


void I_StartupFMVMovie(void)
{
	//memset(pcm_buffer, 0, fname);

	movie_init = 1;
	I_Printf("I_StartupMovie: initialisation OK\n");
}

void I_ShutdownFMVMovie(void)
{
	if (!movie_init)
		return;

	movie_init = 0;
	I_Printf("I_ShutdownMovie: shut down OK\n");
}


int I_PlayFMVMovie(char *fname)
{
	int ret;
	int loop = 0;
	GLuint texture;
	short *chunk;
	int chunk_samples;
	float chunk_scale;
	bool paused = false;
	callbacks_t *cbs;

	epi::file_c *F = epi::FS_Open(fname, epi::file_c::ACCESS_READ | epi::file_c::ACCESS_BINARY);

	if ((!F) && (!movie_init))
	{
#ifdef FMV_DEBUG
		I_Printf("FMV_PLAY: Couldn't open %s\n", fname);
#endif
		return FMV_ERR_NOFILE;
	}

	// get avi info
	ret = parse_avi(F);

	I_Printf("\nI_PlayMovie: [%s]\n", fname);

	if (ret < FMV_ERR_NONE)
	{
		delete F; // close file

#ifdef FMV_DEBUG
		I_Printf("FMV_PLAY: Error %d parsing %s info\n", ret, fname);
#endif
		return ret; // error while parsing avi
	}

	chunk_samples = (96000 * avi_header.TimeBetweenFrames / 1000000 + 1) / 2;
	chunk_scale = (float)astream_format.samples_per_second / 48000.0f;

	ret = cbs->setup(texture_width, texture_height, scale_width, scale_height, chunk_samples);
	if (ret < FMV_ERR_NONE)
	{
		delete F; // close file

#ifdef FMV_DEBUG
		I_Printf("FMV_PLAY: Error %d during setup\n", ret);
#endif
		return ret; // error while setting up player
	}

	// allocate an audio chunk
	chunk = new short[chunk_samples * 2]; // stereo 16-bit pcm

	do
	{
		int size = -1;
		bool vrdy = false, ardy = false;

		// seek to start and return size
		ret = parse_movi(F, &size, false, NULL, false, NULL, 0, 0.0f);
		if (ret < FMV_ERR_NONE)
			break;

		while (size > 0)
		{
			if (!paused)
			{
				while (!vrdy || !ardy)
				{
					ret = parse_movi(F, &size, !vrdy, &texture, !ardy,
									 chunk, chunk_samples, chunk_scale);
					if (ret < FMV_ERR_NONE)
						break;
					if (ret > FMV_ERR_NONE)
					{
						if (ret == FMV_VID_UPDATE || ret == FMV_AV_UPDATE)
							vrdy = true;
						if (ret == FMV_AUD_UPDATE || ret == FMV_AV_UPDATE)
							ardy = true;

						ret = FMV_ERR_NONE;
					}
				}
				if (ret == FMV_ERR_NONE)
				{
					// next frame
					cbs->sync(texture, chunk);
					vrdy = ardy = false;
				}
			}
			if (ret < FMV_ERR_NONE)
				break;

			// handle input
			ret = cbs->input();
			if (ret == FMV_ICODE_STOP)
			{
				ret = FMV_ERR_STOPPED;
				break;
			}
			else if (ret == FMV_ICODE_PAUSE)
			{
				paused = paused ? false : true;
			}
			else if (ret == FMV_ICODE_RESTART)
			{
				size = -1;
				vrdy = ardy = false;
				// seek to start and return size
				ret = parse_movi(F, &size, false, NULL, false, NULL, 0, 0.0f);
				if (ret < FMV_ERR_NONE)
					break;
			}

			ret = FMV_ERR_NONE;
		}
		if (ret == FMV_ERR_ENDMOVI)
			ret = FMV_ERR_NONE;

		if (loop > 0)
			loop--;

	} while (loop && (ret == FMV_ERR_NONE));

	// free audio chunk
	if (chunk)
		delete chunk;

	// delete circular OpenGL texture buffer
	if (tex[0])
		glDeleteTextures(NUM_GL_TEXTURES, tex);

	// delete circular pcm buffer
	if (pcm_buffer)
		delete pcm_buffer;

	// delete parsing buffer
	if (read_buffer)
		delete read_buffer;

	// delete video decode buffers
	if (frame[1])
		delete frame[1];
	if (frame[0])
		delete frame[0];

	// close file
	delete F;

#ifdef FMV_DEBUG
	I_Printf("FMV_PLAY: Exit code %d\n", ret);
#endif
	return ret;
}
