#pragma warning(disable : 4786)
#include "MS3DFile.h"
#include <set>
#include <vector>

#define MAKEDWORD(a, b)      ((unsigned int)(((word)(a)) | ((word)((word)(b))) << 16))

class CMS3DFileI
{
public:
	std::vector<ms3d_vertex_t> arrVertices;
	std::vector<ms3d_triangle_t> arrTriangles;
	std::vector<ms3d_edge_t> arrEdges;
	std::vector<ms3d_group_t> arrGroups;
	std::vector<ms3d_material_t> arrMaterials;
	float fAnimationFPS;
	float fCurrentTime;
	int iTotalFrames;
	std::vector<ms3d_joint_t> arrJoints;

public:
	CMS3DFileI()
	:	fAnimationFPS(24.0f),
		fCurrentTime(0.0f),
		iTotalFrames(0)
	{
	}
};

CMS3DFile::CMS3DFile()
{
	_i = new CMS3DFileI();
}

CMS3DFile::~CMS3DFile()
{
	delete _i;
}

bool CMS3DFile::LoadFromFile(const char* lpszFileName)
{
	FILE *fp = fopen(lpszFileName, "rb");
	if (!fp)
		return false;

	size_t i;
	ms3d_header_t header;
	fread(&header, 1, sizeof(ms3d_header_t), fp);

	if (strncmp(header.id, "MS3D000000", 10) != 0)
		return false;

	if (header.version != 4)
		return false;

	// vertices
	word nNumVertices;
	fread(&nNumVertices, 1, sizeof(word), fp);
	_i->arrVertices.resize(nNumVertices);
	fread(&_i->arrVertices[0], nNumVertices, sizeof(ms3d_vertex_t), fp);

	// triangles
	word nNumTriangles;
	fread(&nNumTriangles, 1, sizeof(word), fp);
	_i->arrTriangles.resize(nNumTriangles);
	fread(&_i->arrTriangles[0], nNumTriangles, sizeof(ms3d_triangle_t), fp);

	// edges
	std::set<unsigned int> setEdgePair;
	for (i = 0; i < _i->arrTriangles.size(); i++)
	{
		word a, b;
		a = _i->arrTriangles[i].vertexIndices[0];
		b = _i->arrTriangles[i].vertexIndices[1];
		if (a > b)
			std::swap(a, b);
		if (setEdgePair.find(MAKEDWORD(a, b)) == setEdgePair.end())
			setEdgePair.insert(MAKEDWORD(a, b));

		a = _i->arrTriangles[i].vertexIndices[1];
		b = _i->arrTriangles[i].vertexIndices[2];
		if (a > b)
			std::swap(a, b);
		if (setEdgePair.find(MAKEDWORD(a, b)) == setEdgePair.end())
			setEdgePair.insert(MAKEDWORD(a, b));

		a = _i->arrTriangles[i].vertexIndices[2];
		b = _i->arrTriangles[i].vertexIndices[0];
		if (a > b)
			std::swap(a, b);
		if (setEdgePair.find(MAKEDWORD(a, b)) == setEdgePair.end())
			setEdgePair.insert(MAKEDWORD(a, b));
	}

	for(std::set<unsigned int>::iterator it = setEdgePair.begin(); it != setEdgePair.end(); ++it)
	{
		unsigned int EdgePair = *it;
		ms3d_edge_t Edge;
		Edge.edgeIndices[0] = (word) EdgePair;
		Edge.edgeIndices[1] = (word) ((EdgePair >> 16) & 0xFFFF);
		_i->arrEdges.push_back(Edge);
	}

	// groups
	word nNumGroups;
	fread(&nNumGroups, 1, sizeof(word), fp);
	_i->arrGroups.resize(nNumGroups);
	for (i = 0; i < nNumGroups; i++)
	{
		fread(&_i->arrGroups[i].flags, 1, sizeof(byte), fp);
		fread(&_i->arrGroups[i].name, 32, sizeof(char), fp);
		fread(&_i->arrGroups[i].numtriangles, 1, sizeof(word), fp);
		_i->arrGroups[i].triangleIndices = new word[_i->arrGroups[i].numtriangles];
		fread(_i->arrGroups[i].triangleIndices, _i->arrGroups[i].numtriangles, sizeof(word), fp);
		fread(&_i->arrGroups[i].materialIndex, 1, sizeof(char), fp);
	}

	// materials
	word nNumMaterials;
	fread(&nNumMaterials, 1, sizeof(word), fp);
	_i->arrMaterials.resize(nNumMaterials);
	fread(&_i->arrMaterials[0], nNumMaterials, sizeof(ms3d_material_t), fp);

	fread(&_i->fAnimationFPS, 1, sizeof(float), fp);
	fread(&_i->fCurrentTime, 1, sizeof(float), fp);
	fread(&_i->iTotalFrames, 1, sizeof(int), fp);

	// joints
	word nNumJoints;
	fread(&nNumJoints, 1, sizeof(word), fp);
	_i->arrJoints.resize(nNumJoints);
	for (i = 0; i < nNumJoints; i++)
	{
		fread(&_i->arrJoints[i].flags, 1, sizeof(byte), fp);
		fread(&_i->arrJoints[i].name, 32, sizeof(char), fp);
		fread(&_i->arrJoints[i].parentName, 32, sizeof(char), fp);
		fread(&_i->arrJoints[i].rotation, 3, sizeof(float), fp);
		fread(&_i->arrJoints[i].position, 3, sizeof(float), fp);
		fread(&_i->arrJoints[i].numKeyFramesRot, 1, sizeof(word), fp);
		fread(&_i->arrJoints[i].numKeyFramesTrans, 1, sizeof(word), fp);
		_i->arrJoints[i].keyFramesRot = new ms3d_keyframe_rot_t[_i->arrJoints[i].numKeyFramesRot];
		_i->arrJoints[i].keyFramesTrans = new ms3d_keyframe_pos_t[_i->arrJoints[i].numKeyFramesTrans];
		fread(_i->arrJoints[i].keyFramesRot, _i->arrJoints[i].numKeyFramesRot, sizeof(ms3d_keyframe_rot_t), fp);
		fread(_i->arrJoints[i].keyFramesTrans, _i->arrJoints[i].numKeyFramesTrans, sizeof(ms3d_keyframe_pos_t), fp);
	}

	fclose(fp);

	return true;
}

void CMS3DFile::Clear()
{
	_i->arrVertices.clear();
	_i->arrTriangles.clear();
	_i->arrEdges.clear();
	_i->arrGroups.clear();
	_i->arrMaterials.clear();
	_i->arrJoints.clear();
}

int CMS3DFile::GetNumVertices()
{
	return (int) _i->arrVertices.size();
}

void CMS3DFile::GetVertexAt(int nIndex, ms3d_vertex_t **ppVertex)
{
	if (nIndex >= 0 && nIndex < (int) _i->arrVertices.size())
		*ppVertex = &_i->arrVertices[nIndex];
}

int CMS3DFile::GetNumTriangles()
{
	return (int) _i->arrTriangles.size();
}

void CMS3DFile::GetTriangleAt(int nIndex, ms3d_triangle_t **ppTriangle)
{
	if (nIndex >= 0 && nIndex < (int) _i->arrTriangles.size())
		*ppTriangle = &_i->arrTriangles[nIndex];
}

int CMS3DFile::GetNumEdges()
{
	return (int) _i->arrEdges.size();
}

void CMS3DFile::GetEdgeAt(int nIndex, ms3d_edge_t **ppEdge)
{
	if (nIndex >= 0 && nIndex < (int) _i->arrEdges.size())
		*ppEdge = &_i->arrEdges[nIndex];
}

int CMS3DFile::GetNumGroups()
{
	return (int) _i->arrGroups.size();
}

void CMS3DFile::GetGroupAt(int nIndex, ms3d_group_t **ppGroup)
{
	if (nIndex >= 0 && nIndex < (int) _i->arrGroups.size())
		*ppGroup = &_i->arrGroups[nIndex];
}

int CMS3DFile::GetNumMaterials()
{
	return (int) _i->arrMaterials.size();
}

void CMS3DFile::GetMaterialAt(int nIndex, ms3d_material_t **ppMaterial)
{
	if (nIndex >= 0 && nIndex < (int) _i->arrMaterials.size())
		*ppMaterial = &_i->arrMaterials[nIndex];
}

int CMS3DFile::GetNumJoints()
{
	return (int) _i->arrJoints.size();
}

void CMS3DFile::GetJointAt(int nIndex, ms3d_joint_t **ppJoint)
{
	if (nIndex >= 0 && nIndex < (int) _i->arrJoints.size())
		*ppJoint = &_i->arrJoints[nIndex];
}

int CMS3DFile::FindJointByName(const char* lpszName)
{
	for (size_t i = 0; i < _i->arrJoints.size(); i++)
	{
		if (!strcmp(_i->arrJoints[i].name, lpszName))
			return i;
	}

	return -1;
}

float CMS3DFile::GetAnimationFPS()
{
	return _i->fAnimationFPS;
}

float CMS3DFile::GetCurrentTime()
{
	return _i->fCurrentTime;
}

int CMS3DFile::GetTotalFrames()
{
	return _i->iTotalFrames;
}
