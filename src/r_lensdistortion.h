
#pragma once

#include "r_shaderprogram.h"

void RGL_LensDistortScene();

class FLensShader
{
public:
	void Bind();

	FBufferedUniformSampler InputTexture;
	FBufferedUniform1f AspectRatio;
	FBufferedUniform1f Scale;
	FBufferedUniform4f LensDistortionCoefficient;
	FBufferedUniform4f CubicDistortionValue;

private:
	FShaderProgram mShader;
};
