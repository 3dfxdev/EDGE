#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <setjmp.h>

#include "../epi/epi.h"
#include "../epi/math_quaternion.h"
#include "md5_parse.h"
#include "md5.h"

#define dbo(a...) do { printf(a); fflush(stdout); } while(0)
#define dbl() do { dbo("[%s %i]\n",__FILE__, __LINE__); } while(0)

void I_Error(const char *error,...);

static void md5_load_joints(MD5model *m) {
	int i;
	expect_token("joints", "");
	expect_token("{", "for start of joint block");
	for(i = 0; i < m->jointcnt; i++) {
		MD5joint *j = m->joints + i;
		
		expect_string(j->name, sizeof(j->name));
		
		j->parent = expect_int();
		if (j->parent < -1)
			md5_parse_error("parent of joint #%i (\"%s\") is %i; it must be greater than -1", i, j->name, j->parent);
		else if (j->parent >= m->jointcnt)
			md5_parse_error("parent of joint #%i (\"%s\") is %i; it must be less  than the number of joints (%i)", i, j->name, j->parent, m->jointcnt);
		else if (j->parent > i)
			md5_parse_error("joint tree is not strictly decending");
		else if (j->parent == i)
			md5_parse_error("joint  #%i (\"%s\") is its own parent", i, j->name);
		
		expect_paren_vector(3, j->pos);
		expect_paren_vector(3, j->rot);
		
		epi::quat_c q(j->rot[0], j->rot[1], j->rot[2]);
		j->rot[0] = q.x;
		j->rot[1] = q.y;
		j->rot[2] = q.z;
		j->rot[3] = q.w;
		
		j->mat = q.ToMat4().SetOrigin(epi::vec3_c(j->pos));
		
		//printf("%i: %s from %s", i, j->name, j->parent == -1 ? "[ROOT]" : m->joints[j->parent].name);
		//std::string m = j->mat.ToStr();
		//printf("%s\n\n",m.c_str());
	}
	expect_token("}", "for end of joint block");
}

static void md5_load_meshes(MD5model *m) {
	int i;
	for(i = 0; i < m->meshcnt; i++) {
		MD5mesh *mesh = m->meshes + i;
		int j;
		
		expect_token("mesh", "for start of mesh block");
		expect_token("{", "for start of mesh block");
		
		expect_token("shader", "");
		expect_string(mesh->shader, sizeof(mesh->shader));
		
		expect_token("numverts", "");
		mesh->vertcnt = expect_int();
		if (mesh->vertcnt <= 0)
			md5_parse_error("mesh #%i has less than zero vertices (%i found)", i, mesh->vertcnt);
		if (mesh->vertcnt >= MD5_MAX_VERTICES_PER_MESH)
			md5_parse_error("mesh #%i has too many vertices (%i found, max is %i)", i, mesh->vertcnt, MD5_MAX_VERTICES_PER_MESH);
		mesh->verts = new MD5vertex[mesh->vertcnt]();
		for(j = 0; j < mesh->vertcnt; j++) {
			MD5vertex *v = mesh->verts + j;
			
			expect_token("vert", "");
			expect_int();	//TODO should really error check more
			expect_paren_vector(2, v->uv);
			v->uv[1] = 1 - v->uv[1];
			v->firstweight = expect_int();
			v->weightcnt = expect_int();
		}
		
		expect_token("numtris", "");
		mesh->tricnt = expect_int();
		if (mesh->tricnt <= 0)
			md5_parse_error("mesh #%i has zero or fewer triangles (%i found)", i, mesh->tricnt);
		mesh->tris = new MD5triangle[mesh->tricnt]();
		for(j = 0; j < mesh->tricnt; j++) {
			MD5triangle *t = mesh->tris + j;
			
			expect_token("tri","");
			expect_int();
			t->vidx[0] = expect_int();
			t->vidx[1] = expect_int();
			t->vidx[2] = expect_int();
		}
		
		expect_token("numweights", "");
		mesh->weightcnt = expect_int();
		if (mesh->weightcnt <= 0)
			md5_parse_error("mesh #%i has zero or fewer weights (%i found)", i, mesh->weightcnt);
		mesh->weights = new MD5weight[mesh->weightcnt]();
		for(j = 0; j < mesh->weightcnt; j++) {
			MD5weight *w = mesh->weights + j;
			
			expect_token("weight","");
			expect_int();
			w->jointidx = expect_int();
			w->weight = expect_float();
			expect_paren_vector(3, w->pos);
		}
		
		expect_token("}", "for end of mesh block");
		
		printf("MESH %i: v %i, t %i, w %i\n", i, mesh->vertcnt, mesh->tricnt, mesh->weightcnt);
	}
}

MD5model * md5_load(char *md5text) {
	MD5model *m = NULL;
	start_parse(md5text);
	if (try_parse()) {
		m = new MD5model[1]();

		expect_token("MD5Version", "");
		m->version = expect_int();
		if (m->version != 10)
			md5_parse_error("expected version 10, but got version %i", m->version);

		next_token_no_eof("");
		if (token_equals("commandline")) {
			expect_string(m->commandline, sizeof(m->commandline));
			expect_token("numJoints", "");
		} else if (!token_equals("numJoints")) {
			md5_parse_error("expected numJoints");
		}
		
		//load joints number
		m->jointcnt = expect_int();
		if (m->jointcnt <= 0)
			md5_parse_error("need to have more than zero joints (found %i)", m->jointcnt);
		if (m->jointcnt >= MD5_MAX_JOINTS)
			md5_parse_error("model has %i joints but max supported is %i", m->jointcnt, MD5_MAX_JOINTS);
		m->joints = new MD5joint[m->jointcnt]();
		printf("JOINTS: %i\n", m->jointcnt);
		
		//load mesh number
		expect_token("numMeshes", "");
		m->meshcnt = expect_int();
		if (m->meshcnt <= 0)
			md5_parse_error("need to have more than zero meshes (found %i)", m->meshcnt);
		m->meshes = new MD5mesh[m->meshcnt]();
		
		
		md5_load_joints(m);
		md5_load_meshes(m);
	} else {
		I_Error("%s\n", get_parse_error());
	}
	end_parse();
	
	return m;
}


// md5 clean up
static int md5_compare_weight_real(const MD5weight * a, const MD5weight * b) {
	if (a->jointidx < b->jointidx) return -1;
	if (a->jointidx > b->jointidx) return 1;
	if (a->weight < b->weight) return -1;
	if (a->weight > b->weight) return 1;
	return 0;
}

int md5_compare_weight(const void * a, const void * b) {
	return md5_compare_weight_real((const MD5weight*)a,(const MD5weight*)b);
}

class MD5weightgroup {
public:
	MD5weight *weights;
	int weightcnt;
	
	MD5weightgroup() : weights(NULL), weightcnt(0) { };
	MD5weightgroup(MD5weight* nweights, int nweightcnt) : weights(nweights), weightcnt(nweightcnt) { };
	~MD5weightgroup() { if (weights) delete[] weights; };
	inline bool operator==(const MD5weightgroup& rhs) const {
		return (rhs.weightcnt == weightcnt) && (memcmp(weights, rhs.weights, sizeof(MD5weight) * weightcnt) == 0);
	}
};

#define MAX_WEIGHT_GROUPS 10000
int weightgroupcnt,totalweights;
MD5weightgroup allweightgroups[MAX_WEIGHT_GROUPS];
epi::vec3_c groupbindpose[MAX_WEIGHT_GROUPS];
epi::vec3_c groupnormals[MAX_WEIGHT_GROUPS];
int **vertexidx_to_weightgroup;

void md5_print_weight(MD5weight *w) {
	printf("J: %2i  W: %02.2f  ( %f, %f, %f )\n",w->jointidx, w->weight, w->pos[0], w->pos[1], w->pos[2]);
}

void md5_print_weights(MD5weight *w, int cnt) {
	while(cnt--)
		md5_print_weight(w++);
}

void md5_print_weight_group(MD5weightgroup *wg) {
	md5_print_weights(wg->weights, wg->weightcnt);
}

//returns a copy of the input weights, with small weights removed and sorted by joint id
MD5weightgroup * md5_copy_canonized_weights(MD5weight *inweight, int inweightcnt, float minweight, int maxweightcnt) {
	int i,outcnt;
	float weightlost = 0;
	
	if (inweightcnt == 0)
		I_Error("have zero weights");
	
	MD5weight *neww = new MD5weight[inweightcnt]();
	
	
	for(i = 0, outcnt = 0; i < inweightcnt; i++) {
		//add weight if it meets threshold, if it doesn't record weight lost
		if (inweight[i].weight >= minweight)
			neww[outcnt++] = inweight[i];
		else
			weightlost += inweight[i].weight;
	}
	
	if (outcnt > maxweightcnt) {
		
	}
	
	//add weight lost back in
	if (outcnt == 0)
		I_Error("have zero weights result");	//TODO should instead find the greatest inweight and just use that
	weightlost /= outcnt;
	for(i = 0; i < outcnt; i++) {
		neww[i].weight += weightlost;
	}
	
	//sort
	qsort(neww, outcnt, sizeof(MD5weight), md5_compare_weight);
	
	return new MD5weightgroup(neww, outcnt);
}

float minimum_weight = 0.05;

int find_matching_weightgroup(MD5weightgroup &wg) {
	//TODO should replace with hash lookup
	int i;
	for(i = 0; i < weightgroupcnt; i++) {
		if (allweightgroups[i] == wg)
			return i;
	}
	return -1;
}

void md5_get_unique_weights(MD5model *m) {
	int i,j;
	
	assert(!vertexidx_to_weightgroup);
	
	vertexidx_to_weightgroup = new int*[m->meshcnt]();
	
	for(i = 0; i < m->meshcnt; i++) {
		MD5mesh *mesh = m->meshes + i;
		vertexidx_to_weightgroup[i] = new int[mesh->vertcnt]();
		
		for(j = 0; j < mesh->vertcnt; j++) {
			MD5vertex *vert = mesh->verts + j;
			MD5weightgroup *nw = md5_copy_canonized_weights(mesh->weights + vert->firstweight, vert->weightcnt, minimum_weight, 10);
			int match = find_matching_weightgroup(*nw);
			if (match == -1) {
				allweightgroups[weightgroupcnt] = *nw;
				vertexidx_to_weightgroup[i][j] = weightgroupcnt++;
				totalweights += nw->weightcnt;
			} else {
				delete nw;
				vertexidx_to_weightgroup[i][j] = match;
			}
		}
	}
	printf("Weight group count: %i\n",weightgroupcnt);
	printf("Total weight count: %i\n",totalweights);
}


void md5_create_bindpose(MD5model *model) {
	epi::mat4_c posemats[MD5_MAX_JOINTS+1];
	epi::mat4_c *matbase = posemats + 1;
	int i,j;
	
	matbase[-1] = epi::mat4_c();
	for(i = 0; i < model->jointcnt; i++) {
		MD5joint *j = model->joints + i;
		matbase[i] = j->mat;
	}
	
	for(i = 0; i < weightgroupcnt; i++) {
		groupbindpose[i] = epi::vec3_c();
		
		MD5weight *w = allweightgroups[i].weights;
		for(j = 0; j < allweightgroups[i].weightcnt; j++) {
			groupbindpose[i] += matbase[w[j].jointidx] * epi::vec3_c(w[j].pos) * w[j].weight;
		}
	}
}

void md5_create_normals(MD5model *model) {
	int i,j;
	
	assert(vertexidx_to_weightgroup);
	
	//clear normals
	for(i = 0; i < weightgroupcnt; i++)
		groupnormals[i] = epi::vec3_c();
	
	//accumulate face normals to vertex normals
	for(i = 0; i < model->meshcnt; i++) {
		MD5mesh *mesh = model->meshes + i;
	
		for(j = 0; j < mesh->tricnt; j++) {
			MD5triangle *tri = mesh->tris + j;
			int a = vertexidx_to_weightgroup[i][tri->vidx[0]];
			int b = vertexidx_to_weightgroup[i][tri->vidx[1]];
			int c = vertexidx_to_weightgroup[i][tri->vidx[2]];
			
			epi::vec3_c edge1 = groupbindpose[a] - groupbindpose[b];
			epi::vec3_c edge2 = groupbindpose[c] - groupbindpose[b];
			epi::vec3_c norm = edge1.Cross(edge2);
			
			groupnormals[a] += norm;
			groupnormals[b] += norm;
			groupnormals[c] += norm;
		}
	}
	
	//create joint-space normals
	for(i = 0; i < weightgroupcnt; i++) {
		groupnormals[i].MakeUnit();
		
		MD5weight *w;
		for(j = 0, w = allweightgroups[i].weights; j < allweightgroups[i].weightcnt; j++, w++) {
			epi::vec3_c jointnormal = epi::quat_c(model->joints[w->jointidx].rot).Invert().Rotate(groupnormals[i]);
			w->normal[0] = jointnormal.x;
			w->normal[1] = jointnormal.y;
			w->normal[2] = jointnormal.z;
		}
		
	}
}

MD5umodel * md5_assemble_unified_model(MD5model *m) {
	int i,j;
	MD5umodel *umodel = new MD5umodel[1]();
	
	//copy weight group data into unified weight array, create weightgroup map
	//weightgroup map is used to convert weight index into index into unified array
	int *weightgroupmap = new int[weightgroupcnt]();
	umodel->weights = new MD5weight[totalweights]();
	umodel->weightcnt = weightgroupcnt;
	MD5weight *curweight = umodel->weights;
	
	for(i = 0; i < weightgroupcnt; i++) {
		memcpy(curweight, allweightgroups[i].weights, sizeof(MD5weight) * allweightgroups[i].weightcnt);
		weightgroupmap[i] = curweight - umodel->weights;
		curweight += allweightgroups[i].weightcnt;
	}
	
	memcpy(&umodel->model, m, sizeof(MD5model));
	
	//copy joints
	umodel->model.joints = new MD5joint[m->jointcnt]();
	memcpy(umodel->model.joints, m->joints, sizeof(MD5joint) * m->jointcnt);
	
	//create new meshes uses unified weights
	umodel->model.meshes = new MD5mesh[m->meshcnt]();
	for(i = 0; i < m->meshcnt; i++) {
		MD5mesh *srcmesh = m->meshes + i, *dstmesh = umodel->model.meshes + i;
		
		//copy source mesh data
		memcpy(dstmesh, srcmesh, sizeof(MD5mesh));
		
		//point weights to unified weights
		dstmesh->weights = umodel->weights;
		dstmesh->weightcnt = umodel->weightcnt;
		
		//copy triangles
		dstmesh->tris = new MD5triangle[dstmesh->tricnt]();
		memcpy(dstmesh->tris, srcmesh->tris, sizeof(MD5triangle) * dstmesh->tricnt);
		
		//copy vertices and remap weights
		dstmesh->verts = new MD5vertex[dstmesh->vertcnt]();
		for(j = 0; j < dstmesh->vertcnt; j++) {
			dstmesh->verts[j].uv[0] = srcmesh->verts[j].uv[0];
			dstmesh->verts[j].uv[1] = srcmesh->verts[j].uv[1];
			dstmesh->verts[j].weightcnt = allweightgroups[vertexidx_to_weightgroup[i][j]].weightcnt;
			dstmesh->verts[j].firstweight = weightgroupmap[vertexidx_to_weightgroup[i][j]];
		}
	}
	
	delete weightgroupmap;
	return umodel;
}

MD5umodel * md5_normalize_model(MD5model *m) {
	//1 step through vertices and generate new weight list with small weights removed
	//2 sort the weights for each vertex by bone
	//3 find vertices with identical positions and set them all to use the same weight idx
	//4 generate a new weight list with unused weights removed
	//5 create vertex list mapped to new weight list
	//6 create vertex normals
	printf("Creating unique weights...\n");
	md5_get_unique_weights(m);
	printf("Creating bind pose...\n");
	md5_create_bindpose(m);
	printf("Creating normals...\n");
	md5_create_normals(m);
	MD5umodel *umodel = md5_assemble_unified_model(m);
	printf("Model normalized\n");
	
	return umodel;
}

void md5_free(MD5model * m) {
	int i;
	
	for(i = 0; i < m->meshcnt; i++) {
		MD5mesh *srcmesh = m->meshes + i;
		
		delete[] srcmesh->verts;
		delete[] srcmesh->tris;
		delete[] srcmesh->weights;
	}
	
	delete[] m->joints;
	delete[] m->meshes;
	delete[] m;
}

void md5_premultiply_unified_model(MD5umodel * m) {
	int i;
	
	for(i = 0; i < m->weightcnt+20; i++) {
		MD5weight *w = m->weights + i;
		w->pos[0] *= w->weight;
		w->pos[1] *= w->weight;
		w->pos[2] *= w->weight;
		w->normal[0] *= w->weight;
		w->normal[1] *= w->weight;
		w->normal[2] *= w->weight;
	}
}

