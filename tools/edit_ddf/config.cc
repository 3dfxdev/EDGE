//------------------------------------------------------------------------
//  CONFIG: reading config data
//------------------------------------------------------------------------
//
//  Editor for DDF   (C) 2005  The EDGE Team
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//------------------------------------------------------------------------

#include "defs.h"
 
keyword_set_c::keyword_set_c() : head(NULL)
{ }

keyword_set_c::~keyword_set_c()
{
	while (head)
	{
		nd_c *cur = head;
		head = cur->next;

		delete cur;
	}
}

keyword_set_c::nd_c::nd_c(const char *K) : next(NULL)
{
	keyword = strdup(K);
}

keyword_set_c::nd_c::~nd_c()
{
	free(keyword);
}


//
// Path Constructor
//
defbox_container_c::defbox_container_c() : point_num(0), points(NULL)
{
}

//
// Path Destructor
//
defbox_container_c::~defbox_container_c()
{
	if (points)
		delete[] points;
}

//
// Config Reading
//
void defbox_container_c::ReadFile(const char *filename)
{
	FILE *fp = fopen(filename, "r");

	if (! fp)
	{
		PrintWarn("Unable to open path file: %s\n", strerror(errno));
		return false;
	}

	// clear previous stuff

	defbox_container_c *P = new defbox_container_c();

	P->points = new int[MAX_PTS * 2];

	for (P->point_num = 0; P->point_num < MAX_PTS; P->point_num++)
	{
		int *cur_pt = P->points + (P->point_num * 2);

		if (fscanf(fp, " %d %d ", cur_pt, cur_pt+1) != 2)
			break;
	}

	fclose(fp);

	return P;
}

