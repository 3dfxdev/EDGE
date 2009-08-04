//----------------------------------------------------------------------
//  COAL PUBLIC API
//----------------------------------------------------------------------
// 
//  Copyright (C)      2009  Andrew Apted
//  Copyright (C) 1996-1997  Id Software, Inc.
//
//  Coal is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as
//  published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  Coal is distributed in the hope that it will be useful, but
//  WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
//  the GNU General Public License for more details.
//
//----------------------------------------------------------------------
//
//  Based on QCC (the Quake-C Compiler) and the corresponding
//  execution engine from the Quake source code.
//
//----------------------------------------------------------------------

#ifndef __COAL_API_H__
#define __COAL_API_H__

namespace coal
{

class vm_c;

typedef void (*print_func_t)(const char *msg, ...);

typedef void (*native_func_t)(vm_c *vm);

class vm_c
{
	/* this is an abstract base class */

public:
	vm_c() { }
	virtual ~vm_c() { }

	virtual void SetErrorFunc(print_func_t func) = 0;
	virtual void SetPrintFunc(print_func_t func) = 0;

	virtual void AddNativeFunction(const char *name, native_func_t func) = 0;

	virtual bool CompileFile(char *buffer, const char *filename) = 0;
	virtual void ShowStats() = 0;

	virtual void SetTrace(bool enable) = 0;

	virtual int FindFunction(const char *name) = 0;
	virtual int FindVariable(const char *name) = 0;

	virtual int Execute(int func_id) = 0;

	virtual double     * AccessParam(int p) = 0;
	virtual const char * AccessParamString(int p) = 0;
};

// create a new Coal virtual machine
vm_c * CreateVM();

} // namespace coal

#endif /* __COAL_API_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
