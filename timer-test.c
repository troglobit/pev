/* This is free and unencumbered software released into the public domain. */

#include <assert.h>
#include <limits.h>
#include "pev.h"

#define LONG_DELAY INT64_C(6000000000)	/* 100 min, > INT_MAX usec */

static int timer, periodic;

static void unexpected_cb(int id, void *arg)
{
	assert(0);
}

static void finish_cb(int id, void *arg)
{
	assert(pev_timer_get(id) == 1000);
	assert(pev_timer_get(timer) == LONG_DELAY);
	assert(pev_timer_get(periodic) == LONG_DELAY);
	assert(pev_timer_del(timer) == 0);
	assert(pev_timer_del(periodic) == 0);
	pev_exit(0);
}

int main(void)
{
	assert(pev_init() >= 0);

	timer = pev_timer_add(LONG_DELAY, 0, unexpected_cb, NULL);
	assert(timer >= 0);
	assert(pev_timer_get(timer) == LONG_DELAY);
	assert(pev_timer_set(timer, (int64_t)INT_MAX + 1) == 0);
	assert(pev_timer_get(timer) == (int64_t)INT_MAX + 1);
	assert(pev_timer_set(timer, LONG_DELAY) == 0);

	periodic = pev_timer_add(0, LONG_DELAY, unexpected_cb, NULL);
	assert(periodic >= 0);
	assert(pev_timer_get(periodic) == LONG_DELAY);

	assert(pev_timer_add(1000, 0, finish_cb, NULL) >= 0);
	assert(pev_run() == 0);

	return 0;
}
