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

#include "system/i_defs.h"

#include "../coal/coal.h"

#include "../epi/file.h"
#include "../epi/filesystem.h"
#include "../epi/path.h"

#include "../ddf/main.h"

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
		I_Error("Coal script terminated with an error in the function: %s\n", name);
}


//------------------------------------------------------------------------
//  SYSTEM MODULE
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


// sys.edge_version()
//
static void SYS_edge_version(coal::vm_c *vm, int argc)
{
	vm->ReturnFloat(EDGEVER / 100.0);
}



//------------------------------------------------------------------------
//  MATH MODULE
//------------------------------------------------------------------------


// math.rint(val)
static void MATH_rint(coal::vm_c *vm, int argc)
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

//Lobo November 2021: math.random2() always between 0 and 10
static void MATH_random2(coal::vm_c *vm, int argc)
{
	int r = rand() % 10;
	
	vm->ReturnFloat(r);
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
//  STRINGS MODULE
//------------------------------------------------------------------------


// strings.len(s)
//
static void STRINGS_len(coal::vm_c *vm, int argc)
{
	const char * s = vm->AccessParamString(0);

	vm->ReturnFloat(strlen(s));
}


// strings.sub(s, start, end)
//
static void STRINGS_sub(coal::vm_c *vm, int argc)
{
	const char * s = vm->AccessParamString(0);

	int start = (int) *vm->AccessParam(1);
	int end   = (int) *vm->AccessParam(2);
	int len   = strlen(s);

	// negative values are relative to END of the string (-1 = last character)
	if (start < 0) start += len + 1;
	if (end   < 0) end   += len + 1;

	if (start < 1) start = 1;
	if (end > len) end = len;

	if (end < start)
	{
		vm->ReturnString("");
		return;
	}

	SYS_ASSERT(end >= 1 && start <= len);

	// translate into C talk
	start--;
	end--;

	int new_len = (end - start + 1);

	vm->ReturnString(s + start, new_len);
}


// strings.tonumber(s)
//
static void STRINGS_tonumber(coal::vm_c *vm, int argc)
{
	const char * s = vm->AccessParamString(0);

	vm->ReturnFloat(atof(s));
}

//------------------------------------------------------------------------
// CAMERA-MAN MODULE
//------------------------------------------------------------------------

// cameraman.reset()
// NOT USED
static void CAM_Reset(coal::vm_c *vm, int argc)
{
	cameraman::Reset();
}

// cameraman.load()
static void CAM_Load(coal::vm_c *vm, int argc)
{
	cameraman::Serialize(1);
}

// cameraman.activate(1|0)
static void CAM_Activate(coal::vm_c *vm, int argc)
{
	int activate = (int)*vm->AccessParam(0);
	cameraman::Activate(activate);
}

// cameraman.add(pos.x, pos.y, pos.z, viewvertangle, viewangle, fov)
// NOT USED
static void CAM_Add(coal::vm_c *vm, int argc)
{
	int id = -1;
	if (argc == 6)
	{
		float x = (float)*vm->AccessParam(0);
		float y = (float)*vm->AccessParam(1);
		float z = (float)*vm->AccessParam(2);
		float ax = (float)*vm->AccessParam(3);
		float ay = (float)*vm->AccessParam(4);
		float fov = (float)*vm->AccessParam(5);
		id = cameraman::Add(x, y, z, ax, ay, fov, NULL);
	}
	vm->ReturnFloat(id);
}

// cameraman.remove(id)
// NOT USED
static void CAM_Remove(coal::vm_c *vm, int argc)
{
	int done = -1;
	if (argc == 1)
	{
		int id = (int)*vm->AccessParam(0);
		done = cameraman::Remove(id);
	}
	vm->ReturnFloat(done);
}

// cameraman.set_position(id, [name,] pos.x, pos.y, pos.z)
// NOT USED
static void CAM_SetPosition(coal::vm_c *vm, int argc)
{
	int done = -1, idx = 0;
	if (argc >= 4)
	{
		int id = (int)*vm->AccessParam(idx++);

		if (argc == 5 && id < 0)
			id = cameraman::GetId(std::string(vm->AccessParamString(idx++)));

		float x = (float)*vm->AccessParam(idx++);
		float y = (float)*vm->AccessParam(idx++);
		float z = (float)*vm->AccessParam(idx++);
		done = cameraman::SetPosition(id, x, y, z);
	}
	vm->ReturnFloat(done);
}

// cameraman.set_angles(id, [name,] viewvertangle, viewangle)
// NOT USED
static void CAM_SetAngles(coal::vm_c *vm, int argc)
{
	int done = -1, idx = 0;
	if (argc >= 3)
	{
		int id = (int)*vm->AccessParam(idx++);

		if (argc == 4 && id < 0)
			id = cameraman::GetId(std::string(vm->AccessParamString(idx++)));

		float ax = (float)*vm->AccessParam(idx++);
		float ay = (float)*vm->AccessParam(idx++);
		done = cameraman::SetAngles(id, ax, ay);
	}
	vm->ReturnFloat(done);
}

// cameraman.set_fov(id, [name,] fov)
// NOT USED
static void CAM_SetFov(coal::vm_c *vm, int argc)
{
	int done = -1, idx = 0;
	if (argc >= 2)
	{
		int id = (int)*vm->AccessParam(idx++);

		if (argc == 3 && id < 0)
			id = cameraman::GetId(std::string(vm->AccessParamString(idx++)));

		float fov = (float)*vm->AccessParam(idx++);
		done = cameraman::SetFov(id, fov);
	}
	vm->ReturnFloat(done);
}

// cameraman.is_active()
static void CAM_IsActive(coal::vm_c *vm, int argc)
{
	int active = cameraman::IsActive();
	vm->ReturnFloat(active);
}

// cameraman.get_start_id()
static void CAM_GetStartId(coal::vm_c *vm, int argc)
{
	int id = cameraman::GetStartId();
	vm->ReturnFloat(id);
}

// cameraman.get_end_id()
static void CAM_GetEndId(coal::vm_c *vm, int argc)
{
	int id = cameraman::GetEndId();
	vm->ReturnFloat(id);
}

// cameraman.set_start_id(id [, name])
static void CAM_SetStartId(coal::vm_c *vm, int argc)
{
	if (argc >= 1)
	{
		int idx = 0;
		int id = (int)*vm->AccessParam(idx++);

		if (argc == 2 && id < 0)
			id = cameraman::GetId(std::string(vm->AccessParamString(idx++)));

		cameraman::SetStartId(id);
	}
}

// cameraman.set_end_id(id [, name])
static void CAM_SetEndId(coal::vm_c *vm, int argc)
{
	if (argc >= 1)
	{
		int idx = 0;
		int id = (int)*vm->AccessParam(idx++);

		if (argc == 2 && id < 0)
			id = cameraman::GetId(std::string(vm->AccessParamString(idx++)));

		cameraman::SetEndId(id);
	}
}

// cameraman.set_step(time_step)
static void CAM_SetStep(coal::vm_c *vm, int argc)
{
	if (argc == 1)
	{
		float step = (float)*vm->AccessParam(0);
		cameraman::SetStep(step);
	}
}

// cameraman.switch_to(id [, name])
static void CAM_SwitchTo(coal::vm_c *vm, int argc)
{
	if (argc >= 1)
	{
		int idx = 0;
		int id = (int)*vm->AccessParam(idx++);

		if (argc == 2 && id < 0)
			id = cameraman::GetId(std::string(vm->AccessParamString(idx++)));

		cameraman::SetStartId(id);
		cameraman::SetEndId(id);
	}
}

// cam.set_vert_bob(1.2)
static void GAME_SetCameraVerticalBobbing(coal::vm_c *vm, int argc)
{
	if (argc >= 1)
	{
		int idx = 0;
		extern float bob_z_scale;
		bob_z_scale = (float)*vm->AccessParam(idx++);
		vm->ReturnFloat(bob_z_scale);
	}
}

// cam.set_roll_bob(0.5)
static void GAME_SetCameraRollBobbing(coal::vm_c *vm, int argc)
{
	if (argc >= 1)
	{
		int idx = 0;
		extern float bob_r_scale;
		bob_r_scale = (float)*vm->AccessParam(idx++);
		vm->ReturnFloat(bob_r_scale);
	}
}

//------------------------------------------------------------------------

void VM_RegisterBASE(coal::vm_c *vm)
{
	// SYSTEM
    vm->AddNativeFunction("sys.error",       SYS_error);
    vm->AddNativeFunction("sys.print",       SYS_print);
    vm->AddNativeFunction("sys.debug_print", SYS_debug_print);
    vm->AddNativeFunction("sys.edge_version", SYS_edge_version);

	// MATH
    vm->AddNativeFunction("math.rint",      MATH_rint);
    vm->AddNativeFunction("math.floor",     MATH_floor);
    vm->AddNativeFunction("math.ceil",      MATH_ceil);
    vm->AddNativeFunction("math.random",    MATH_random);

	//Lobo November 2021
	vm->AddNativeFunction("math.random2",    MATH_random2);
	
    vm->AddNativeFunction("math.cos",       MATH_cos);
    vm->AddNativeFunction("math.sin",       MATH_sin);
    vm->AddNativeFunction("math.tan",       MATH_tan);
    vm->AddNativeFunction("math.acos",      MATH_acos);
    vm->AddNativeFunction("math.asin",      MATH_asin);
    vm->AddNativeFunction("math.atan",      MATH_atan);
    vm->AddNativeFunction("math.atan2",     MATH_atan2);
    vm->AddNativeFunction("math.log",       MATH_log);

	// STRINGS
    vm->AddNativeFunction("strings.len",    STRINGS_len);
    vm->AddNativeFunction("strings.sub",    STRINGS_sub);
    vm->AddNativeFunction("strings.tonumber", STRINGS_tonumber);
}

void VM_RegisterCameraMan(coal::vm_c *vm)
{
//	vm->AddNativeFunction("cameraman.reset", CAM_Reset);
	vm->AddNativeFunction("cameraman.load", CAM_Load);
	vm->AddNativeFunction("cameraman.activate", CAM_Activate);
//	vm->AddNativeFunction("cameraman.add", CAM_Add);
//	vm->AddNativeFunction("cameraman.remove", CAM_Remove);
//	vm->AddNativeFunction("cameraman.set_position", CAM_SetPosition);
//	vm->AddNativeFunction("cameraman.set_angles", CAM_SetAngles);
//	vm->AddNativeFunction("cameraman.set_fov", CAM_SetFov);
	vm->AddNativeFunction("cameraman.is_active", CAM_IsActive);
	vm->AddNativeFunction("cameraman.get_start_id", CAM_GetStartId);
	vm->AddNativeFunction("cameraman.get_end_id", CAM_GetEndId);
	vm->AddNativeFunction("cameraman.set_start_id", CAM_SetStartId);
	vm->AddNativeFunction("cameraman.set_end_id", CAM_SetEndId);
	vm->AddNativeFunction("cameraman.set_step", CAM_SetStep);
	vm->AddNativeFunction("cameraman.switch_to", CAM_SwitchTo);

	vm->AddNativeFunction("cam.set_vert_bob", GAME_SetCameraVerticalBobbing);
	vm->AddNativeFunction("cam.set_roll_bob", GAME_SetCameraRollBobbing);
}

void VM_InitCoal()
{
	ui_vm = coal::CreateVM();

	ui_vm->SetPrinter(VM_Printer);

	VM_RegisterBASE(ui_vm);
	VM_RegisterHUD(ui_vm);
	VM_RegisterPlaysim(ui_vm);
	VM_RegisterCameraMan(ui_vm);
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
	{
		//I_Warning("Could not open coal script: %s\n", filename);
		return;
	}

	I_Printf("Compiling COAL script: %s\n", filename);

	byte *data = F->LoadIntoMemory();

	if (! ui_vm->CompileFile((char *)data, filename))
		I_Error("Errors compiling coal script: %s\n", filename);

	delete[] data;
	delete F;
}

void VM_LoadLumpOfCoal(int lump)
{
	const char *name = W_GetLumpName(lump);

	int length;
	byte *data = W_ReadLumpAlloc(lump, &length);

	I_Printf("Compiling %s lump\n", name);

	if (! ui_vm->CompileFile((char *)data, name))
		I_Error("Errors compiling %s lump.\n", name);

	delete[] data;
}


void VM_LoadScripts()
{
	//We should be using ddf_dir.c_str()...

	// Script_Dir will be useful for OTHER, non-3DGE scripts. For now, use the god damn ddf_dir string!!

	//std::string script_dir = epi::PATH_Join(ddf_dir.c_str(), "doom_ddf");
	std::string fn;

	fn = epi::PATH_Join(ddf_dir.c_str(), "coal_api.ec");
	VM_LoadCoalFire(fn.c_str());

	fn = epi::PATH_Join(ddf_dir.c_str(), "coal_hud.ec");
	VM_LoadCoalFire(fn.c_str());

	fn.clear();
	ddf_dir.clear(); //used to be script_dir...

	W_ReadCoalLumps();
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
