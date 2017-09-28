//----------------------------------------------------------------------------
//  EDGE2 FXAA Shader (OpenGL)
//----------------------------------------------------------------------------
//
// Copyright(C) 2016 The EDGE2 TEAM
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
#include "r_fxaa.h"
#include "r_renderbuffers.h"
#include "r_postprocessstate.h"
#include "r_misc.h"
#include "r_modes.h"


cvar_c r_fxaa;
#define gl_fxaa (bool)(r_fxaa.d != 0)

//-----------------------------------------------------------------------------
//
// Apply FXAA and place the result in the HUD/2D texture
//
//-----------------------------------------------------------------------------

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

#if 0
	mBuffers->BindNextFB();
	mBuffers->BindCurrentTexture(0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	mBuffers->FXAALumaShader.Bind();
	mBuffers->FXAALumaShader.InputTexture.Set(0);
	mBuffers->FXAALumaShader.ReciprocalResolution.Set(rpcRes);
	RGL_RenderScreenQuad();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	mBuffers->NextTexture();
#endif // 0


	//FGLDebug::PopGroup();
}

std::string RGL_FXAAFragmentCode()
{
	return R"(

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D InputTexture;

#ifdef FXAA_LUMA_PASS

void main()
{
	vec3 tex = texture(InputTexture, TexCoord).rgb;
	vec3 luma = vec3(0.299, 0.587, 0.114);
	FragColor = vec4(tex, dot(tex, luma));
}


#endif // FXAA_LUMA_PASS
)";
}


void FFXAALumaShader::Bind()
{
	if (!mShader)
	{
		mShader.Compile(FShaderProgram::Vertex, "shaders/glsl/screenquad.vp", RGL_ScreenQuadVertexCode(), "", 330);
		mShader.Compile(FShaderProgram::Fragment, "shaders/glsl/fxaa.fp", RGL_FXAAFragmentCode(), "#define FXAA_LUMA_PASS\n", 330);
		mShader.SetFragDataLocation(0, "FragColor");
		mShader.Link("shaders/glsl/fxaa");
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
	//const char *glslversion = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
	//std::string glstr_glsl(SafeStr(
	const char *glslversion = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
	return *glslversion >= 4.f ? 400 : 330;
}
 // 0


static epi::strent_c GetDefines()
{
	int quality;

	switch (gl_fxaa)
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
	epi::strent_c result;
	epi::STR_Format(
		"#define FXAA_QUALITY__PRESET %i\n"
		"#define FXAA_GATHER4_ALPHA %i\n",
		quality, gatherAlpha);

	return result;
}

void FFXAAShader::Bind()
{
	SYS_ASSERT(gl_fxaa > 0 && gl_fxaa < Count);
	FShaderProgram &shader = mShaders[gl_fxaa];

	if (!shader)
	{
		const epi::strent_c defines = GetDefines();
		const int maxVersion = GetMaxVersion();

		shader.Compile(FShaderProgram::Vertex, "shaders/glsl/screenquad.vp", RGL_ScreenQuadVertexCode(), "", 330);
		shader.Compile(FShaderProgram::Fragment, "shaders/glsl/fxaa.fp", RGL_FXAAFragmentCode(), "", 330);  //TODO: SAVE THESE FOR LATER, FOR NOW RENDER THE FXAA AS-IS GetDefines(), maxVersion);
		shader.SetFragDataLocation(0, "FragColor");
		shader.Link("shaders/glsl/fxaa");
		shader.SetAttribLocation(0, "PositionInProjection");
		InputTexture.Init(shader, "InputTexture");
		ReciprocalResolution.Init(shader, "ReciprocalResolution");
	}

	shader.Bind();
}
