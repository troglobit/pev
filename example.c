/* This is free and unencumbered software released into the public domain. */

#include <stdio.h>
#include <signal.h>
#include "pev.h"

#define TIMEOUT  500000		/* 0.5 sec */
#define PERIOD  1000000		/* 1.0 sec */

int id;

static void cb(int timeout, void *arg)
{
	printf("Hej %d\n", timeout);
	pev_timer_set(id, timeout + TIMEOUT);
}

static void periodic(int timeout, void *arg)
{
	puts("+++++++++");
}

static void br(int signo, void *arg)
{
	pev_exit(10);
}

int main(void)
{
	pev_init();
	pev_sig_add(SIGINT, br, NULL);

	pev_timer_add(0, PERIOD, periodic, NULL);
	id = pev_timer_add(TIMEOUT, 0, cb, NULL);

	return pev_run();
}

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
