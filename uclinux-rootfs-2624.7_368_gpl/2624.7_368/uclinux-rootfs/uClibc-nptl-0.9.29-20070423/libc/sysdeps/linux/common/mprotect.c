/* vi: set sw=4 ts=4: */
/*
 * mprotect() for uClibc
 *
 * Copyright (C) 2000-2006 Erik Andersen <andersen@uclibc.org>
 *
 * Licensed under the LGPL v2.1, see the file COPYING.LIB in this tarball.
 */

#include "syscalls.h"
#include <sys/mman.h>
_syscall3(int, mprotect, void *, addr, size_t, len, int, prot);
