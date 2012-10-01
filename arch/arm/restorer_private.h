#ifndef ARM_RESTORER_PRIVATE_H_
#define ARM_RESTORER_PRIVATE_H_

#define RUN_CLONE_RESTORE_FN						\
	asm volatile(							\
		     "clone_emul:				\n"	\
		     "ldr %%r1, %2				\n"	\
		     "sub %%r1, #16			        \n"	\
		     "mov %%r0, %%%6				\n"	\
		     "str %%r0, [%%r1, #4]			\n"	\
		     "mov %%r0, %%%5				\n"	\
		     "str %%r0, [%%r1]			        \n"	\
		     "ldr %%r0, %1				\n"	\
		     "mov %%r2, %%%3				\n"	\
		     "mov %%r3, %%%4				\n"	\
		     "mov %%r7, #"__stringify(__NR_clone)"	\n"	\
		     "svc #0				        \n"	\
		     							\
		     "cmp %%r0, #0			        \n"	\
		     "beq thread_run				\n"	\
									\
		     "mov %%r0, #0				\n"	\
		     "b   clone_end				\n"	\
		     							\
		     "thread_run:				\n"	\
		     "pop { %%r1 }			        \n"	\
		     "pop { %%r0 }			        \n"	\
		     "bx  %%r1				        \n"	\
									\
		     "clone_end:				\n"	\
		     : "=r"(ret)					\
		     : "g"(clone_flags),				\
		       "g"(new_sp),					\
		       "r"(&parent_tid),				\
		       "r"(&thread_args[i].pid),			\
		       "r"(args->clone_restore_fn),			\
		       "r"(&thread_args[i])				\
		     : "r0", "r1", "r2", "r3", "memory")


#define ARCH_RT_SIGRETURN						\
	asm volatile(							\
		     "mov %%sp, %0				    \n"	\
		     "mov %%r7,  #"__stringify(__NR_rt_sigreturn)"  \n" \
		     "svc #0					    \n"	\
		     :							\
		     : "r"(new_sp)					\
		     : "sp","memory")

#define ARCH_FAIL_CORE_RESTORE					\
	asm volatile(						\
		     "mov %%sp, %0			    \n"	\
		     "mov %%r0, 0			    \n"	\
		     "bx  %%r0			            \n"	\
		     :						\
		     : "r"(ret)					\
		     : "memory")


static int arch_restore_thread_sigframe(struct thread_restore_args *args) {
	struct rt_sigframe *rt_sigframe;

#define CPREGT1(d)	rt_sigframe->sig.uc.uc_mcontext.arm_##d = args->gpregs.d
#define CPREGT2(d, s)	rt_sigframe->sig.uc.uc_mcontext.d = args->gpregs.s

	rt_sigframe = (void *)args->mem_zone.rt_sigframe + 8;

	CPREGT1(r0);
	CPREGT1(r1);
	CPREGT1(r2);
	CPREGT1(r3);
	CPREGT1(r4);
	CPREGT1(r5);
	CPREGT1(r6);
	CPREGT1(r7);
	CPREGT1(r8);
	CPREGT1(r9);
	CPREGT1(r10);
	CPREGT1(fp);
	CPREGT1(ip);
	CPREGT1(sp);
	CPREGT1(lr);
	CPREGT1(pc);
	CPREGT1(cpsr);

	return 0;
}


static int arch_restore_task_sigframe(struct task_restore_core_args *args) {
	struct rt_sigframe *rt_sigframe;

	rt_sigframe = (void *)args->mem_zone.rt_sigframe + 8;

#define CPREG1(d)	rt_sigframe->sig.uc.uc_mcontext.arm_##d = args->gpregs.d
#define CPREG2(d, s)	rt_sigframe->sig.uc.uc_mcontext.d = args->gpregs.s

	CPREG1(r0);
	CPREG1(r1);
	CPREG1(r2);
	CPREG1(r3);
	CPREG1(r4);
	CPREG1(r5);
	CPREG1(r6);
	CPREG1(r7);
	CPREG1(r8);
	CPREG1(r9);
	CPREG1(r10);
	CPREG1(fp);
	CPREG1(ip);
	CPREG1(sp);
	CPREG1(lr);
	CPREG1(pc);
	CPREG1(cpsr);

	/*
	 * Blocked signals.
	 */
	rt_sigframe->sig.uc.uc_sigmask.sig[0] = args->blk_sigset;

	return 0;
}

#endif
