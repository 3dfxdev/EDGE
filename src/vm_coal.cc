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


// sys.error(str)
//
static void SYS_error(coal::vm_c *vm, int argc)
{
	const char * s = vm->AccessParamString(0);

	I_Error("%s\n", s);
}

// sys.print(str)
//
static void SYS_print(coal::vm_c *vm, int argc)
{
	const char * s = vm->AccessParamString(0);

	I_Printf("%s\n", s);
}

// sys.debug_print(str)
//
static void SYS_debug_print(coal::vm_c *vm, int argc)
{
	const char * s = vm->AccessParamString(0);

	I_Debugf("%s\n", s);
}


// sys.assert(val)
//
static void SYS_assert(coal::vm_c *vm, int argc)
{
	double val = *vm->AccessParam(0);

	if (val == 0)
		I_Error("Assertion in coal script failed.\n");
}


// sys.edge_version()
//
static void SYS_edge_version(coal::vm_c *vm, int argc)
{
	vm->ReturnFloat(EDGEVER / 100.0);
}


// math.round(val)
static void MATH_round(coal::vm_c *vm, int argc)
{
	double val = *vm->AccessParam(0);
	vm->ReturnFloat(I_ROUND(val));
}

// math.floor(val)
static void MATH_floor(coal::vm_c *vm, int argc)
{
	double val = *vm->AccessParam(0);
	vm->ReturnFloat(floor(val));
}

// math.ceil(val)
static void MATH_ceil(coal::vm_c *vm, int argc)
{
	double val = *vm->AccessParam(0);
	vm->ReturnFloat(ceil(val));
}


// math.random()
static void MATH_random(coal::vm_c *vm, int argc)
{
	int r = rand();

	r = (r ^ (r >> 18)) & 0xFFFF;

	vm->ReturnFloat(r / double(0x10000));
}


// math.cos(val)
static void MATH_cos(coal::vm_c *vm, int argc)
{
	double val = *vm->AccessParam(0);
	vm->ReturnFloat(cos(val * M_PI / 180.0));
}

// math.sin(val)
static void MATH_sin(coal::vm_c *vm, int argc)
{
	double val = *vm->AccessParam(0);
	vm->ReturnFloat(sin(val * M_PI / 180.0));
}

// math.tan(val)
static void MATH_tan(coal::vm_c *vm, int argc)
{
	double val = *vm->AccessParam(0);
	vm->ReturnFloat(tan(val * M_PI / 180.0));
}

// math.acos(val)
static void MATH_acos(coal::vm_c *vm, int argc)
{
	double val = *vm->AccessParam(0);
	vm->ReturnFloat(acos(val) * 180.0 / M_PI);
}

// math.asin(val)
static void MATH_asin(coal::vm_c *vm, int argc)
{
	double val = *vm->AccessParam(0);
	vm->ReturnFloat(asin(val) * 180.0 / M_PI);
}

// math.atan(val)
static void MATH_atan(coal::vm_c *vm, int argc)
{
	double val = *vm->AccessParam(0);
	vm->ReturnFloat(atan(val) * 180.0 / M_PI);
}

// math.atan2(x, y)
static void MATH_atan2(coal::vm_c *vm, int argc)
{
	double x = *vm->AccessParam(0);
	double y = *vm->AccessParam(1);

	vm->ReturnFloat(atan2(y, x) * 180.0 / M_PI);
}

// math.log(val)
static void MATH_log(coal::vm_c *vm, int argc)
{
	double val = *vm->AccessParam(0);

	if (val <= 0)
		I_Error("math.log: illegal input: %g\n", val);

	vm->ReturnFloat(log(val));
}


//------------------------------------------------------------------------

void VM_RegisterBASE(coal::vm_c *vm)
{
	// SYSTEM
    vm->AddNativeFunction("sys.error",       SYS_error);
    vm->AddNativeFunction("sys.print",       SYS_print);
    vm->AddNativeFunction("sys.debug_print", SYS_debug_print);
    vm->AddNativeFunction("sys.assert",      SYS_assert);
    vm->AddNativeFunction("sys.edge_version", SYS_edge_version);

	// MATH
    vm->AddNativeFunction("math.round",     MATH_round);
    vm->AddNativeFunction("math.floor",     MATH_floor);
    vm->AddNativeFunction("math.ceil",      MATH_ceil);
    vm->AddNativeFunction("math.random",    MATH_random);

    vm->AddNativeFunction("math.cos",       MATH_cos);
    vm->AddNativeFunction("math.sin",       MATH_sin);
    vm->AddNativeFunction("math.tan",       MATH_tan);
    vm->AddNativeFunction("math.acos",      MATH_acos);
    vm->AddNativeFunction("math.asin",      MATH_asin);
    vm->AddNativeFunction("math.atan",      MATH_atan);
    vm->AddNativeFunction("math.atan2",     MATH_atan2);
    vm->AddNativeFunction("math.log",       MATH_log);

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
