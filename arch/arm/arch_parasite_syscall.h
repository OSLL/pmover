#ifndef ARM_PARASITE_SYSCALL_H_
#define ARM_PARASITE_SYSCALL_H_

#define ARCH_SI_TRAP TRAP_BRKPT

static int __parasite_execute(struct parasite_ctl *ctl, pid_t pid, user_regs_struct_t *regs);

/*
 * Injected syscall instruction
 */

static const char code_syscall[] = {
        0x00, 0x00, 0x00, 0xef,         /* SVC #0  */
        0xf0, 0x01, 0xf0, 0xe7          /* UDF #32 */
};


/*
 * The ARM-specific parasite setup
 */

static void parasite_setup_regs(unsigned long new_ip, user_regs_struct_t *regs)
{
	regs->ARM_pc = new_ip;

	/* Avoid end of syscall processing */
	regs->ARM_ORIG_r0 = -1;

	/* Make sure flags are in known state */
	regs->ARM_cpsr &= PSR_f | PSR_s | PSR_x | PSR_T_BIT | MODE32_BIT;
}

static void *mmap_seized(struct parasite_ctl *ctl,
			 void *addr, size_t length, int prot,
			 int flags, int fd, off_t offset)
{
	user_regs_struct_t regs = ctl->regs_orig;
	void *map = NULL;
	int ret;

	regs.ARM_r7 = (unsigned long)__NR_mmap;	/* mmap		*/
	regs.ARM_r0 = (unsigned long)addr;	/* @addr	*/
	regs.ARM_r1 = (unsigned long)length;	/* @length	*/
	regs.ARM_r2 = (unsigned long)prot;	/* @prot	*/
	regs.ARM_r3 = (unsigned long)flags;	/* @flags	*/
	regs.ARM_r4 = (unsigned long)fd;	/* @fd		*/
	regs.ARM_r5 = (unsigned long)offset;	/* @offset	*/

	parasite_setup_regs(ctl->syscall_ip, &regs);

	ret = __parasite_execute(ctl, ctl->pid, &regs);
	if (ret)
		goto err;

	if ((long)regs.ARM_r0 > 0)
		map = (void *)regs.ARM_r0;
err:
	return map;
}

static int munmap_seized(struct parasite_ctl *ctl, void *addr, size_t length)
{
	user_regs_struct_t regs = ctl->regs_orig;
	int ret;

	regs.ARM_r7 = (unsigned long)__NR_munmap;	/* mmap		*/
	regs.ARM_r0 = (unsigned long)addr;		/* @addr	*/
	regs.ARM_r1 = (unsigned long)length;	        /* @length	*/

	parasite_setup_regs(ctl->syscall_ip, &regs);

	ret = __parasite_execute(ctl, ctl->pid, &regs);
	if (!ret)
		ret = (int)regs.ARM_r0;

	return ret;
}

#endif
