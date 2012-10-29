#ifndef SEIZE_H_
#define SEIZE_H_

#include <sys/ptrace.h>

/* some constants for ptrace */
#ifndef PTRACE_SEIZE
# define PTRACE_SEIZE		0x4206
#endif

#ifndef PTRACE_INTERRUPT
# define PTRACE_INTERRUPT	0x4207
#endif

#define PTRACE_LISTEN		0x4208

#define PTRACE_SEIZE_DEVEL	0x80000000

#define PTRACE_EVENT_FORK	1
#define PTRACE_EVENT_VFORK	2
#define PTRACE_EVENT_CLONE	3
#define PTRACE_EVENT_EXEC	4
#define PTRACE_EVENT_VFORK_DONE	5
#define PTRACE_EVENT_EXIT	6
#define PTRACE_EVENT_STOP	128

#define PTRACE_O_TRACESYSGOOD	0x00000001
#define PTRACE_O_TRACEFORK	0x00000002
#define PTRACE_O_TRACEVFORK	0x00000004
#define PTRACE_O_TRACECLONE	0x00000008
#define PTRACE_O_TRACEEXEC	0x00000010
#define PTRACE_O_TRACEVFORKDONE	0x00000020
#define PTRACE_O_TRACEEXIT	0x00000040

#define SI_EVENT(_si_code)	(((_si_code) & 0xFFFF) >> 8)

extern int seize_task(pid_t pid, pid_t ppid, pid_t *pgid, pid_t *sid);
extern int unseize_task(pid_t pid, int state);
extern int ptrace_peek_area(pid_t pid, void *dst, void *addr, long bytes);
extern int ptrace_poke_area(pid_t pid, void *src, void *addr, long bytes);
extern int ptrace_swap_area(pid_t pid, void *dst, void *src, long bytes);
extern int ptrace_show_area(pid_t pid, void *addr, long bytes);
extern int ptrace_show_area_r(pid_t pid, void *addr, long bytes);

#endif /* SEIZE_H_ */
