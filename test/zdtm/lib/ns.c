#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/param.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <signal.h>
#include <sched.h>

#include "ns.h"

extern int pivot_root(const char *new_root, const char *put_old);
static int prepare_mntns()
{
	FILE *f;
	unsigned fs_cnt, fs_cnt_last = 0;
	char buf[1024];
	char *root;

	root = getenv("ZDTM_ROOT");
	fprintf(stderr, "'%s'\n", root);
again:
	fs_cnt = 0;
	f = fopen("/proc/self/mountinfo", "r");
	if (!f) {
		fprintf(stderr, "Can't open mountinfo");
		return -1;
	}

	while (fgets(buf, sizeof(buf), f) != NULL) {
		char *mp = buf, *end;

		mp = strchr(mp, ' ') + 1;
		mp = strchr(mp, ' ') + 1;
		mp = strchr(mp, ' ') + 1;
		mp = strchr(mp, ' ') + 1;
		end = strchr(mp, ' ');
		*end = '\0';

		if (!strcmp(mp, "/"))
			continue;
		if (!strcmp(mp, "/proc"))
			continue;
		if (root && !strcmp(mp, root))
			continue;

		fprintf(stderr, "%s\n", mp);
		if (umount(mp))
			fprintf(stderr, "%s - %m\n", mp);
		fs_cnt++;
	}

	fclose(f);

	if (fs_cnt == 0)
		goto done;

	if (fs_cnt != fs_cnt_last) {
		fs_cnt_last = fs_cnt;
		goto again;
	}

	fprintf(stderr, "Can't umount all the filesystems");
	return -1;
done:
	if (root) {
		int dfd;

		dfd = open(".", O_RDONLY);
		if (dfd == -1) {
			fprintf(stderr, "open(.) failed: %m\n");
			return -1;
		}

		if (umount("/proc")) {
			fprintf(stderr, "umount(/proc) failed: %m\n");
			return -1;
		}
		if (chdir(root)) {
			fprintf(stderr, "chdir(%s) failed: %m\n", root);
			return -1;
		}
		if (mkdir("old", 0777) && errno != EEXIST) {
			fprintf(stderr, "mkdir(old) failed: %m\n");
			return -1;
		}
		if (pivot_root(".", "./old")) {
			fprintf(stderr, "pivot_root(., ./old) failed: %m\n");
			return -1;
		}
		if (umount("./old")) {
			fprintf(stderr, "umount(./old) failed: %m\n");
			return -1;
		}
		if (mkdir("proc", 0777) && errno != EEXIST) {
			fprintf(stderr, "mkdir(proc) failed: %m\n");
			return -1;
		}
		if (mount("proc", "/proc", "proc", MS_MGC_VAL, NULL)) {
			fprintf(stderr, "mount(/proc) failed: %m\n");
			return -1;
		}
		if (fchdir(dfd)) {
			fprintf(stderr, "fchdir() failed: %m\n");
			return -1;
		}
		close(dfd);
	}
	mkdir("/dev", 0777);
	mknod("/dev/null", 0777 | S_IFCHR, makedev(1, 3));
	return 0;
}

#define NS_STACK_SIZE	4096

/* All arguments should be above stack, because it grows down */
struct ns_exec_args {
	char stack[NS_STACK_SIZE];
	char stack_ptr[0];
	int argc;
	char **argv;
	int status_pipe[2];
};

static void ns_sig_hand(int signo)
{
	int status, len = 0;
	pid_t pid;
	char buf[128] = "";

	if (signo == SIGTERM) {
		sig_received = signo;
		len = snprintf(buf, sizeof(buf), "Time to stop and check\n");
		goto write_out;
	}

	while (1) {
		pid = waitpid(-1, &status, WNOHANG);
		if (pid == 0)
			return;
		if (pid == -1) {
			if (errno == ECHILD) {
				if (sig_received)
					return;
				sig_received = signo;
				len = snprintf(buf, sizeof(buf),
						"All test processes exited\n");
			} else {
				len = snprintf(buf, sizeof(buf),
						"wait() failed: %m\n");
			}
				goto write_out;
		}
		if (status)
			fprintf(stderr, "%d return %d\n", pid, status);
	}

	return;
write_out:
	/* fprintf can't be used in a sighandler due to glibc locks */
	write(STDERR_FILENO, buf, MAX(len, sizeof(buf)));
}

#define STATUS_FD 255
int ns_exec(void *_arg)
{
	struct ns_exec_args *args = (struct ns_exec_args *) _arg;
	int ret;

	close(args->status_pipe[0]);

	setsid();

	system("ip link set up dev lo");

	ret = dup2(args->status_pipe[1], STATUS_FD);
	if (ret < 0) {
		fprintf(stderr, "dup2() failed: %m\n");
		return -1;
	}
	close(args->status_pipe[1]);
	if (prepare_mntns())
		return -1;

	setenv("ZDTM_EXE", "1", 0);
	execvp(args->argv[0], args->argv);
	fprintf(stderr, "exec(%s) failed: %m\n", args->argv[0]);
	return -1;
}

int ns_init(int argc, char **argv)
{
	struct sigaction sa = {
		.sa_handler	= ns_sig_hand,
		.sa_flags	= SA_RESTART,
	};
	int ret, fd, status_pipe = STATUS_FD;
	char buf[128];
	pid_t pid;

	ret = fcntl(status_pipe, F_SETFD, FD_CLOEXEC);
	if (ret == -1) {
		fprintf(stderr, "fcntl failed %m\n");
		exit(1);
	}

	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGTERM);
	sigaddset(&sa.sa_mask, SIGCHLD);

	if (sigaction(SIGTERM, &sa, NULL)) {
		fprintf(stderr, "Can't set SIGTERM handler: %m\n");
		exit(1);
	}
	if (sigaction(SIGCHLD, &sa, NULL)) {
		fprintf(stderr, "Can't set SIGCHLD handler: %m\n");
		exit(1);
	}

	/* Start test */
	pid = fork();
	if (pid < 0) {
		fprintf(stderr, "fork() failed: %m\n");
		exit(1);
	} else if (pid == 0) {
		ret = execvp(argv[0], argv);
		fprintf(stderr, "exec(%s) failed: %m\n", argv[0]);
		return ret;
	}
	ret = 1;
	waitpid(pid, &ret, 0);

	/* Daemonize */
	write(status_pipe, &ret, sizeof(ret));
	close(status_pipe);
	if (ret)
		return ret;

	/* suspend/resume */
	test_waitsig();

	fd = open(pidfile, O_RDONLY);
	if (fd == -1) {
		fprintf(stderr, "open(%s) failed: %m\n", pidfile);
		exit(1);
	}
	ret = read(fd, buf, sizeof(buf) - 1);
	buf[ret] = '\0';
	if (ret == -1) {
		fprintf(stderr, "read() failed: %m\n");
		exit(1);
	}

	pid = atoi(buf);
	fprintf(stderr, "kill(%d, SIGTERM)\n", pid);
	if (pid > 0)
		kill(pid, SIGTERM);

	ret = 0;
	while (ret != -1)
		ret = wait(NULL);

	exit(1);
}

void ns_create(int argc, char **argv)
{
	pid_t pid;
	int ret, status;
	struct ns_exec_args args;
	int fd;

	args.argc = argc;
	args.argv = argv;

	ret = pipe(args.status_pipe);
	if (ret) {
		fprintf(stderr, "Pipe() failed %m\n");
		exit(1);
	}
	pid = clone(ns_exec, args.stack_ptr,
			CLONE_NEWPID | CLONE_NEWNS | CLONE_NEWUTS |
			CLONE_NEWNET | CLONE_NEWIPC | SIGCHLD, &args);
	if (pid < 0) {
		fprintf(stderr, "clone() failed: %m\n");
		exit(1);
	}
	close(args.status_pipe[1]);

	status = 1;
	ret = read(args.status_pipe[0], &status, sizeof(status));
	if (ret != sizeof(status) || status)
		exit(1);
	ret = read(args.status_pipe[0], &status, sizeof(status));
	if (ret != 0)
		exit(1);

	pidfile = getenv("ZDTM_PIDFILE");
	if (pidfile == NULL)
		exit(1);
	fd = open(pidfile, O_CREAT | O_EXCL | O_WRONLY, 0666);
	if (fd == -1) {
		fprintf(stderr, "Can't create the file %s: %m\n", pidfile);
		exit(1);
	}
	if (dprintf(fd, "%d", pid) == -1) {
		fprintf(stderr, "Can't write in the file %s: %m\n", pidfile);
		exit(1);
	}
	close(fd);

	exit(0);
}

