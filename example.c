/* This is free and unencumbered software released into the public domain. */

#include <stdio.h>
#include <signal.h>
#include "pev.h"

#define TIMEOUT  500000		/* 0.5 sec */
#define PERIOD  1000000		/* 1.0 sec */

/*
 * For each call, the timer is incremented with TIMEOUT
 */
static void cb(int id, void *arg)
{
	int timeout = pev_timer_get(id);

	printf("Hej %d\n", timeout);
	pev_timer_set(id, timeout + TIMEOUT);
}

/*
 * Every one second
 */
static void periodic(int id, void *arg)
{
	puts("+++++++++");
}

static void br(int signo, void *arg)
{
	pev_exit(10);
}

int main(void)
{
	int id1, id2, rc;

	pev_init();
	pev_sig_add(SIGINT, br, NULL);

	id1 = pev_timer_add(0, PERIOD, periodic, NULL);
	if (id1 < 0)
		return 1;

	id2 = pev_timer_add(TIMEOUT, 0, cb, NULL);
	if (id2 < 0)
		return 1;

	rc = pev_run();
	pev_timer_del(id1);
	pev_timer_del(id2);

	return rc;
}

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
