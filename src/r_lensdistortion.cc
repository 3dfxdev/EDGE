//----------------------------------------------------------------------------
//  EDGE Lens Distortion Shader Buffer
//----------------------------------------------------------------------------
//
//  Copyright (c) 2016 Magnus Norddahl
//  Copyright (c) 2018 The EDGE Team
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
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------
//
#include "system/i_defs.h"
#include "system/i_defs_gl.h"


#include "r_lensdistortion.h"
#include "r_renderbuffers.h"
#include "r_postprocessstate.h"
#include "r_misc.h"
#include "r_modes.h"

DEF_CVAR(r_lens, int, "c", 1);
#define gl_lens (bool)(r_lens != 0)

namespace
{
	//bool gl_lens = true; // CVAR(Bool, gl_lens, false, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)
	float gl_lens_k = -0.12f; // CVAR(Float, gl_lens_k, -0.12f, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)
	float gl_lens_kcube = 0.1f; // CVAR(Float, gl_lens_kcube, 0.1f, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)
	float gl_lens_chromatic = 1.12f; // CVAR(Float, gl_lens_chromatic, 1.12f, CVAR_ARCHIVE | CVAR_GLOBALCONFIG)
}

void RGL_LensDistortScene()
{
	if (gl_lens == 0)
		return;

	//FGLDebug::PushGroup("LensDistortScene");

	float k[4] =
	{
		gl_lens_k,
		gl_lens_k * gl_lens_chromatic,
		gl_lens_k * gl_lens_chromatic * gl_lens_chromatic,
		0.0f
	};
	float kcube[4] =
	{
		gl_lens_kcube,
		gl_lens_kcube * gl_lens_chromatic,
		gl_lens_kcube * gl_lens_chromatic * gl_lens_chromatic,
		0.0f
	};

	float aspect = viewwindow_w / (float)viewwindow_h;

	// Scale factor to keep sampling within the input texture
	float r2 = aspect * aspect * 0.25 + 0.25f;
	float sqrt_r2 = sqrt(r2);
	float f0 = 1.0f + MAX(r2 * (k[0] + kcube[0] * sqrt_r2), 0.0f);
	float f2 = 1.0f + MAX(r2 * (k[2] + kcube[2] * sqrt_r2), 0.0f);
	float f = MAX(f0, f2);
	float scale = 1.0f / f;

	FGLPostProcessState savedState;

	auto mBuffers = FGLRenderBuffers::Instance();
	mBuffers->BindNextFB();
	mBuffers->BindCurrentTexture(0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	mBuffers->Lens.Bind();
	mBuffers->Lens.InputTexture.Set(0);
	mBuffers->Lens.AspectRatio.Set(aspect);
	mBuffers->Lens.Scale.Set(scale);
	mBuffers->Lens.LensDistortionCoefficient.Set(k);
	mBuffers->Lens.CubicDistortionValue.Set(kcube);
	RGL_RenderScreenQuad();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	mBuffers->NextTexture();

	//FGLDebug::PopGroup();
}

std::string RGL_LensDistortionFragmentCode()
{
	return R"(
		/*
			Original Lens Distortion Algorithm from SSontech
			http://www.ssontech.com/content/lensalg.htm

			If (u,v) are the coordinates of a feature in the undistorted perfect
			image plane, then (u', v') are the coordinates of the feature on the
			distorted image plate, ie the scanned or captured image from the
			camera. The distortion occurs radially away from the image center,
			with correction for the image aspect ratio (image_aspect = physical
			image width/height), as follows:

			r2 = image_aspect*image_aspect*u*u + v*v
			f = 1 + r2*(k + kcube*sqrt(r2))
			u' = f*u
			v' = f*v

			The constant k is the distortion coefficient that appears on the lens
			panel and through Sizzle. It is generally a small positive or negative
			number under 1%. The constant kcube is the cubic distortion value found
			on the image preprocessor's lens panel: it can be used to undistort or
			redistort images, but it does not affect or get computed by the solver.
			When no cubic distortion is needed, neither is the square root, saving
			time.

			Chromatic Aberration example,
			using red distord channel with green and blue undistord channel:

			k = vec3(-0.15, 0.0, 0.0);
			kcube = vec3(0.15, 0.0, 0.0);
		*/

		in vec2 TexCoord;
		out vec4 FragColor;

		uniform sampler2D InputTexture;
		uniform float Aspect; // image width/height
		uniform float Scale;  // 1/max(f)
		uniform vec4 k;       // lens distortion coefficient 
		uniform vec4 kcube;   // cubic distortion value

		void main()
		{
			vec2 position = (TexCoord - vec2(0.5));

			vec2 p = vec2(position.x * Aspect, position.y);
			float r2 = dot(p, p);
			vec3 f = vec3(1.0) + r2 * (k.rgb + kcube.rgb * sqrt(r2));

			vec3 x = f * position.x * Scale + 0.5;
			vec3 y = f * position.y * Scale + 0.5;

			vec3 c;
			c.r = texture(InputTexture, vec2(x.r, y.r)).r;
			c.g = texture(InputTexture, vec2(x.g, y.g)).g;
			c.b = texture(InputTexture, vec2(x.b, y.b)).b;

			FragColor = vec4(c, 1.0);
		}
	)";
}

void FLensShader::Bind()
{
	if (!mShader)
	{
		mShader.Compile(FShaderProgram::Vertex, "/pack0/shaders/glsl/screenquad.vp", "", 330);
		mShader.Compile(FShaderProgram::Fragment, "/pack0/shaders/glsl/lensdistortion.fp", "", 330);
		mShader.SetFragDataLocation(0, "FragColor");
		mShader.Link("/pack0/shaders/glsl/lensdistortion");
		mShader.SetAttribLocation(0, "PositionInProjection");
		mShader.SetAttribLocation(1, "UV");
		InputTexture.Init(mShader, "InputTexture");
		AspectRatio.Init(mShader, "Aspect");
		Scale.Init(mShader, "Scale");
		LensDistortionCoefficient.Init(mShader, "k");
		CubicDistortionValue.Init(mShader, "kcube");
	}
	mShader.Bind();
}
