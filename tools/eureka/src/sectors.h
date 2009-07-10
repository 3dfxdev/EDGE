/*
 *  s_centre.h
 *  AYM 1998-11-22
 */


#include "objid.h"
#include "selectn.h"


void centre_of_sector (obj_no_t s, int *x, int *y);
void centre_of_sectors (SelPtr list, int *x, int *y);


class bitvec_c;

bitvec_c *linedefs_of_sector (obj_no_t s);
bitvec_c *linedefs_of_sectors (SelPtr list);
int linedefs_of_sector (obj_no_t s, obj_no_t *&array);

bitvec_c *bv_vertices_of_sector (obj_no_t s);
bitvec_c *bv_vertices_of_sectors (SelPtr list);
SelPtr list_vertices_of_sectors (SelPtr list);

void swap_flats (SelPtr list);

