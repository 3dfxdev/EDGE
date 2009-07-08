/*
 *  objects.h
 *  AYM 2000-11-06
 */


#ifndef YH_OBJECTS  /* DO NOT INSERT ANYTHING BEFORE THIS LINE */
#define YH_OBJECTS


#include "objid.h"


void  HighlightSelection (int, SelPtr);
obj_no_t GetMaxObjectNum (int);
void  GetCurObject (Objid& o, int objtype, int x, int y);
void  SelectObjectsInBox (SelPtr *list, int, int, int, int, int);
void  HighlightObject (int, int, int);
void  DeleteObject (const Objid&);
void  DeleteObjects (int, SelPtr *);
void  InsertObject (obj_type_t, obj_no_t, int, int);
bool  IsLineDefInside (int, int, int, int, int);
int GetOppositeSector (int, bool);
void  CopyObjects (int, SelPtr);
bool  MoveObjectsToCoords (int, SelPtr, int, int, int);
void  GetObjectCoords (int, int, int *, int *);
int FindFreeTag (void);


#endif  /* DO NOT ADD ANYTHING AFTER THIS LINE */
