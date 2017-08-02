//----------------------------------------------------------------------------
//  EDGE2 MD5 Library
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2015 Isotope SoftWorks and Contributors.
//  Portions (C) GSP, LLC.
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
#include <string.h>
#include <stdarg.h>
#include <setjmp.h>

#include "../epi/epi.h"
#include "../epi/math_vector.h"

#define MAX_MD5_ERROR_MSG	200
static jmp_buf md5_error_jmp;
static char md5_error_msg[MAX_MD5_ERROR_MSG];
static struct { char *data, *curtoken; int parsing; } md5_parse;

void I_Error(const char *error,...);
extern "C" 
{
	int Q_strcasecmp (const char *s1, const char *s2);
	char *COM_Parse (char **data_p);
}

#define dbo(a, ...) do { printf(a, __VA_ARGS__); fflush(stdout); } while(0)
#define dbl() do { dbo("[%s %i]\n",__FILE__, __LINE__); } while(0)


void md5_parse_error(const char *fmt, ...) 
{
	//TODO check for message truncation
	va_list args;
	va_start(args, fmt);
	vsnprintf(md5_error_msg, MAX_MD5_ERROR_MSG, fmt,args);
	va_end(args);
	
	longjmp(md5_error_jmp,1);
}

const char * get_token() 
{
	return md5_parse.curtoken;
}

void next_token() 
{
	md5_parse.curtoken = COM_Parse (&md5_parse.data);
}

int token_equals(const char *str) 
{
	return !Q_strcasecmp(get_token(), str);
}

void check_eof(const char *msg) 
{
	if (!md5_parse.data)
		md5_parse_error ("md5 error: unexpected EOF %s\n", msg);
}

void next_token_no_eof(const char *eoferrormsg) 
{
	next_token();
	check_eof(eoferrormsg);
}

void expect_token(const char *str, const char *msg) 
{
	next_token_no_eof("expected token");

	if (!token_equals(str))
		md5_parse_error ("md5 error: expected \"%s\" but got \"%s\" %s\n", str, get_token(), msg);
}

float expect_float() {
	//TODO throw error if not a number!
	float num;
	next_token_no_eof("when expecting float");
	num = strtof(get_token(), NULL);
	return num;
}

int expect_int() {
	//TODO throw error if not a number!
	int num;
	next_token_no_eof("when expecting integer");
	num = strtol(get_token(), NULL, 10);
	return num;
}

void expect_vector(int length, float *vec) {
	while(length--)
		*vec++ = expect_float();
}

void expect_paren_vector(int length, float *vec) 
{
	expect_token("(", "for part of vector");
	expect_vector(length, vec);
	expect_token(")", "for part of vector");
}

epi::vec3_c expect_vec3() 
{
	float t[3];
	expect_paren_vector(3, t);
	return epi::vec3_c(t);
}

void expect_string(char *dst, size_t maxlen) 
{
	next_token_no_eof("expected string");

	size_t len = strlen(get_token());

	if (maxlen && (len < maxlen)) 
	{
		strncpy(dst, get_token(), maxlen);
		dst[maxlen-1] = 0;
	} 
	else 
	{
		md5_parse_error ("the string %s is %i characters long, but the maximum supported length is %i\n", get_token(), len, maxlen-1);
	}
}

void start_parse(char * src) 
{
	if (!src)
	{
		md5_parse_error("Trying to parse null");
	}
	if (md5_parse.parsing) 
	{
		md5_parse_error("Already parsing something");
	}
	md5_error_msg[0] = 0;
	md5_parse.data = src;
	md5_parse.curtoken = src;
	md5_parse.parsing = 1;
}

jmp_buf * parse_jmpbuf() 
{
	return &md5_error_jmp;
}

void end_parse() 
{
	if (!md5_parse.parsing) 
	{
		I_Error("MD5_parse: Tried to end parse when nothing was being parsed!\n");
	}

	md5_parse.data = NULL;
	md5_parse.curtoken = NULL;
	md5_parse.parsing = 0;
}

const char * get_parse_error() 
{
	return md5_error_msg;
}


