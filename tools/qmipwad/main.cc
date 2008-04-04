
#include "headers.h"

#include <time.h>


void FatalError(const char *message, ...)
{
	fprintf(stderr, "ERROR: ");

	va_list argptr;

	va_start(argptr, message);
	vfprintf(stderr, message, argptr);
	va_end(argptr);

	exit(9);
}


int main(int argc, char **argv)
{
	// TODO

	return 0;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
