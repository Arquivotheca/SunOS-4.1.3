#ifndef lint
static char sccsid[] = "@(#)umount_tfs.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Usage:  umount_tfs dir
 *
 * TFS unmounts dir
 */

#include <nse/param.h>
#include <stdio.h>
#include <nse/util.h>
#include <mntent.h>
#include <nse_impl/tfs_user.h>
#include <nse/tfs_mount.h>

char		*tfs_loopback_dir = "/tmp_mnt/Tfs_native";
char		*tfs_empty_dir = "/tmp_mnt/Tfs_empty";

Nse_err		unmount_dir();
void		count_tfs_mounts();


main(argc, argv)
	int		argc;
	char		*argv[];
{
	char		dir[MAXPATHLEN];
	char		*dirname;
	Tfs_unmount_args_rec ua;
	Nse_list	mtab_list;
	Nse_list	real_mtab_list;
	int		num_tfs_mounts;
	bool_t		invoked_from_umount = FALSE;
	Nse_err		err;

	nse_set_cmdname(argv[0]);
	if (geteuid() != 0) {
		fprintf(stderr, "Must be root to use %s\n", nse_get_cmdname());
		exit(1);
	}
	if (argc == 5 && NSE_STREQ(argv[3], "tfs")) {
		/*
		 * Allow this command to be exec'ed from the umount command,
		 * in which case the usage line is:
		 *	umount_tfs fsname dir tfs opts
		 */
		dirname = argv[2];
		invoked_from_umount = TRUE;
	} else {
		if (argc != 2) {
			usage();
		}
		dirname = argv[1];
	}
	if (realpath(dirname, dir) == NULL) {
		fprintf(stderr, "%s: ", nse_get_cmdname());
		perror(dirname);
		exit (1);
	}
	mtab_list = nse_mtablist_create();
	/*
	 * Block signals while the unmounts are taking place, because if
	 * the process is killed between the unmount of the TFS mount
	 * over 'dir' and the unmount of the loopback mount over 'dir',
	 * 'dir' will be left empty.  If unmount_tfs was invoked from
	 * /etc/umount, then there's not much that we can do to prevent
	 * this situation, since /etc/umount makes one unmount() system
	 * call itself, leaving mount_tfs to unmount only the loopback
	 * mount.
	 */
	(void) sigblock(sigmask(SIGINT) | sigmask(SIGTERM) | sigmask(SIGQUIT));
	if (!invoked_from_umount &&
	    (err = unmount_dir(dir, "tfs", "tfs", mtab_list))) {
		goto error;
	}
	ua.environ = "tfs_mount";
	ua.tfs_mount_pt = dir;
	ua.hostname = _nse_hostname();
	ua.pid = 1;
	if (err = nse_tfs_unmount(&ua, _nse_hostname(), (u_long) 2)) {
		goto error;
	}
	if (err = unmount_dir(dir, "loopback", "lo", mtab_list)) {
		goto error;
	}
	if (err = nse_mtab_read(&real_mtab_list)) {
		goto error;
	}
	num_tfs_mounts = 0;
	nse_list_iterate(real_mtab_list, count_tfs_mounts, &num_tfs_mounts);
	if (num_tfs_mounts == 1) {
		if (err = unmount_dir(tfs_loopback_dir, "loopback", "lo",
					  mtab_list)) {
			fprintf(stderr, "Warning: ");
		}
		(void) rmdir(tfs_loopback_dir);
		(void) rmdir(tfs_empty_dir);
	}
error:
	if (err) {
		nse_err_print(err);
	}
	if (nse_list_nelems(mtab_list) > 0 &&
	    (err = nse_mtab_remove(mtab_list))) {
		fprintf(stderr, "Warning: ");
		nse_err_print(err);
	}
	if (err) {
		exit (1);
	}
	exit (0);
}


Nse_err
unmount_dir(dir, err_str, mnt_type, mtab_list)
	char		*dir;
	char		*err_str;
	char		*mnt_type;
	Nse_list	mtab_list;
{
	struct mntent	*mnt;

	if (unmount(dir) < 0) {
		return nse_err_format_errno("Unmount of '%s' (%s)", dir,
					    err_str);
	}
	mnt = (struct mntent *) nse_calloc(sizeof(struct mntent), 1);
	mnt->mnt_dir = dir;
	mnt->mnt_type = mnt_type;
	(void) nse_list_add_new_data(mtab_list, mnt);
	return NULL;
}


void
count_tfs_mounts(mnt, nump)
	struct mntent	*mnt;
	int		*nump;
{
	if (NSE_STREQ(mnt->mnt_fsname, tfs_empty_dir)) {
		(*nump)++;
	}
}


usage()
{
	fprintf(stderr, "Usage: %s dir\n", nse_get_cmdname());
	exit(1);
}
