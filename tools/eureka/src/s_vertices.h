/*
 *  s_vertices.h
 *  AYM 1998-11-22
 */


class bitvec_c;
#include "selectn.h"


bitvec_c *bv_vertices_of_sector (obj_no_t s);
bitvec_c *bv_vertices_of_sectors (SelPtr list);
SelPtr list_vertices_of_sectors (SelPtr list);

