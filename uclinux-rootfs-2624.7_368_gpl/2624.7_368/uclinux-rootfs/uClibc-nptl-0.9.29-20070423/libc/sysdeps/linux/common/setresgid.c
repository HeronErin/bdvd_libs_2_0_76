/* vi: set sw=4 ts=4: */
/*
 * setresgid() for uClibc
 *
 * Copyright (C) 2000-2006 Erik Andersen <andersen@uclibc.org>
 *
 * Licensed under the LGPL v2.1, see the file COPYING.LIB in this tarball.
 */

#include "syscalls.h"
#ifdef __USE_GNU
#include <unistd.h>

libc_hidden_proto(setresgid)

#if defined(__NR_setresgid32)
# undef __NR_setresgid
# define __NR_setresgid __NR_setresgid32
_syscall3(int, setresgid, gid_t, rgid, gid_t, egid, gid_t, sgid)

#elif defined(__NR_setresgid)
# define __NR___syscall_setresgid __NR_setresgid
static inline _syscall3(int, __syscall_setresgid,
		__kernel_gid_t, rgid, __kernel_gid_t, egid, __kernel_gid_t, sgid);

int setresgid(gid_t rgid, gid_t egid, gid_t sgid)
{
	if (((rgid + 1) > (gid_t) ((__kernel_gid_t) - 1U))
		|| ((egid + 1) > (gid_t) ((__kernel_gid_t) - 1U))
		|| ((sgid + 1) > (gid_t) ((__kernel_gid_t) - 1U))) {
		__set_errno(EINVAL);
		return -1;
	}
	return (__syscall_setresgid(rgid, egid, sgid));
}
#endif

libc_hidden_def(setresgid)
#endif
