// 
//---------------------------------------------------------------------------
//
// Copyright(C) 2016 Magnus Norddahl
// All rights reserved.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/
//
//--------------------------------------------------------------------------
//

#include "../epi/str_format.h"
#include "system/i_defs.h"
#include "system/i_defs_gl.h"
#include "system/i_sdlinc.h"
#include "r_shaderprogram.h"
#include "w_wad.h"

namespace
{
	void I_FatalError(const char *, ...) { }

	int GLShaderVersion()
	{
		return 330; // return (int)round(gl.glslversion * 10) * 10;
	}
}

FShaderProgram::FShaderProgram()
{
	for (int i = 0; i < NumShaderTypes; i++)
		mShaders[i] = 0;
}

//==========================================================================
//
// Free shader program resources
//
//==========================================================================

FShaderProgram::~FShaderProgram()
{
	if (mProgram != 0)
		glDeleteProgram(mProgram);

	for (int i = 0; i < NumShaderTypes; i++)
	{
		if (mShaders[i] != 0)
			glDeleteShader(mShaders[i]);
	}
}

//==========================================================================
//
// Creates an OpenGL shader object for the specified type of shader
//
//==========================================================================

void FShaderProgram::CreateShader(ShaderType type)
{
	GLenum gltype = 0;
	switch (type)
	{
	default:
	case Vertex: gltype = GL_VERTEX_SHADER; break;
	case Fragment: gltype = GL_FRAGMENT_SHADER; break;
	}
	mShaders[type] = glCreateShader(gltype);
}

//==========================================================================
//
// Compiles a shader and attaches it the program object
//
//==========================================================================


#if 0
void FShaderProgram::Compile(ShaderType type, const char *lumpName, const char *defines, int maxGlslVersion)
{
	int lump = W_CheckNumForName(lumpName, 0);
	const char *lumpName = W_GetLumpName(lump);

	if (lump == -1) I_FatalError("Unable to load '%s'", lumpName);

	std::string code = W_LoadLumpName(lumpName, defines, maxGlslVersion);

	Compile(type, lumpName, code, defines, maxGlslVersion);
}
#endif // 0




void FShaderProgram::Compile(ShaderType type, const char *name, const std::string &code, const char *defines, int maxGlslVersion)
{
	CreateShader(type);

	const auto &handle = mShaders[type];

	//FGLDebug::LabelObject(GL_SHADER, handle, name);

	std::string patchedCode = PatchShader(type, code, defines, maxGlslVersion);
	int lengths[1] = { (int)patchedCode.size() };
	const char *sources[1] = { patchedCode.c_str() };
	glShaderSource(handle, 1, sources, lengths);

	glCompileShader(handle);

	GLint status = 0;
	glGetShaderiv(handle, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		I_FatalError("Compile Shader '%s':\n%s\n", name, GetShaderInfoLog(handle).c_str());
	}
	else
	{
		if (mProgram == 0)
			mProgram = glCreateProgram();
		glAttachShader(mProgram, handle);
	}
}


//==========================================================================
//
// Binds a fragment output variable to a frame buffer render target
//
//==========================================================================

void FShaderProgram::SetFragDataLocation(int index, const char *name)
{
	glBindFragDataLocation(mProgram, index, name);
}

//==========================================================================
//
// Links a program with the compiled shaders
//
//==========================================================================

void FShaderProgram::Link(const char *name)
{
	//FGLDebug::LabelObject(GL_PROGRAM, mProgram, name);
	glLinkProgram(mProgram);

	GLint status = 0;
	glGetProgramiv(mProgram, GL_LINK_STATUS, &status);
	if (status == GL_FALSE)
	{
		I_FatalError("Link Shader '%s':\n%s\n", name, GetProgramInfoLog(mProgram).c_str());
	}
}

//==========================================================================
//
// Set vertex attribute location
//
//==========================================================================

void FShaderProgram::SetAttribLocation(int index, const char *name)
{
	glBindAttribLocation(mProgram, index, name);
}

//==========================================================================
//
// Makes the shader the active program
//
//==========================================================================

void FShaderProgram::Bind()
{
	glUseProgram(mProgram);
}

//==========================================================================
//
// Returns the shader info log (warnings and compile errors)
//
//==========================================================================

std::string FShaderProgram::GetShaderInfoLog(GLuint handle)
{
	static char buffer[10000];
	GLsizei length = 0;
	buffer[0] = 0;
	glGetShaderInfoLog(handle, 10000, &length, buffer);
	return std::string(buffer);
}

//==========================================================================
//
// Returns the program info log (warnings and compile errors)
//
//==========================================================================

std::string FShaderProgram::GetProgramInfoLog(GLuint handle)
{
	static char buffer[10000];
	GLsizei length = 0;
	buffer[0] = 0;
	glGetProgramInfoLog(handle, 10000, &length, buffer);
	return std::string(buffer);
}

//==========================================================================
//
// Patches a shader to be compatible with the version of OpenGL in use
//
//==========================================================================

std::string FShaderProgram::PatchShader(ShaderType type, const std::string &code, const char *defines, int maxGlslVersion)
{
	std::string patchedCode;

	int shaderVersion = GLShaderVersion();
	if (shaderVersion > maxGlslVersion)
		shaderVersion = maxGlslVersion;
	patchedCode += "#version " + std::to_string(shaderVersion) + "\n";

	if (defines)
		patchedCode += defines;

	// these settings are actually pointless but there seem to be some old ATI drivers that fail to compile the shader without setting the precision here.
	patchedCode += "precision highp int;\n";
	patchedCode += "precision highp float;\n";

	patchedCode += "#line 1\n";
	patchedCode += code;

	return patchedCode;
}
