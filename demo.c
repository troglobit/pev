/* This is free and unencumbered software released into the public domain. */

#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include "pev.h"

#define TIMEOUT 2000000		/* 2 sec */

static void cb(int period, void *arg)
{
	struct timeval *start = (struct timeval *)arg;
        struct timeval now;

	(void)period;
        gettimeofday(&now, NULL);
	if (now.tv_sec < start->tv_sec + (TIMEOUT / 1000000))
		puts("wut?");
	else
		puts("Hej");
}

static void wow(int period, void *arg)
{
	putchar(' ');
}

static void nohej(int period, void *arg)
{
	static int once = 1;
	int *id = (int *)arg;

	(void)period;
	if (once) {
		pev_timer_del(*id);
		pev_timer_add(0, 50, wow, NULL);
		once = 0;
	}
	puts("Killed Hej, kill me with Ctrl-C");
}

static void greg(int period, void *arg)
{
	putchar('*');
}

static void dotty(int period, void *arg)
{
	putchar('.');
}

static void br(int signo, void *arg)
{
	(void)signo;
	(void)arg;
	printf("\ngot sig %d\n", signo);
	pev_exit(10);
}

int main(void)
{
	struct timeval start;
	int id;

	setvbuf(stdout, NULL, _IONBF, 0);
        gettimeofday(&start, NULL);

        pev_init();
	pev_sig_add(SIGINT, br, NULL);
        id = pev_timer_add(0, TIMEOUT, cb, &start);
        pev_timer_add(0, TIMEOUT * 3, nohej, &id);

	/* sub-second timers, to verify timer impl. */
        pev_timer_add(0, 100000, dotty, NULL);
        pev_timer_add(0, 500000, greg, NULL);

        return pev_run();
}

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
