//----------------------------------------------------------------------------
//  EDGE FXAA Shader
//----------------------------------------------------------------------------
//
//  Copyright (c) 2016 Magnus Norddahl
//  Copyright (c) 2018 The EDGE Team
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

#include "system/i_defs.h"
#include "system/i_defs_gl.h"

#include "../epi/str_format.h"

#include "r_fxaa.h"
#include "r_renderbuffers.h"
#include "r_postprocessstate.h"
#include "r_misc.h"
#include "r_modes.h"

RenderContext gl;
DEF_CVAR(r_fxaa, int, "c", 0);
DEF_CVAR(r_fxaa_quality, int, "c", 0);

#define gl_fxaa (bool)(r_fxaa != 0)
#define gl_fxaa_quality (int)(r_fxaa_quality != 0)

//-----------------------------------------------------------------------------
//
// Apply FXAA and place the result in the HUD/2D texture
//
//-----------------------------------------------------------------------------


// configure fxaa
#define FXAA_PC 1
#define FXAA_GLSL_130 1
#define FXAA_QUALITY__PRESET 29

#define FXAA_GATHER4_ALPHA 0



void RGL_ApplyFXAA()
{
	if (0 == gl_fxaa)
	{
		return;
	}

	//FGLDebug::PushGroup("ApplyFXAA");
	//Move this to top so it can call mBuffers instance
	auto mBuffers = FGLRenderBuffers::Instance();

	const GLfloat rpcRes[2] =
	{
		1.0f / mBuffers->GetWidth(),
		1.0f / mBuffers->GetHeight()
	};

	FGLPostProcessState savedState;

	
	mBuffers->BindNextFB();
	mBuffers->BindCurrentTexture(0);
	mBuffers->FXAALumaShader.Bind();
	mBuffers->FXAALumaShader.InputTexture.Set(0);
	RGL_RenderScreenQuad();
	mBuffers->NextTexture();

	mBuffers->BindNextFB();
	mBuffers->BindCurrentTexture(0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	mBuffers->FXAAShader.Bind();
	mBuffers->FXAAShader.InputTexture.Set(0);
	mBuffers->FXAAShader.ReciprocalResolution.Set(rpcRes);
	RGL_RenderScreenQuad();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	mBuffers->NextTexture();

	//FGLDebug::PopGroup();
}


void FFXAALumaShader::Bind()
{
	if (!mShader)
	{
		mShader.Compile(FShaderProgram::Vertex, "/pack0/shaders/glsl/screenquad.vp", "", 330);
		mShader.Compile(FShaderProgram::Fragment, "/pack0/shaders/glsl/fxaa.fp", "#define FXAA_LUMA_PASS\n", 330);
		mShader.SetFragDataLocation(0, "FragColor");
		mShader.Link("/pack0/shaders/glsl/fxaa");
		mShader.SetAttribLocation(0, "PositionInProjection");
		InputTexture.Init(mShader, "InputTexture");
	}

	mShader.Bind();
}

static inline const char *SafeStr(const void *s)
{
	return s ? (const char *)s : "";
}


static int GetMaxVersion()
{
	return gl.glslversion >= 4.f ? 400 : 330;
}


static std::string GetDefines()
{
	int quality;

	switch (gl_fxaa_quality)
	{
	default:
	case FFXAAShader::Low:     quality = 10; break;
	case FFXAAShader::Medium:  quality = 12; break;
	case FFXAAShader::High:    quality = 29; break;
	case FFXAAShader::Extreme: quality = 39; break;
	}


	const int gatherAlpha = GetMaxVersion() >= 400 ? 1 : 0;

	// TODO: enable FXAA_GATHER4_ALPHA on OpenGL earlier than 4.0
	// when GL_ARB_gpu_shader5/GL_NV_gpu_shader5 extensions are supported
	std::string result = epi::STR_FormatCStr(
		"#define FXAA_QUALITY__PRESET %i\n"
		"#define FXAA_GATHER4_ALPHA %i\n",
		quality, gatherAlpha);

	return result;


}

void FFXAAShader::Bind()
{
	SYS_ASSERT(gl_fxaa_quality > 0 && gl_fxaa_quality < Count);
	FShaderProgram &shader = mShaders[gl_fxaa_quality];

	if (!shader)
	{

		const std::string defines = GetDefines();
		const int maxVersion = GetMaxVersion();

		shader.Compile(FShaderProgram::Vertex, "/pack0/shaders/glsl/screenquad.vp", "", 330);
		shader.Compile(FShaderProgram::Fragment, "/pack0/shaders/glsl/fxaa.fp", defines.c_str(), maxVersion);
		shader.SetFragDataLocation(0, "FragColor");
		shader.Link("/pack0/shaders/glsl/fxaa");
		shader.SetAttribLocation(0, "PositionInProjection");
		InputTexture.Init(shader, "InputTexture");
		ReciprocalResolution.Init(shader, "ReciprocalResolution");
	}

	shader.Bind();
}
