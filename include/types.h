#ifndef CR_TYPES_H_
#define CR_TYPES_H_

#include <stdint.h>
#include <stdbool.h>

#include "bitops.h"

/* prctl */
#define ARCH_SET_GS 0x1001
#define ARCH_SET_FS 0x1002
#define ARCH_GET_FS 0x1003
#define ARCH_GET_GS 0x1004

#define FS_TLS 0
#define GS_TLS 1

/* prctl.h */
#define PR_SET_NAME		15
#define PR_GET_NAME		16

#define PR_CAPBSET_DROP		24
#define PR_GET_SECUREBITS	27
#define PR_SET_SECUREBITS	28

#define SECURE_NO_SETUID_FIXUP	2

#define PR_SET_MM		35
# define PR_SET_MM_START_CODE		1
# define PR_SET_MM_END_CODE		2
# define PR_SET_MM_START_DATA		3
# define PR_SET_MM_END_DATA		4
# define PR_SET_MM_START_STACK		5
# define PR_SET_MM_START_BRK		6
# define PR_SET_MM_BRK			7
# define PR_SET_MM_ARG_START		8
# define PR_SET_MM_ARG_END		9
# define PR_SET_MM_ENV_START		10
# define PR_SET_MM_ENV_END		11
# define PR_SET_MM_AUXV			12
# define PR_SET_MM_EXE_FILE		13

#define PR_GET_TID_ADDRESS     40

/* fcntl */
#ifndef F_LINUX_SPECIFIC_BASE
#define F_LINUX_SPECIFIC_BASE	1024
#endif
#ifndef F_SETPIPE_SZ
# define F_SETPIPE_SZ	(F_LINUX_SPECIFIC_BASE + 7)
#endif
#ifndef F_GETPIPE_SZ
# define F_GETPIPE_SZ	(F_LINUX_SPECIFIC_BASE + 8)
#endif

#ifndef F_GETOWNER_UIDS
#define F_GETOWNER_UIDS	17
#endif

#define CLONE_CHILD_USEPID      0x02000000
#define CLONE_VFORK		0x00004000

#define SIGMAX			32

#define ERESTARTSYS		512
#define ERESTARTNOINTR		513
#define ERESTARTNOHAND		514
#define ERESTART_RESTARTBLOCK	516

typedef uint64_t		u64;
typedef int64_t			s64;
typedef unsigned int		u32;
typedef signed int		s32;
typedef unsigned short		u16;
typedef signed short		s16;
typedef unsigned char		u8;
typedef signed char		s8;

#define MAJOR(dev)		((dev)>>8)

#define _LINUX_CAPABILITY_VERSION_3	0x20080522
#define _LINUX_CAPABILITY_U32S_3	2

#ifdef CONFIG_X86_64

typedef struct {
	unsigned long sig[1];
} rt_sigset_t;

struct siginfo;
typedef void rt_signalfn_t(int, struct siginfo *, void *);
typedef rt_signalfn_t *rt_sighandler_t;

typedef void rt_restorefn_t(void);
typedef rt_restorefn_t *rt_sigrestore_t;

typedef struct {
	rt_sighandler_t	rt_sa_handler;
	unsigned long	rt_sa_flags;
	rt_sigrestore_t	rt_sa_restorer;
	rt_sigset_t	rt_sa_mask;
} rt_sigaction_t;

#define _KNSIG           64
# define _NSIG_BPW      64

#define _KNSIG_WORDS     (_KNSIG / _NSIG_BPW)

typedef struct {
	unsigned long sig[_KNSIG_WORDS];
} k_rtsigset_t;

static inline void ksigfillset(k_rtsigset_t *set)
{
	int i;
	for (i = 0; i < _KNSIG_WORDS; i++)
		set->sig[i] = (unsigned long)-1;
}

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

#else /* CONFIG_X86_64 */
# error x86-32 bit mode not yet implemented
#endif /* CONFIG_X86_64 */

#define ASSIGN_TYPED(a, b) do { a = (typeof(a))b; } while (0)
#define ASSIGN_MEMBER(a,b,m) do { ASSIGN_TYPED((a)->m, (b)->m); } while (0)

#ifndef PAGE_SIZE
# define PAGE_SIZE	4096
#endif

#ifndef PAGE_MASK
# define PAGE_MASK	(~(PAGE_SIZE - 1))
#endif

/* For sys_kcmp */
enum kcmp_type {
	KCMP_FILE,
	KCMP_VM,
	KCMP_FILES,
	KCMP_FS,
	KCMP_SIGHAND,
	KCMP_IO,
	KCMP_SYSVSEM,

	KCMP_TYPES,
};

/* For UNIX sockets data */
#ifndef SCM_MAX_FD
# define SCM_MAX_FD	253
#endif

#include <fcntl.h>

#ifndef F_SETOWN_EX
#define F_SETOWN_EX	15
#define F_GETOWN_EX	16

struct f_owner_ex {
	int	type;
	pid_t	pid;
};
#endif

/* File handle */
typedef struct {
	u32 bytes;
	u32 type;
	u64 __handle[16];
} fh_t;

#endif /* CR_TYPES_H_ */
