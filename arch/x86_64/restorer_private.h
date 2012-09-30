#ifndef X86_64_RESTORER_PRIVATE_
#define X86_64_RESTORER_PRIVATE_

static int arch_restore_thread_sigframe(struct thread_restore_args *args) {
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
}


static int arch_restore_task_sigframe(struct task_restore_core_args *args) {
	struct rt_sigframe *rt_sigframe;
	unsigned long fsgs_base;
	int ret;

	rt_sigframe = (void *)args->mem_zone.rt_sigframe + 8;

#define CPREG1(d)	rt_sigframe->uc.uc_mcontext.d = args->gpregs.d
#define CPREG2(d, s)	rt_sigframe->uc.uc_mcontext.d = args->gpregs.s

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

	/*
	 * Blocked signals.
	 */
	rt_sigframe->uc.uc_sigmask.sig[0] = args->blk_sigset;

	return 0;
}

#endif
