// EDGE BLOOM BACKEND
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

#include "system/i_defs.h"
#include "system/i_defs_gl.h"
#include "system/i_sdlinc.h"
#include "r_bloom.h"
#include "r_renderbuffers.h"
#include "r_postprocessstate.h"
#include "r_misc.h"
#include "r_modes.h"


//FIXME: Ugly hacks.
DEF_CVAR(r_bloom, int, "c", 1);
#define gl_bloom (bool)(r_bloom != 0)
DEF_CVAR(r_bloom_amount, float, "c", 1.4f);
#define gl_bloom_amount (float)(r_bloom_amount) //defaults to 1.4f

#if 0

cvar_c r_exposure_scale;
#define gl_exposure_scale (float)(r_exposure_scale.d) //float gl_exposure_scale = 2.0f; // CVAR(Float, gl_exposure_scale, 1.3f, CVAR_ARCHIVE)

cvar_c r_exposure_min;
#define gl_exposure_min (float)(r_exposure_min.d) //float gl_exposure_min = 0.1f; // CVAR(Float, gl_exposure_min, 0.35f, CVAR_ARCHIVE)

cvar_c r_exposure_base;
#define gl_exposure_base (float)(r_exposure_base.d) //float gl_exposure_base = 0.1f; // CVAR(Float, gl_exposure_base, 0.35f, CVAR_ARCHIVE)

cvar_c r_exposure_speed;
#define gl_exposure_speed (float)(r_exposure_speed.d) //float gl_exposure_speed = 0.05f; // CVAR(Float, gl_exposure_speed, 0.05f, CVAR_ARCHIVE)

#endif // 0

//cvar_c r_bloom_kernal_size;
//#define gl_bloom_kernel_size (int)(r_bloom_kernal_size.d) //int gl_bloom_kernel_size = 7; // CUSTOM_CVAR(Int, gl_bloom_kernel_size, 7, CVAR_ARCHIVE)

namespace
{
	//bool gl_bloom = true; // CVAR(Bool, gl_bloom, false, CVAR_ARCHIVE);
	//float gl_bloom_amount = 1.4f; // CUSTOM_CVAR(Float, gl_bloom_amount, 1.4f, CVAR_ARCHIVE)
	float gl_exposure_scale = 2.0f; // CVAR(Float, gl_exposure_scale, 1.3f, CVAR_ARCHIVE)
	float gl_exposure_min = 0.1f; // CVAR(Float, gl_exposure_min, 0.35f, CVAR_ARCHIVE)
	float gl_exposure_base = 0.1f; // CVAR(Float, gl_exposure_base, 0.35f, CVAR_ARCHIVE)
	float gl_exposure_speed = 0.05f; // CVAR(Float, gl_exposure_speed, 0.05f, CVAR_ARCHIVE)
	int gl_bloom_kernel_size = 7; // CUSTOM_CVAR(Int, gl_bloom_kernel_size, 7, CVAR_ARCHIVE)
}


//-----------------------------------------------------------------------------
//
// Adds bloom contribution to scene texture
//
//-----------------------------------------------------------------------------

void RGL_BloomScene()
{
	// Only bloom things if enabled and no special fixed light mode is active
	if (!gl_bloom /*|| fixedcm != CM_DEFAULT || gl_ssao_debug*/)
		return;

	//FGLDebug::PushGroup("BloomScene");

	FGLPostProcessState savedState;
	savedState.SaveTextureBindings(2);

	const float blurAmount = gl_bloom_amount;
	int sampleCount = gl_bloom_kernel_size;

	auto renderbuffers = FGLRenderBuffers::Instance();
	const auto &level0 = renderbuffers->BloomLevels[0];

	// Extract blooming pixels from scene texture:
	glBindFramebuffer(GL_FRAMEBUFFER, level0.VFramebuffer);
	glViewport(0, 0, level0.Width, level0.Height);
	renderbuffers->BindCurrentTexture(0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, renderbuffers->ExposureTexture);
	glActiveTexture(GL_TEXTURE0);
	renderbuffers->BloomExtract.Bind();
	renderbuffers->BloomExtract.SceneTexture.Set(0);
	renderbuffers->BloomExtract.ExposureTexture.Set(1);
	renderbuffers->BloomExtract.Scale.Set(viewwindow_w / (float)SCREENWIDTH, viewwindow_h / (float)SCREENHEIGHT);
	renderbuffers->BloomExtract.Offset.Set(viewwindow_x / (float)SCREENWIDTH, viewwindow_y / (float)SCREENHEIGHT);
	RGL_RenderScreenQuad();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Blur and downscale:
	for (int i = 0; i < FGLRenderBuffers::NumBloomLevels - 1; i++)
	{
		const auto &level = renderbuffers->BloomLevels[i];
		const auto &next = renderbuffers->BloomLevels[i + 1];
		renderbuffers->Blur.BlurHorizontal(blurAmount, sampleCount, level.VTexture, level.HFramebuffer, level.Width, level.Height);
		renderbuffers->Blur.BlurVertical(blurAmount, sampleCount, level.HTexture, next.VFramebuffer, next.Width, next.Height);
	}

	// Blur and upscale:
	for (int i = FGLRenderBuffers::NumBloomLevels - 1; i > 0; i--)
	{
		const auto &level = renderbuffers->BloomLevels[i];
		const auto &next = renderbuffers->BloomLevels[i - 1];

		renderbuffers->Blur.BlurHorizontal(blurAmount, sampleCount, level.VTexture, level.HFramebuffer, level.Width, level.Height);
		renderbuffers->Blur.BlurVertical(blurAmount, sampleCount, level.HTexture, level.VFramebuffer, level.Width, level.Height);

		// Linear upscale:
		glBindFramebuffer(GL_FRAMEBUFFER, next.VFramebuffer);
		glViewport(0, 0, next.Width, next.Height);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, level.VTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		renderbuffers->BloomCombine.Bind();
		renderbuffers->BloomCombine.BloomTexture.Set(0);
		RGL_RenderScreenQuad();
	}

	renderbuffers->Blur.BlurHorizontal(blurAmount, sampleCount, level0.VTexture, level0.HFramebuffer, level0.Width, level0.Height);
	renderbuffers->Blur.BlurVertical(blurAmount, sampleCount, level0.HTexture, level0.VFramebuffer, level0.Width, level0.Height);

	// Add bloom back to scene texture:
	renderbuffers->BindCurrentFB();
	glViewport(viewwindow_x, viewwindow_y, viewwindow_w, viewwindow_h);
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_ONE, GL_ONE);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, level0.VTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	renderbuffers->BloomCombine.Bind();
	renderbuffers->BloomCombine.BloomTexture.Set(0);
	RGL_RenderScreenQuad();
	glViewport(0, 0, SCREENWIDTH, SCREENHEIGHT);

	//FGLDebug::PopGroup();
}

//-----------------------------------------------------------------------------
//
// Extracts light average from the scene and updates the camera exposure texture
//
//-----------------------------------------------------------------------------

void RGL_UpdateCameraExposure()
{
	if (!gl_bloom/* && gl_tonemap == 0*/)
		return;

	//FGLDebug::PushGroup("UpdateCameraExposure");

	FGLPostProcessState savedState;
	savedState.SaveTextureBindings(2);

	auto renderbuffers = FGLRenderBuffers::Instance();

	// Extract light level from scene texture:
	const auto &level0 = renderbuffers->ExposureLevels[0];
	glBindFramebuffer(GL_FRAMEBUFFER, level0.Framebuffer);
	glViewport(0, 0, level0.Width, level0.Height);
	renderbuffers->BindCurrentTexture(0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	renderbuffers->ExposureExtract.Bind();
	renderbuffers->ExposureExtract.SceneTexture.Set(0);
	renderbuffers->ExposureExtract.Scale.Set(viewwindow_w / (float)SCREENWIDTH, viewwindow_h / (float)SCREENHEIGHT);
	renderbuffers->ExposureExtract.Offset.Set(viewwindow_x / (float)SCREENWIDTH, viewwindow_y / (float)SCREENHEIGHT);
	RGL_RenderScreenQuad();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Find the average value:
	for (unsigned int i = 0; i + 1 < renderbuffers->ExposureLevels.size(); i++)
	{
		const auto &level = renderbuffers->ExposureLevels[i];
		const auto &next = renderbuffers->ExposureLevels[i + 1];

		glBindFramebuffer(GL_FRAMEBUFFER, next.Framebuffer);
		glViewport(0, 0, next.Width, next.Height);
		glBindTexture(GL_TEXTURE_2D, level.Texture);
		renderbuffers->ExposureAverage.Bind();
		renderbuffers->ExposureAverage.ExposureTexture.Set(0);
		RGL_RenderScreenQuad();
	}

	// Combine average value with current camera exposure:
	glBindFramebuffer(GL_FRAMEBUFFER, renderbuffers->ExposureFB);
	glViewport(0, 0, 1, 1);
	if (!renderbuffers->FirstExposureFrame)
	{
		glEnable(GL_BLEND);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	else
	{
		renderbuffers->FirstExposureFrame = false;
	}
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, renderbuffers->ExposureLevels.back().Texture);
	renderbuffers->ExposureCombine.Bind();
	renderbuffers->ExposureCombine.ExposureTexture.Set(0);
	renderbuffers->ExposureCombine.ExposureBase.Set(gl_exposure_base);
	renderbuffers->ExposureCombine.ExposureMin.Set(gl_exposure_min);
	renderbuffers->ExposureCombine.ExposureScale.Set(gl_exposure_scale);
	renderbuffers->ExposureCombine.ExposureSpeed.Set(gl_exposure_speed);
	RGL_RenderScreenQuad();
	glViewport(0, 0, SCREENWIDTH, SCREENHEIGHT);

	//FGLDebug::PopGroup();
}

std::string RGL_BloomExtractFragmentCode()
{
	return R"(
		in vec2 TexCoord;
		out vec4 FragColor;

		uniform sampler2D SceneTexture;
		uniform sampler2D ExposureTexture;
		uniform vec2 Scale;
		uniform vec2 Offset;

		void main()
		{
			float exposureAdjustment = texture(ExposureTexture, vec2(0.5)).x;
			vec4 color = texture(SceneTexture, Offset + TexCoord * Scale);
			FragColor = max(vec4((color.rgb + vec3(0.001)) * exposureAdjustment - 1, 1), vec4(0));
		}
	)";
}

std::string RGL_BloomCombineFragmentCode()
{
	return R"(
		in vec2 TexCoord;
		out vec4 FragColor;

		uniform sampler2D Bloom;

		void main()
		{
			FragColor = vec4(texture(Bloom, TexCoord).rgb, 0.0);
		}
	)";
}

std::string RGL_ExposureExtractFragmentCode()
{
	return R"(
		in vec2 TexCoord;
		out vec4 FragColor;

		uniform sampler2D SceneTexture;
		uniform vec2 Scale;
		uniform vec2 Offset;

		void main()
		{
			vec4 color = texture(SceneTexture, Offset + TexCoord * Scale);
			FragColor = vec4(max(max(color.r, color.g), color.b), 0.0, 0.0, 1.0);
		}
	)";
}

std::string RGL_ExposureAverageFragmentCode()
{
	return R"(
		in vec2 TexCoord;
		out vec4 FragColor;

		uniform sampler2D ExposureTexture;

		void main()
		{
		#if __VERSION__ < 400
			ivec2 size = textureSize(ExposureTexture, 0);
			ivec2 tl = max(ivec2(TexCoord * vec2(size) - 0.5), ivec2(0));
			ivec2 br = min(tl + ivec2(1), size - ivec2(1));
			vec4 values = vec4(
				texelFetch(ExposureTexture, tl, 0).x,
				texelFetch(ExposureTexture, ivec2(tl.x, br.y), 0).x,
				texelFetch(ExposureTexture, ivec2(br.x, tl.y), 0).x,
				texelFetch(ExposureTexture, br, 0).x);
		#else
			vec4 values = textureGather(ExposureTexture, TexCoord);
		#endif

			FragColor = vec4((values.x + values.y + values.z + values.w) * 0.25, 0.0, 0.0, 1.0);
		}
	)";
}

std::string RGL_ExposureCombineFragmentCode()
{
	return R"(
		in vec2 TexCoord;
		out vec4 FragColor;

		uniform sampler2D ExposureTexture;
		uniform float ExposureBase;
		uniform float ExposureMin;
		uniform float ExposureScale;
		uniform float ExposureSpeed;

		void main()
		{
			float light = texture(ExposureTexture, TexCoord).x;
			float exposureAdjustment = 1.0 / max(ExposureBase + light * ExposureScale, ExposureMin);
			FragColor = vec4(exposureAdjustment, 0.0, 0.0, ExposureSpeed);
		}
	)";
}

void FBloomExtractShader::Bind()
{
	if (!mShader)
	{
		mShader.Compile(FShaderProgram::Vertex, "/pack0/shaders/glsl/screenquad.vp", "", 330);//RGL_ScreenQuadVertexCode(), "", 330);
		mShader.Compile(FShaderProgram::Fragment, "/pack0/shaders/glsl/bloomextract.fp", "", 330);//RGL_BloomExtractFragmentCode(), "", 330);
		mShader.SetFragDataLocation(0, "FragColor");
		mShader.Link("/pack0/shaders/glsl/bloomextract");
		mShader.SetAttribLocation(0, "PositionInProjection");
		mShader.SetAttribLocation(1, "UV");
		SceneTexture.Init(mShader, "SceneTexture");
		ExposureTexture.Init(mShader, "ExposureTexture");
		Scale.Init(mShader, "Scale");
		Offset.Init(mShader, "Offset");
	}
	mShader.Bind();
}

void FBloomCombineShader::Bind()
{
	if (!mShader)
	{
		mShader.Compile(FShaderProgram::Vertex, "/pack0/shaders/glsl/screenquad.vp", "", 330);//RGL_ScreenQuadVertexCode(), "", 330);
		mShader.Compile(FShaderProgram::Fragment, "/pack0/shaders/glsl/bloomcombine.fp", "", 330);//RGL_BloomCombineFragmentCode(), "", 330);
		mShader.SetFragDataLocation(0, "FragColor");
		mShader.Link("/pack0/shaders/glsl/bloomcombine");
		mShader.SetAttribLocation(0, "PositionInProjection");
		mShader.SetAttribLocation(1, "UV");
		BloomTexture.Init(mShader, "Bloom");
	}
	mShader.Bind();
}

void FExposureExtractShader::Bind()
{
	if (!mShader)
	{
		mShader.Compile(FShaderProgram::Vertex, "/pack0/shaders/glsl/screenquad.vp", "", 330);//RGL_ScreenQuadVertexCode(), "", 330);
		mShader.Compile(FShaderProgram::Fragment, "/pack0/shaders/glsl/exposureextract.fp", "", 330);//RGL_ExposureExtractFragmentCode(), "", 330);
		mShader.SetFragDataLocation(0, "FragColor");
		mShader.Link("/pack0/shaders/glsl/exposureextract");
		mShader.SetAttribLocation(0, "PositionInProjection");
		mShader.SetAttribLocation(1, "UV");
		SceneTexture.Init(mShader, "SceneTexture");
		Scale.Init(mShader, "Scale");
		Offset.Init(mShader, "Offset");
	}
	mShader.Bind();
}

void FExposureAverageShader::Bind()
{
	if (!mShader)
	{
		mShader.Compile(FShaderProgram::Vertex, "/pack0/shaders/glsl/screenquad.vp", "", 330);//RGL_ScreenQuadVertexCode(), "", 400);
		mShader.Compile(FShaderProgram::Fragment, "/pack0/shaders/glsl/exposureaverage.fp", "", 330);//RGL_ExposureAverageFragmentCode(), "", 400);
		mShader.SetFragDataLocation(0, "FragColor");
		mShader.Link("/pack0/shaders/glsl/exposureaverage");
		mShader.SetAttribLocation(0, "PositionInProjection");
		mShader.SetAttribLocation(1, "UV");
		ExposureTexture.Init(mShader, "ExposureTexture");
	}
	mShader.Bind();
}

void FExposureCombineShader::Bind()
{
	if (!mShader)
	{
		mShader.Compile(FShaderProgram::Vertex, "/pack0/shaders/glsl/screenquad.vp", "", 330);//RGL_ScreenQuadVertexCode(), "", 330);
		mShader.Compile(FShaderProgram::Fragment, "/pack0/shaders/glsl/exposurecombine.fp", "", 330);//RGL_ExposureCombineFragmentCode(), "", 330);
		mShader.SetFragDataLocation(0, "FragColor");
		mShader.Link("/pack0/shaders/glsl/exposurecombine");
		mShader.SetAttribLocation(0, "PositionInProjection");
		mShader.SetAttribLocation(1, "UV");
		ExposureTexture.Init(mShader, "ExposureTexture");
		ExposureBase.Init(mShader, "ExposureBase");
		ExposureMin.Init(mShader, "ExposureMin");
		ExposureScale.Init(mShader, "ExposureScale");
		ExposureSpeed.Init(mShader, "ExposureSpeed");
	}
	mShader.Bind();
}

//==========================================================================
//
// Performs a vertical gaussian blur pass
//
//==========================================================================

void FBlurShader::BlurVertical(float blurAmount, int sampleCount, GLuint inputTexture, GLuint outputFrameBuffer, int width, int height)
{
	Blur(blurAmount, sampleCount, inputTexture, outputFrameBuffer, width, height, true);
}

//==========================================================================
//
// Performs a horizontal gaussian blur pass
//
//==========================================================================

void FBlurShader::BlurHorizontal(float blurAmount, int sampleCount, GLuint inputTexture, GLuint outputFrameBuffer, int width, int height)
{
	Blur(blurAmount, sampleCount, inputTexture, outputFrameBuffer, width, height, false);
}

//==========================================================================
//
// Helper for BlurVertical and BlurHorizontal. Executes the actual pass
//
//==========================================================================

void FBlurShader::Blur(float blurAmount, int sampleCount, GLuint inputTexture, GLuint outputFrameBuffer, int width, int height, bool vertical)
{
	BlurSetup *setup = GetSetup(blurAmount, sampleCount);
	if (vertical)
		setup->VerticalShader->Bind();
	else
		setup->HorizontalShader->Bind();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, inputTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glBindFramebuffer(GL_FRAMEBUFFER, outputFrameBuffer);
	glViewport(0, 0, width, height);
	glDisable(GL_BLEND);

	RGL_RenderScreenQuad();
}

//==========================================================================
//
// Compiles the blur shaders needed for the specified blur amount and
// kernel size
//
//==========================================================================

FBlurShader::BlurSetup *FBlurShader::GetSetup(float blurAmount, int sampleCount)
{
	for (size_t mBlurSetupIndex = 0; mBlurSetupIndex < mBlurSetups.size(); mBlurSetupIndex++)
	{
		if (mBlurSetups[mBlurSetupIndex].blurAmount == blurAmount && mBlurSetups[mBlurSetupIndex].sampleCount == sampleCount)
		{
			return &mBlurSetups[mBlurSetupIndex];
		}
	}

	BlurSetup blurSetup(blurAmount, sampleCount);

	std::string vertexCode = VertexShaderCode();
	std::string horizontalCode = FragmentShaderCode(blurAmount, sampleCount, false);
	std::string verticalCode = FragmentShaderCode(blurAmount, sampleCount, true);

	blurSetup.VerticalShader = std::make_shared<FShaderProgram>();
	blurSetup.VerticalShader->Compile(FShaderProgram::Vertex, "vertical blur vertex shader", vertexCode, "", 330);
	blurSetup.VerticalShader->Compile(FShaderProgram::Fragment, "vertical blur fragment shader", verticalCode, "", 330);
	blurSetup.VerticalShader->SetFragDataLocation(0, "FragColor");
	blurSetup.VerticalShader->SetAttribLocation(0, "PositionInProjection");
	blurSetup.VerticalShader->Link("vertical blur");
	blurSetup.VerticalShader->Bind();
	glUniform1i(glGetUniformLocation(*blurSetup.VerticalShader.get(), "SourceTexture"), 0);

	blurSetup.HorizontalShader = std::make_shared<FShaderProgram>();
	blurSetup.HorizontalShader->Compile(FShaderProgram::Vertex, "horizontal blur vertex shader", vertexCode, "", 330);
	blurSetup.HorizontalShader->Compile(FShaderProgram::Fragment, "horizontal blur fragment shader", horizontalCode, "", 330);
	blurSetup.HorizontalShader->SetFragDataLocation(0, "FragColor");
	blurSetup.HorizontalShader->SetAttribLocation(0, "PositionInProjection");
	blurSetup.HorizontalShader->Link("horizontal blur");
	blurSetup.HorizontalShader->Bind();
	glUniform1i(glGetUniformLocation(*blurSetup.HorizontalShader.get(), "SourceTexture"), 0);

	mBlurSetups.push_back(blurSetup);

	return &mBlurSetups[mBlurSetups.size() - 1];
}

//==========================================================================
//
// The vertex shader GLSL code
//
//==========================================================================

std::string FBlurShader::VertexShaderCode()
{
	return R"(
		in vec4 PositionInProjection;
		out vec2 TexCoord;

		void main()
		{
			gl_Position = PositionInProjection;
			TexCoord = (gl_Position.xy + 1.0) * 0.5;
		}
	)";
}

//==========================================================================
//
// Generates the fragment shader GLSL code for a specific blur setup
//
//==========================================================================

std::string FBlurShader::FragmentShaderCode(float blurAmount, int sampleCount, bool vertical)
{
	std::vector<float> sampleWeights;
	std::vector<int> sampleOffsets;
	ComputeBlurSamples(sampleCount, blurAmount, sampleWeights, sampleOffsets);

	const char *fragmentShader =
		R"(
		in vec2 TexCoord;
		uniform sampler2D SourceTexture;
		out vec4 FragColor;
		#if __VERSION__ < 130
		uniform float ScaleX;
		uniform float ScaleY;
		vec4 textureOffset(sampler2D s, vec2 texCoord, ivec2 offset)
		{
			return texture2D(s, texCoord + vec2(ScaleX * float(offset.x), ScaleY * float(offset.y)));
		}
		#endif
		)";

	std::string loopCode;
	for (int i = 0; i < sampleCount; i++)
	{
		if (i > 0)
			loopCode += " + ";

		if (vertical)
			loopCode += "\r\n\t\t\ttextureOffset(SourceTexture, TexCoord, ivec2(0, " + std::to_string(sampleOffsets[i]) + ")) * " + std::to_string((double)sampleWeights[i]);
		else
			loopCode += "\r\n\t\t\ttextureOffset(SourceTexture, TexCoord, ivec2(" + std::to_string(sampleOffsets[i]) + ", 0)) * " + std::to_string((double)sampleWeights[i]);
	}

	std::string code = fragmentShader;
	code += "void main() { FragColor = " + loopCode + "; }\r\n";
	return code;
}

//==========================================================================
//
// Calculates the sample weight for a specific offset in the kernel
//
//==========================================================================

float FBlurShader::ComputeGaussian(float n, float theta) // theta = Blur Amount
{
	return (float)((1.0f / sqrtf(2 * (float)M_PI * theta)) * expf(-(n * n) / (2.0f * theta * theta)));
}

//==========================================================================
//
// Calculates the sample weights and offsets
//
//==========================================================================

void FBlurShader::ComputeBlurSamples(int sampleCount, float blurAmount, std::vector<float> &sampleWeights, std::vector<int> &sampleOffsets)
{
	sampleWeights.resize(sampleCount);
	sampleOffsets.resize(sampleCount);

	sampleWeights[0] = ComputeGaussian(0, blurAmount);
	sampleOffsets[0] = 0;

	float totalWeights = sampleWeights[0];

	for (int i = 0; i < sampleCount / 2; i++)
	{
		float weight = ComputeGaussian(i + 1.0f, blurAmount);

		sampleWeights[i * 2 + 1] = weight;
		sampleWeights[i * 2 + 2] = weight;
		sampleOffsets[i * 2 + 1] = i + 1;
		sampleOffsets[i * 2 + 2] = -i - 1;

		totalWeights += weight * 2;
	}

	for (int i = 0; i < sampleCount; i++)
	{
		sampleWeights[i] /= totalWeights;
	}
}
