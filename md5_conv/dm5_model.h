#ifndef _DM5_H_
#define _DM5_H_

#include "md5.h"
#include "md5_anim.h"

typedef float vec2_t[2];
typedef float vec3_t[3];
typedef float vec4_t[4];
typedef vec4_t quat4_t;

typedef struct {
	char jointcnt;	//number of weights per vertex
	char joint[4];	//bone id
	short vertcnt;	//number of vertices
	int shaderid;	//texture id
	
	//pointers to vertex data
	
	//position, thare are vertcnt*jointcnt vectors
	//first vertcnt# of vectors are for the first bone, then the next vertcnt# of vectors are for the second bone, etc
	//given a source vector of (x,y,z,weight), the value stored in the array is (x*weight,y*weight,z*weight,weight)
	vec4_t *pos;
	
	//vertex normal, thare are vertcnt*jointcnt vectors
	//first vertcnt# of vectors are for the first bone, then the next vertcnt# of vectors are for the second bone, etc
	//given a source vector of (x,y,z,weight), the value stored in the array is (x*weight,y*weight,z*weight,0)
	vec4_t *norm;
	
	//uv, there are vertcnt vectors
	vec2_t *uv;
	
	short *strips;	//first short is length of strip, (a length of zero terminates the list) followed by list vertex idxs. repeats
} DM5group;

typedef struct {
	vec3_t pos;
	int parentidx;
	quat4_t rot;
} DM5joint;

#define MAX_DM5_NAME 31
typedef struct {
	short version;	//version of model format
	short vertbuffsize;	//size of required vertex buffer
	int modelsize;	//size of entire model in bytes

	int jointcnt;
	DM5joint *joints;
	
	int groupcnt;
	DM5group *groups;
	
	char (*jointnames)[MAX_DM5_NAME+1];
} DM5model;

#endif
