#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>

#include "zdtmtst.h"

const char *test_doc	= "Check that we can migrate with a named pipe, "
			"opened in WRONLY mode";
#define BUF_SIZE 256
char *filename;
TEST_OPTION(filename, string, "file name", 1);

int main(int argc, char **argv)
{
	int fd, fd1;
	struct stat st;
	mode_t mode = S_IFIFO | 0600;
	int pid;
	int chret;

	test_init(argc, argv);

	if (mknod(filename, mode, 0)) {
		err("can't make fifo \"%s\": %m\n", filename);
		exit(1);
	}

	pid = test_fork();
	if (pid < 0) {
		err("Can't fork");
		exit(1);
	}

	if (pid == 0) {
		char rbuf[BUF_SIZE];
		int res;
		fd1 = open(filename, O_RDONLY);
		if (fd1 < 0) {
			err("open(%s, O_RDONLY) Failed: %m\n", filename);
			chret = errno;
			return chret;
		}
		res = read(fd1, rbuf, 7);
		if (res < 0) {
			err("read error %s: %m\n", filename);
			chret = errno;
			return chret;
		}
		else if (res == 0) {
			err("read(%d, rbuf, 7) return 0", fd1);
			return 1;
		}
		if (close(fd1) < 0) {
			fail("can't close %d, %s: %m", fd1, filename);
			chret = errno;
			return chret;
		}

	} else {

		fd = open(filename, O_WRONLY);
		if (fd < 0) {
			err("open(%s, O_WRONLY) Failed: %m\n", filename);
			kill(pid, SIGKILL);
			wait(NULL);
			return 1;
		}

		test_daemon();
		test_waitsig();

		if (write(fd, "string", 7) == -1) {
			err("write(%d, 'string', 7) Failed: %m\n", fd);
			return 1;
		}

		wait(&chret);
		chret = WEXITSTATUS(chret);
		if (chret) {
			fail("child exited with non-zero code %d (%s)\n",
				chret, strerror(chret));
			return 1;
		}

		if (close(fd) < 0) {
			fail("can't close %d, %s: %m", fd, filename);
			return 1;
		}

		if (stat(filename, &st) < 0) {
			fail("can't stat %s: %m", filename);
			return 1;
		}

		if (st.st_mode != mode) {
			fail("%s is no longer the fifo we had", filename);
			return 1;
		}

		if (unlink(filename) < 0) {
			fail("can't unlink %s: %m", filename);
			return 1;
		}
	}

	pass();
	return 0;
}
