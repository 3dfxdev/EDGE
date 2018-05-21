//----------------------------------------------------------------------------
//  EPI Quake II FileSystem
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2015-2017 Isotope SoftWorks and Contributors.
//  Based on the Quake 2 GPL Source, (C) id Software.
//
//  Uses code from KMQuake II, (C) Mark "KnightMare" Shan.
// 
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include "epi_quake2limits.h"

//q2 filesystem.c
int FS_LoadFile (const char *path, void **buffer)
{
	FILE * inf;
	size_t len = 0;
	const int resize_step = 6000;
	char * buff;
	
	buff = (char*)malloc(32);
	
	inf = fopen (path,"r");
	if (inf != NULL) {
		while(!feof(inf)) {	//resize buffer until it all fits
			//TODO check success
			buff = (char*)realloc(buff,len+resize_step+1);
			len += fread(buff+len,sizeof(char),resize_step, inf);
		}
		fclose(inf);
		buff[len] = 0;
		//TODO check success
		buff = (char*)realloc(buff,len+1);	//trim excess
	}
	
	if (buffer)
		*buffer = buff;
	else
		free(buff);
	
	return len;
}

//from q_shared.c
int Q_strncasecmp (const char *s1, const char *s2, int n)
{
	int		c1, c2;
	
	do
	{
		c1 = *s1++;
		c2 = *s2++;

		if (!n--)
			return 0;		// strings are equal until end point
		
		if (c1 != c2)
		{
			if (c1 >= 'a' && c1 <= 'z')
				c1 -= ('a' - 'A');
			if (c2 >= 'a' && c2 <= 'z')
				c2 -= ('a' - 'A');
			if (c1 != c2)
				return -1;		// strings not equal
		}
	} while (c1);
	
	return 0;		// strings are equal
}

int Q_strcasecmp (const char *s1, const char *s2)
{
	return Q_strncasecmp (s1, s2, 99999);
}

// This eventually needs to be swapped for a better way of token-handling,
// for instance; look in console parsing code! 
char com_token[MAX_TOKEN_CHARS];


char *COM_Parse (char **data_p)
{
	int		c;
	int		len;
	char	*data;

	data = *data_p;
	len = 0;
	com_token[0] = 0;
	

	if (!data)
	{
		*data_p = NULL;
		return "";
	}
		
// skip whitespace
skipwhite:
	while ( (c = *data) <= ' ')
	{
		if (c == 0)
		{
			*data_p = NULL;
			return "";
		}
		data++;
	}
	
// skip // comments
	if (c=='/' && data[1] == '/')
	{
		while (*data && *data != '\n')
			data++;
		goto skipwhite;
	}


// Coraline 7.27.17:
	// Rewrote this code for handling quoted strings, as it would trigger an Access Violation in VS otherwise. 
	if ( c == '\"' ) 
	{
		data++;
		while ( 1 ) 
		{
			c = *data++;
			if ( ( c == '\\' ) && ( *data == '\"' ) ) 
			{
				// allow quoted strings to safely use \" to indicate the " character
				data++;
			} else if ( c == '\"' || !c ) 
			{
				com_token[len] = 0;
				*data_p = ( char * ) data;
				return com_token;

			} 
			else if ( *data == '\n' ) 
			{
				// Nothing yet!
			}
			if ( len < MAX_TOKEN_CHARS - 1 ) 
			{
				com_token[len] = c;
				len++;
			}
		}
	}
#if 0
	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;
			if (c == '\"' || !c)
			{
				com_token[len] = 0;
				*data_p = data;
				return com_token;
			}
			if (len < MAX_TOKEN_CHARS)
			{
				com_token[len] = c;
				len++;
			}
		}
	}
#endif // 0


// parse a regular word
	do
	{
		if (len < MAX_TOKEN_CHARS)
		{
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
	} while (c>32);

	if (len == MAX_TOKEN_CHARS)
	{
//		Com_Printf ("Token exceeded %i chars, discarded.\n", MAX_TOKEN_CHARS);
		len = 0;
	}
	com_token[len] = 0;

	*data_p = data;
	return com_token;
}

/**
* @brief Removes the file extension from a filename
* @sa Com_SkipPath
* @param[in] in The incoming filename
* @param[out] out The stripped filename
* @param[in] size The size of the output buffer
*/
void Com_StripExtension(const char* in, char* out, const size_t size)
{
	char* out_ext = nullptr;
	int i = 1;

	while (in && *in && i < size) {
		*out++ = *in++;
		i++;

		if (*in == '.')
			out_ext = out;
	}

	if (out_ext)
		*out_ext = '\0';
	else
		*out = '\0';
}
