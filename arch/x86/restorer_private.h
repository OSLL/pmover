#ifndef X86_64_RESTORER_PRIVATE_
#define X86_64_RESTORER_PRIVATE_

struct pt_regs {
	unsigned long	r15;
	unsigned long	r14;
	unsigned long	r13;
	unsigned long	r12;
	unsigned long	bp;
	unsigned long	bx;

	unsigned long	r11;
	unsigned long	r10;
	unsigned long	r9;
	unsigned long	r8;
	unsigned long	ax;
	unsigned long	cx;
	unsigned long	dx;
	unsigned long	si;
	unsigned long	di;
	unsigned long	orig_ax;

	unsigned long	ip;
	unsigned long	cs;
	unsigned long	flags;
	unsigned long	sp;
	unsigned long	ss;
};

struct rt_sigcontext {
	unsigned long			r8;
	unsigned long			r9;
	unsigned long			r10;
	unsigned long			r11;
	unsigned long			r12;
	unsigned long			r13;
	unsigned long			r14;
	unsigned long			r15;
	unsigned long			rdi;
	unsigned long			rsi;
	unsigned long			rbp;
	unsigned long			rbx;
	unsigned long			rdx;
	unsigned long			rax;
	unsigned long			rcx;
	unsigned long			rsp;
	unsigned long			rip;
	unsigned long			eflags;
	unsigned short			cs;
	unsigned short			gs;
	unsigned short			fs;
	unsigned short			__pad0;
	unsigned long			err;
	unsigned long			trapno;
	unsigned long			oldmask;
	unsigned long			cr2;
	struct user_fpregs_entry	*fpstate;
	unsigned long			reserved1[8];
};

#include <sigframe.h>

struct rt_sigframe {
	char			*pretcode;
	struct rt_ucontext	uc;
	struct rt_siginfo	info;

	/* fp state follows here */
};

#define RT_SIGFRAME_UC(rt_sigframe) rt_sigframe->uc

#define RUN_CLONE_RESTORE_FN						\
	asm volatile(							\
		     "clone_emul:				\n"	\
		     "movq %2, %%rsi				\n"	\
		     "subq $16, %%rsi			        \n"	\
		     "movq %6, %%rdi				\n"	\
		     "movq %%rdi, 8(%%rsi)			\n"	\
		     "movq %5, %%rdi				\n"	\
		     "movq %%rdi, 0(%%rsi)			\n"	\
		     "movq %1, %%rdi				\n"	\
		     "movq %3, %%rdx				\n"	\
		     "movq %4, %%r10				\n"	\
		     "movl $"__stringify(__NR_clone)", %%eax	\n"	\
		     "syscall				        \n"	\
		     							\
		     "testq %%rax,%%rax			        \n"	\
		     "jz thread_run				\n"	\
									\
		     "movq %%rax, %0				\n"	\
		     "jmp clone_end				\n"	\
		     							\
		     "thread_run:				\n"	\
		     "xorq %%rbp, %%rbp			        \n"	\
		     "popq %%rax				\n"	\
		     "popq %%rdi				\n"	\
		     "callq *%%rax				\n"	\
									\
		     "clone_end:				\n"	\
		     : "=r"(ret)					\
		     : "g"(clone_flags),				\
		       "g"(new_sp),					\
		       "g"(&parent_tid),				\
		       "g"(&thread_args[i].pid),			\
		       "g"(args->clone_restore_fn),			\
		       "g"(&thread_args[i])				\
		     : "rax", "rdi", "rsi", "rdx", "r10", "memory")


#define ARCH_RT_SIGRETURN						\
	asm volatile(							\
		     "movq %0, %%rax				    \n"	\
		     "movq %%rax, %%rsp				    \n"	\
		     "movl $"__stringify(__NR_rt_sigreturn)", %%eax \n" \
		     "syscall					    \n"	\
		     :							\
		     : "r"(new_sp)					\
		     : "rax","rsp","memory")

#define ARCH_FAIL_CORE_RESTORE					\
	asm volatile(						\
		     "movq %0, %%rsp			    \n"	\
		     "movq 0, %%rax			    \n"	\
		     "jmp *%%rax			    \n"	\
		     :						\
		     : "r"(ret)					\
		     : "memory")


/*static int arch_restore_thread_sigframe(struct thread_restore_args *args) {
	struct rt_sigframe *rt_sigframe;
	unsigned long fsgs_base;
	int ret;

#define CPREGT1(d)	rt_sigframe->uc.uc_mcontext.d = args->gpregs.d
#define CPREGT2(d, s)	rt_sigframe->uc.uc_mcontext.d = args->gpregs.s

	rt_sigframe = (void *)args->mem_zone.rt_sigframe + 8;

	CPREGT1(r8);
	CPREGT1(r9);
	CPREGT1(r10);
	CPREGT1(r11);
	CPREGT1(r12);
	CPREGT1(r13);
	CPREGT1(r14);
	CPREGT1(r15);
	CPREGT2(rdi, di);
	CPREGT2(rsi, si);
	CPREGT2(rbp, bp);
	CPREGT2(rbx, bx);
	CPREGT2(rdx, dx);
	CPREGT2(rax, ax);
	CPREGT2(rcx, cx);
	CPREGT2(rsp, sp);
	CPREGT2(rip, ip);
	CPREGT2(eflags, flags);
	CPREGT1(cs);
	CPREGT1(gs);
	CPREGT1(fs);

	fsgs_base = args->gpregs.fs_base;
	ret = sys_arch_prctl(ARCH_SET_FS, fsgs_base);
	if (ret) {
		return ret;
		
	}

	fsgs_base = args->gpregs.gs_base;
	ret = sys_arch_prctl(ARCH_SET_GS, fsgs_base);
	if (ret) {
		return ret;
	}

	return 0;
}*/

static int restore_gpregs(struct rt_sigframe *f, UserX86RegsEntry *r)
{
	long ret;
	unsigned long fsgs_base;

#define CPREG1(d)	f->uc.uc_mcontext.d = r->d
#define CPREG2(d, s)	f->uc.uc_mcontext.d = r->s

	CPREG1(r8);
	CPREG1(r9);
	CPREG1(r10);
	CPREG1(r11);
	CPREG1(r12);
	CPREG1(r13);
	CPREG1(r14);
	CPREG1(r15);
	CPREG2(rdi, di);
	CPREG2(rsi, si);
	CPREG2(rbp, bp);
	CPREG2(rbx, bx);
	CPREG2(rdx, dx);
	CPREG2(rax, ax);
	CPREG2(rcx, cx);
	CPREG2(rsp, sp);
	CPREG2(rip, ip);
	CPREG2(eflags, flags);
	CPREG1(cs);
	CPREG1(gs);
	CPREG1(fs);

	fsgs_base = r->fs_base;
	ret = sys_arch_prctl(ARCH_SET_FS, fsgs_base);
	if (ret) {
		pr_info("SET_FS fail %ld\n", ret);
		return -1;
	}

	fsgs_base = r->gs_base;
	ret = sys_arch_prctl(ARCH_SET_GS, fsgs_base);
	if (ret) {
		pr_info("SET_GS fail %ld\n", ret);
		return -1;
	}

	return 0;
}

#endif
