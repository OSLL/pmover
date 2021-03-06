#ifndef CR_DUMP_ARM_H_
#define CR_DUMP_ARM_H_

#define TI_SP(core) ((core)->ti_arm->gpregs->sp)

#include "../protobuf/core.pb-c.h"

#define assign_reg(dst, src, e)		dst->e = (__typeof__(dst->e))src.ARM_##e

static int get_task_regs(pid_t pid, CoreEntry *core, const struct parasite_ctl *ctl)
{
	user_regs_struct_t regs = {{-1}};
	struct user_vfp vfp;

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

	if (ptrace(PTRACE_GETFPREGS, pid, NULL, &vfp)) {
		pr_err("Can't obtain FPU registers for %d\n", pid);
		goto err;
	}

	/* Did we come from a system call? */
	if ((int)regs.ARM_ORIG_r0 >= 0) {
		/* Restart the system call */
		switch ((long)(int)regs.ARM_r0) {
		case -ERESTARTNOHAND:
		case -ERESTARTSYS:
		case -ERESTARTNOINTR:
			regs.ARM_r0 = regs.ARM_ORIG_r0;
			regs.ARM_pc -= 4;
			break;
		case -ERESTART_RESTARTBLOCK:
			regs.ARM_r0 = __NR_restart_syscall;
			regs.ARM_pc -= 4;
			break;
		}
	}


	// Save the ARM CPU state

	assign_reg(core->ti_arm->gpregs, regs, r0);
	assign_reg(core->ti_arm->gpregs, regs, r1);
	assign_reg(core->ti_arm->gpregs, regs, r2);
	assign_reg(core->ti_arm->gpregs, regs, r3);
	assign_reg(core->ti_arm->gpregs, regs, r4);
	assign_reg(core->ti_arm->gpregs, regs, r5);
	assign_reg(core->ti_arm->gpregs, regs, r6);
	assign_reg(core->ti_arm->gpregs, regs, r7);
	assign_reg(core->ti_arm->gpregs, regs, r8);
	assign_reg(core->ti_arm->gpregs, regs, r9);
	assign_reg(core->ti_arm->gpregs, regs, r10);
	assign_reg(core->ti_arm->gpregs, regs, fp);
	assign_reg(core->ti_arm->gpregs, regs, ip);
	assign_reg(core->ti_arm->gpregs, regs, sp);
	assign_reg(core->ti_arm->gpregs, regs, lr);
	assign_reg(core->ti_arm->gpregs, regs, pc);
	assign_reg(core->ti_arm->gpregs, regs, cpsr);
	core->ti_arm->gpregs->orig_r0 = regs.ARM_ORIG_r0;


	// Save the VFP state

	memcpy(CORE_THREAD_INFO(core)->fpstate->vfp_regs, &vfp.fpregs, sizeof(vfp.fpregs));
	CORE_THREAD_INFO(core)->fpstate->fpscr = vfp.fpscr;

	ret = 0;

err:
	return ret;
}

static int arch_alloc_thread_info(CoreEntry* core) {
        ThreadInfoArm *ti_arm;
        UserArmRegsEntry *gpregs;
	UserArmVfpstateEntry *fpstate;

        ti_arm = xmalloc(sizeof(*ti_arm));
        thread_info_arm__init(ti_arm);

        gpregs = xmalloc(sizeof(*gpregs));
        user_arm_regs_entry__init(gpregs);
        ti_arm->gpregs = gpregs;

	fpstate = xmalloc(sizeof(*fpstate));
	user_arm_vfpstate_entry__init(fpstate);
	fpstate->vfp_regs = xmalloc(32*sizeof(unsigned long long));
	fpstate->n_vfp_regs = 32;
	ti_arm->fpstate = fpstate;

        core->ti_arm = ti_arm;

        return 0;
}

static void core_entry_free(CoreEntry *core)
{
	if (core) {
		if (CORE_THREAD_INFO(core)) {
			if (CORE_THREAD_INFO(core)->fpstate) {
				xfree(CORE_THREAD_INFO(core)->fpstate->vfp_regs);
				xfree(CORE_THREAD_INFO(core)->fpstate);
			}
			xfree(CORE_THREAD_INFO(core)->gpregs);
		}
		xfree(CORE_THREAD_INFO(core));
		xfree(core->thread_core);
		xfree(core->tc);
		xfree(core->ids);
	}
}

#endif
