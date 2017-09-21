/* KallistiOS ##version##

   sem.c
   Copyright (C) 2001, 2002, 2003 Dan Potter
   Copyright (C) 2012 Lawrence Sebald
*/

/* Defines semaphores */

/**************************************/

#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>

#include <kos/thread.h>
#include <kos/limits.h>
#include <kos/sem.h>
#include <kos/genwait.h>

/**************************************/

/* Allocate a new semaphore; the semaphore will be assigned
   to the calling process and when that process dies, the semaphore
   will also die. */
semaphore_t *sem_create(int value) {
    semaphore_t *sm;

    dbglog(DBG_WARNING, "Creating semaphore with deprecated sem_create(). "
           "Please update your code!\n");

    if(value < 0) {
        errno = EINVAL;
        return NULL;
    }

    /* Create a semaphore structure */
    if(!(sm = (semaphore_t*)malloc(sizeof(semaphore_t)))) {
        errno = ENOMEM;
        return NULL;
    }

    sm->count = value;
    sm->initialized = 2;

    return sm;
}

int sem_init(semaphore_t *sm, int count) {
    if(sm->count < 0) {
        errno = EINVAL;
        return -1;
    }

    sm->count = count;
    sm->initialized = 1;
    return 0;
}

/* Take care of destroying a semaphore */
int sem_destroy(semaphore_t *sm) {
    /* Wake up any queued threads with an error */
    genwait_wake_all_err(sm, ENOTRECOVERABLE);

    if(sm->initialized == 2) {
        /* Free the memory */
        free(sm);
    }
    else {
        sm->count = 0;
        sm->initialized = 0;
    }

    return 0;
}

/* Wait on a semaphore, with timeout (in milliseconds) */
int sem_wait_timed(semaphore_t *sem, int timeout) {
    int old, rv = 0;

    /* Make sure we're not inside an interrupt */
    if(irq_inside_int()) {
        dbglog(DBG_WARNING, "sem_wait_timed: called inside interrupt\n");
        errno = EPERM;
        return -1;
    }

    /* Check for smarty clients */
    if(timeout < 0) {
        errno = EINVAL;
        return -1;
    }

    /* Disable interrupts */
    old = irq_disable();

    if(sem->initialized != 1 && sem->initialized != 2) {
        errno = EINVAL;
        rv = -1;
    }
    /* If there's enough count left, then let the thread proceed */
    else if(sem->count > 0) {
        sem->count--;
    }
    else {
        /* Block us until we're signaled */
        sem->count--;
        rv = genwait_wait(sem, timeout ? "sem_wait_timed" : "sem_wait", timeout,
                          NULL);

        /* Did we fail to get the lock? */
        if(rv < 0) {
            rv = -1;
            ++sem->count;

            if(errno == EAGAIN)
                errno = ETIMEDOUT;
        }
    }

    irq_restore(old);

    return rv;
}

int sem_wait(semaphore_t *sm) {
    return sem_wait_timed(sm, 0);
}

/* Attempt to wait on a semaphore. If the semaphore would block,
   then return an error instead of actually blocking. */
int sem_trywait(semaphore_t *sm) {
    int old, rv = 0;

    old = irq_disable();

    if(sm->initialized != 1 && sm->initialized != 2) {
        errno = EINVAL;
        rv = -1;
    }
    /* Is there enough count left? */
    else if(sm->count > 0) {
        sm->count--;
    }
    else {
        rv = -1;
        errno = EWOULDBLOCK;
    }

    /* Restore interrupts */
    irq_restore(old);

    return rv;
}

/* Signal a semaphore */
int sem_signal(semaphore_t *sm) {
    int old, woken, rv = 0;

    old = irq_disable();

    if(sm->initialized != 1 && sm->initialized != 2) {
        errno = EINVAL;
        rv = -1;
    }
    /* Is there anyone waiting? If so, pass off to them */
    else if(sm->count < 0) {
        woken = genwait_wake_cnt(sm, 1, 0);
        //assert(woken == 1);
		if (woken != 1)
			//CA: Code to trap assertion failure (for debugging)
			__asm__(" TRAP #0xff"); 
        sm->count++;
    }
    else {
        /* No one is waiting, so just add another tick */
        sm->count++;
    }

    irq_restore(old);

    return rv;
}

/* Return the semaphore count */
int sem_count(semaphore_t *sm) {
    /* Look for the semaphore */
    return sm->count;
}
