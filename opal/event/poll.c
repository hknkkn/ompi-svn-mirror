/*	$OpenBSD: poll.c,v 1.2 2002/06/25 15:50:15 mickey Exp $	*/

/*
 * Copyright 2000-2003 Niels Provos <provos@citi.umich.edu>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "opal_config.h"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <sys/_time.h>
#endif
#include <sys/queue.h>
#include <sys/tree.h>
#ifdef HAVE_POLL_H
#include <poll.h>
#endif
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>
#ifdef CHECK_INVARIANTS
#include <assert.h>
#endif

#include "event.h"
#include "event-internal.h"
#if OPAL_EVENT_USE_SIGNALS
#include "evsignal.h"
#endif
#include "log.h"

#include "opal/util/output.h"
#include "opal/threads/mutex.h"

extern volatile sig_atomic_t opal_evsignal_caught;
extern opal_mutex_t opal_event_lock;

extern volatile sig_atomic_t evsignal_caught;

struct pollop {
	int event_count;		/* Highest number alloc */
	int nfds;                       /* Size of event_* */
	int fd_count;                   /* Size of idxplus1_by_fd */
	struct pollfd *event_set;
	struct opal_event **event_r_back;
	struct opal_event **event_w_back;
	int *idxplus1_by_fd; /* Index into event_set by fd; we add 1 so
			      * that 0 (which is easy to memset) can mean
			      * "no entry." */
#if OPAL_EVENT_USE_SIGNALS
	sigset_t evsigmask;
#endif
};

void *poll_init	(void);
int poll_add		(void *, struct opal_event *);
int poll_del		(void *, struct opal_event *);
int poll_recalc		(struct event_base *, void *, int);
int poll_dispatch	(struct event_base *, void *, struct timeval *);

struct opal_eventop opal_pollops = {
	"poll",
	poll_init,
	poll_add,
	poll_del,
	poll_recalc,
	poll_dispatch
};

void *
poll_init(void)
{
	struct pollop *pollop;

	/* Disable kqueue when this environment variable is set */
	if (getenv("EVENT_NOPOLL"))
		return (NULL);

        if (!(pollop = calloc(1, sizeof(struct pollop))))
		return (NULL);

#if OPAL_EVENT_USE_SIGNALS
	opal_evsignal_init(&pollop->evsigmask);
#endif

	return (pollop);
}

/*
 * Called with the highest fd that we know about.  If it is 0, completely
 * recalculate everything.
 */

int
poll_recalc(struct event_base *base, void *arg, int max)
{
#if OPAL_EVENT_USE_SIGNALS
	struct pollop *pop = arg;

	return (opal_evsignal_recalc(&pop->evsigmask));
#else
        return 0;
#endif
}

#ifdef CHECK_INVARIANTS
static void
poll_check_ok(struct pollop *pop)
{
	int i, idx;
	struct opal_event *ev;

	for (i = 0; i < pop->fd_count; ++i) {
		idx = pop->idxplus1_by_fd[i]-1;
		if (idx < 0)
			continue;
		assert(pop->event_set[idx].fd == i);
		if (pop->event_set[idx].events & POLLIN) {
			ev = pop->event_r_back[idx];
			assert(ev);
			assert(ev->ev_events & OPAL_EV_READ);
			assert(ev->ev_fd == i);
		}
		if (pop->event_set[idx].events & POLLOUT) {
			ev = pop->event_w_back[idx];
			assert(ev);
			assert(ev->ev_events & OPAL_EV_WRITE);
			assert(ev->ev_fd == i);
		}
	}
	for (i = 0; i < pop->nfds; ++i) {
		struct pollfd *pfd = &pop->event_set[i];
		assert(pop->idxplus1_by_fd[pfd->fd] == i+1);
	}
}
#else
#define poll_check_ok(pop)
#endif

int
poll_dispatch(struct event_base *base, void *arg, struct timeval *tv)
{
	int res, i, sec, nfds;
	struct pollop *pop = arg;

#if OPAL_EVENT_USE_SIGNALS
	if (opal_evsignal_deliver(&pop->evsigmask) == -1)
		return (-1);
#endif

	poll_check_ok(pop);
	sec = tv->tv_sec * 1000 + (tv->tv_usec + 999) / 1000;
	nfds = pop->nfds;
        /* we should release the lock if we're going to enter the
           kernel in a multi-threaded application.  However, if we're
           single threaded, there's really no advantage to releasing
           the lock and it just takes up time we could spend doing
           something else. */
        OPAL_THREAD_UNLOCK(&opal_event_lock);
	res = poll(pop->event_set, nfds, sec);
	OPAL_THREAD_LOCK(&opal_event_lock);

#if OPAL_EVENT_USE_SIGNALS
	if (opal_evsignal_recalc(&pop->evsigmask) == -1)
		return (-1);
#endif

	if (res == -1) {
		if (errno != EINTR) {
                        event_warn("poll");
			return (-1);
		}

#if OPAL_EVENT_USE_SIGNALS
		opal_evsignal_process();
#endif
		return (0);
	} else if (opal_evsignal_caught)
#if OPAL_EVENT_USE_SIGNALS
		opal_evsignal_process();
#endif

	event_debug(("%s: poll reports %d", __func__, res));

	if (res == 0)
		return (0);

	for (i = 0; i < nfds; i++) {
                int what = pop->event_set[i].revents;
		struct opal_event *r_ev = NULL, *w_ev = NULL;
		if (!what)
			continue;

		res = 0;

		/* If the file gets closed notify */
		if (what & (POLLHUP|POLLERR))
			what |= POLLIN|POLLOUT;
		if (what & POLLIN) {
			res |= OPAL_EV_READ;
			r_ev = pop->event_r_back[i];
		}
		if (what & POLLOUT) {
			res |= OPAL_EV_WRITE;
			w_ev = pop->event_w_back[i];
		}
		if (res == 0)
			continue;

		if (r_ev && (res & r_ev->ev_events)) {
			if (!(r_ev->ev_events & OPAL_EV_PERSIST))
				opal_event_del(r_ev);
			opal_event_active(r_ev, res & r_ev->ev_events, 1);
		}
		if (w_ev && w_ev != r_ev && (res & w_ev->ev_events)) {
			if (!(w_ev->ev_events & OPAL_EV_PERSIST))
				opal_event_del(w_ev);
			opal_event_active(w_ev, res & w_ev->ev_events, 1);
		}
	}

	return (0);
}

int
poll_add(void *arg, struct opal_event *ev)
{
	struct pollop *pop = arg;
	struct pollfd *pfd = NULL;
	int i;

#if OPAL_EVENT_USE_SIGNALS
	if (ev->ev_events & OPAL_EV_SIGNAL)
		return (opal_evsignal_add(&pop->evsigmask, ev));
#endif
	if (!(ev->ev_events & (OPAL_EV_READ|OPAL_EV_WRITE)))
		return (0);

	poll_check_ok(pop);
	if (pop->nfds + 1 >= pop->event_count) {
		if (pop->event_count < 32)
			pop->event_count = 32;
		else
			pop->event_count *= 2;

		/* We need more file descriptors */
		pop->event_set = realloc(pop->event_set,
				 pop->event_count * sizeof(struct pollfd));
		if (pop->event_set == NULL) {
			event_warn("realloc");
			return (-1);
		}
		pop->event_r_back = realloc(pop->event_r_back,
			    pop->event_count * sizeof(struct opal_event *));
		pop->event_w_back = realloc(pop->event_w_back,
			    pop->event_count * sizeof(struct opal_event *));
		if (pop->event_r_back == NULL ||
		    pop->event_w_back == NULL) {
			event_warn("realloc");
			return (-1);
		}
	}
	if (ev->ev_fd >= pop->fd_count) {
		int new_count;
		if (pop->fd_count < 32)
			new_count = 32;
		else
			new_count = pop->fd_count * 2;
		while (new_count <= ev->ev_fd)
			new_count *= 2;
		pop->idxplus1_by_fd =
			realloc(pop->idxplus1_by_fd, new_count*sizeof(int));
		if (pop->idxplus1_by_fd == NULL) {
			event_warn("realloc");
			return (-1);
		}
		memset(pop->idxplus1_by_fd + pop->fd_count,
		       0, sizeof(int)*(new_count - pop->fd_count));
		pop->fd_count = new_count;
	}

	i = pop->idxplus1_by_fd[ev->ev_fd] - 1;
	if (i >= 0) {
		pfd = &pop->event_set[i];
	} else {
		i = pop->nfds++;
		pfd = &pop->event_set[i];
		pfd->events = 0;
		pfd->fd = ev->ev_fd;
		pop->event_w_back[i] = pop->event_r_back[i] = NULL;
		pop->idxplus1_by_fd[ev->ev_fd] = i + 1;
	}

	pfd->revents = 0;
	if (ev->ev_events & OPAL_EV_WRITE) {
		pfd->events |= POLLOUT;
		pop->event_w_back[i] = ev;
	}
	if (ev->ev_events & OPAL_EV_READ) {
		pfd->events |= POLLIN;
		pop->event_r_back[i] = ev;
	}
	poll_check_ok(pop);

	return (0);
}

/*
 * Nothing to be done here.
 */

int
poll_del(void *arg, struct opal_event *ev)
{
	struct pollop *pop = arg;
	struct pollfd *pfd = NULL;
	int i;

#if OPAL_EVENT_USE_SIGNALS
	if (ev->ev_events & OPAL_EV_SIGNAL)
		return (opal_evsignal_del(&pop->evsigmask, ev));
#endif

	if (!(ev->ev_events & (OPAL_EV_READ|OPAL_EV_WRITE)))
		return (0);

	poll_check_ok(pop);
	i = pop->idxplus1_by_fd[ev->ev_fd] - 1;
	if (i < 0)
		return (-1);

	/* Do we still want to read or write? */
	pfd = &pop->event_set[i];
	if (ev->ev_events & OPAL_EV_READ) {
		pfd->events &= ~POLLIN;
		pop->event_r_back[i] = NULL;
	}
	if (ev->ev_events & OPAL_EV_WRITE) {
		pfd->events &= ~POLLOUT;
		pop->event_w_back[i] = NULL;
	}
	poll_check_ok(pop);
	if (pfd->events)
		/* Another event cares about that fd. */
		return (0);

	/* Okay, so we aren't interested in that fd anymore. */
	pop->idxplus1_by_fd[ev->ev_fd] = 0;

	--pop->nfds;
	if (i != pop->nfds) {
		/* 
		 * Shift the last pollfd down into the now-unoccupied
		 * position.
		 */
		memcpy(&pop->event_set[i], &pop->event_set[pop->nfds],
		       sizeof(struct pollfd));
		pop->event_r_back[i] = pop->event_r_back[pop->nfds];
		pop->event_w_back[i] = pop->event_w_back[pop->nfds];
		pop->idxplus1_by_fd[pop->event_set[i].fd] = i + 1;
	}

	poll_check_ok(pop);
	return (0);
}
