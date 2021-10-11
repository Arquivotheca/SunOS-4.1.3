#ifndef lint
static char sccsid[] = "@(#)mount_tfs.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Usage:  mount_tfs [-r] fs1 fs2 ... fsN dir
 *
 * TFS mounts fs1 on fs2 on ... on dir.
 *
 * Sets up searchlinks from the directory fs1 to fs2, from fs2 to fs3, ...
 * from fsN to dir, then TFS-mounts fs1 onto dir.  Before TFS-mounting
 * fs1 onto 'dir', these two steps are taken:
 *	1) loopback mount / onto /tmp_mnt/Tfs_native
 *	2) loopback mount /tmp_mnt/Tfs_empty onto 'dir'
 *
 * The searchlink from fsN points to /tmp_mnt/Tfs_native/'dir' instead of
 * 'dir'.  The two loopback mounts prevent the tfsd from deadlocking with
 * itself.  (After the TFS mount, the tfsd will be serving filesystem
 * requests for files under 'dir'.  The back layer that the tfsd is serving
 * is also 'dir'.  We want to prevent the tfsd from waiting for itself when
 * it accesses files in 'dir'.  The first loopback mount duplicates the
 * native file system so that the native version of 'dir' can be accessed
 * through /tmp_mnt/Tfs_native/'dir'.  The second loopback mount of an
 * empty directory over 'dir' prevents the kernel from using the TFS
 * version of 'dir' when files are accessed in /tmp_mnt/Tfs_native/'dir'.)
 *
 * When the searchlinks between layers are set, searchlinks are also set
 * in the sub-directories of each layer.
 *
 * NOTE: this command does not interact well with NSE exec-sets.
 * If you activate an environment whose exec-set has /usr/lib as a control
 * point, and also mount_tfs a directory over /usr/lib, then the tfsd
 * will deadlock when you try to reference /usr/lib.
 *
 * The directories fs1, ..., fsN must be writeable, so that searchlinks
 * can be set in them.
 */

#include <nse/param.h>
#include <stdio.h>
#include <pwd.h>
#include <nse/util.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <sys/file.h>
#include <mntent.h>
#include <errno.h>
#include <nse/searchlink.h>
#include <nse/tfs_calls.h>
#include <nse_impl/tfs.h>
#include <nse_impl/tfs_user.h>
#include <nse/tfs_mount.h>

#define TFSD_ENV_NAME "tfs_mount"	/* Environment name given to the
					 * tfsd for TFS mounts */

bool_t		read_only;
char		*tfs_loopback_dir = "/tmp_mnt/Tfs_native";
char		*tfs_empty_dir = "/tmp_mnt/Tfs_empty";
int		tfs_major_dev;

Nse_err		do_mounts();
Nse_err		do_loopback_mounts();
Nse_err		tfs_mount();
void		get_mountpt_devid();
Nse_err		ping_tfsd();
void		find_subdirs();
Nse_err		build_subdirs();
Nse_err		build_searchlinks();
Nse_err		recursive_set_searchlinks();
Nse_err		set_searchlink();
Nse_err		make_directory();
void		unmount_all();
Nse_list	nse_stringlist_create();


main(argc, argv)
	int		argc;
	char		*argv[];
{
	char		**dir_names;
	int		*dir_owners;
	int		i;
	char		*cp;
	struct stat	statb;
	char		fname[MAXPATHLEN];
	Nse_list	mtab_list;
	Nse_list	subdir_list;
	bool_t		invoked_from_mount = FALSE;
	Nse_err		err;

	nse_set_cmdname(argv[0]);
	if (geteuid() != 0) {
		fprintf(stderr, "%s: Must be root\n", nse_get_cmdname());
		exit(1);
	}
	if (argc < 3) {
		usage();
	}
	argv++;
	argc--;
	if (options(&argc, &argv)) {
		exit (1);
	}
	if (argc < 2) {
		usage();
	}
	if (argc == 4 && NSE_STREQ(argv[2], "tfs")) {
		/*
		 * Allow this command to be exec'ed from the mount command,
		 * in which case the usage line is:
		 *	mount_tfs fsname dir tfs opts
		 */
		struct mntent	mt;

		argc -= 2;
		mt.mnt_opts = argv[3];
		read_only = (hasmntopt(&mt, MNTOPT_RO) != NULL);
		invoked_from_mount = TRUE;
	}
	/*
	 * Assert: argc == # of directories
	 *	   argv[0] contains the name of the frontmost directory
	 *	   argv[argc - 1] contains the name of the backmost directory
	 *		(the one being mounted on)
	 *
	 * First fully qualify the directory names.
	 */
	dir_names = (char **) nse_malloc((unsigned)(argc + 1) *
					 sizeof(char *));
	dir_owners = (int *) nse_malloc((unsigned)(argc + 1) * sizeof(int));
	for (i = 0; i < argc; i++) {
		if (realpath(argv[i], fname) == NULL) {
			fprintf(stderr, "%s: ", nse_get_cmdname());
			perror(argv[i]);
			exit (1);
		}
		dir_names[i] = NSE_STRDUP(fname);
	}
	dir_names[argc] = NULL;
	dir_owners[argc] = NULL;
	/*
	 * Make sure that all of the specified directories are indeed
	 * directories.
	 */
	for (i = 0; i < argc; i++) {
		if (stat(dir_names[i], &statb) < 0) {
			fprintf(stderr, "%s: ", nse_get_cmdname());
			perror(argv[i]);
			exit (1);
		}
		if (!NSE_ISDIR(&statb)) {
			fprintf(stderr, "%s: %s is not a directory\n",
				nse_get_cmdname(), argv[i]);
			exit (1);
		}
		dir_owners[i] = statb.st_uid;
	}
	/*
	 * Don't allow TFS mounts over '/'.
	 */
	for (cp = dir_names[argc - 1]; *cp == '/'; cp++) ;
	if (*cp == '\0') {
		fprintf(stderr, "%s: Cannot mount over \"/\"\n",
			nse_get_cmdname());
		exit (1);
	}
	if (err = nse_mtab_read(&mtab_list)) {
		nse_err_print(err);
		exit (1);
	}
	/*
	 * Build list of sub-TFS mounts that will be covered by this TFS
	 * mount.  This list will be used by build_subdirs() to make the
	 * subdirectories under the new TFS mount so that set_searchlink()
	 * can set searchlinks to the directories currently TFS-mounted on
	 * the subdirectories.  This allows the following sequence to work:
	 * 1) TFS mount /foo on /usr/src/sys; 2) TFS mount /bar on
	 * /usr/src, such that the first TFS mount shows through under
	 * /usr/src/sys.
	 */
	subdir_list = nse_stringlist_create();
	nse_list_iterate(mtab_list, find_subdirs, dir_names[argc - 1],
			 subdir_list);
	/*
	 * Build searchlinks between all the directories 'fs1' ... 'fsN',
	 * 'dir'.  The searchlink from 'fsN' is set to point to
	 * /tmp_mnt/Tfs_native/'dir'.
	 */
	for (i = 0; i < argc - 1; i++) {
		/*
		 * Don't want to build searchlinks as root, because root
		 * may have inadequate permissions.
		 */
		setregid(0, group_of_user(dir_owners[i]));
		setreuid(0, dir_owners[i]);
		err = (Nse_err) nse_list_iterate_or(subdir_list, build_subdirs,
						    dir_names[i]);
		if (!err) {
			err = build_searchlinks(dir_names[i], dir_names[i + 1],
						(i == argc - 2));
		}
		setreuid(0, 0);
		setregid(0, 0);
		if (err) {
			nse_err_print(err);
			exit (1);
		}
	}
	err = do_mounts(dir_names[0], dir_names[argc - 1], invoked_from_mount);
	if (err) {
		nse_err_print(err);
		exit (1);
	} else {
		exit (0);
	}
}


Nse_err
do_mounts(fsname, dir, invoked_from_mount)
	char		*fsname;
	char		*dir;
	bool_t		invoked_from_mount;
{
	Nse_list	mtab_list;
	struct mntent	*mnt;
	int		old_mask;
	Nse_err		err;

	if (err = ping_tfsd()) {
		nse_err_print(err);
		exit (1);
	}
	mtab_list = nse_mtablist_create();
	/*
	 * Block signals while the mounts are taking place, because if
	 * the process is killed between the loopback mount over 'dir'
	 * and the TFS mount over 'dir', 'dir' will be left empty.
	 */
	old_mask = sigblock(sigmask(SIGINT) | sigmask(SIGTERM) |
			    sigmask(SIGQUIT));
	if ((err = do_loopback_mounts(dir, mtab_list)) ||
	    (err = tfs_mount(fsname, dir, &mnt))) {
		unmount_all(mtab_list);
		(void) sigsetmask(old_mask);
		return err;
	}
	/*
	 * Don't add mtab entry for the TFS mount to /etc/mtab if command
	 * was invoked from /etc/mount, because /etc/mount will add this
	 * entry itself.  
	 */
	if (!invoked_from_mount) {
		get_mountpt_devid(mnt);
		(void) nse_list_add_new_data(mtab_list, mnt);
	}
	if (err = nse_mtab_add(mtab_list)) {
		fprintf(stderr, "%s: Warning: ", nse_get_cmdname());
		nse_err_print(err);
	}
	(void) sigsetmask(old_mask);
	return NULL;
}


/*
 * Make the loopback mounts: 1) / onto /tmp_mnt/Tfs_native, and
 * 2) /tmp_mnt/Tfs_empty onto 'covered_dir'.
 */
Nse_err
do_loopback_mounts(covered_dir, mtab_list)
	char		*covered_dir;
	Nse_list	mtab_list;
{
	struct stat	statb;
	int		rootino;
	int		rootdev;
	struct mntent	*mnt;
	Nse_err		err;

	if ((err = make_directory(tfs_loopback_dir)) ||
	    (err = make_directory(tfs_empty_dir))) {
		return err;
	}
	if (stat("/", &statb) < 0) {
		return nse_err_format_errno("Stat of \"/\"");
	}
	rootino = statb.st_ino;
	rootdev = statb.st_dev;
	if (stat(tfs_loopback_dir, &statb) < 0) {
		return nse_err_format_errno("Stat of \"%s\"",
					    tfs_loopback_dir);
	}
	if (statb.st_ino != rootino || statb.st_dev != rootdev) {
		if (err = nse_mount_loopback("/", tfs_loopback_dir, FALSE,
					     &mnt)) {
			return err;
		}
		get_mountpt_devid(mnt);
		(void) nse_list_add_new_data(mtab_list, mnt);
	}
	if (err = nse_mount_loopback(tfs_empty_dir, covered_dir, FALSE,
				     &mnt)) {
		return err;
	}
	get_mountpt_devid(mnt);
	(void) nse_list_add_new_data(mtab_list, mnt);
	return NULL;
}


Nse_err
tfs_mount(fsname, dir, mntp)
	char		*fsname;
	char		*dir;
	struct mntent	**mntp;
{
	Tfs_mount_args_rec ma;

	ma.environ = TFSD_ENV_NAME;
	ma.tfs_mount_pt = dir;
	ma.directory = fsname;
	ma.hostname = _nse_hostname();
	ma.pid = 1;
	ma.writeable_layers = 1;
	ma.back_owner = -1;
	ma.back_read_only = FALSE;
	ma.default_view = "";
	ma.conditional_view = "";
	return nse_tfs_mount(&ma, dir, _nse_hostname(), read_only, mntp,
			     TFS_VERSION);
}


/*
 * Get the device # of the new mountpoint and put it into the mount option
 * string.
 */
void
get_mountpt_devid(mnt)
	struct mntent	*mnt;
{
	struct stat	statb;
	char		opts[100];

	if (stat(mnt->mnt_dir, &statb) == 0) {
		sprintf(opts, "%s,%s=%04x", mnt->mnt_opts, MNTINFO_DEV,
			statb.st_dev & 0xffff);
		free(mnt->mnt_opts);
		mnt->mnt_opts = NSE_STRDUP(opts);
	}
}


/*
 * Ping the tfsd and make sure it's alive.  This ensures that the tfsd is
 * alive before a loopback mount can cover a directory whose contents may
 * be necessary to start the tfsd.  E.g., if the tfsd is being started
 * from /usr/etc, and the user wants to TFS mount a directory onto /usr/etc,
 * then the loopback mount of /tmp_mnt/Tfs_empty onto /usr/etc will be
 * made before the TFS mount, and /usr/etc will be empty until the TFS
 * mount is done.
 */
Nse_err
ping_tfsd()
{
	return nse_tfs_rpc_call_to_host(_nse_hostname(), RFS_NULL,
					xdr_void, (char *) NULL,
					xdr_void, (char *) NULL);
}


/*
 * If the mtab entry 'mnt' is for a TFS mount on a subdirectory of 'path',
 * add it to subdir_list.  subdir_list is built so that a TFS mount over
 * a parent directory of an existing TFS mount will not obscure the
 * existing TFS mount.
 */
void
find_subdirs(mnt, path, subdir_list)
	struct mntent	*mnt;
	char		*path;
	Nse_list	subdir_list;
{
	struct stat	statb;
	int		len = strlen(path);
	char		*dir;

	if (!NSE_STREQ(mnt->mnt_type, "tfs")) {
		return;
	}
	if (tfs_major_dev == 0 && stat(mnt->mnt_dir, &statb) == 0) {
		tfs_major_dev = statb.st_dev >> 8;
	}
	if (0 == strncmp(mnt->mnt_dir, path, len) &&
	    mnt->mnt_dir[len] == '/') {
		dir = mnt->mnt_dir + len + 1;
		while (*dir == '/') {
			dir++;
		}
		(void) nse_list_add_new_copy(subdir_list, dir);
	}
}


Nse_err
build_subdirs(subdir, dir)
	char		*subdir;
	char		*dir;
{
	char		path[MAXPATHLEN];

	strcpy(path, dir);
	nse_pathcat(path, subdir);
	return make_directory(path);
}


/*
 * Set searchlinks from the directory 'from' to the directory 'to', so
 * that when 'from' is TFS-mounted, the tfsd will be able to follow the
 * searchlinks to make it appear to the user that the contents of 'from'
 * are overlaid over the contents of 'to'.  Recursively set searchlinks
 * for all sub-directories of 'from' which have matching sub-directories
 * under 'to'.  'backmost' is TRUE if 'to' is the layer furthest in back
 * (i.e. the directory being mounted on.)
 */
Nse_err
build_searchlinks(from, to, backmost)
	char		*from;
	char		*to;
	bool_t		backmost;
{
	char		dir[MAXPATHLEN];
	char		searchlink[MAXPATHLEN];
	char		*str;
	Nse_err		err;

	/*
	 * First make sure that the directory is writeable, so that we can
	 * print a decent error message if it isn't.  (If the directory isn't
	 * writeable, then we won't be able to write .tfs_info to set
	 * searchlinks.)  The directory will be unwriteable if either the
	 * user doesn't have write permissions for the directory or the
	 * directory is in a filesystem that was mounted read-only.
	 */
	strcpy(dir, from);
	nse_pathcat(dir, ".tfs_tmp");
	if (open(dir, O_RDWR | O_CREAT, 0666) < 0) {
		err = nse_err_alloc_rec(errno, &str);
		sprintf(str, "%s is not writeable", from);
		return err;
	}
	if (unlink(dir) < 0) {
		fprintf(stderr, "%s: Warning: can't unlink %s\n",
			nse_get_cmdname(), dir);
	}
	strcpy(dir, from);
	strcpy(searchlink, to);
	return recursive_set_searchlinks(dir, searchlink, backmost);
}


Nse_err
recursive_set_searchlinks(path, searchlink, backmost)
	char		*path;
	char		*searchlink;
{
	DIR		*dirp;
	struct direct	*dp;
	struct stat	statb;
	char		*curname;
	Nse_err		err;

	dirp = opendir(path);
	if (dirp == NULL) {
		return nse_err_format_errno("Opendir of '%s'\n", path);
	}
	while (dp = readdir(dirp)) {
		if (NSE_STREQ(dp->d_name, ".") ||
		    NSE_STREQ(dp->d_name, "..") ||
		    0 == strncmp(dp->d_name, NSE_TFS_FILE_PREFIX,
				 NSE_TFS_FILE_PREFIX_LEN)) {
			continue;
		}
		nse_pathcat(path, dp->d_name);
		if (lstat(path, &statb) < 0) {
			closedir(dirp);
			return nse_err_format_errno("Lstat of '%s'\n", path);
		}
		if (NSE_ISDIR(&statb)) {
			nse_pathcat(searchlink, dp->d_name);
			closedir(dirp);
			if (err = recursive_set_searchlinks(path,
							    searchlink,
							    backmost)) {
				return err;
			}
			curname = rindex(path, '/');
			*curname++ = '\0';
			*rindex(searchlink, '/') = '\0';
			dirp = opendir(path);
			if (dirp == NULL) {
				return nse_err_format_errno(
						"Opendir of '%s'\n", path);
			}
			/*
			 * Pick up where we left off.
			 */
			while (dp = readdir(dirp)) {
				if (NSE_STREQ(curname, dp->d_name)) {
					break;
				}
			}
		} else {
			*rindex(path, '/') = '\0';
		}
	}
	closedir(dirp);
	return set_searchlink(path, searchlink, backmost);
}


/*
 * Sets searchlink of directory 'path' to point to directory 'back_path'.
 * Doesn't set searchlink if 'back_path' is a non-existent directory.
 */
Nse_err
set_searchlink(path, back_path, backmost)
	char		*path;
	char		*back_path;
	bool_t		backmost;
{
	struct stat	statb;
	char		old_slink[MAXPATHLEN];
	char		back_slink[MAXPATHLEN];
	char		*searchlink = back_path;
	bool_t		ok_to_set;
	bool_t		is_tfs_dir;
	int		euid;
	Nse_err		err;

	if ((err = nse_get_searchlink(path, old_slink)) &&
	    err->code != ENOENT) {
		return err;
	}
	ok_to_set = (stat(back_path, &statb) == 0 && NSE_ISDIR(&statb));
	if (ok_to_set && backmost) {
		/*
		 * Determine if the directory in back is under an existing
		 * TFS mount.  (TFS mounts have a fixed major dev #.)
		 */
		is_tfs_dir = ((statb.st_dev >> 8) == tfs_major_dev);
		if (is_tfs_dir) {
			/*
			 * If we're doing a TFS mount over an existing TFS
			 * mount, then the searchlink should be set to the
			 * tfsd's current frontmost layer for this dir,
			 * which is the name returned by nse_tfs_getname().
			 * Make the RPC call to the tfsd as root, not as
			 * the current effective user, so that the RPC
			 * client handle can be used by root later to send
			 * the TFS_MOUNT request to the tfsd.
			 */
			euid = geteuid();
			setreuid(0, 0);
			setregid(0, 0);
			err = nse_tfs_getname(TFSD_ENV_NAME, back_path,
					      back_slink);
			setregid(0, group_of_user(euid));
			setreuid(0, euid);
			if (err) {
				if (err->code == NSE_TFS_NOT_UNDER_MOUNTPT) {
					is_tfs_dir = FALSE;
				} else {
					return err;
				}
			}
		}
		if (!is_tfs_dir) {
			/*
			 * Searchlink to backmost directory points to
			 * /tmp_mnt/Tfs_native/'back_path'.  (See header
			 * comments.)
			 */
			strcpy(back_slink, tfs_loopback_dir);
			strcat(back_slink, back_path);
			/*
			 * If backmost directory contains a .tfs_info file,
			 * remove it (otherwise, if there is a searchlink
			 * in the .tfs_info, then there will be another
			 * directory in back of the 'backmost' directory.)
			 */
			nse_pathcat(back_path, NSE_TFS_FILE);
			if (stat(back_path, &statb) == 0) {
				(void) unlink(back_path);
			}
			*rindex(back_path, '/') = '\0';
		}
		searchlink = back_slink;
	}
	if (!ok_to_set) {
		if (old_slink[0] != '\0' &&
		    (err = nse_set_searchlink(path, ""))) {
			return err;
		}
		return NULL;
	}
	if (NSE_STREQ(searchlink, old_slink)) {
		return NULL;
	}
	return nse_set_searchlink(path, searchlink);
}


Nse_err
make_directory(dir)
	char		*dir;
{
	char		*cp;
	Nse_err		err;

	if (access(dir, 0) == 0) {
		return NULL;
	}
	cp = rindex(dir, '/');
	*cp = '\0';
	if (err = make_directory(dir)) {
		return err;
	}
	*cp = '/';
	if (mkdir(dir, 0775) < 0) {
		return nse_err_format_errno("Mkdir of '%s'\n", dir);
	}
	return NULL;
}


void
unmount_all(mtab_list)
	Nse_list	mtab_list;
{
	struct mntent	*mnt;
	Nse_listelem	elem;
	Nse_listelem	end;

	NSE_LIST_ITERATE_REV(mtab_list, struct mntent *, mnt, elem, end) {
		if (unmount(mnt->mnt_dir) < 0) {
			fprintf(stderr, "%s: Warning: can't unmount '%s': ",
				nse_get_cmdname(), mnt->mnt_dir);
			perror("");
		}
	}
}


int
group_of_user(uid)
	int		uid;
{
	struct passwd	*pwd;
	static int	last_uid;
	static int	last_gid;

	if (uid == last_uid) {
		return last_gid;
	}
	pwd = getpwuid(uid);
	if (pwd) {
		last_uid = uid;
		last_gid = pwd->pw_gid;
		return pwd->pw_gid;
	} else {
		last_uid = 0;
		last_gid = 0;
		return 0;
	}
}


char *
str_copy(str)
	char		*str;
{
	char		*str2;

	if (str != NULL) {
		str2 = NSE_STRDUP(str);
		return(str2);
	}
	return(NULL);
}


/*
 * Ops vector for elements of a string list.
 */
static	Nse_listops_rec string_ops= {
	NULL,
	str_copy,
	(Nse_voidfunc) free,
	NULL,
	(Nse_boolfunc) _nse_streq_func,
};

/*
 * Create and return a string list.
 */
Nse_list
nse_stringlist_create()
{
	Nse_list	list;

	list = nse_list_alloc(&string_ops);
	return list;
}


options(argcp, argvp)
	int *argcp;
	char ***argvp;
{
	char		*opts;
	int		argc;
	char		**argv;

	argc = *argcp;
	argv = *argvp;

	while (argc && **argv == '-') {
		opts = &argv[0][1];
		switch (*opts) {
		case 'r':
			read_only = TRUE;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
		argv++;
		argc--;
	}
	*argcp = argc;
	*argvp = argv;
	return (0);
}


usage()
{
	fprintf(stderr, "Usage: %s [-r] fs1 fs2 ... fsN dir\n",
		nse_get_cmdname());
	exit(1);
}
