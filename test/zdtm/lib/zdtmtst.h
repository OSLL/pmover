#ifndef _VIMITESU_H_
#define _VIMITESU_H_

#include <sys/types.h>

/* set up test */
extern void test_ext_init(int argc, char **argv);
extern void test_init(int argc, char **argv);
extern void test_init_ns(int argc, char **argv, unsigned long clone_flags, int (*fn)(int , char **));

#ifndef CLONE_NEWUTS
#define CLONE_NEWUTS 0x04000000
#endif

#ifndef CLONE_NEWIPC
#define CLONE_NEWIPC 0x08000000
#endif

/*wrapper for fork: init log offset*/
#define test_fork() test_fork_id(-1)
extern int test_fork_id(int id);
/* finish setting up the test, write out pid file, and go to background */
extern void test_daemon(void);
/* store a message to a static buffer */
extern void test_msg(const char *format, ...);
/* tell if SIGTERM hasn't been received yet */
extern int test_go(void);
/* sleep until SIGTERM is delivered */
extern void test_waitsig(void);

#include <stdint.h>

/* generate data with crc32 at the end of the buffer */
extern void datagen(uint8_t *buffer, unsigned length, uint32_t *crc);
/* check the data buffer against its crc32 */
extern int datachk(const uint8_t *buffer, unsigned length, uint32_t *crc);

/* streaming helpers */
extern int set_nonblock(int fd, int on);
extern int pipe_in2out(int infd, int outfd, uint8_t *buffer, int length);

/* command line args */
struct long_opt {
	const char *name;
	const char *type;
	const char *doc;
	int is_required;

	int (*parse_opt)(char *arg, void *value);
	void *value;
	struct long_opt *next;
};

extern void __push_opt(struct long_opt *opt);

#define TEST_OPTION(name, type, doc, is_required) 				\
	param_check_##type(name, &(name));					\
	static struct long_opt __long_opt_##name = {				\
		#name, #type, doc, is_required, parse_opt_##type, &name };	\
	static void __init_opt_##name(void) __attribute__ ((constructor));	\
	static void __init_opt_##name(void) { __push_opt(&__long_opt_##name); }

#define __param_check(name, p, type) \
	static inline type *__check_##name(void) { return(p); }

extern void parseargs(int, char **);
extern int parse_opt_bool(char *param, void *arg);
#define param_check_bool(name, p) __param_check(name, p, int)
extern int parse_opt_int(char *param, void *arg);
#define param_check_int(name, p) __param_check(name, p, int)
extern int parse_opt_uint(char *param, void *arg);
#define param_check_uint(name, p) __param_check(name, p, unsigned int)
extern int parse_opt_long(char *param, void *arg);
#define param_check_long(name, p) __param_check(name, p, long)
extern int parse_opt_ulong(char *param, void *arg);
#define param_check_ulong(name, p) __param_check(name, p, unsigned long)
extern int parse_opt_string(char *param, void *arg);
#define param_check_string(name, p) __param_check(name, p, char *)

#include <stdio.h>
#include <errno.h>

#define __stringify_1(x)        #x
#define __stringify(x)          __stringify_1(x)

/* message helpers */
extern void setup_outfile(void);
extern int test_log_init(const char *outfile, const char *suffix);
#define err(format, arg...)	\
	test_msg("ERR: %s:%d: " format " (errno = %d)\n", \
		__FILE__, __LINE__, ## arg, errno)
#define fail(format, arg...)	\
	test_msg("FAIL: %s:%d: " format " (errno = %d)\n", \
		 __FILE__, __LINE__, ## arg, errno)
#define skip(format, arg...)	\
	test_msg("SKIP: %s:%d: " format "\n", \
		 __FILE__, __LINE__, ## arg)
#define pass()	test_msg("PASS\n")

typedef struct {
	unsigned long	seed;
	int		pipes[2];
} task_waiter_t;

extern void task_waiter_init(task_waiter_t *t);
extern void task_waiter_fini(task_waiter_t *t);
extern void task_waiter_wait4(task_waiter_t *t, unsigned int lockid);
extern void task_waiter_complete(task_waiter_t *t, unsigned int lockid);
extern void task_waiter_complete_current(task_waiter_t *t);
extern int tcp_init_server(int *port);
extern int tcp_accept_server(int sock);
extern int tcp_init_client(char *servIP, unsigned short servPort);

#endif /* _VIMITESU_H_ */
