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

int pev_init (void);
int pev_run  (void);

int socket_register(int sd, void (*cb)(int, void *), void *arg);
int socket_create  (int domain, int type, int proto, void (*cb)(int, void *), void *arg);
int socket_close   (int sd);
int socket_poll    (struct timeval *timeout);

int timer_init (void);

int timer_add  (int period, void (*cb)(void *), void *arg);
int timer_del  (void (*cb)(void *), void *arg);

#endif /* PEV_H_ */
