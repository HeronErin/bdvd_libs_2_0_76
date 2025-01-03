/* Copyright (C) 2004       Manuel Novoa III    <mjn3@codepoet.org>
 *
 * GNU Library General Public License (LGPL) version 2 or later.
 *
 * Dedicated to Toni.  See uClibc/DEDICATION.mjn3 for details.
 */

#include "_stdio.h"

#ifdef __UCLIBC_HAS_LFS__
libc_hidden_proto(ftello64)
#endif
libc_hidden_proto(ftell)

#ifndef __DO_LARGEFILE
# define FTELL         ftell
# define OFFSET_TYPE   long int
#endif

OFFSET_TYPE FTELL(register FILE *stream)
{
#if defined(__UCLIBC_HAS_LFS__) && !defined(__DO_LARGEFILE)

	__offmax_t pos = ftello64(stream);

	if ((sizeof(long) >= sizeof(__offmax_t)) || (((long) pos) == pos)) {
		return ((long) pos);
	} else {
		__set_errno(EOVERFLOW);
		return -1;
	}

#else

	__offmax_t pos = 0;
	__STDIO_AUTO_THREADLOCK_VAR;

	__STDIO_AUTO_THREADLOCK(stream);

	__STDIO_STREAM_VALIDATE(stream);

	if ((__SEEK(stream, &pos, SEEK_CUR) < 0)
		|| (__stdio_adjust_position(stream, &pos) < 0)) {
		pos = -1;
	}

	__STDIO_AUTO_THREADUNLOCK(stream);

	return pos;

#endif
}

#ifdef __DO_LARGEFILE
libc_hidden_def(ftello64)
#else
libc_hidden_def(ftell)
strong_alias(ftell,ftello)
#endif
