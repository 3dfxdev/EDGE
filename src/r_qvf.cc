#include "r_qvf.h"

RQVertexFormat RQVertex3f::format[] {
    { VFATTR_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(RQVertex3f), offsetof(RQVertex3f, x) },
    { VFATTR_FINAL }
};

RQVertexFormat RQVertex3fTextured::format[]{
    {VFATTR_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(RQVertex3fTextured), offsetof(RQVertex3fTextured, x)},
    {VFATTR_TEXCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(RQVertex3fTextured), offsetof(RQVertex3fTextured, u)},
    {VFATTR_FINAL}};
