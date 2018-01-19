//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (Shaders)
//----------------------------------------------------------------------------
//
//  Copyright (c) 2018  The EDGE2 Team.
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
//
// User-based Shader Parser Code
//

#include "local.h"

#include "../epi/path.h"

#include "shaders.h"

static shaderdef_c *dynamic_shader;

static void DDF_ShaderGetType(const char *info, void *storage);
// -ACB- 1998/08/10 Use DDF_MainGetLumpName for getting the..lump name.
// -KM- 1998/09/27 Use DDF_MainGetTime for getting tics

#define DDF_CMD_BASE  dummy_shader
static shaderdef_c dummy_shader;

static const commandlist_t shader_commands[] =
{
	DDF_FIELD("SHADER_DATA", type,     DDF_ShaderGetType),
	// Maybe a DDF_FIELD to send to FShader::Compile (or not...will get that one ironed out eventually)
	DDF_CMD_END
};

shaderdef_container_c shaderdefs;

static shader_namespace_e GetShaderNamespace(const char *prefix)
{
	if (DDF_CompareName(prefix, "frag") == 0)
		return SNS_Fragment;

	if (DDF_CompareName(prefix, "vert") == 0)
		return SNS_Vertex;

	DDF_Error("Invalid GLSL Shader prefix '%s' (use: frag or vert!)\n", prefix);
	return; /* NOT REACHED */
}

//
//  DDF PARSE ROUTINES
//

static void ShaderStartEntry(const char *name, bool extend)
{
	if (!name || !name[0])
		DDF_Error("New shader entry is missing a name!\n");

	I_Debugf("ShaderStartEntry [%s]\n", name);

	shader_namespace_e belong = SNS_Fragment;

	const char *pos = strchr(name, ':');

	if (!pos)
		DDF_Error("Missing shader prefix.\n");

	if (pos)
	{
		std::string nspace(name, pos - name);

		if (nspace.empty())
			DDF_Error("Missing shader prefix.\n");

		belong = GetShaderNamespace(nspace.c_str());

		name = pos + 1;

		if (!name[0])
			DDF_Error("Missing shader name.\n");
	}

	// W_Image code has limited space for the name
	if (strlen(name) > 25)
		DDF_Error("Shader name [%s] too long.\n", name);

	dynamic_shader = shaderdefs.Lookup(name, belong);

	if (extend)
	{
		//if (!shader_image)
		//	DDF_Error("Unknown shader to extend: %s\n", name);
		return;
	}

	// replaces an existing entry?
	if (dynamic_shader)
	{
		dynamic_shader->Default();
		return;
	}

	// not found, create a new one
	dynamic_shader = new shaderdef_c;

	dynamic_shader->name = name;
	dynamic_shader->belong = belong;

	shaderdefs.Insert(dynamic_shader);
}

static void ShaderParseField(const char *field, const char *contents, int index, bool is_last)
{
#if (DEBUG_DDF)
	I_Debugf("SHADER_PARSE: %s = %s;\n", field, contents);
#endif

	if (DDF_MainParseField(shader_commands, field, contents, (byte *)dynamic_shader))
		return;  // OK

	DDF_Error("Unknown shaders.ddf command: %s\n", field);
}

static void ShaderFinishEntry(void)
{
	if (dynamic_shader->type == SHADR_File)
	{
		const char *filename = dynamic_shader->info.c_str();

		// determine format
		std::string ext(epi::PATH_GetExtension(filename));

		if (DDF_CompareName(ext.c_str(), "fp") == 0)
			dynamic_shader->format = LSF_FRAG;
		else if (DDF_CompareName(ext.c_str(), "vp") == 0)
			dynamic_shader->format = LSF_VERT;
		else
			DDF_Error("Unknown GLSL shader extension for '%s'\n", filename);

	}

	//Send to ::Bind() or create a new ShaderType thing and compile/link that way (?)
}

static void ShaderClearAll(void)
{
	I_Warning("Ignoring #CLEARALL in shaders.ddf\n");
}

bool DDF_ReadShaders(void *data, int size)
{
	readinfo_t shaders;

	shaders.memfile = (char*)data;
	shaders.memsize = size;
	shaders.tag = "SHADERS";
	shaders.entries_per_dot = 2;

	if (shaders.memfile)
	{
		shaders.message = NULL;
		shaders.filename = NULL;
		shaders.lumpname = "DDFGLSL";
	}
	else
	{
		shaders.message = "DDF_InitShaders";
		shaders.filename = "shaders.ddf";
		shaders.lumpname = NULL;
	}

	shaders.start_entry = ShaderStartEntry;
	shaders.parse_field = ShaderParseField;
	shaders.finish_entry = ShaderFinishEntry;
	shaders.clear_all = ShaderClearAll;

	return DDF_MainReadFile(&shaders);
}

void DDF_ImageInit(void)
{
	shaderdefs.Clear();
}

static void AddEssentialShaders(void)
{
	// -CA-  this is a hack, these really should just be added to
	//       our standard SHADERS.DDF file.  However some Mods use
	//       standalone DDF and in that case these essential shaders
	//       would never get loaded.

	if (!shaderdefs.Lookup("SCREENQUAD", SNS_Vertex))
	{
		shaderdef_c *def = new shaderdef_c;

		def->name = "SCREENQUAD";
		def->belong = SNS_Vertex;

		def->info.Set("/shaders/glsl/screenquad.vp");

		def->type = SHADR_Lump;
		def->format = LSF_VERT;
		//def->special = (image_special_e)(IMGSP_Clamp | IMGSP_Smooth | IMGSP_NoMip);

		shaderdefs.Insert(def);
	}

	if (!shaderdefs.Lookup("SCREENQUADSCALE", SNS_Vertex))
	{
		shaderdef_c *def = new shaderdef_c;

		def->name = "SCREENQUADSCALE";
		def->belong = SNS_Vertex;

		def->info.Set("/shaders/glsl/screenquadscale.vp");

		def->type = SHADR_Lump;
		def->format = LSF_VERT;
		//def->special = (image_special_e)(IMGSP_NoSmooth | IMGSP_NoMip);

		shaderdefs.Insert(def);
	}
#if 0

	if (!shaderdefs.Lookup("CON_FONT_2", INS_Graphic))
	{
		imagedef_c *def = new imagedef_c;

		def->name = "CON_FONT_2";
		def->belong = INS_Graphic;

		def->info.Set("CONFONT2");

		def->type = IMGDT_Lump;
		def->format = LIF_PNG;
		def->special = (image_special_e)(IMGSP_Clamp | IMGSP_Smooth | IMGSP_NoMip);

		imagedefs.Insert(def);
	}
#endif // 0

}

void DDF_ShaderCleanUp(void)
{
	AddEssentialShaders();

	shaderdefs.Trim();		// <-- Reduce to allocated size
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab