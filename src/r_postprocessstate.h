
#pragma once

#include <vector>

class FGLPostProcessState
{
public:
	FGLPostProcessState();
	~FGLPostProcessState();

	void SaveTextureBindings(unsigned int numUnits);

private:
	FGLPostProcessState(const FGLPostProcessState &) = delete;
	FGLPostProcessState &operator=(const FGLPostProcessState &) = delete;

	GLint activeTex;
	std::vector<GLint> textureBinding;
	std::vector<GLint> samplerBinding;
	GLboolean blendEnabled;
	GLboolean scissorEnabled;
	GLboolean depthEnabled;
	GLboolean multisampleEnabled;
	GLint currentProgram;
	GLint blendEquationRgb;
	GLint blendEquationAlpha;
	GLint blendSrcRgb;
	GLint blendSrcAlpha;
	GLint blendDestRgb;
	GLint blendDestAlpha;
};
