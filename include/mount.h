#ifndef MOUNT_H__
#define MOUNT_H__

extern int mntns_root;
int mntns_collect_root(pid_t pid);

struct proc_mountinfo;

extern int open_mount(unsigned int s_dev);
extern int collect_mount_info(void);
extern struct fstype *find_fstype_by_name(char *fst);

struct cr_fdset;
extern int dump_mnt_ns(int pid, struct cr_fdset *);
struct cr_options;
extern void show_mountpoints(int fd, struct cr_options *);
int prepare_mnt_ns(int pid);

extern int pivot_root(const char *new_root, const char *put_old);
#endif /* MOUNT_H__ */
