/* vi: set sw=4 ts=4: */
/*
 * fchdir() for uClibc
 *
 * Copyright (C) 2000-2006 Erik Andersen <andersen@uclibc.org>
 *
 * Licensed under the LGPL v2.1, see the file COPYING.LIB in this tarball.
 */

#include "syscalls.h"
#include <unistd.h>

libc_hidden_proto(fchdir)

_syscall1(int, fchdir, int, fd);
libc_hidden_def(fchdir)