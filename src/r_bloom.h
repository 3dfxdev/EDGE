
#pragma once

#include "r_shaderprogram.h"
#include <memory>

void RGL_BloomScene();
void RGL_UpdateCameraExposure();

class FBloomExtractShader
{
public:
	void Bind();

	FBufferedUniformSampler SceneTexture;
	FBufferedUniformSampler ExposureTexture;
	FBufferedUniform2f Scale;
	FBufferedUniform2f Offset;

private:
	FShaderProgram mShader;
};

class FBloomCombineShader
{
public:
	void Bind();

	FBufferedUniformSampler BloomTexture;

private:
	FShaderProgram mShader;
};

class FExposureExtractShader
{
public:
	void Bind();

	FBufferedUniformSampler SceneTexture;
	FBufferedUniform2f Scale;
	FBufferedUniform2f Offset;

private:
	FShaderProgram mShader;
};

class FExposureAverageShader
{
public:
	void Bind();

	FBufferedUniformSampler ExposureTexture;

private:
	FShaderProgram mShader;
};

class FExposureCombineShader
{
public:
	void Bind();

	FBufferedUniformSampler ExposureTexture;
	FBufferedUniform1f ExposureBase;
	FBufferedUniform1f ExposureMin;
	FBufferedUniform1f ExposureScale;
	FBufferedUniform1f ExposureSpeed;

private:
	FShaderProgram mShader;
};

class FBlurShader
{
public:
	void BlurVertical(float blurAmount, int sampleCount, GLuint inputTexture, GLuint outputFrameBuffer, int width, int height);
	void BlurHorizontal(float blurAmount, int sampleCount, GLuint inputTexture, GLuint outputFrameBuffer, int width, int height);

private:
	void Blur(float blurAmount, int sampleCount, GLuint inputTexture, GLuint outputFrameBuffer, int width, int height, bool vertical);

	struct BlurSetup
	{
		BlurSetup(float blurAmount, int sampleCount) : blurAmount(blurAmount), sampleCount(sampleCount) { }

		float blurAmount;
		int sampleCount;
		std::shared_ptr<FShaderProgram> VerticalShader;
		std::shared_ptr<FShaderProgram> HorizontalShader;
		FBufferedUniform1f VerticalScaleX, VerticalScaleY;
		FBufferedUniform1f HorizontalScaleX, HorizontalScaleY;
	};

	BlurSetup *GetSetup(float blurAmount, int sampleCount);

	std::string VertexShaderCode();
	std::string FragmentShaderCode(float blurAmount, int sampleCount, bool vertical);

	float ComputeGaussian(float n, float theta);
	void ComputeBlurSamples(int sampleCount, float blurAmount, std::vector<float> &sample_weights, std::vector<int> &sample_offsets);

	std::vector<BlurSetup> mBlurSetups;
};
