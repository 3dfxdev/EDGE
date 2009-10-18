//------------------------------------------------------------------------
//  COAL General Stuff
//------------------------------------------------------------------------
//
//  Copyright (c) 2006-2009  The EDGE Team.
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
//------------------------------------------------------------------------

#include "i_defs.h"

#include "coal/coal.h"

#include "epi/file.h"
#include "epi/filesystem.h"

#include "ddf/main.h"

#include "vm_coal.h"
#include "dm_state.h"
#include "e_main.h"
#include "g_game.h"
#include "version.h"

#include "e_player.h"
#include "hu_font.h"
#include "hu_draw.h"
#include "r_modes.h"
#include "w_wad.h"
#include "z_zone.h"


// user interface VM
coal::vm_c *ui_vm;


void VM_Printer(const char *msg, ...)
{
	static char buffer[1024];

	va_list argptr;

	va_start(argptr, msg);
	vsnprintf(buffer, sizeof(buffer), msg, argptr);
	va_end(argptr);

	buffer[sizeof(buffer)-1] = 0;

	I_Printf("COAL: %s", buffer);
}


void VM_SetFloat(coal::vm_c *vm, const char *name, double value)
{
	// FIXME !!!!!  VM_SetFloat
}

void VM_SetString(coal::vm_c *vm, const char *name, const char *value)
{
	// TODO
}


void VM_CallFunction(coal::vm_c *vm, const char *name)
{
	int func = vm->FindFunction(name);

	if (func == coal::vm_c::NOT_FOUND)
		I_Error("Missing coal function: %s\n", name);

	if (vm->Execute(func) != 0)
		I_Error("Coal script terminated with an error.\n");
}


//------------------------------------------------------------------------
//  BASE MODULES
//------------------------------------------------------------------------


// sys.print(str)
//
static void SYS_print(coal::vm_c *vm, int argc)
{
	const char * s = vm->AccessParamString(0);

	I_Printf("%s\n", s);
}


// sys.debug(str)
//
static void SYS_debug(coal::vm_c *vm, int argc)
{
	const char * s = vm->AccessParamString(0);

	I_Debugf("%s\n", s);
}


// sys.assert(val)
//
static void SYS_assert(coal::vm_c *vm, int argc)
{
	double * p = vm->AccessParam(0);

	if (*p == 0)
		I_Error("Assertion in coal script failed.\n");
}


// math.floor(val)
//
static void MATH_floor(coal::vm_c *vm, int argc)
{
	double * p = vm->AccessParam(0);

	vm->ReturnFloat(floor(*p));
}


// math.ceil(val)
//
static void MATH_ceil(coal::vm_c *vm, int argc)
{
	double * p = vm->AccessParam(0);

	vm->ReturnFloat(ceil(*p));
}


// math.random()
//
static void MATH_random(coal::vm_c *vm, int argc)
{
	int r = rand();

	r = (r ^ (r >> 18)) & 0xFFFF;

	vm->ReturnFloat(r / double(0x10000));
}


//------------------------------------------------------------------------

void VM_RegisterBASE(coal::vm_c *vm)
{
	// SYSTEM
    vm->AddNativeFunction("sys.print",       SYS_print);
    vm->AddNativeFunction("sys.debug",       SYS_debug);
    vm->AddNativeFunction("sys.assert",      SYS_assert);

	// MATH
    vm->AddNativeFunction("math.floor",      MATH_floor);
    vm->AddNativeFunction("math.ceil",       MATH_ceil);
    vm->AddNativeFunction("math.random",     MATH_random);

	// STRINGS
}


void VM_InitCoal()
{
	ui_vm = coal::CreateVM();

	ui_vm->SetPrinter(VM_Printer);

	VM_RegisterBASE(ui_vm);
	VM_RegisterHUD();
	VM_RegisterPlaysim();
}

void VM_QuitCoal()
{
	if (ui_vm)
	{
		delete ui_vm;
		ui_vm = NULL;
	}
}


void VM_LoadCoalFire(const char *filename)
{
	epi::file_c *F = epi::FS_Open(filename, epi::file_c::ACCESS_READ | epi::file_c::ACCESS_BINARY);

	if (! F)
		I_Error("Could not open coal script: %s\n", filename);

	I_Printf("Compiling COAL script: %s\n", filename);

	byte *data = F->LoadIntoMemory();

	if (! ui_vm->CompileFile((char *)data, filename))
		I_Error("Errors compiling coal script: %s\n", filename);

	delete[] data;
	delete F;
}

void VM_LoadScripts()
{
	VM_LoadCoalFire("doom_ddf/coal_api.ec");
	VM_LoadCoalFire("doom_ddf/coal_hud.ec");
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
