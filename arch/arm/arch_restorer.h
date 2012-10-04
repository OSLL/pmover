#ifndef ARM_RESTORER_H_
#define ARM_RESTORER_H_

// Copied from arch/arm/kernel/signal.c

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

static void get_core_fpstate(CoreEntry *core, struct task_restore_core_args *args) {
	builtin_memcpy(&args->fpstate.vfp.fpregs, core->ti_arm->fpstate->vfp_regs, sizeof(args->fpstate.vfp.fpregs));
	args->fpstate.vfp.fpscr = core->ti_arm->fpstate->fpscr;
}

static void get_core_tls(CoreEntry *core, struct task_restore_core_args *args) {
	args->tls = core->ti_arm->tls;
}

#endif
