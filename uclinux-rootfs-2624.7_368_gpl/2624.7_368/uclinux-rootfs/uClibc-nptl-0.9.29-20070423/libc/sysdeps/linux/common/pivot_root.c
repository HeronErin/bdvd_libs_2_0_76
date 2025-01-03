/* vi: set sw=4 ts=4: */
/*
 * pivot_root() for uClibc
 *
 * Copyright (C) 2000-2006 Erik Andersen <andersen@uclibc.org>
 *
 * Licensed under the LGPL v2.1, see the file COPYING.LIB in this tarball.
 */

#include "syscalls.h"

int pivot_root(const char *new_root, const char *put_old);
#ifdef __NR_pivot_root
_syscall2(int, pivot_root, const char *, new_root, const char *, put_old);
#else
int pivot_root(const char *new_root, const char *put_old)
{
	__set_errno(ENOSYS);
	return -1;
}
#endif
