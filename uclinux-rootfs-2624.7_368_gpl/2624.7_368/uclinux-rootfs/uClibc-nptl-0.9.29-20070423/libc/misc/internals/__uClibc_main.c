/* vi: set sw=4 ts=4: */
/*
 * Copyright (C) 2006 by Steven J. Hill <sjhill@realitydiluted.com>
 * Copyright (C) 2001 by Manuel Novoa III <mjn3@uclibc.org>
 * Copyright (C) 2000-2005 Erik Andersen <andersen@uclibc.org>
 *
 * Licensed under the LGPL v2.1, see the file COPYING.LIB in this tarball.
 *
 * __uClibc_main is the routine to be called by all the arch-specific
 * versions of crt1.S in uClibc.
 *
 * It is meant to handle any special initialization needed by the library
 * such as setting the global variable(s) __environ (environ) and
 * initializing the stdio package.  Using weak symbols, the latter is
 * avoided in the static library case.
 */

#define	_ERRNO_H
#include <features.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <elf.h>
#include <link.h>
#include <bits/uClibc_page.h>
#include <paths.h>
#include <unistd.h>
#include <asm/errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#ifdef __UCLIBC_HAS_THREADS_NATIVE__
#include <pthread-functions.h>
#include <not-cancel.h>
#endif

libc_hidden_proto(exit)

#ifdef __UCLIBC_HAS_PROGRAM_INVOCATION_NAME__
libc_hidden_proto(strrchr)
#endif
#ifdef __ARCH_USE_MMU__
libc_hidden_proto(memcpy)
libc_hidden_proto(getgid)
libc_hidden_proto(getuid)
libc_hidden_proto(getegid)
libc_hidden_proto(geteuid)
libc_hidden_proto(fstat)
libc_hidden_proto(abort)

extern __typeof(open) __libc_open;
libc_hidden_proto(__libc_open)
extern __typeof(fcntl) __libc_fcntl;
libc_hidden_proto(__libc_fcntl)

extern void internal_function _dl_aux_init(ElfW(auxv_t) *);
#endif

#ifndef SHARED
void *__libc_stack_end=NULL;

# ifdef __UCLIBC_HAS_SSP__
#  include <dl-osinfo.h>
#  ifndef THREAD_SET_STACK_GUARD
/* Only exported for architectures that don't store the stack guard canary
 * in thread local area. */
#   include <stdint.h>
uintptr_t stack_chk_guard;
/* for gcc-4.1 non-TLS */
uintptr_t __stack_chk_guard attribute_relro;
/* for gcc-3.x + Etoh ssp */
#   ifdef __UCLIBC_HAS_SSP_COMPAT__
#    ifdef __HAVE_SHARED__
strong_alias(__stack_chk_guard,__guard)
#    else
uintptr_t __guard attribute_relro;
#    endif
#   endif
#  elif defined __UCLIBC_HAS_SSP_COMPAT__
uintptr_t __guard attribute_relro;
#  endif
# endif

#endif /* !SHARED */

/*
 * Prototypes.
 */
extern void weak_function _stdio_init(void) attribute_hidden;
extern int *weak_const_function __errno_location(void);
extern int *weak_const_function __h_errno_location(void);
#ifdef __UCLIBC_HAS_LOCALE__
extern void weak_function _locale_init(void) attribute_hidden;
#endif
#ifdef __UCLIBC_HAS_THREADS__
extern void weak_function __pthread_initialize_minimal(void);
extern void ____pthread_initialize_minimal(void);
#endif

#ifdef __UCLIBC_CTOR_DTOR__
extern void _dl_app_init_array(void);
extern void _dl_app_fini_array(void);
# ifndef SHARED
/* These magic symbols are provided by the linker.  */
extern void (*__preinit_array_start []) (void) attribute_hidden;
extern void (*__preinit_array_end []) (void) attribute_hidden;
extern void (*__init_array_start []) (void) attribute_hidden;
extern void (*__init_array_end []) (void) attribute_hidden;
extern void (*__fini_array_start []) (void) attribute_hidden;
extern void (*__fini_array_end []) (void) attribute_hidden;
# endif
#endif

attribute_hidden const char *__uclibc_progname = NULL;
#ifdef __UCLIBC_HAS___PROGNAME__
strong_alias (__uclibc_progname, __progname)
#endif
#ifdef __UCLIBC_HAS_PROGRAM_INVOCATION_NAME__
attribute_hidden const char *__progname_full = NULL;
strong_alias (__uclibc_progname, program_invocation_short_name)
strong_alias (__progname_full, program_invocation_name)
#endif

/*
 * Declare the __environ global variable and create a strong alias environ.
 * Note: Apparently we must initialize __environ to ensure that the strong
 * environ symbol is also included.
 */
char **__environ = 0;
weak_alias(__environ, environ)

/* TODO: don't export __pagesize; we cant now because libpthread uses it */
size_t __pagesize = 0;

#ifndef O_NOFOLLOW
# define O_NOFOLLOW	0
#endif

#ifdef __ARCH_USE_MMU__
static void __check_one_fd(int fd, int mode)
{
	/* Check if the specified fd is already open */
	if (unlikely(fcntl(fd, F_GETFD)==-1 && *(__errno_location())==EBADF))
	{
		/* The descriptor is probably not open, so try to use /dev/null */
		struct stat st;

#ifndef __UCLIBC_HAS_THREADS_NATIVE__
		int nullfd = open(_PATH_DEVNULL, mode);
#else
		int nullfd = open_not_cancel (_PATH_DEVNULL, mode, 0);
#endif

		/* /dev/null is major=1 minor=3.  Make absolutely certain
		 * that is in fact the device that we have opened and not
		 * some other wierd file... */
		if ((nullfd != fd) || fstat(fd, &st) || !S_ISCHR(st.st_mode) ||
			(st.st_rdev != makedev(1, 3)))
		{
		    /* Somebody is trying some trickery here... */
			abort();
		}
	}
}

static int __check_suid(void)
{
    uid_t uid, euid;
    gid_t gid, egid;

    uid  = getuid();
    euid = geteuid();
    gid  = getgid();
    egid = getegid();

    if(uid == euid && gid == egid) {
	return 0;
    }
    return 1;
}
#endif

/* __uClibc_init completely initialize uClibc so it is ready to use.
 *
 * On ELF systems (with a dynamic loader) this function must be called
 * from the dynamic loader (see TIS and ELF Specification), so that
 * constructors of shared libraries (which depend on libc) can use all
 * the libc code without restriction.  For this we link the shared
 * version of the uClibc with -init __uClibc_init so DT_INIT for
 * uClibc is the address of __uClibc_init
 *
 * In all other cases we call it from the main stub
 * __uClibc_main.
 */

extern void __uClibc_init(void);
libc_hidden_proto(__uClibc_init)
void __uClibc_init(void)
{
    static int been_there_done_that = 0;

    if (been_there_done_that)
	return;
    been_there_done_that++;

    /* Setup an initial value.  This may not be perfect, but is
     * better than  malloc using __pagesize=0 for atexit, ctors, etc.  */
    __pagesize = PAGE_SIZE;

#ifdef __UCLIBC_HAS_THREADS__
    /* Before we start initializing uClibc we have to call
     * __pthread_initialize_minimal so we can use pthread_locks
     * whenever they are needed.
     */
    if (likely(__pthread_initialize_minimal!=NULL))
	    __pthread_initialize_minimal();
#ifndef SHARED
    else
	    ____pthread_initialize_minimal();
#endif
#endif

#ifndef SHARED
# ifdef __UCLIBC_HAS_SSP__
    /* Set up the stack checker's canary.  */
    stack_chk_guard = _dl_setup_stack_chk_guard();
#  ifdef THREAD_SET_STACK_GUARD
    THREAD_SET_STACK_GUARD (stack_chk_guard);
#   ifdef __UCLIBC_HAS_SSP_COMPAT__
    __guard = stack_chk_guard;
#   endif
#  else
    __stack_chk_guard = stack_chk_guard;
#   if !defined __HAVE_SHARED__ && defined __UCLIBC_HAS_SSP_COMPAT__
     __guard = stack_chk_guard;
#   endif
#  endif
# endif
#endif

#ifdef __UCLIBC_HAS_LOCALE__
    /* Initialize the global locale structure. */
    if (likely(_locale_init!=NULL))
	_locale_init();
#endif

    /*
     * Initialize stdio here.  In the static library case, this will
     * be bypassed if not needed because of the weak alias above.
     */
    if (likely(_stdio_init != NULL))
	_stdio_init();

}
libc_hidden_def(__uClibc_init)

#ifdef __UCLIBC_CTOR_DTOR__
void attribute_hidden (*__app_fini)(void) = NULL;
#endif

void attribute_hidden (*__rtld_fini)(void) = NULL;

extern void __uClibc_fini(void);
libc_hidden_proto(__uClibc_fini)
void __uClibc_fini(void)
{
#ifdef __UCLIBC_CTOR_DTOR__
# ifdef SHARED
    _dl_app_fini_array();
# else
    size_t i = __fini_array_end - __fini_array_start;
    while (i-- > 0)
	(*__fini_array_start [i]) ();
# endif
    if (__app_fini != NULL)
	(__app_fini)();
#endif
    if (__rtld_fini != NULL)
	(__rtld_fini)();
}
libc_hidden_def(__uClibc_fini)

/* __uClibc_main is the new main stub for uClibc. This function is
 * called from crt1 (version 0.9.28 or newer), after ALL shared libraries
 * are initialized, just before we call the application's main function.
 */
void __uClibc_main(int (*main)(int, char **, char **), int argc,
		    char **argv, void (*app_init)(void), void (*app_fini)(void),
		    void (*rtld_fini)(void), void *stack_end) attribute_noreturn;
void __uClibc_main(int (*main)(int, char **, char **), int argc,
		    char **argv, void (*app_init)(void), void (*app_fini)(void),
		    void (*rtld_fini)(void), void *stack_end)
{
#ifdef __ARCH_USE_MMU__
    unsigned long *aux_dat;
    ElfW(auxv_t) auxvt[AT_EGID + 1];
#endif

#ifdef __UCLIBC_HAS_THREADS_NATIVE__
	/* Result of the 'main' function.  */
	int result;
#endif

#ifndef SHARED
    __libc_stack_end = stack_end;
#endif

    __rtld_fini = rtld_fini;

    /* The environment begins right after argv.  */
    __environ = &argv[argc + 1];

    /* If the first thing after argv is the arguments
     * the the environment is empty. */
    if ((char *) __environ == *argv) {
	/* Make __environ point to the NULL at argv[argc] */
	__environ = &argv[argc];
    }

#ifdef __ARCH_USE_MMU__
    /* Pull stuff from the ELF header when possible */
    aux_dat = (unsigned long*)__environ;
    while (*aux_dat) {
	aux_dat++;
    }
    aux_dat++;
#ifndef SHARED
	extern char *_dl_argv0;
    _dl_aux_init((ElfW(auxv_t) *)aux_dat);
	if(argc)
		_dl_argv0 = argv[0];
#endif
    while (*aux_dat) {
	ElfW(auxv_t) *auxv_entry = (ElfW(auxv_t) *) aux_dat;
	if (auxv_entry->a_type <= AT_EGID) {
	    memcpy(&(auxvt[auxv_entry->a_type]), auxv_entry, sizeof(ElfW(auxv_t)));
	}
	aux_dat += 2;
    }
#endif

    /* We need to initialize uClibc.  If we are dynamically linked this
     * may have already been completed by the shared lib loader.  We call
     * __uClibc_init() regardless, to be sure the right thing happens. */
    __uClibc_init();

#ifdef __ARCH_USE_MMU__
    /* Make certain getpagesize() gives the correct answer */
    __pagesize = (auxvt[AT_PAGESZ].a_un.a_val)? auxvt[AT_PAGESZ].a_un.a_val : PAGE_SIZE;

    /* Prevent starting SUID binaries where the stdin. stdout, and
     * stderr file descriptors are not already opened. */
    if ((auxvt[AT_UID].a_un.a_val == (size_t)-1 && __check_suid()) ||
	    (auxvt[AT_UID].a_un.a_val != (size_t)-1 &&
	    (auxvt[AT_UID].a_un.a_val != auxvt[AT_EUID].a_un.a_val ||
	     auxvt[AT_GID].a_un.a_val != auxvt[AT_EGID].a_un.a_val)))
    {
	__check_one_fd (STDIN_FILENO, O_RDONLY | O_NOFOLLOW);
	__check_one_fd (STDOUT_FILENO, O_RDWR | O_NOFOLLOW);
	__check_one_fd (STDERR_FILENO, O_RDWR | O_NOFOLLOW);
    }
#endif

#ifdef __UCLIBC_HAS_PROGRAM_INVOCATION_NAME__
    __progname_full = *argv;
    __progname = strrchr(*argv, '/');
    if (__progname != NULL)
	++__progname;
    else
	__progname = __progname_full;
#else
    __uclibc_progname = *argv;
#endif

#ifdef __UCLIBC_CTOR_DTOR__
    /* Arrange for the application's dtors to run before we exit.  */
    __app_fini = app_fini;

# ifndef SHARED
    /* For dynamically linked executables the preinit array is executed by
       the dynamic linker (before initializing any shared object).
       For static executables, preinit happens rights before init.  */
    {
	const size_t size = __preinit_array_end - __preinit_array_start;
	size_t i;
	for (i = 0; i < size; i++)
	    (*__preinit_array_start [i]) ();
    }
# endif
    /* Run all the application's ctors now.  */
    if (app_init!=NULL) {
	app_init();
    }
# ifdef SHARED
    _dl_app_init_array();
# else
    {
	const size_t size = __init_array_end - __init_array_start;
	size_t i;
	for (i = 0; i < size; i++)
	    (*__init_array_start [i]) ();
    }
# endif
#endif

    /* Note: It is possible that any initialization done above could
     * have resulted in errno being set nonzero, so set it to 0 before
     * we call main.
     */
    if (likely(__errno_location!=NULL))
	*(__errno_location()) = 0;

    /* Set h_errno to 0 as well */
    if (likely(__h_errno_location!=NULL))
	*(__h_errno_location()) = 0;

#if defined HAVE_CLEANUP_JMP_BUF && defined __UCLIBC_HAS_THREADS_NATIVE__
	/* Memory for the cancellation buffer.  */
	struct pthread_unwind_buf unwind_buf;

	int not_first_call;
	not_first_call =
		setjmp ((struct __jmp_buf_tag *) unwind_buf.cancel_jmp_buf);
	if (__builtin_expect (! not_first_call, 1))
	{
		struct pthread *self = THREAD_SELF;

		/* Store old info.  */
		unwind_buf.priv.data.prev = THREAD_GETMEM (self, cleanup_jmp_buf);
		unwind_buf.priv.data.cleanup = THREAD_GETMEM (self, cleanup);

		/* Store the new cleanup handler info.  */
		THREAD_SETMEM (self, cleanup_jmp_buf, &unwind_buf);

		/* Run the program.  */
		result = main (argc, argv, __environ);
	}
	else
	{
		/* Remove the thread-local data.  */
# ifdef SHARED
		__libc_pthread_functions.ptr__nptl_deallocate_tsd ();
# else
		extern void __nptl_deallocate_tsd (void) __attribute ((weak));
		__nptl_deallocate_tsd ();
# endif

		/* One less thread.  Decrement the counter.  If it is zero we
		   terminate the entire process.  */
		result = 0;
# ifdef SHARED
		unsigned int *const ptr = __libc_pthread_functions.ptr_nthreads;
# else
		extern unsigned int __nptl_nthreads __attribute ((weak));
		unsigned int *const ptr = &__nptl_nthreads;
# endif

		if (! atomic_decrement_and_test (ptr))
			/* Not much left to do but to exit the thread, not the process.  */
			__exit_thread (0);
	}

	exit (result);
#else
	/*
	 * Finally, invoke application's main and then exit.
	 */
	exit (main (argc, argv, __environ));
#endif
}
