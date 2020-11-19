#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include "pev.h"

#define TIMEOUT 2               /* 2 sec */

static void cb(int period, void *arg)
{
	struct timeval *start = (struct timeval *)arg;
        struct timeval now;

	(void)period;
        gettimeofday(&now, NULL);
	if (now.tv_sec < start->tv_sec + TIMEOUT)
		fprintf(stderr, "wut?");
	else
		printf("Hej\n");
}

static void br(int signo, void *arg)
{
	(void)signo;
	(void)arg;
	printf("\ngot sig %d\n", signo);
	pev_exit();
}

int main(void)
{
	struct timeval start;

        gettimeofday(&start, NULL);

        pev_init();
	pev_sig_add(SIGINT, br, NULL);
        pev_timer_add(TIMEOUT, cb, &start);

        return pev_run();
}

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
