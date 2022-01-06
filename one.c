/* This is free and unencumbered software released into the public domain. */

#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include "pev.h"

#define TIMEOUT 1000000		/* 1 sec */

static void cb(int timeout, void *arg)
{
	printf("Hej %d\n", timeout);
	pev_timer_add(timeout + TIMEOUT, 0, cb, NULL);
}

static void br(int signo, void *arg)
{
	(void)signo;
	(void)arg;
	pev_exit(10);
}

int main(void)
{
	struct timeval start;

	setvbuf(stdout, NULL, _IONBF, 0);
        gettimeofday(&start, NULL);

        pev_init();
	pev_sig_add(SIGINT, br, NULL);
        pev_timer_add(TIMEOUT, 0, cb, NULL);

        return pev_run();
}

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
