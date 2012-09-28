#ifndef X86_64_PARASITE_SYSCALL_H_
#define X86_64_PARASITE_SYSCALL_H_

#define ARCH_SI_TRAP SI_KERNEL

static int __parasite_execute(struct parasite_ctl *ctl, pid_t pid, user_regs_struct_t *regs);

/*
 * Injected syscall instruction
 */

static const char code_syscall[] = {
        0x0f, 0x05,                          /* syscall    */
        0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc   /* int 3, ... */
};


/*
 * The x86-64-specific parasite setup
 */

static void parasite_setup_regs(unsigned long new_ip, user_regs_struct_t *regs)
{
	regs->ip = new_ip;

	/* Avoid end of syscall processing */
	regs->orig_ax = -1;

	/* Make sure flags are in known state */
	regs->flags &= ~(X86_EFLAGS_TF | X86_EFLAGS_DF | X86_EFLAGS_IF);
}

static void *mmap_seized(struct parasite_ctl *ctl,
			 void *addr, size_t length, int prot,
			 int flags, int fd, off_t offset)
{
	user_regs_struct_t regs = ctl->regs_orig;
	void *map = NULL;
	int ret;

	regs.ax = (unsigned long)__NR_mmap;	/* mmap		*/
	regs.di = (unsigned long)addr;		/* @addr	*/
	regs.si = (unsigned long)length;	/* @length	*/
	regs.dx = (unsigned long)prot;		/* @prot	*/
	regs.r10= (unsigned long)flags;		/* @flags	*/
	regs.r8 = (unsigned long)fd;		/* @fd		*/
	regs.r9 = (unsigned long)offset;	/* @offset	*/

	parasite_setup_regs(ctl->syscall_ip, &regs);

	ret = __parasite_execute(ctl, ctl->pid, &regs);
	if (ret)
		goto err;

	if ((long)regs.ax > 0)
		map = (void *)regs.ax;
err:
	return map;
}

static int munmap_seized(struct parasite_ctl *ctl, void *addr, size_t length)
{
	user_regs_struct_t regs = ctl->regs_orig;
	int ret;

	regs.ax = (unsigned long)__NR_munmap;	/* mmap		*/
	regs.di = (unsigned long)addr;		/* @addr	*/
	regs.si = (unsigned long)length;	/* @length	*/

	parasite_setup_regs(ctl->syscall_ip, &regs);

	ret = __parasite_execute(ctl, ctl->pid, &regs);
	if (!ret)
		ret = (int)regs.ax;

	return ret;
}

#endif
