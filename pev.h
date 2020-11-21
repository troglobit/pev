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

#ifndef PEV_H_
#define PEV_H_

#include <stdarg.h>
#include <string.h>
#include <sys/time.h>

/*
 * Call this before creating any signal, socket, or timer callbacks.
 */
int pev_init       (void);

/*
 * Call this from a callback to exit the event loop.
 */
int pev_exit       (void);

/*
 * The event loop itself.  Call after pev_init() and all the signal,
 * socket, or timer callbacks have been created.
 */
int pev_run        (void);

/*
 * Signal callbacks are identified by signal number, only one callback
 * per signal.  Signals are serialized like timers, which use SIGALRM,
 * using a pipe.  Delete by giving id returned from pev_sig_add()
 */
int pev_sig_add    (int signo, void (*cb)(int, void *), void *arg);
int pev_sig_del    (int id);

/*
 * Socket or file descriptor callback by sd/fd, only one callback per
 * descriptor.  API changes to CLOEXEC and NONBLOCK.  Delete by id
 * returned from pev_sock_add()
 */
int pev_sock_add   (int sd, void (*cb)(int, void *), void *arg);
int pev_sock_del   (int id);

/*
 * Same as pev_sock_add() but creates/closes socket as well.
 * Delete by id returned from pev_sock_open()
 */
int pev_sock_open  (int domain, int type, int proto, void (*cb)(int, void *), void *arg);
int pev_sock_close (int id);

/*
 * Periodic timers use SIGALRM via setitimer() API, may affect use of
 * sleep(), usleep(), and alarm() APIs.  See your respective OS for
 * details.  Otherwise works like the other pev APIs, returns id.
 */
int pev_timer_add  (int period, void (*cb)(int, void *), void *arg);
int pev_timer_del  (int id);

#endif /* PEV_H_ */
