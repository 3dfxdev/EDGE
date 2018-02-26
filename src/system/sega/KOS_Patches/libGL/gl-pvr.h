/* KallistiGL for KallistiOS ##version##

   libgl/gl-pvr.h
   Copyright (C) 2013-2014 Josh Pearson

   Vertex Buffer Routines for interfacing the Dreamcast's SH4 CPU and PowerVR GPU.
*/

#ifndef GL_PVR_H
#define GL_PVR_H

/* GL->PVR Commands */
typedef struct {
    unsigned int cmd[8];
} pvr_cmd_t; /* Generic 32byte command for the pvr */

typedef struct {
    unsigned int flags;      /* Constant PVR_CMD_USERCLIP */
    unsigned int d1, d2, d3; /* Ignored for this type */
    unsigned int sx,         /* Start x */
             sy,         /* Start y */
             ex,         /* End x */
             ey;         /* End y */
} pvr_cmd_tclip_t; /* Tile Clip command for the pvr */

#define GL_PVR_VERTEX_BUF_SIZE 2560 * 256 /* PVR Vertex buffer size */
#define GL_KOS_MAX_VERTS       1024*16    /* SH4 Vertex Count */
//CA: We modify GL_KOS_MAX_VERTS count since SLaVE doesn't need it any higher (saves memory)

#define GL_KOS_LIST_OP 0
#define GL_KOS_LIST_TR 1

#define GL_KOS_USE_MALLOC 1 /* Use Dynamic Vertex Array */
//#define GL_KOS_USE_DMA    1 /* Use PVR DMA for vertex data transfer instead of store queues */
//#define GL_USE_FLOAT  0 /* Use PVR's floating-point color Vertex Type (64bit) *NoOp* */

/* Misc SH4->PVR Commands */
#define TA_SQ_ADDR (unsigned int *)(void *) \
    (0xe0000000 | (((unsigned long)0x10000000) & 0x03ffffe0))

#define QACRTA ((((unsigned int)0x10000000)>>26)<<2)&0x1c

#define PVR_TA_TXR_FILTER_SHIFT     14

#define GL_PVR_FILTER_POINT       0x00
#define GL_PVR_FILTER_BILINEAR    0x01
#define GL_PVR_FILTER_TRILINEAR_A 0x10
#define GL_PVR_FILTER_TRILINEAR_B 0x11

#define PVR_TA_SUPER_SAMPLE_SHIFT   12

#define GL_PVR_SAMPLE_POINT 0x0
#define GL_PVR_SAMPLE_SUPER 0x1

#endif
