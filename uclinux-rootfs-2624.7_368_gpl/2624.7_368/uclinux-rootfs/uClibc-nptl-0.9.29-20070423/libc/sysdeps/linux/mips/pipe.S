/* pipe system call for Linux/MIPS */

/*see uClibc's sh/pipe.c and glibc-2.2.4's mips/pipe.S */

#include <features.h>
#include <asm/asm.h>
#include <asm/unistd.h>
#include <asm/regdef.h>

	.globl	pipe
	.ent	pipe, 0
pipe:
#ifdef __PIC__
	.set	noreorder
	.cpload	t9
	.set	reorder
#endif
	li	v0,__NR_pipe
	syscall
	bnez	a3, 1f
	sw	v0, 0(a0)
	sw	v1, 4(a0)
	li	v0, 0
	j	ra
1:
	/* uClibc change -- start */
	move		a0,v0		/* Pass return val to C function. */
	/* uClibc change -- stop */

#ifdef __PIC__
	la	t9, __syscall_error
	jr	t9
#else
	j	__syscall_error
#endif
1:
	.end	pipe
	.size	pipe,.-pipe
libc_hidden_def(pipe)
