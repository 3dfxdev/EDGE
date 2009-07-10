/*
 *  l_centre.h
 *  AYM 1998-11-22
 */

class bitvec_c;
#include "selectn.h"

bitvec_c *bv_vertices_of_linedefs (bitvec_c *linedefs);
bitvec_c *bv_vertices_of_linedefs (SelPtr list);
SelPtr list_vertices_of_linedefs (SelPtr list);

void centre_of_linedefs (SelPtr list, int *x, int *y);

void frob_linedefs_flags (SelPtr list, int op, int operand);

