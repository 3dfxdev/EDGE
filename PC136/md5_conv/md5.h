#ifndef _MD5_H_
#define _MD5_H_

#include "../epi/epi.h"
#include "../epi/math_vector.h"
#include "../epi/math_matrix.h"
#include "../epi/math_quaternion.h"

typedef float vec2_t[2];
typedef float vec3_t[3];
typedef float vec4_t[4];
typedef vec3_t quat3_t;
typedef vec4_t quat4_t;

typedef char MD5jointidx;

#define MAX_MD5_NAME 79
#define MD5_MAX_JOINTS (100)
#define MD5_MAX_VERTICES_PER_MESH (30000)

//md5 mesh
typedef struct MD5joint_s MD5joint;
struct MD5joint_s {
	int parent;
	vec3_t pos;
	quat4_t rot;
	epi::mat4_c mat;
	char name[MAX_MD5_NAME];
};

typedef struct {
	int jointidx;
	float weight;
	vec3_t pos;
	vec3_t normal;
} MD5weight;

typedef unsigned short md5_vert_idx;
typedef struct {
	md5_vert_idx vidx[3];
} MD5triangle;

typedef struct {
	vec2_t uv;
	int firstweight, weightcnt;
} MD5vertex;

typedef struct {
	char shader[MAX_MD5_NAME];
	int gltex;
	
	int vertcnt;
	MD5vertex *verts;
	
	int tricnt;
	MD5triangle *tris;
	
	int weightcnt;
	MD5weight *weights;
} MD5mesh;

typedef struct {
	int version;
	char commandline[MAX_MD5_NAME];
	
	int jointcnt;
	MD5joint *joints;
	
	int meshcnt;
	MD5mesh *meshes;
} MD5model;


void md5_free(MD5model * m);

//unified md5
typedef struct {
	MD5model model;
	int weightcnt;
	MD5weight *weights;
} MD5umodel;

#endif
