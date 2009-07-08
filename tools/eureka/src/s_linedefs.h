/*
 *  s_linedefs.h
 *  AYM 1998-11-22
 */


class bitvec_c;


bitvec_c *linedefs_of_sector (obj_no_t s);
bitvec_c *linedefs_of_sectors (SelPtr list);
int linedefs_of_sector (obj_no_t s, obj_no_t *&array);


