#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/user.h>
#include <string.h>
#include <linux/limits.h>
#include <sys/time.h>
#include <time.h>

#include "zdtmtst.h"

int test_log_init(const char *fname, const char *suffix)
{
	char path[PATH_MAX];
	int logfd;

	snprintf(path, sizeof(path), "%s%s", fname, suffix);
	logfd = open(path, O_WRONLY | O_EXCL | O_CREAT | O_APPEND, 0644);
	if (logfd < 0) {
		err("Can't open file %s", fname);
		return -1;
	}

	dup2(logfd, STDERR_FILENO);
	dup2(logfd, STDOUT_FILENO);

	close(logfd);

	setbuf(stdout, NULL);
	setbuf(stderr, NULL);

	return 0;
}

void test_msg(const char *format, ...)
{
	va_list arg;
	int off = 0;
	char buf[PAGE_SIZE];
	struct timeval tv;
	struct tm *tm;

	gettimeofday(&tv, NULL);
	tm = localtime(&tv.tv_sec);
	if (tm == NULL) {
		err("localtime() failed");
	} else {
		off += strftime(buf, sizeof(buf), "%H:%M:%S", tm);
	}

	off += sprintf(buf + off, ".%.3ld: ", tv.tv_usec / 1000);
	off += sprintf(buf + off, "%5d: ", getpid());
	va_start(arg, format);
	off += vsnprintf(buf + off, sizeof(buf) - off, format, arg);
	va_end(arg);

	fprintf(stderr, buf);
}
