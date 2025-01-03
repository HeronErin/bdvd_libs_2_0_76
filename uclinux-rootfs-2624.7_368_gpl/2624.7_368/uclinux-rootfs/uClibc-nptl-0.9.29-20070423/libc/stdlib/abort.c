/* Copyright (C) 1991 Free Software Foundation, Inc.
This file is part of the GNU C Library.

The GNU C Library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The GNU C Library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with the GNU C Library; see the file COPYING.LIB.  If
not, write to the Free Software Foundation, Inc., 675 Mass Ave,
Cambridge, MA 02139, USA.  */

/* Hacked up for uClibc by Erik Andersen */

#include <features.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

libc_hidden_proto(abort)

libc_hidden_proto(memset)
libc_hidden_proto(sigaction)
libc_hidden_proto(sigprocmask)
libc_hidden_proto(raise)
libc_hidden_proto(_exit)

/* Our last ditch effort to commit suicide */
#ifdef __UCLIBC_ABORT_INSTRUCTION__
# define ABORT_INSTRUCTION __asm__(__UCLIBC_ABORT_INSTRUCTION__)
#else
# define ABORT_INSTRUCTION
# warning "no abort instruction defined for your arch"
#endif

#ifdef __UCLIBC_HAS_STDIO_SHUTDOWN_ON_ABORT__
extern void weak_function _stdio_term(void) attribute_hidden;
#endif
static int been_there_done_that = 0;

/* Be prepared in case multiple threads try to abort() */
#ifdef __UCLIBC_HAS_THREADS__
# include <pthread.h>
static pthread_mutex_t mylock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#endif
#define LOCK	__PTHREAD_MUTEX_LOCK(&mylock)
#define UNLOCK	__PTHREAD_MUTEX_UNLOCK(&mylock)

/* Cause an abnormal program termination with core-dump */
void abort(void)
{
	sigset_t sigs;

	/* Make sure we acquire the lock before proceeding */
	LOCK;

	/* Unmask SIGABRT to be sure we can get it */
	if (__sigemptyset(&sigs) == 0 && __sigaddset(&sigs, SIGABRT) == 0) {
		sigprocmask(SIG_UNBLOCK, &sigs, (sigset_t *) NULL);
	}

	while (1) {
		/* Try to suicide with a SIGABRT */
		if (been_there_done_that == 0) {
			been_there_done_that++;

#ifdef __UCLIBC_HAS_STDIO_SHUTDOWN_ON_ABORT__
			/* If we are using stdio, try to shut it down.  At the very least,
			 * this will attempt to commit all buffered writes.  It may also
			 * unbuffer all writable files, or close them outright.
			 * Check the stdio routines for details. */
			if (_stdio_term) {
				_stdio_term();
			}
#endif

abort_it:
			UNLOCK;
			raise(SIGABRT);
			LOCK;
		}

		/* Still here?  Try to remove any signal handlers */
		if (been_there_done_that == 1) {
			struct sigaction act;

			been_there_done_that++;
			memset(&act, '\0', sizeof(struct sigaction));
			act.sa_handler = SIG_DFL;
			__sigfillset(&act.sa_mask);
			act.sa_flags = 0;
			sigaction(SIGABRT, &act, NULL);

			goto abort_it;
		}

		/* Still here?  Try to suicide with an illegal instruction */
		if (been_there_done_that == 2) {
			been_there_done_that++;
			ABORT_INSTRUCTION;
		}

		/* Still here?  Try to at least exit */
		if (been_there_done_that == 3) {
			been_there_done_that++;
			_exit(127);
		}

		/* Still here?  We're screwed.  Sleepy time.  Good night. */
		while (1)
			/* Try for ever and ever */
			ABORT_INSTRUCTION;
	}
}
libc_hidden_def(abort)
