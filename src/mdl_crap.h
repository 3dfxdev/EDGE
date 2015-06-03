//MOVED INTO R_MDL.CC

typedef u32_t f32_t;
//#define HLMDLHEADER		"IDST"
#define MDL_IDENTIFIER  "IDST"
/*
 -----------------------------------------------------------------------------------------------------------------------
    main model header
 -----------------------------------------------------------------------------------------------------------------------
 */
typedef struct
{
    int		filetypeid;	//IDSP
	int		version;	//10
    char	name[64];
    int		filesize;
    vec3_t	unknown3[5];
    int		unknown4;
    int		numbones;
    int		boneindex;
    int		numcontrollers;
    int		controllerindex;
    int		unknown5[2];
    int		numseq;
    int		seqindex;
    int		unknown6;
    int		seqgroups;
    int		numtextures;
    int		textures;
    int		unknown7[3];
    int		skins;
    int		numbodyparts;
    int		bodypartindex;
    int		unknown9[8];
} hlmdl_header_t;

/*
 -----------------------------------------------------------------------------------------------------------------------
    skin info
 -----------------------------------------------------------------------------------------------------------------------
 */
typedef struct
{
    char	name[64];
    int		flags;
    int		w;	/* width */
    int		h;	/* height */
    int		offset;	/* index */
} hlmdl_tex_t;

/*
 -----------------------------------------------------------------------------------------------------------------------
    body part index
 -----------------------------------------------------------------------------------------------------------------------
 */
typedef struct
{
    char	name[64];
    int		nummodels;
    int		base;
    int		modelindex;
} hlmdl_bodypart_t;

/*
 -----------------------------------------------------------------------------------------------------------------------
    meshes
 -----------------------------------------------------------------------------------------------------------------------
 */
typedef struct
{
    int numtris;
    int index;
    int skinindex;
    int unknown2;
    int unknown3;
} hlmdl_mesh_t;

/*
 -----------------------------------------------------------------------------------------------------------------------
    bones
 -----------------------------------------------------------------------------------------------------------------------
 */
typedef struct
{
    char	name[32];
    int		parent;
    int		unknown1;
    int		bonecontroller[6];
    float	value[6];
    float	scale[6];
} hlmdl_bone_t;

/*
 -----------------------------------------------------------------------------------------------------------------------
    bone controllers
 -----------------------------------------------------------------------------------------------------------------------
 */
typedef struct
{
    int		name;
    int		type;
    float	start;
    float	end;
    int		unknown1;
    int		index;
} hlmdl_bonecontroller_t;

/*
 -----------------------------------------------------------------------------------------------------------------------
    halflife model descriptor
 -----------------------------------------------------------------------------------------------------------------------
 */
typedef struct
{
    char	name[64];
    int		unknown1;
    float	unknown2;
    int		nummesh;
    int		meshindex;
    int		numverts;
    int		vertinfoindex;
    int		vertindex;
    int		unknown3[5];
} hlmdl_model_t;

/*
 -----------------------------------------------------------------------------------------------------------------------
    animation
 -----------------------------------------------------------------------------------------------------------------------
 */
typedef struct
{
    unsigned short	offset[6];
} hlmdl_anim_t;

/*
 -----------------------------------------------------------------------------------------------------------------------
    animation frames
 -----------------------------------------------------------------------------------------------------------------------
 */
typedef union
{
    struct {
        qbyte	valid;
        qbyte	total;
    } num;
    short	value;
} hlmdl_animvalue_t;

/*
 -----------------------------------------------------------------------------------------------------------------------
    sequence descriptions
 -----------------------------------------------------------------------------------------------------------------------
 */
typedef struct
{
    char	name[32];
    float	timing;
	int		loop;
    int		unknown1[4];
    int		numframes;
    int		unknown2[2];
    int		motiontype;
    int		motionbone;
    vec3_t	unknown3;
    int		unknown4[2];
    vec3_t	bbox[2];
    int		hasblendseq;
    int		index;
    int		unknown7[2];
    float	unknown[4];
    int		unknown8;
    int		seqindex;
    int		unknown9[4];
} hlmdl_sequencelist_t;

/*
 -----------------------------------------------------------------------------------------------------------------------
    sequence groups
 -----------------------------------------------------------------------------------------------------------------------
 */
typedef struct
{
    char			name[96];	/* should be split label[32] and name[64] */
    void *		cache;
    int				data;
} hlmdl_sequencedata_t;

/*
 -----------------------------------------------------------------------------------------------------------------------
    halflife model internal structure
 -----------------------------------------------------------------------------------------------------------------------
 */
typedef struct
{
    float	controller[5];				/* Position of bone controllers */
    float	adjust[5];

    /* Static pointers */
    hlmdl_header_t			*header;
	hlmdl_header_t			*texheader;
    hlmdl_tex_t				*textures;
    hlmdl_bone_t			*bones;
    hlmdl_bonecontroller_t		*bonectls;
	texid_t					*texnums;
} hlmodel_t;

typedef struct	//this is stored as the cache. an hlmodel_t is generated when drawing
{
    int header;
	int texheader;
    int textures;
    int bones;
    int bonectls;
	int texnums;
} hlmodelcache_t;

/* HL mathlib prototypes: */
void	QuaternionGLAngle(const vec3_t angles, vec4_t quaternion);
void	QuaternionGLMatrix(float x, float y, float z, float w, vec4_t *GLM);
//void	UploadTexture(hlmdl_tex_t *ptexture, qbyte *data, qbyte *pal);

/* HL drawing */
//qboolean Mod_LoadHLModel (model_t *mod, void *buffer);


/* physics stuff */
//void *Mod_GetHalfLifeModelData(model_t *mod);
