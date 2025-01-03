/* vi: set sw=4 ts=4: */
/*
 * swapoff() for uClibc
 *
 * Copyright (C) 2000-2006 Erik Andersen <andersen@uclibc.org>
 *
 * Licensed under the LGPL v2.1, see the file COPYING.LIB in this tarball.
 */

#include "syscalls.h"
#include <sys/swap.h>
_syscall1(int, swapoff, const char *, path);
