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
TRAPHANDLER_NOEC(trapfun0,T_DIVIDE)
TRAPHANDLER_NOEC(trapfun1,T_DEBUG)
TRAPHANDLER_NOEC(trapfun2,T_NMI)
TRAPHANDLER_NOEC(trapfun3,T_BRKPT)
TRAPHANDLER_NOEC(trapfun4,T_OFLOW)
TRAPHANDLER_NOEC(trapfun5,T_BOUND)
TRAPHANDLER_NOEC(trapfun6,T_ILLOP)
TRAPHANDLER_NOEC(trapfun7,T_DEVICE)
TRAPHANDLER(trapfun8,T_DBLFLT)
TRAPHANDLER(trapfun10,T_TSS)
TRAPHANDLER(trapfun11,T_SEGNP)
TRAPHANDLER(trapfun12,T_STACK)
TRAPHANDLER(trapfun13,T_GPFLT)
TRAPHANDLER(trapfun14,T_PGFLT)
TRAPHANDLER_NOEC(trapfun16,T_FPERR)
TRAPHANDLER_NOEC(trapfun17,T_ALIGN)
TRAPHANDLER_NOEC(trapfun18,T_MCHK)
TRAPHANDLER_NOEC(trapfun19,T_SIMDERR)
TRAPHANDLER_NOEC(trapfun48,T_SYSCALL)



.globl sysenter_handler;
.type sysenter_handler, @function;
.align 2;
sysenter_handler:
  pushl %ds
  pushl %es
  pushl %esi
  pushl %ebp
  pushl $0
  pushl %edi
  pushl %ebx
  pushl %ecx
  pushl %edx
  pushl %eax
  call saveRegisters
  call syscall
  addl $24,%esp
  popl %ebp
  popl %esi
  popl %es
  popl %ds
  movl %ebp,%ecx /*return esp*/
  movl %esi,%edx /*return value to edx*/
  sysexit
/*
 * Lab 3: Your code here for system call handling
 */


/*
 * Lab 3: Your code here for _alltraps
 */
_alltraps:
  pushl %ds
  pushl %es
  pushal
  movw $(GD_KD), %ax
  movw %ax, %ds
  movw %ax, %es
  pushl %esp
  call trap
  addl $4,%esp
