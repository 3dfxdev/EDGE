#ifndef _WATCHDOGGUARD
#define _WATCHDOGGUARD

#ifdef __CPLUSPLUS
extern "C" {
#endif

void watchdog_init();
void watchdog_pet();
int watchdog_running();
void watchdog_pause();
void watchdog_resume();
void watchdog_stop();


#ifdef __CPLUSPLUS
}
#endif


#endif

