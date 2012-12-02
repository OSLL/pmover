#ifndef ARM_RESTORER_H_
#define ARM_RESTORER_H_

// Copied from arch/arm/kernel/signal.c

#define jump_to_restorer_blob(new_sp, restore_task_exec_start,		\
			      task_args)				\
	asm volatile(							\
		     "mov %%sp, %%%0				    \n" \
		     "mov %%r1, %%%1				    \n" \
		     "mov %%r0, %%%2				    \n" \
		     "bx  %%r1				            \n"	\
		     :							\
		     : "r"(new_sp),					\
		       "r"(restore_task_exec_start),			\
		       "r"(task_args)					\
		     : "sp", "r0", "r1", "memory")


static void get_core_tls(CoreEntry *core, struct task_restore_core_args *args) {
	args->tls = core->ti_arm->tls;
}

#endif
