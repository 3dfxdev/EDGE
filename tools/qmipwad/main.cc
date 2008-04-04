
#include "headers.h"

#include <time.h>



void MainShutdown(void);


static char msg_buffer[2000];


void GlobalMessage(const char *source, const char *func,
		               const char *message, ...)
{
	/* try to prevent re-entrancy problems */
	static volatile bool already_active = false;
	if (already_active) return;
	already_active = true;

	va_list argptr;

	va_start(argptr, message);
	vsnprintf(msg_buffer, sizeof(msg_buffer), message, argptr);
	va_end(argptr);

	msg_buffer[sizeof(msg_buffer)-1] = 0;

  // !!!! TEMPORARY
  printf("[%s%s] %s\n", source, func, msg_buffer);

	already_active = false;
}


void GlobalError(const char *message, ...)
{
//raise(11); //!!!!!

	char buffer[1024+4];

	va_list argptr;

	va_start(argptr, message);
	vsnprintf(buffer, 1024, message, argptr);
	va_end(argptr);

	buffer[1023] = 0;

	GlobalMessage("0ER", "", "%s", buffer);

	// Possibilities:
	//
	// 1. Custom error screen, then exit
	// 2. DLG_ShowError(), then exit
	// 3. dump core

	MainShutdown();

	DLG_ShowError(buffer);

	exit(9);
}





int main(int argc, char **argv)
{

	return 0;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
