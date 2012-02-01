#define _GNU_SOURCE
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>

#include "zdtmtst.h"

const char *test_doc	= "Check that aps are preserved";
const char *test_author	= "Pavel Emelianov <xemul@parallels.com>";

struct cap_hdr {
	unsigned int version;
	int pid;
};

struct cap_data {
	unsigned int	eff;
	unsigned int	prm;
	unsigned int	inh;
};

#define _LINUX_CAPABILITY_VERSION_3     0x20080522
#define _LINUX_CAPABILITY_U32S_3        2
#define CAP_CHOWN            0
#define CAP_DAC_OVERRIDE     1

int capget(struct cap_hdr *hdrp, struct cap_data *datap);
int capset(struct cap_hdr *hdrp, const struct cap_data *datap);

int main(int argc, char **argv)
{
	int pid, s_p[2], f_p[2], r_p[3];
	char res = 'x';

	test_init(argc, argv);

	pipe(s_p);
	pipe(f_p);
	pipe(r_p);

	pid = fork();
	if (pid == 0) {
		struct cap_hdr hdr;
		struct cap_data data[_LINUX_CAPABILITY_U32S_3];
		struct cap_data data_2[_LINUX_CAPABILITY_U32S_3];

		close(s_p[0]);
		close(f_p[1]);
		close(r_p[0]);

		hdr.version = _LINUX_CAPABILITY_VERSION_3;
		hdr.pid = 0;

		capget(&hdr, data);

		hdr.version = _LINUX_CAPABILITY_VERSION_3;
		hdr.pid = 0;

		data[0].eff &= ~((1 << CAP_CHOWN) | (1 << CAP_DAC_OVERRIDE));
		data[0].prm &= ~(1 << CAP_DAC_OVERRIDE);

		capset(&hdr, data);

		close(s_p[1]);

		read(f_p[0], &res, 1);
		close(f_p[0]);

		hdr.version = _LINUX_CAPABILITY_VERSION_3;
		hdr.pid = 0;

		capget(&hdr, data_2);

		if (data[0].eff != data_2[0].eff) {
			res = '1';
			goto bad;
		}
		if (data[1].eff != data_2[1].eff) {
			res = '2';
			goto bad;
		}
		if (data[0].prm != data_2[0].prm) {
			res = '3';
			goto bad;
		}
		if (data[1].prm != data_2[1].prm) {
			res = '4';
			goto bad;
		}
		if (data[0].inh != data_2[0].inh) {
			res = '3';
			goto bad;
		}
		if (data[1].inh != data_2[1].inh) {
			res = '4';
			goto bad;
		}

		res = '0';
bad:
		write(r_p[1], &res, 1);
		close(r_p[1]);
		_exit(0);
	}

	close(f_p[0]);
	close(s_p[1]);
	close(r_p[1]);

	read(s_p[0], &res, 1);
	close(s_p[0]);

	test_daemon();
	test_waitsig();

	close(f_p[1]);

	read(r_p[0], &res, 1);
	if (res == '0')
		pass();
	else
		fail("Fail: %c", res);

	return 0;
}