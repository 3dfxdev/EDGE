/*
 *  ymemory.h
 *  Memory allocation functions.
 *  AYM 1999-03-24
 */


void *GetMemory (unsigned long size);
void *ResizeMemory (void *, unsigned long size);
void  FreeMemory (void *);


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
