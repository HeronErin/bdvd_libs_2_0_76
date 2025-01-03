/* Convert between the kernel's `struct stat' format, and libc's.
   Copyright (C) 1991,1995,1996,1997,2000,2002 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA. 
   
   Modified for uClibc by Erik Andersen <andersen@codepoet.org>
   */

#include "syscalls.h"
#include <sys/stat.h>
#include "xstatconv.h"

void attribute_hidden __xstat_conv(struct kernel_stat *kbuf, struct stat *buf)
{
	/* Convert to current kernel version of `struct stat'. */
	buf->st_dev = kbuf->st_dev;
	buf->st_ino = kbuf->st_ino;
	buf->st_mode = kbuf->st_mode;
	buf->st_nlink = kbuf->st_nlink;
	buf->st_uid = kbuf->st_uid;
	buf->st_gid = kbuf->st_gid;
	buf->st_rdev = kbuf->st_rdev;
	buf->st_size = kbuf->st_size;
	buf->st_blksize = kbuf->st_blksize;
	buf->st_blocks = kbuf->st_blocks;
	buf->st_atime = kbuf->st_atime;
	buf->st_mtime = kbuf->st_mtime;
	buf->st_ctime = kbuf->st_ctime;
#ifdef STAT_HAVE_NSEC
	buf->st_atimensec = kbuf->st_atime_nsec;
	buf->st_mtimensec = kbuf->st_mtime_nsec;
	buf->st_ctimensec = kbuf->st_ctime_nsec;
#endif
}

#ifdef __UCLIBC_HAS_LFS__

void attribute_hidden __xstat64_conv(struct kernel_stat64 *kbuf, struct stat64 *buf)
{
	/* Convert to current kernel version of `struct stat64'. */
	buf->st_dev = kbuf->st_dev;
	buf->st_ino = kbuf->st_ino;
# ifdef _HAVE_STAT64___ST_INO
	buf->__st_ino = kbuf->__st_ino;
# endif
	buf->st_mode = kbuf->st_mode;
	buf->st_nlink = kbuf->st_nlink;
	buf->st_uid = kbuf->st_uid;
	buf->st_gid = kbuf->st_gid;
	buf->st_rdev = kbuf->st_rdev;
	buf->st_size = kbuf->st_size;
	buf->st_blksize = kbuf->st_blksize;
	buf->st_blocks = kbuf->st_blocks;
	buf->st_atime = kbuf->st_atime;
	buf->st_mtime = kbuf->st_mtime;
	buf->st_ctime = kbuf->st_ctime;
# ifdef STAT_HAVE_NSEC
	buf->st_atimensec = kbuf->st_atime_nsec;
	buf->st_mtimensec = kbuf->st_mtime_nsec;
	buf->st_ctimensec = kbuf->st_ctime_nsec;
# endif
}

#endif /* __UCLIBC_HAS_LFS__ */
