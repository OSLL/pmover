#ifndef ARM_RESTORER_H_
#define ARM_RESTORER_H_

// Copied from arch/arm/include/asm/sigcontext.h

struct rt_sigcontext {
	unsigned long trap_no;
	unsigned long error_code;
	unsigned long oldmask;
	unsigned long arm_r0;
	unsigned long arm_r1;
	unsigned long arm_r2;
	unsigned long arm_r3;
	unsigned long arm_r4;
	unsigned long arm_r5;
	unsigned long arm_r6;
	unsigned long arm_r7;
	unsigned long arm_r8;
	unsigned long arm_r9;
	unsigned long arm_r10;
	unsigned long arm_fp;
	unsigned long arm_ip;
	unsigned long arm_sp;
	unsigned long arm_lr;
	unsigned long arm_pc;
	unsigned long arm_cpsr;
	unsigned long fault_address;
}; 

// Copied from arch/arm/kernel/signal.c

#include <sigframe.h>

struct sigframe {
	struct rt_ucontext uc;
	unsigned long retcode[2];
};

struct rt_sigframe {
	struct rt_siginfo info;
	struct sigframe sig;
};


#define jump_to_restorer_blob						\
	asm volatile(							\
		     "ldr %%sp, %0				    \n" \
		     "ldr %%r1, %1				    \n" \
		     "ldr %%r0, %2				    \n" \
		     "bx  %%r1				            \n"	\
		     :							\
		     : "g"(new_sp),					\
		       "g"(restore_task_exec_start),			\
		       "g"(task_args)					\
		     : "sp", "r0", "r1", "memory")

#endif
