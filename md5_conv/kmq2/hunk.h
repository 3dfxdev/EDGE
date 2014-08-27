#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *Hunk_Alloc_Aligned (int size, int alignment);

void *Hunk_Begin (int maxsize);
void *Hunk_Alloc (int size);
int Hunk_End (void);
void Hunk_Free (void *base);

