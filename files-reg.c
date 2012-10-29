#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "crtools.h"

#include "mount.h"
#include "files.h"
#include "image.h"
#include "list.h"
#include "util.h"
#include "atomic.h"

#include "protobuf.h"
#include "protobuf/regfile.pb-c.h"
#include "protobuf/remap-file-path.pb-c.h"

#include "files-reg.h"

/*
 * Ghost files are those not visible from the FS. Dumping them is
 * nasty and the only way we have -- just carry its contents with
 * us. Any brave soul to implement link unlinked file back?
 */
struct ghost_file {
	struct list_head	list;
	u32			id;
	union {
		struct /* for dumping */ {
			u32	dev;
			u32	ino;
		};
		struct /* for restoring */ {
			char *path;
		};
	};
	unsigned int users;
};

static u32 ghost_file_ids = 1;
static LIST_HEAD(ghost_files);

static mutex_t *ghost_file_mutex;

/*
 * This constant is selected without any calculations. Just do not
 * want to pick up too big files with us in the image.
 */
#define MAX_GHOST_FILE_SIZE	(1 * 1024 * 1024)

static int open_remap_ghost(struct reg_file_info *rfi,
		RemapFilePathEntry *rfe)
{
	struct ghost_file *gf;
	GhostFileEntry *gfe = NULL;
	int gfd, ifd, ghost_flags;

	list_for_each_entry(gf, &ghost_files, list)
		if (gf->id == rfe->remap_id)
			goto gf_found;

	/*
	 * Ghost not found. We will create one in the same dir
	 * as the very first client of it thus resolving any
	 * issues with cross-device links.
	 */

	pr_info("Opening ghost file %#x for %s\n", rfe->remap_id, rfi->path);

	gf = shmalloc(sizeof(*gf));
	if (!gf)
		return -1;
	gf->path = xmalloc(PATH_MAX);
	if (!gf->path)
		goto err;

	ifd = open_image_ro(CR_FD_GHOST_FILE, rfe->remap_id);
	if (ifd < 0)
		goto err;

	if (pb_read_one(ifd, &gfe, PB_GHOST_FILE) < 0)
		goto err;

	snprintf(gf->path, PATH_MAX, "%s.cr.%x.ghost", rfi->path, rfe->remap_id);

	if (S_ISFIFO(gfe->mode)) {
		if (mknod(gf->path, gfe->mode, 0)) {
			pr_perror("Can't create node for ghost file\n");
			goto err;
		}
		ghost_flags = O_RDWR; /* To not block */
	} else
		ghost_flags = O_WRONLY | O_CREAT | O_EXCL;

	gfd = open(gf->path, ghost_flags, gfe->mode);
	if (gfd < 0) {
		pr_perror("Can't open ghost file");
		goto err;
	}

	if (fchown(gfd, gfe->uid, gfe->gid) < 0) {
		pr_perror("Can't reset user/group on ghost %#x\n", rfe->remap_id);
		goto err;
	}

	if (S_ISREG(gfe->mode)) {
		if (copy_file(ifd, gfd, 0) < 0)
			goto err;
	}

	ghost_file_entry__free_unpacked(gfe, NULL);
	close(ifd);
	close(gfd);

	gf->id = rfe->remap_id;
	gf->users = 0;
	list_add_tail(&gf->list, &ghost_files);
gf_found:
	gf->users++;
	rfi->ghost = gf;
	return 0;

err:
	if (gfe)
		ghost_file_entry__free_unpacked(gfe, NULL);
	xfree(gf->path);
	shfree_last(gf);
	return -1;
}

static int collect_remaps(void)
{
	int fd, ret = 0;

	fd = open_image_ro(CR_FD_REMAP_FPATH);
	if (fd < 0)
		return -1;

	while (1) {
		RemapFilePathEntry *rfe = NULL;
		struct file_desc *fdesc;
		struct reg_file_info *rfi;

		ret = pb_read_one_eof(fd, &rfe, PB_REMAP_FPATH);
		if (ret <= 0)
			break;

		ret = -1;

		if (!(rfe->remap_id & REMAP_GHOST)) {
			pr_err("Non ghost remap not supported @%#x\n",
					rfe->orig_id);
			goto tail;
		}

		fdesc = find_file_desc_raw(FD_TYPES__REG, rfe->orig_id);
		if (fdesc == NULL) {
			pr_err("Remap for non existing file %#x\n",
					rfe->orig_id);
			goto tail;
		}

		rfe->remap_id &= ~REMAP_GHOST;
		rfi = container_of(fdesc, struct reg_file_info, d);
		pr_info("Configuring remap %#x -> %#x\n", rfi->rfe->id, rfe->remap_id);
		ret = open_remap_ghost(rfi, rfe);
tail:
		remap_file_path_entry__free_unpacked(rfe, NULL);
		if (ret)
			break;
	}

	close(fd);
	return ret;
}

static int dump_ghost_file(int _fd, u32 id, const struct stat *st)
{
	int img, fd = -1;
	GhostFileEntry gfe = GHOST_FILE_ENTRY__INIT;
	char lpath[32];

	pr_info("Dumping ghost file contents (id %#x)\n", id);

	img = open_image(CR_FD_GHOST_FILE, O_DUMP, id);
	if (img < 0)
		return -1;

	gfe.uid = st->st_uid;
	gfe.gid = st->st_gid;
	gfe.mode = st->st_mode;

	if (pb_write_one(img, &gfe, PB_GHOST_FILE))
		return -1;

	if (S_ISREG(st->st_mode)) {
		/*
		 * Reopen file locally since it may have no read
		 * permissions when drained
		 */
		snprintf(lpath, sizeof(lpath), "/proc/self/fd/%d", _fd);
		fd = open(lpath, O_RDONLY);
		if (fd < 0) {
			pr_perror("Can't open ghost original file");
			return -1;
		}
		if (copy_file(fd, img, st->st_size))
			return -1;
	}

	close_safe(&fd);
	close(img);
	return 0;
}

static int dump_ghost_remap(char *path, const struct stat *st, int lfd, u32 id)
{
	struct ghost_file *gf;
	RemapFilePathEntry rpe = REMAP_FILE_PATH_ENTRY__INIT;

	pr_info("Dumping ghost file for fd %d id %#x\n", lfd, id);

	if (st->st_size > MAX_GHOST_FILE_SIZE) {
		pr_err("Can't dump ghost file %s of %lu size\n",
				path, st->st_size);
		return -1;
	}

	list_for_each_entry(gf, &ghost_files, list)
		if ((gf->dev == st->st_dev) && (gf->ino == st->st_ino))
			goto dump_entry;

	gf = xmalloc(sizeof(*gf));
	if (gf == NULL)
		return -1;

	gf->dev = st->st_dev;
	gf->ino = st->st_ino;
	gf->id = ghost_file_ids++;
	list_add_tail(&gf->list, &ghost_files);

	if (dump_ghost_file(lfd, gf->id, st))
		return -1;

dump_entry:
	rpe.orig_id = id;
	rpe.remap_id = gf->id | REMAP_GHOST;

	return pb_write_one(fdset_fd(glob_fdset, CR_FD_REMAP_FPATH),
			&rpe, PB_REMAP_FPATH);
}

static int check_path_remap(char *rpath, const struct stat *ost, int lfd, u32 id)
{
	int ret;
	struct stat pst;

	if (ost->st_nlink == 0)
		/*
		 * Unpleasant, but easy case. File is completely invisible
		 * from the FS. Just dump its contents and that's it. But
		 * be careful whether anybody still has any of its hardlinks
		 * also open.
		 */
		return dump_ghost_remap(rpath + 1, ost, lfd, id);

	ret = fstatat(mntns_root, rpath, &pst, 0);
	if (ret < 0) {
		/*
		 * FIXME linked file, but path is not accessible (unless any
		 * other error occurred). We can create a temporary link to it
		 * uning linkat with AT_EMPTY_PATH flag and remap it to this
		 * name.
		 */
		pr_perror("Can't stat path");
		return -1;
	}

	if ((pst.st_ino != ost->st_ino) || (pst.st_dev != ost->st_dev)) {
		if (opts.evasive_devices &&
		    (S_ISCHR(ost->st_mode) || S_ISBLK(ost->st_mode)) &&
		    pst.st_rdev == ost->st_rdev)
			return 0;
		/*
		 * FIXME linked file, but the name we see it by is reused
		 * by somebody else.
		 */
		pr_err("Unaccessible path opened %u:%u, need %u:%u\n",
				(int)pst.st_dev, (int)pst.st_ino,
				(int)ost->st_dev, (int)ost->st_ino);
		return -1;
	}

	/*
	 * File is linked and visible by the name it is opened by
	 * this task. Go ahead and dump it.
	 */
	return 0;
}

int dump_one_reg_file(int lfd, u32 id, const struct fd_parms *p)
{
	char fd_str[128];
	char rpath[PATH_MAX + 1] = ".", *path = rpath + 1;
	int len, rfd;

	RegFileEntry rfe = REG_FILE_ENTRY__INIT;

	snprintf(fd_str, sizeof(fd_str), "/proc/self/fd/%d", lfd);
	len = readlink(fd_str, path, sizeof(rpath) - 2);
	if (len < 0) {
		pr_perror("Can't readlink %s", fd_str);
		return len;
	}

	path[len] = '\0';
	pr_info("Dumping path for %d fd via self %d [%s]\n",
			p->fd, lfd, path);

	if (check_path_remap(rpath, &p->stat, lfd, id))
		return -1;

	rfe.id		= id;
	rfe.flags	= p->flags;
	rfe.pos		= p->pos;
	rfe.fown	= (FownEntry *)&p->fown;
	rfe.name	= path;

	rfd = fdset_fd(glob_fdset, CR_FD_REG_FILES);

	return pb_write_one(rfd, &rfe, PB_REG_FILES);
}

static const struct fdtype_ops regfile_ops = {
	.type		= FD_TYPES__REG,
	.dump		= dump_one_reg_file,
};

int dump_reg_file(struct fd_parms *p, int lfd,
			     const struct cr_fdset *cr_fdset)
{
	return do_dump_gen_file(p, lfd, &regfile_ops, cr_fdset);
}

static int open_path(struct file_desc *d,
		int(*open_cb)(struct reg_file_info *, void *), void *arg)
{
	struct reg_file_info *rfi;
	int tmp;

	rfi = container_of(d, struct reg_file_info, d);

	if (rfi->ghost) {
		mutex_lock(ghost_file_mutex);
		if (link(rfi->ghost->path, rfi->path) < 0) {
			pr_perror("Can't link %s -> %s\n",
					rfi->ghost->path, rfi->path);
			return -1;
		}
	}

	tmp = open_cb(rfi, arg);
	if (tmp < 0) {
		pr_perror("Can't open file %s", rfi->path);
		return -1;
	}

	if (rfi->ghost) {
		unlink(rfi->path);
		BUG_ON(!rfi->ghost->users);
		if (--rfi->ghost->users == 0) {
			pr_info("Unlink the ghost %s\n", rfi->ghost->path);
			unlink(rfi->ghost->path);
		}
		mutex_unlock(ghost_file_mutex);
	}

	if (restore_fown(tmp, rfi->rfe->fown))
		return -1;

	return tmp;
}

int open_path_by_id(u32 id, int (*open_cb)(struct reg_file_info *, void *), void *arg)
{
	struct file_desc *fd;

	fd = find_file_desc_raw(FD_TYPES__REG, id);
	if (fd == NULL) {
		pr_perror("Can't find regfile for %#x\n", id);
		return -1;
	}

	return open_path(fd, open_cb, arg);
}

static int do_open_reg(struct reg_file_info *rfi, void *arg)
{
	int fd;

	fd = open(rfi->path, rfi->rfe->flags);
	if (fd < 0) {
		pr_perror("Can't open file on restore");
		return fd;
	}

	if (lseek(fd, rfi->rfe->pos, SEEK_SET) < 0) {
		pr_perror("Can't restore file pos");
		return -1;
	}

	return fd;
}

static int open_fe_fd(struct file_desc *fd)
{
	return open_path(fd, do_open_reg, NULL);
}

int open_reg_by_id(u32 id)
{
	return open_path_by_id(id, do_open_reg, NULL);
}

static struct file_desc_ops reg_desc_ops = {
	.type = FD_TYPES__REG,
	.open = open_fe_fd,
};

static int collect_one_regfile(void *o, ProtobufCMessage *base)
{
	struct reg_file_info *rfi = o;

	rfi->rfe = pb_msg(base, RegFileEntry);
	rfi->path = rfi->rfe->name;
	rfi->ghost = NULL;

	pr_info("Collected [%s] ID %#x\n", rfi->path, rfi->rfe->id);
	file_desc_add(&rfi->d, rfi->rfe->id, &reg_desc_ops);

	return 0;
}

int prepare_shared_reg_files(void)
{
	ghost_file_mutex = shmalloc(sizeof(*ghost_file_mutex));
	if (!ghost_file_mutex)
		return -1;

	mutex_init(ghost_file_mutex);
	return 0;
}

int collect_reg_files(void)
{
	int ret;

	ret = collect_image(CR_FD_REG_FILES, PB_REG_FILES,
			sizeof(struct reg_file_info), collect_one_regfile);
	if (!ret)
		ret = collect_remaps();

	return ret;
}
