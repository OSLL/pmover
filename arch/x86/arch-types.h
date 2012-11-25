#ifndef PROCESSOR_X86_64_H_
#define PROCESSOR_X86_64_H_

#define PRIs PRIu64

typedef struct {
	unsigned int	entry_number;
	unsigned int	base_addr;
	unsigned int	limit;
	unsigned int	seg_32bit:1;
	unsigned int	contents:2;
	unsigned int	read_exec_only:1;
	unsigned int	limit_in_pages:1;
	unsigned int	seg_not_present:1;
	unsigned int	useable:1;
	unsigned int	lm:1;
} user_desc_t;

typedef struct {
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
	unsigned long	fs_base;
	unsigned long	gs_base;
	unsigned long	ds;
	unsigned long	es;
	unsigned long	fs;
	unsigned long	gs;
} user_regs_struct_t;

#define REG_IP(regs)  regs.ip
#define REG_RES(regs) regs.ax

typedef struct {
	unsigned short	cwd;
	unsigned short	swd;
	unsigned short	twd;	/* Note this is not the same as
				   the 32bit/x87/FSAVE twd */
	unsigned short	fop;
	u64		rip;
	u64		rdp;
	u32		mxcsr;
	u32		mxcsr_mask;
	u32		st_space[32];	/* 8*16 bytes for each FP-reg = 128 bytes */
	u32		xmm_space[64];	/* 16*16 bytes for each XMM-reg = 256 bytes */
	u32		padding[24];
} user_fpregs_struct_t;


#define _NSIG_BPW 64
#define TASK_SIZE (1UL << 47) - PAGE_SIZE

#define CORE_ENTRY__MARCH CORE_ENTRY__MARCH__X86_64

#define CORE_THREAD_INFO(core) core->thread_info
#define CORE_GPREGS(core) (core->thread_info->gpregs)

#ifdef CONFIG_X86_64
# define AT_VECTOR_SIZE 44
#else
# define AT_VECTOR_SIZE 22		/* Not needed at moment */
#endif

typedef uint64_t auxv_t;

#define SIGFRAME_OFFSET 8

#define UserRegsEntry UserX86RegsEntry

typedef struct {
	int __unused;
} UserFPState;

#endif
