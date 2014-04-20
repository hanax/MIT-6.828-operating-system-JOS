/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

#include <kern/picirq.h>


###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */
TRAPHANDLER(vec8, T_DBLFLT);
TRAPHANDLER(vec10, T_TSS);
TRAPHANDLER(vec11, T_SEGNP);
TRAPHANDLER(vec12, T_STACK);
TRAPHANDLER(vec13, T_GPFLT);
TRAPHANDLER(vec14, T_PGFLT);
TRAPHANDLER(vec17, T_ALIGN);

TRAPHANDLER_NOEC(vec0, T_DIVIDE);
TRAPHANDLER_NOEC(vec1, T_DEBUG);
TRAPHANDLER_NOEC(vec2, T_NMI);
TRAPHANDLER_NOEC(vec3, T_BRKPT);
TRAPHANDLER_NOEC(vec4, T_OFLOW);
TRAPHANDLER_NOEC(vec5, T_BOUND);
TRAPHANDLER_NOEC(vec6, T_ILLOP);
TRAPHANDLER_NOEC(vec7, T_DEVICE);
TRAPHANDLER_NOEC(vec16, T_FPERR);
TRAPHANDLER_NOEC(vec18, T_MCHK);
TRAPHANDLER_NOEC(vec19, T_SIMDERR);

TRAPHANDLER_NOEC(vec48, T_SYSCALL);

.data
	.globl vectors
vectors:
	.long vec0
	.long vec1
	.long vec2
	.long vec3
	.long vec4
	.long vec5
	.long vec6
	.long vec7
	.long vec8
	.long 0
	.long vec10
	.long vec11
	.long vec12
	.long vec13
	.long vec14
	.long 0
	.long vec16
	.long vec17
	.long vec18
	.long vec19
/*
 * Lab 3: Your code here for _alltraps
 */
.text
_alltraps:
	pushl %ds;
	pushl %es;
	pushal;

	movl $GD_KD, %eax;
	movw %ax, %ds;
	movw %ax, %es;

	pushl %esp;
	call trap;
