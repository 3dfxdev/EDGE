#ifndef _MS3DFILE_H_
#define _MS3DFILE_H_

#include <pshpack1.h>

#ifndef byte
typedef unsigned char byte;
#endif // byte

#ifndef word
typedef unsigned short word;
#endif // word

typedef struct
{
    char    id[10];                                     // always "MS3D000000"
    int     version;                                    // 4
} ms3d_header_t;

typedef struct
{
    byte    flags;                                      // SELECTED | SELECTED2 | HIDDEN
    float   vertex[3];                                  //
    char    boneId;                                     // -1 = no bone
    byte    referenceCount;
} ms3d_vertex_t;

typedef struct
{
    word    flags;                                      // SELECTED | SELECTED2 | HIDDEN
    word    vertexIndices[3];                           //
    float   vertexNormals[3][3];                        //
    float   s[3];                                       //
    float   t[3];                                       //
    byte    smoothingGroup;                             // 1 - 32
    byte    groupIndex;                                 //
} ms3d_triangle_t;

typedef struct
{
	word edgeIndices[2];
} ms3d_edge_t;

typedef struct
{
    byte            flags;                              // SELECTED | HIDDEN
    char            name[32];                           //
    word            numtriangles;                       //
	word*			triangleIndices;					// the groups group the triangles
    char            materialIndex;                      // -1 = no material
} ms3d_group_t;

typedef struct
{
    char            name[32];                           //
    float           ambient[4];                         //
    float           diffuse[4];                         //
    float           specular[4];                        //
    float           emissive[4];                        //
    float           shininess;                          // 0.0f - 128.0f
    float           transparency;                       // 0.0f - 1.0f
    char            mode;                               // 0, 1, 2 is unused now
    char            texture[128];                        // texture.bmp
    char            alphamap[128];                       // alpha.bmp
} ms3d_material_t;

typedef struct
{
    float           time;                               // time in seconds
    float           rotation[3];                        // x, y, z angles
} ms3d_keyframe_rot_t;

typedef struct
{
    float           time;                               // time in seconds
    float           position[3];                        // local position
} ms3d_keyframe_pos_t;

typedef struct
{
    byte            flags;                              // SELECTED | DIRTY
    char            name[32];                           //
    char            parentName[32];                     //
    float           rotation[3];                        // local reference matrix
    float           position[3];

    word            numKeyFramesRot;                    //
    word            numKeyFramesTrans;                  //

	ms3d_keyframe_rot_t* keyFramesRot;      // local animation matrices
    ms3d_keyframe_pos_t* keyFramesTrans;  // local animation matrices
} ms3d_joint_t;

#include <poppack.h>

class CMS3DFileI;
class CMS3DFile
{
public:
	CMS3DFile();
	virtual ~CMS3DFile();

public:
	bool LoadFromFile(const char* lpszFileName);
	void Clear();

	int GetNumVertices();
	void GetVertexAt(int nIndex, ms3d_vertex_t **ppVertex);
	int GetNumTriangles();
	void GetTriangleAt(int nIndex, ms3d_triangle_t **ppTriangle);
	int GetNumEdges();
	void GetEdgeAt(int nIndex, ms3d_edge_t **ppEdge);
	int GetNumGroups();
	void GetGroupAt(int nIndex, ms3d_group_t **ppGroup);
	int GetNumMaterials();
	void GetMaterialAt(int nIndex, ms3d_material_t **ppMaterial);
	int GetNumJoints();
	void GetJointAt(int nIndex, ms3d_joint_t **ppJoint);
	int FindJointByName(const char* lpszName);

	float GetAnimationFPS();
	float GetCurrentTime();
	int GetTotalFrames();

private:
	CMS3DFileI *_i;

private:
	CMS3DFile(const CMS3DFile& rhs);
	CMS3DFile& operator=(const CMS3DFile& rhs);
};

#endif // _MS3DFILE_H_