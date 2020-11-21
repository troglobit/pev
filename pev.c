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

#define PEV_SOCK   1
#define PEV_TIMER  2
#define PEV_SIG    3

struct pev {
	LIST_ENTRY(pev) link;
	int id;
	char type;
	char active;

	union {
		int sd;
		int signo;
		struct {
			int period;
			struct timespec timeout;
		};
	};

	void (*cb)(int period, void *arg);
	void *arg;
};

static LIST_HEAD(, pev) pl = LIST_HEAD_INITIALIZER();

static int events[2];
static int max_fdnum = -1;
static int id = 1;
static int running;

static struct pev *pev_new  (int type, void (*cb)(int, void *), void *arg);
static struct pev *pev_find (int type, int signo);

/******************************* SIGNALS ******************************/

static int sig_init(void)
{
	sigset_t mask;

	sigfillset(&mask);
	sigprocmask(SIG_BLOCK, &mask, NULL);

	return 0;
}

static int sig_exit(void)
{
	sigset_t mask;

	sigfillset(&mask);
	sigprocmask(SIG_UNBLOCK, &mask, NULL);

	return 0;
}

static void sig_handler(int signo)
{
	char buf[1] = { (char)signo };

	while (write(events[1], buf, 1) < 0) {
		if (errno != EINTR)
			return;
	}
}

static int sig_run(void)
{
	struct pev *entry;
	sigset_t mask;

	sigfillset(&mask);
	LIST_FOREACH(entry, &pl, link) {
		if (entry->type != PEV_SIG)
			continue;

		sigdelset(&mask, entry->signo);
	}

	return sigprocmask(SIG_SETMASK, &mask, NULL);
}

int pev_sig_add(int signo, void (*cb)(int, void *), void *arg)
{
	struct sigaction sa;
	struct pev *entry;

	if (pev_find(PEV_SIG, signo)) {
		errno = EEXIST;
		return -1;
	}

	entry = pev_new(PEV_SIG, cb, arg);
	if (!entry)
		return -1;

	sa.sa_handler = sig_handler;
	sa.sa_flags = 0;
	sigfillset(&sa.sa_mask);
	sigaction(signo, &sa, NULL);

	entry->signo = signo;

	return entry->id;
}

int pev_sig_del(int id)
{
	struct pev *entry;

	entry = pev_find(PEV_SIG, id);
	if (!entry)
		return -1;

	sigaction(entry->signo, NULL, NULL);

	return pev_sock_del(id);
}

/******************************* SOCKETS ******************************/

static int nfds(void)
{
	return max_fdnum + 1;
}

static void sock_run(fd_set *fds)
{
	struct pev *entry;

	FD_ZERO(fds);
	LIST_FOREACH(entry, &pl, link) {
		if (entry->type != PEV_SOCK)
			continue;

		FD_SET(entry->sd, fds);
	}
}

int pev_sock_add(int sd, void (*cb)(int, void *), void *arg)
{
	struct pev *entry;

	entry = pev_new(PEV_SOCK, cb, arg);
	if (!entry)
		return -1;

	entry->sd = sd;
	fcntl(sd, F_SETFD, fcntl(sd, F_GETFD) | O_CLOEXEC | O_NONBLOCK);

	/* Keep track for select() */
	if (sd > max_fdnum)
		max_fdnum = sd;

	return entry->id;
}

int pev_sock_del(int id)
{
	struct pev *entry, *tmp;

	LIST_FOREACH_SAFE(entry, &pl, link, tmp) {
		if (entry->id == id) {
			/* Mark for deletion and issue a new run */
			entry->active = 0;
			sig_handler(0);

			return 0;
		}
	}

	errno = ENOENT;
	return -1;
}

int pev_sock_open(int domain, int type, int proto, void (*cb)(int, void *), void *arg)
{
	int sd;

	sd = socket(domain, type, proto);
	if (sd < 0)
		return -1;

	if (pev_sock_add(sd, cb, arg) < 0) {
		close(sd);
		return -1;
	}

	return sd;
}

int pev_sock_close(int sd)
{
	struct pev *entry;

	entry = pev_find(PEV_SOCK, sd);
	if (!entry)
		return -1;

	pev_sock_del(entry->id);
	close(sd);

	return 0;
}

/******************************* TIMERS *******************************/

static struct pev *timer_ffs(void)
{
	struct pev *entry;

	LIST_FOREACH(entry, &pl, link) {
		if (entry->type == PEV_TIMER && entry->active)
			return entry;
	}

	return NULL;
}

static struct pev *timer_compare(struct pev *a, struct pev *b)
{
	if (b->type != PEV_TIMER || !b->active)
		return a;

	if (a->timeout.tv_sec <= b->timeout.tv_sec) {
		if (a->timeout.tv_nsec <= b->timeout.tv_nsec)
			return a;

		return b;
	}

	return b;
}

static int timer_start(struct timespec *now)
{
	struct pev *next, *entry;
	struct itimerval it;

	next = timer_ffs();
	if (!next)
		return -1;

	LIST_FOREACH(entry, &pl, link)
		next = timer_compare(next, entry);

	memset(&it, 0, sizeof(it));
	it.it_value.tv_sec = next->timeout.tv_sec - now->tv_sec;
	it.it_value.tv_usec = (next->timeout.tv_nsec - now->tv_nsec) / 1000;
	if (it.it_value.tv_usec < 0) {
		it.it_value.tv_sec -= 1;
		it.it_value.tv_usec = 1000000 + it.it_value.tv_usec;
	}
	if (it.it_value.tv_sec < 0)
		it.it_value.tv_sec = 0;

	return setitimer(ITIMER_REAL, &it, NULL);
}

static int timer_expired(struct pev *t, struct timespec *now)
{
	long rounded;

	if (t->type != PEV_TIMER)
		return 0;

	rounded = now->tv_nsec + 250000000;
	rounded = rounded > 999999999 ? 999999999 : rounded;

	if (t->timeout.tv_sec < now->tv_sec)
		return 1;

	if (t->timeout.tv_sec == now->tv_sec && t->timeout.tv_nsec <= rounded)
		return 1;

	return 0;
}

static void timer_run(int signo, void *arg)
{
	struct timespec now;
	struct pev *entry;

	(void)arg;
	clock_gettime(CLOCK_MONOTONIC, &now);

	LIST_FOREACH(entry, &pl, link) {
		if (!timer_expired(entry, &now))
			continue;

		entry->timeout.tv_sec  = now.tv_sec + entry->period;
		entry->timeout.tv_nsec = now.tv_nsec;

		if (signo && entry->cb)
			entry->cb(entry->period, entry->arg);
	}

	timer_start(&now);
}

static int timer_init(void)
{
	return pev_sig_add(SIGALRM, timer_run, NULL);
}

static int timer_exit(void)
{
	struct itimerval it = { 0 };

	return setitimer(ITIMER_REAL, &it, NULL);
}

int pev_timer_add(int period, void (*cb)(int, void *), void *arg)
{
	struct pev *entry;

	entry = pev_new(PEV_TIMER, cb, arg);
	if (!entry)
		return -1;

	entry->period = period;

	return entry->id;
}

int pev_timer_del(int id)
{
	int rc;

	rc = pev_sock_del(id);
	timer_run(0, NULL);

	return rc;
}

/******************************* GENERIC ******************************/

static struct pev *pev_new(int type, void (*cb)(int, void *), void *arg)
{
	struct pev *entry;

	entry = malloc(sizeof(*entry));
	if (!entry)
		return NULL;

	entry->id = id++;
	entry->type = type;
	entry->active = 1;

	entry->cb  = cb;
	entry->arg = arg;

	LIST_INSERT_HEAD(&pl, entry, link);

	return entry;
}

static struct pev *pev_find(int type, int signo)
{
	struct pev *entry;

	LIST_FOREACH(entry, &pl, link) {
		if (entry->type != type)
			continue;

		if (entry->signo != signo)
			continue;

		return entry;
	}

	errno = ENOENT;
	return NULL;
}

static void pev_cleanup(void)
{
	struct pev *entry, *tmp;

	LIST_FOREACH_SAFE(entry, &pl, link, tmp) {
		if (entry->active)
			continue;

		LIST_REMOVE(entry, link);
		free(entry);
	}
}

static void pev_event(int sd, void *arg)
{
	struct pev *entry;
	char signo;

	(void)arg;
	while (read(sd, &signo, 1) < 0) {
		if (errno != EINTR)
			return;
	}

	entry = pev_find(PEV_SIG, signo);
	if (!entry)
		return;

	if (!entry->cb)
		return;

	entry->cb(entry->signo, entry->arg);
}

int pev_init(void)
{
	if (pipe(events))
		return -1;
	if (pev_sock_add(events[0], pev_event, NULL) < 0)
		return -1;

	running = 1;

	return sig_init() || timer_init();
}

int pev_exit(void)
{
	pev_sock_close(events[0]);
	pev_sock_close(events[1]);
	running = 0;

	return sig_exit() || timer_exit();
}

int pev_run(void)
{
	struct pev *entry;
	fd_set fds;
	int num;

	timer_run(0, NULL);

	while (running) {
		sig_run();
		sock_run(&fds);

		errno = 0;
		num = select(nfds(), &fds, NULL, NULL, NULL);
		if (num <= 0)
			continue;

		LIST_FOREACH(entry, &pl, link) {
			if (!FD_ISSET(entry->sd, &fds))
				continue;

			if (entry->cb)
				entry->cb(entry->sd, entry->arg);
		}

		pev_cleanup();
	}

	return 0;
}

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
