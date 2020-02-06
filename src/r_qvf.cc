#include "r_qvf.h"

RQVertexFormat RQVertex3f::format[] {
    { VFATTR_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(RQVertex3f), offsetof(RQVertex3f, x) },
    { VFATTR_FINAL }
};

RQVertexFormat RQVertex3fTextured::format[] {
    {VFATTR_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(RQVertex3fTextured), offsetof(RQVertex3fTextured, x)},
    {VFATTR_TEXCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(RQVertex3fTextured), offsetof(RQVertex3fTextured, u)},
    {VFATTR_FINAL}
};


RQVertexFormat RQVertex3fSprite::format[] {
    {VFATTR_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(RQVertex3fSprite), offsetof(RQVertex3fSprite, x)},
    {VFATTR_TEXCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(RQVertex3fSprite), offsetof(RQVertex3fSprite, u0)},
    {VFATTR_TEXCOORD2, 2, GL_FLOAT, GL_FALSE, sizeof(RQVertex3fSprite), offsetof(RQVertex3fSprite, u1)},
    {VFATTR_COLOR, 4, GL_FLOAT, GL_FALSE, sizeof(RQVertex3fSprite), offsetof(RQVertex3fSprite, r)},
    {VFATTR_NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(RQVertex3fSprite), offsetof(RQVertex3fSprite, nx)},
    {3, 3, GL_FLOAT, GL_FALSE, sizeof(RQVertex3fSprite), offsetof(RQVertex3fSprite, tx)},
    {VFATTR_FINAL}
};
