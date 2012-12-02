#ifndef X86_64_RESTORER_H_
#define X86_64_RESTORER_H_

#define jump_to_restorer_blob(new_sp, restore_task_exec_start,          \
			      task_args)				\
	asm volatile(							\
		     "movq %0, %%rbx				    \n" \
		     "movq %1, %%rax				    \n" \
		     "movq %2, %%rdi				    \n" \
		     "movq %%rbx, %%rsp			       	    \n"	\
		     "callq *%%rax				    \n" \
		     :							\
		     : "g"(new_sp),					\
		       "g"(restore_task_exec_start),			\
		       "g"(task_args)					\
		     : "rsp", "rdi", "rsi", "rbx", "rax", "memory")

#endif
