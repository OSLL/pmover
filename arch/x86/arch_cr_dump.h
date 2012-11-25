#ifndef CR_DUMP_ARCH_H_X86_64_
#define CR_DUMP_ARCH_H_X86_64_

#define TI_SP(core) ((core)->thread_info->gpregs->sp)

#define assign_reg(dst, src, e)		dst->e = (__typeof__(dst->e))src.e
#define assign_array(dst, src, e)	memcpy(dst->e, &src.e, sizeof(src.e))

static int get_task_regs(pid_t pid, CoreEntry *core, const struct parasite_ctl *ctl)
{
	user_fpregs_struct_t fpregs	= {-1};
	user_regs_struct_t regs		= {-1};

	int ret = -1;

	pr_info("Dumping GP/FPU registers ... ");

	if (ctl)
		regs = ctl->regs_orig;
	else {
		if (ptrace(PTRACE_GETREGS, pid, NULL, &regs)) {
			pr_err("Can't obtain GP registers for %d\n", pid);
			goto err;
		}
	}

	if (ptrace(PTRACE_GETFPREGS, pid, NULL, &fpregs)) {
		pr_err("Can't obtain FPU registers for %d\n", pid);
		goto err;
	}

	/* Did we come from a system call? */
	if ((int)regs.orig_ax >= 0) {
		/* Restart the system call */
		switch ((long)(int)regs.ax) {
		case -ERESTARTNOHAND:
		case -ERESTARTSYS:
		case -ERESTARTNOINTR:
			regs.ax = regs.orig_ax;
			regs.ip -= 2;
			break;
		case -ERESTART_RESTARTBLOCK:
			regs.ax = __NR_restart_syscall;
			regs.ip -= 2;
			break;
		}
	}

	assign_reg(core->thread_info->gpregs, regs, r15);
	assign_reg(core->thread_info->gpregs, regs, r14);
	assign_reg(core->thread_info->gpregs, regs, r13);
	assign_reg(core->thread_info->gpregs, regs, r12);
	assign_reg(core->thread_info->gpregs, regs, bp);
	assign_reg(core->thread_info->gpregs, regs, bx);
	assign_reg(core->thread_info->gpregs, regs, r11);
	assign_reg(core->thread_info->gpregs, regs, r10);
	assign_reg(core->thread_info->gpregs, regs, r9);
	assign_reg(core->thread_info->gpregs, regs, r8);
	assign_reg(core->thread_info->gpregs, regs, ax);
	assign_reg(core->thread_info->gpregs, regs, cx);
	assign_reg(core->thread_info->gpregs, regs, dx);
	assign_reg(core->thread_info->gpregs, regs, si);
	assign_reg(core->thread_info->gpregs, regs, di);
	assign_reg(core->thread_info->gpregs, regs, orig_ax);
	assign_reg(core->thread_info->gpregs, regs, ip);
	assign_reg(core->thread_info->gpregs, regs, cs);
	assign_reg(core->thread_info->gpregs, regs, flags);
	assign_reg(core->thread_info->gpregs, regs, sp);
	assign_reg(core->thread_info->gpregs, regs, ss);
	assign_reg(core->thread_info->gpregs, regs, fs_base);
	assign_reg(core->thread_info->gpregs, regs, gs_base);
	assign_reg(core->thread_info->gpregs, regs, ds);
	assign_reg(core->thread_info->gpregs, regs, es);
	assign_reg(core->thread_info->gpregs, regs, fs);
	assign_reg(core->thread_info->gpregs, regs, gs);

	assign_reg(core->thread_info->fpregs, fpregs, cwd);
	assign_reg(core->thread_info->fpregs, fpregs, swd);
	assign_reg(core->thread_info->fpregs, fpregs, twd);
	assign_reg(core->thread_info->fpregs, fpregs, fop);
	assign_reg(core->thread_info->fpregs, fpregs, rip);
	assign_reg(core->thread_info->fpregs, fpregs, rdp);
	assign_reg(core->thread_info->fpregs, fpregs, mxcsr);
	assign_reg(core->thread_info->fpregs, fpregs, mxcsr_mask);

	/* Make sure we have enough space */
	BUG_ON(core->thread_info->fpregs->n_st_space != ARRAY_SIZE(fpregs.st_space));
	BUG_ON(core->thread_info->fpregs->n_xmm_space != ARRAY_SIZE(fpregs.xmm_space));
	BUG_ON(core->thread_info->fpregs->n_padding != ARRAY_SIZE(fpregs.padding));

	assign_array(core->thread_info->fpregs, fpregs,	st_space);
	assign_array(core->thread_info->fpregs, fpregs,	xmm_space);
	assign_array(core->thread_info->fpregs, fpregs,	padding);

	ret = 0;

err:
	return ret;
}


static int arch_alloc_thread_info(CoreEntry *core) {
	ThreadInfoX86 *thread_info;
	UserX86RegsEntry *gpregs;
	UserX86FpregsEntry *fpregs;

	thread_info = xmalloc(sizeof(*thread_info));
	if (!thread_info)
		goto err;
	thread_info_x86__init(thread_info);
	core->thread_info = thread_info;

	gpregs = xmalloc(sizeof(*gpregs));
	if (!gpregs)
		goto err;
	user_x86_regs_entry__init(gpregs);
	thread_info->gpregs = gpregs;

	fpregs = xmalloc(sizeof(*fpregs));
	if (!fpregs)
		goto err;
	user_x86_fpregs_entry__init(fpregs);
	thread_info->fpregs = fpregs;

	/* These are numbers from kernel */
	fpregs->n_st_space	= 32;
	fpregs->n_xmm_space	= 64;
	fpregs->n_padding	= 24;

	fpregs->st_space	= xzalloc(pb_repeated_size(fpregs, st_space));
	fpregs->xmm_space	= xzalloc(pb_repeated_size(fpregs, xmm_space));
	fpregs->padding		= xzalloc(pb_repeated_size(fpregs, padding));

	if (!fpregs->st_space || !fpregs->xmm_space || !fpregs->padding)
		goto err;

	return 0;

 err:
	return 1;
}


static void core_entry_free(CoreEntry *core)
{
	if (core) {
		if (CORE_THREAD_INFO(core)) {
			if (CORE_THREAD_INFO(core)->fpregs) {
				xfree(core->thread_info->fpregs->st_space);
				xfree(core->thread_info->fpregs->xmm_space);
				xfree(core->thread_info->fpregs->padding);
			}
			xfree(CORE_THREAD_INFO(core)->gpregs);
			xfree(CORE_THREAD_INFO(core)->fpregs);
		}
		xfree(CORE_THREAD_INFO(core));
		xfree(core->thread_core);
		xfree(core->tc);
		xfree(core->ids);
	}
}

#endif
