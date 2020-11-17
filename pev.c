/* Copyright (C) 2017-2020  Joachim Wiberg <troglobit@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "pev.h"
#include "queue.h"

struct sock {
	LIST_ENTRY(sock) link;

	int sd;

	void (*cb)(int, void *arg);
	void *arg;
};

/*
 * TODO
 * - Timers should ideally be sorted in priority order, and/or
 * - Investigate using the pipe to notify which timer expired
 */
struct timer {
	LIST_ENTRY(timer) link;
	int             active;	/* Set to 0 to delete */

	int             period;	/* period time in seconds */
	struct timespec timeout;

	void (*cb)(void *arg);
	void *arg;
};

static timer_t timer;
static int timerfd[2];
static LIST_HEAD(, timer) tl = LIST_HEAD_INITIALIZER();

static int max_fdnum = -1;
static LIST_HEAD(, sock) fl = LIST_HEAD_INITIALIZER();

volatile sig_atomic_t running;


static int nfds(void)
{
	return max_fdnum + 1;
}

/*
 * register socket/fd/pipe created elsewhere, optional callback
 */
int pev_sock_add(int sd, void (*cb)(int, void *), void *arg)
{
	struct sock *entry;

	entry = malloc(sizeof(*entry));
	if (!entry)
		return -1;

	entry->sd  = sd;
	entry->cb  = cb;
	entry->arg = arg;
	LIST_INSERT_HEAD(&fl, entry, link);

	fcntl(sd, F_SETFD, fcntl(sd, F_GETFD) | FD_CLOEXEC);

	/* Keep track for select() */
	if (sd > max_fdnum)
		max_fdnum = sd;

	return sd;
}

/*
 * deregister socket/df/pipe created elsewhere
 */
int pev_sock_del(int sd)
{
	struct sock *entry, *tmp;

	LIST_FOREACH_SAFE(entry, &fl, link, tmp) {
		if (entry->sd == sd) {
			LIST_REMOVE(entry, link);
			free(entry);

			return 0;
		}
	}

	errno = ENOENT;
	return -1;
}

/*
 * create and register socket, with optional callback for reading inbound data
 */
int pev_sock_open(int domain, int type, int proto, void (*cb)(int, void *), void *arg)
{
	int sd;

	type |= SOCK_CLOEXEC;
	sd = socket(domain, type, proto);
	if (sd < 0)
		return -1;

	if (pev_sock_add(sd, cb, arg) < 0) {
		close(sd);
		return -1;
	}

	return sd;
}

/*
 * close and deregister socket created with pev_sock_open()
 */
int pev_sock_close(int sd)
{
	pev_sock_del(sd);
	close(sd);

	return 0;
}

static int socket_poll(struct timeval *timeout)
{
	int num;
	fd_set fds;
	struct sock *entry;

	FD_ZERO(&fds);
	LIST_FOREACH(entry, &fl, link)
		FD_SET(entry->sd, &fds);

	errno = 0;
	num = select(nfds(), &fds, NULL, NULL, timeout);
	if (num <= 0)
		return -1;

	LIST_FOREACH(entry, &fl, link) {
		if (!FD_ISSET(entry->sd, &fds))
			continue;

		if (entry->cb)
			entry->cb(entry->sd, entry->arg);
	}

	return num;
}

static void set(struct timer *t, struct timespec *now)
{
	t->timeout.tv_sec  = now->tv_sec + t->period;
	t->timeout.tv_nsec = now->tv_nsec;
}

static int expired(struct timer *t, struct timespec *now)
{
	long round_nsec = now->tv_nsec + 250000000;
	round_nsec = round_nsec > 999999999 ? 999999999 : round_nsec;

	if (t->timeout.tv_sec < now->tv_sec)
		return 1;

	if (t->timeout.tv_sec == now->tv_sec && t->timeout.tv_nsec <= round_nsec)
		return 1;

	return 0;
}

static struct timer *compare(struct timer *a, struct timer *b)
{
	if (a->timeout.tv_sec <= b->timeout.tv_sec) {
		if (a->timeout.tv_nsec <= b->timeout.tv_nsec)
			return a;

		return b;
	}

	return b;
}

static struct timer *find(void (*cb), void *arg)
{
	struct timer *entry;

	LIST_FOREACH(entry, &tl, link) {
		if (entry->cb != cb || entry->arg != arg)
			continue;

		return entry;
	}

	return NULL;
}


static int start(struct timespec *now)
{
	struct timer *next, *entry;
	struct itimerspec it;

	if (LIST_EMPTY(&tl))
		return -1;

	next = LIST_FIRST(&tl);
	LIST_FOREACH(entry, &tl, link)
		next = compare(next, entry);

	memset(&it, 0, sizeof(it));
	it.it_value.tv_sec  = next->timeout.tv_sec - now->tv_sec;
	it.it_value.tv_nsec = next->timeout.tv_nsec - now->tv_nsec;
	if (it.it_value.tv_nsec < 0) {
		it.it_value.tv_sec -= 1;
		it.it_value.tv_nsec = 1000000000 + it.it_value.tv_nsec;
	}
	if (it.it_value.tv_sec < 0)
		it.it_value.tv_sec = 0;

	return timer_settime(timer, 0, &it, NULL);
}

static int stop(void)
{
	struct itimerspec it = { 0 };

	return timer_settime(timer, 0, &it, NULL);
}

/* callback for activity on pipe */
static void run(int sd, void *arg)
{
	char dummy;
	struct timespec now;
	struct timer *entry, *tmp;

	(void)arg;
	if (read(sd, &dummy, 1) < 0)
		return;

	clock_gettime(CLOCK_MONOTONIC, &now);
	LIST_FOREACH_SAFE(entry, &tl, link, tmp) {
		if (expired(entry, &now)) {
			if (entry->cb)
				entry->cb(entry->arg);
			set(entry, &now);
		}

		if (!entry->active) {
			LIST_REMOVE(entry, link);
			free(entry);
		}
	}

	start(&now);
}

/* write to pipe to create an event for select() on SIGALRM */
static void handler(int signo)
{
	(void)signo;
	if (write(timerfd[1], "!", 1) < 0)
		return;
}

/*
 * register signal pipe and callbacks
 */
static int timer_init(void)
{
	struct sigaction sa;

	if (pipe(timerfd))
		return -1;

	if (pev_sock_add(timerfd[0], run, NULL) < 0)
		return -1;
	if (pev_sock_add(timerfd[1], NULL, NULL) < 0)
		return -1;

	sa.sa_handler = handler;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGALRM, &sa, NULL);

	if (timer_create(CLOCK_MONOTONIC, NULL, &timer)) {
		pev_sock_close(timerfd[0]);
		pev_sock_close(timerfd[1]);
		return -1;
	}

	return 0;
}

static int timer_exit(void)
{
	stop();
	timer_delete(timer);
	pev_sock_close(timerfd[0]);
	pev_sock_close(timerfd[1]);

	return 0;
}

/*
 * create periodic timer (seconds)
 */
int pev_timer_add(int period, void (*cb)(void *), void *arg)
{
	struct timer *t;
	struct timespec now;

	t = find(cb, arg);
	if (t && t->active) {
		errno = EEXIST;
		return -1;
	}

	if (clock_gettime(CLOCK_MONOTONIC, &now) < 0)
		return -1;

	t = malloc(sizeof(*t));
	if (!t)
		return -1;

	t->active = 1;
	t->period = period;
	t->cb     = cb;
	t->arg    = arg;

	set(t, &now);

	LIST_INSERT_HEAD(&tl, t, link);

	return start(&now);
}

/*
 * delete a timer
 */
int pev_timer_del(void (*cb)(void *), void *arg)
{
	struct timer *entry;

	entry = find(cb, arg);
	if (!entry)
		return 1;

	/* Mark for deletion and issue a new run */
	entry->active = 0;
	handler(0);

	return 0;
}

int pev_init(void)
{
	running = 1;

	return timer_init();
}

int pev_exit(void)
{
	running = 0;

	return timer_exit();
}

int pev_run(void)
{
	while (running) {
		if (socket_poll(NULL) < 0) {
			if (errno == EINTR)
				continue;
		}
	}

	return 0;
}

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
