#include <kos.h>
#include <assert.h>
//#include "common.h"
#include "profile.h"

static void * watchdog(void *v);


static volatile int sleeptime = 0;
static volatile int paused = 0;
static kthread_t * dogthd = NULL;
static kthread_t * parentthd = NULL;


void watchdog_init()
{
	assert(dogthd == NULL);
	parentthd = thd_current;
	dogthd = thd_create(0, &watchdog, NULL);
}

void watchdog_pet()
{
	sleeptime = 0;
}

int watchdog_running()
{
	return !paused;
}

void watchdog_pause()
{
	paused++;
	//DBOS("DOG PAWS: %i\n",paused);
}

void watchdog_resume()
{
	paused--;
	if (pause < 0)
		paused = 0;
	//DBOS("DOG UNPAWS: %i\n",paused);
}

void watchdog_stop()
{
	//TODO
	assert(0);
	
	dogthd = NULL;
	parentthd = NULL;
}

static void * watchdog(void *v)
{
	//SETGBR(v);
	do {
		thd_sleep(51);
		if (profile_running())
			profile_record_pc();
		
		if (!paused) {
			sleeptime++;
			if (sleeptime > 100) {
				printf("At PC: %08X, PR: %08X\n",(unsigned int)parentthd->context.pc,(unsigned int)parentthd->context.pr);
				//DBOS("Watchdog timeout\n");
				//sys_menu();
				//arch_reboot();
				panic("Watchdog timeout\n");
			}
		}
	} while (1);
}

