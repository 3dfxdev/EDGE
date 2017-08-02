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


#ifndef _MD5_PARSE_H_
#define _MD5_PARSE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <setjmp.h>

#define MAX_MD5_ERROR_MSG	200

void md5_parse_error(const char *fmt, ...);
const char * get_token();
void next_token();
int token_equals(const char *str);
void expect_token(const char *str, const char *msg);
void check_eof(const char *msg);
void next_token_no_eof(const char *eoferrormsg);

float expect_float();
int expect_int();
void expect_vector(int length, float *vec);
void expect_paren_vector(int length, float *vec);
void expect_string(char *dst, size_t maxlen);

jmp_buf * parse_jmpbuf();
void start_parse(char * src);
#define try_parse() !setjmp(*parse_jmpbuf())

void end_parse();
const char * get_parse_error();

#endif
