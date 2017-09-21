//**************************************************************************
//**
//**	##   ##    ##    ##   ##   ####     ####   ###     ###
//**	##   ##  ##  ##  ##   ##  ##  ##   ##  ##  ####   ####
//**	 ## ##  ##    ##  ## ##  ##    ## ##    ## ## ## ## ##
//**	 ## ##  ########  ## ##  ##    ## ##    ## ##  ###  ##
//**	  ###   ##    ##   ###    ##  ##   ##  ##  ##       ##
//**	   #    ##    ##    #      ####     ####   ##       ##
//**
//**	$Id: glvis.h 4206 2010-04-03 16:32:33Z dj_jl $
//**
//**	Copyright (C) 1999-2006 Jānis Legzdiņš
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//**  GNU General Public License for more details.
//**
//**************************************************************************

#ifndef GLVIS_H
#define GLVIS_H

// HEADER FILES ------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>

// MACROS ------------------------------------------------------------------

#if !defined __GNUC__ && !defined __attribute__
#define __attribute__(whatever)
#endif

// TYPES -------------------------------------------------------------------

class GLVisError
{
public:
	GLVisError(const char *, ...) __attribute__((format(printf, 2, 3)));

	char message[256];
};

class TGLVis
{
public:
	TGLVis() : fastvis(false), verbose(false),
		no_reject(false), testlevel(2), num_specified_maps(0)
	{}
	virtual ~TGLVis()
	{}

	void Build(const char *srcfile, const char* gwafile = NULL);
	virtual void DisplayMessage(const char *text, ...)
		__attribute__((format(printf, 2, 3))) = 0;
	virtual void DisplayStartMap(const char *levelname) = 0;
	virtual void DisplayBaseVisProgress(int count, int total) = 0;
	virtual void DisplayPortalVisProgress(int count, int total) = 0;
	virtual void DisplayMapDone(int accepts, int total) = 0;

	bool fastvis;
	bool verbose;
	bool no_reject;

	int testlevel;

	int num_specified_maps;
	char specified_maps[100][16];
};

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PUBLIC DATA DECLARATIONS ------------------------------------------------

#endif
