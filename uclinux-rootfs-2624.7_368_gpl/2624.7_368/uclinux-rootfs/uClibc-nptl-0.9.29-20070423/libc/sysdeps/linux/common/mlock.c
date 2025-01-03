/* vi: set sw=4 ts=4: */
/*
 * mlock() for uClibc
 *
 * Copyright (C) 2000-2006 Erik Andersen <andersen@uclibc.org>
 *
 * Licensed under the LGPL v2.1, see the file COPYING.LIB in this tarball.
 */

#include "syscalls.h"
#include <sys/mman.h>
#if defined __ARCH_USE_MMU__ && defined __NR_mlock
_syscall2(int, mlock, const void *, addr, size_t, len);
#endif
