//----------------------------------------------------------------------------
//  3DGE GL2 Shader
//----------------------------------------------------------------------------
// 
//  Copyright (c) 2015 Isotope SoftWorks
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
//  Note: OpenGL 2 only!! 
//----------------------------------------------------------------------------

#ifndef __R_BUMPMAP_H__
#define __R_BUMPMAP_H__

#include "system/i_defs_gl.h"

class bump_map_shader {
public:
	bump_map_shader();
	~bump_map_shader();
	bool supported();
	void bind();
	void unbind();
	void deinit();

	//colors are 0-1
	void lightParamAmbient(float r,float g,float b);
	void lightParam(int index,float x,float y,float z,float r,float g,float b,float radius);
	void lightDisable(int index);
	void lightApply();
	void setCamMatrix(float mat[16]);

	void debugDrawLights();

	GLuint attr_tan;	//tangent attribute location
	//GLuint vbo_tan;		//tangent vbo handle

	GLuint uni_light_pos;
	GLuint uni_light_color;
	GLuint uni_light_radius;
	GLuint uni_light_color_ambient;

	GLuint tex_default_normal;
	GLuint tex_default_specular;

	static const int max_lights;

private:
	bool _inited;
	bool _supported;
	void check_init();

	GLuint h_prog;		//shader program handle
	GLuint h_vertex;	//vertex shader handle
	GLuint h_fragment;	//fragment shader handle

	float *data_light_pos;
	float *data_light_color;
	float *data_light_radius;
	float data_light_color_ambient[4];
	float cam_mat[16];
};

#endif

