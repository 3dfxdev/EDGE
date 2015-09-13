#include "r_bumpmap.h"
#include "i_defs.h"


#include <assert.h>
#include <stdio.h>

const int bump_map_shader::max_lights=5;

static const char* src_vertex=
"#version 120\n"
"#define MAX_LIGHTS 5\n"
"attribute vec3 tangent;\n"
"varying vec3 surfaceNormal;\n"
"varying vec3 surfaceTan;\n"
"varying vec3 surfaceBitan;\n"
"varying vec4 surfacePos;\n"
"void main() {\n"
"	surfaceNormal=normalize(gl_NormalMatrix*gl_Normal);\n"
"	surfaceTan=normalize(gl_NormalMatrix*tangent);\n"
"	surfaceBitan=cross(surfaceNormal,surfaceTan);\n"
"	surfacePos=gl_ModelViewMatrix*gl_Vertex;\n"
"	gl_TexCoord[0]=gl_MultiTexCoord0;\n"
"	gl_TexCoord[1]=gl_MultiTexCoord1;\n"
"	gl_Position=ftransform();\n"
"	gl_FrontColor = gl_Color;\n"
"}\n";

static const char* src_fragment=
"#version 120\n"
"#define MAX_LIGHTS 5\n"
"uniform sampler2D tex_color;\n"
"uniform sampler2D tex_normal;\n"
"uniform sampler2D tex_specular;\n"
"uniform vec4 light_pos[MAX_LIGHTS];\n"
"uniform vec4 light_color[MAX_LIGHTS];\n"
"uniform float light_r[MAX_LIGHTS];\n"
"varying vec4 surfacePos;\n"
"varying vec3 surfaceNormal;\n"
"varying vec3 surfaceTan;\n"
"varying vec3 surfaceBitan;\n"
"void main() {\n"
"	vec2 tx=gl_TexCoord[0].xy;\n"
"	vec3 normal = normalize(2.0 * texture2D (tex_normal, tx).rgb - 1.0);\n"
"	vec4 diffuseMaterial = texture2D(tex_color, tx); //*gl_Color;\n"
"	vec4 specularMaterial =  texture2D(tex_specular, tx);\n"
"	mat3 normal_mat=mat3(surfaceTan,surfaceBitan,surfaceNormal);\n"
"	vec3 n=normalize(normal_mat*normal);\n"
"	gl_FragColor=vec4(0);\n"
"	for(int i=0;i<MAX_LIGHTS;i++) {\n"
"		vec3 L = normalize(light_pos[i].xyz - surfacePos.xyz);\n"
"		vec3 E = normalize(-surfacePos.xyz);\n"
"		vec3 R = normalize(-reflect(L,n));\n"
"		vec4 Idiff = vec4(max(dot(n,L),0.0))*diffuseMaterial;\n"
"		Idiff = clamp(Idiff, 0.0, 1.0);\n"
"		vec4 Ispec = vec4(pow(max(dot(R,E),0.0),0.3))*specularMaterial;\n"
"		Ispec = clamp(Ispec, 0.0, 1.0);\n"
"		float power=max(0.0, (light_r[i]-distance(light_pos[i],surfacePos))/light_r[i] );\n"
"		//float power=step(distance(light_pos[i],surfacePos),light_r[i]);\n"
"		//float power = 1.0 / (1.0 + 1 * pow(distance(light_pos[i],surfacePos), 2));\n"
"		gl_FragColor+=(Idiff+Ispec)*light_color[i]*power;\n"
"	}\n"
"	gl_FragColor.a=diffuseMaterial.a;\n"
"	//gl_FragColor=vec4(surfaceTan,1);\n"
"}\n";

void matrix_mult(const float m[16],const float vec_in[3],float vec_out[3]);

GLuint compile_shader(GLenum type,const char* source_c) {
	GLuint handle=glCreateShader(type);

	// Compile the shader.
	glShaderSource(handle,1,&source_c,NULL);
	glCompileShader(handle);

	GLint vShaderCompiled = GL_FALSE;
	glGetShaderiv(handle,GL_COMPILE_STATUS,&vShaderCompiled);
	if(vShaderCompiled!=GL_TRUE) {
		I_Printf("Unable to compile shader %d!\n",handle);
		int infoLogLength = 0;
		int maxLength = infoLogLength;
		glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &maxLength);
		char* infoLog = new char[ maxLength ];
		glGetShaderInfoLog( handle, maxLength, &infoLogLength, infoLog );
		infoLog[infoLogLength]=0;
		if( infoLogLength > 0 ) {
			I_Printf( "%s\n", infoLog );
		}
		delete[] infoLog;

		glDeleteShader(handle);
		return 0;
	}
	return handle;
}

bump_map_shader::bump_map_shader():
	_inited(false),
	_supported(false),
	h_prog(0) {
	data_light_pos=new float[4*max_lights *2];
	data_light_color=new float[4*max_lights];
	data_light_radius=new float[max_lights];

	for(int i=0;i<max_lights;i++) {
		data_light_pos[i*4+0]=0;
		data_light_pos[i*4+1]=0;
		data_light_pos[i*4+2]=0;
		data_light_pos[i*4+3]=1;
		data_light_color[i*4+0]=0;
		data_light_color[i*4+1]=0;
		data_light_color[i*4+2]=0;
		data_light_color[i*4+3]=1;
		data_light_radius[i]=0;
	}
}

bump_map_shader::~bump_map_shader() {
	deinit();
	delete[] data_light_pos;
	delete[] data_light_color;
	delete[] data_light_radius;
}

bool bump_map_shader::supported() {
	check_init();
	return _supported;
}
void bump_map_shader::bind() {
	glUseProgram(h_prog);
}
void bump_map_shader::unbind() {
	glUseProgram(0);
}

void bump_map_shader::check_init() {
	if(_inited) {
		return;
	}
	_inited=true;

	printf("BUMP INIT\n");

	_supported=(glCreateProgram);

	if(!_supported) {
		return;
	}

	h_prog=glCreateProgram();
	h_vertex=compile_shader(GL_VERTEX_SHADER,src_vertex);
	h_fragment=compile_shader(GL_FRAGMENT_SHADER,src_fragment);

	glAttachShader(h_prog,h_vertex);
	glAttachShader(h_prog,h_fragment);
	glLinkProgram(h_prog);
	glUseProgram(h_prog);

	glUniform1i(glGetUniformLocation(h_prog,"tex_color"),0);
	glUniform1i(glGetUniformLocation(h_prog,"tex_normal"),1);
	glUniform1i(glGetUniformLocation(h_prog,"tex_specular"),2);
	attr_tan=glGetAttribLocation(h_prog, "tangent");
	uni_light_pos=glGetUniformLocation(h_prog,"light_pos");
	uni_light_color=glGetUniformLocation(h_prog,"light_color");
	uni_light_radius=glGetUniformLocation(h_prog,"light_r");

	//glGenBuffers(1,&vbo_tan);
	lightApply();

	unbind();
}
void bump_map_shader::deinit() {
	if(!_inited) {
		return;
	}
	_inited=false;

	printf("BUMP DEINIT\n");

	if(!_supported) {
		return;
	}

	glDeleteProgram(h_prog);
	glDeleteShader(h_vertex);
	glDeleteShader(h_fragment);
	//glDeleteBuffers(1,&vbo_tan);

}
void bump_map_shader::lightDisable(int index) {
	if(index>=max_lights) {
		return;
	}
	data_light_color[index*4+0]=0;
	data_light_color[index*4+1]=0;
	data_light_color[index*4+2]=0;
}
void bump_map_shader::lightParam(int index,float x,float y,float z,float r,float g,float b,float radius) {
	if(index>=max_lights) {
		return;
	}
	data_light_pos[index*4+0]=x;
	data_light_pos[index*4+1]=y;
	data_light_pos[index*4+2]=z;
	data_light_color[index*4+0]=r;
	data_light_color[index*4+1]=g;
	data_light_color[index*4+2]=b;
	data_light_radius[index]=radius;
}
void bump_map_shader::lightApply() {
	int pos_offset=4*max_lights;
	for(int i=0;i<max_lights;i++) {
		int off=i*4;
		matrix_mult(cam_mat,data_light_pos+off,data_light_pos+pos_offset+off);
	}

	glUniform4fv(uni_light_pos, max_lights, data_light_pos+pos_offset);
	glUniform4fv(uni_light_color, max_lights, data_light_color);
	glUniform1fv(uni_light_radius, max_lights, data_light_radius);
}

void bump_map_shader::setCamMatrix(float mat[16]) {
	for(int i=0;i<16;i++) cam_mat[i]=mat[i];
}

void bump_map_shader::debugDrawLights() {
	glBegin(GL_LINES);
	for(int i=0;i<max_lights;i++) {
		if(data_light_radius[i]==0.0f) {
			continue;
		}

		glColor3fv(data_light_color+i*4);
		float* p=data_light_pos+i*4;
		float r=data_light_radius[i];

		glVertex3f(p[0]-r,p[1],p[2]);
		glVertex3f(p[0]+r,p[1],p[2]);
		glVertex3f(p[0],p[1]-r,p[2]);
		glVertex3f(p[0],p[1]+r,p[2]);
		glVertex3f(p[0],p[1],p[2]-r);
		glVertex3f(p[0],p[1],p[2]+r);

	}
	glEnd();
}

void matrix_mult(const float m[16],const float vec_in[3],float vec_out[3]) {
	for (int i=0;i<3;i++){
		vec_out[i]=m[0*4+i]*vec_in[0] + m[1*4+i]*vec_in[1] + m[2*4+i]*vec_in[2] + m[3*4+i];
	}
}

