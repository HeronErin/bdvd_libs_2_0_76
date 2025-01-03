/* vi: set sw=4 ts=4: */
/*
 * sigpending() for uClibc
 *
 * Copyright (C) 2000-2006 Erik Andersen <andersen@uclibc.org>
 *
 * Licensed under the LGPL v2.1, see the file COPYING.LIB in this tarball.
 */

#include "syscalls.h"
#include <signal.h>
#undef sigpending

#ifdef __NR_rt_sigpending
# define __NR___rt_sigpending __NR_rt_sigpending
static inline _syscall2(int, __rt_sigpending, sigset_t *, set, size_t, size);

int sigpending(sigset_t * set)
{
	return __rt_sigpending(set, _NSIG / 8);
}
#else
_syscall1(int, sigpending, sigset_t *, set);
#endif
