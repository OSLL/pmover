#include <unistd.h>
#include <fcntl.h>
#include <sys/utsname.h>

#include "util.h"
#include "crtools.h"
#include "syscall.h"
#include "namespaces.h"

static int dump_uts_string(int fd, char *str)
{
	int ret;
	u32 len;

	len = strlen(str);
	ret = write_img(fd, &len);
	if (!ret)
		ret = write_img_buf(fd, str, len);

	return ret;
}

int dump_uts_ns(int ns_pid, struct cr_fdset *fdset)
{
	int fd, ret;
	struct utsname ubuf;

	ret = switch_ns(ns_pid, CLONE_NEWUTS, "uts");
	if (ret < 0)
		return ret;

	ret = uname(&ubuf);
	if (ret < 0) {
		pr_perror("Error calling uname");
		return ret;
	}

	fd = fdset->fds[CR_FD_UTSNS];

	ret = dump_uts_string(fd, ubuf.nodename);
	if (!ret)
		ret = dump_uts_string(fd, ubuf.domainname);

	return ret;
}

static int prepare_uts_str(int fd, char *n)
{
	int ret;
	u32 len;
	char str[65], path[128];

	ret = read_img(fd, &len);
	if (ret > 0) {
		if (len >= sizeof(str)) {
			pr_err("Corrupted %s\n", n);
			return -1;
		}

		ret = read_img_buf(fd, str, len);
		if (ret < 0)
			return -1;

		str[len] = '\0';

		snprintf(path, sizeof(path),
				"/proc/sys/kernel/%s", n);
		fd = open(path, O_WRONLY);
		if (fd < 0) {
			pr_perror("Can't open %s", path);
			return -1;
		}

		pr_info("Restoring %s to [%s]\n", n, str);

		ret = write(fd, str, len);
		close(fd);
		if (ret != len) {
			pr_perror("Can't write %s to %s",
					str, path);
			return -1;
		}

		ret = 0;
	}

	return ret;
}

int prepare_utsns(int pid)
{
	int fd, ret;
	u32 len;

	fd = open_image_ro(CR_FD_UTSNS, pid);
	if (fd < 0)
		return -1;

	ret = prepare_uts_str(fd, "hostname");
	if (!ret)
		ret = prepare_uts_str(fd, "domainname");

	close(fd);
	return ret;
}

static void show_uts_string(int fd, char *n)
{
	int ret;
	u32 len;
	char str[65];

	ret = read_img(fd, &len);
	if (ret > 0) {
		if (len >= sizeof(str)) {
			pr_err("Corrupted hostname\n");
			return;
		}

		ret = read_img_buf(fd, str, len);
		if (ret < 0)
			return;

		str[len] = '\0';
		pr_info("%s: [%s]\n", n, str);
	}
}

void show_utsns(int fd)
{
	pr_img_head(CR_FD_UTSNS);
	show_uts_string(fd, "hostname");
	show_uts_string(fd, "domainname");
	pr_img_tail(CR_FD_UTSNS);
}