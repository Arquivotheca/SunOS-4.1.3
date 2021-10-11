#ifndef lint
#ifdef SunB1
static  char    sccsid[] = 	"@(#)installation.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static  char    sccsid[] = 	"@(#)installation.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1990 Sun Microsystems, Inc.
 */

/*
 *	Name:		installation.c
 *
 *	Description:	At this point all the information gathering is done
 *		so the dirty work of installing the system can begin.  Except
 *		for requests to change media there is no need to bother the
 *		user for anything more.  This program is usually called from
 *		suninstall
 */

#include <sys/file.h>
#include <sys/reboot.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "install.h"
#include "menu.h"




/*
 *	External functions:
 */
extern	char *		getwd();
extern	char *		sprintf();
extern	void		clean_yp();



/*
 *	Local functions:
 */
static	int		ask_to_nuke();
static	void		clean_filesys();
static	void		deselect_software();
static	void		fix_term_type();
static	char *		generic_disk_name();
static	void		init_filesys();
static	void		install_boot();
static	void		label_disks();
static	void		load_filesys();
static	void		load_server();
static	void		mk_disk_devs();
static	void		mk_media_devs();
static	void		mk_partitions();
static	void		tune_filesys();




/*
 *	Global variables:
 */
#ifdef SunB1
int		old_mac_state;			/* old mac-exempt state */
#endif /* SunB1 */
char *		progname;			/* name for error mesgs */

int      re_flag=0;             /* Re-preinstalled if re_flag=1 */



main(argc, argv)
	int		argc;
	char **		argv;
{
	arch_info	*arch;		/* architecture info buffer */
	mnt_ent		mount_list[NMOUNT];	/* mount list buffer */
	int		ret_code;		/* return code */
	sys_info	sys;			/* system info */

    char        pathname[MAXPATHLEN];
    char        cmd[MAXPATHLEN * 2];
    mnt_ent *   re_list;


#ifdef lint
	argc = argc;
#endif lint

	(void) umask(UMASK);			/* set file creation mask */

	progname = basename(argv[0]);		/* get name for error mesgs */

    if ((argc==2)&&(strcmp(argv[1],"r")==NULL)) {
         re_flag = 1;
    }


	if (!is_miniroot()) {
		menu_mesg(
		 "%s: you must be booted on the MINIROOT to run this command.",
			  progname);
		menu_abort(1);
	}

                                                /* get system info */
        ret_code = read_sys_info(SYS_INFO, &sys);
        if (ret_code != 1) {
		/*
		 *	Need to add error message for ret_code == 0
		 */
		if (ret_code == 0)
                	menu_log("%s: Error in %s.", progname, SYS_INFO);
                menu_abort(1);
        }

	ret_code = read_arch_info(ARCH_INFO, &arch);
	if (ret_code != 1) {
		/*
		 *	Need to add error message for ret_code == 0
		 */
		if (ret_code == 0)
			menu_log("%s: %s: cannot read file.", progname,
				 ARCH_INFO);

		menu_abort(1);
	}
	
	/*
	 *	if software is already loaded, and the user want to restart
	 *	the installation, ask for confirmation before starting and
	 *	reloading all that blasted software.
	 */
	if (is_software_loaded(arch)) {
		if (ask_to_nuke())
			deselect_software(arch);
		else
			exit(1);
	}

	ret_code = read_mount_list(MOUNT_LIST, mount_list);
	if (ret_code != 1) {
		/*
		 *	Need to add error message for ret_code == 0
		 */
		if (ret_code == 0)
			menu_log("%s: %s: cannot read file.", progname,
				 MOUNT_LIST);
		menu_abort(1);
	}

	menu_log("System Installation Begins:");

	init_filesys(mount_list, &sys);

	load_filesys(&sys);

	tune_filesys(arch, &sys);

	if (sys.sys_type == SYS_SERVER)
		load_server(arch, &sys);

	/*
	 *	Save suninstall files in FILE_DIR on the root mount point.
	 */
	mkdir_path(RMP_FILE_DIR);
	copy_tree(FILE_DIR, RMP_FILE_DIR);

	clean_filesys(mount_list);

	if (sys.reboot == ANS_YES) {
		switch(sys.yp_type) {
		case YP_MASTER :
			menu_log("\nRebooting the system single user.");
			menu_log(
		    "Remember to run 'ypinit -m' before booting multi-user!");
			reboot(RB_SINGLE);
			break;
		case YP_SLAVE :
			menu_log("\nRebooting the system mult-user.");
			menu_log(
		    "Remember to run 'ypinit -s <NIS-server>' after booting!");
			break;
		default :
			menu_log("\nRebooting the system.");
		}
		reboot(RB_AUTOBOOT);
	} else {
		sync();
		sync();

		menu_log("\nSystem installation completed:");

		switch(sys.yp_type) {
		case YP_MASTER :
			menu_log("\nReboot the system single user.");
			menu_log(
		    "Remember to run 'ypinit -m' before booting multi-user!");
			break;
		case YP_SLAVE :
			menu_log("\nReboot the system mult-user.");
			menu_log(
		   "Remember to run 'ypinit -s <NIS-server>' after booting!");
			break;
		default :
            if (re_flag) {
                for (re_list=mount_list;re_list->partition[0];re_list++) {
                     if (strcmp(re_list->mount_pt, "/") ==0)
                         (void) strcpy(pathname, dir_prefix());
                     else
                         (void) sprintf(pathname, "%s%s", dir_prefix(),
                                        re_list->mount_pt);
                     mkdir_path(pathname);
                     (void) sprintf(cmd,"mount /dev/%s %s > /dev/null 2>&1",
                                    re_list->partition, pathname);
                     x_system(cmd);
                }
            } else {
			menu_log(
		    "\nIf you do not wish to install a small pre-configured kernel,");
		    	menu_log(
		    "\nreboot the system and configure a kernel for the system.");
            }
		}
	}
	exit(0);
	/*NOTREACHED*/
} /* end main() */



/*
 *	Name:		clean_filesys()
 *
 *	Description:	Unmount all the filesystems and clean them with fsck.
 */

static	void
clean_filesys(mount_list)
	mnt_ent *	mount_list;
{
	char		cmd[MAXPATHLEN * 2];	/* command buffer */
	mnt_ent *	mp;			/* mount entry pointer */


	x_chdir("/");

	/*
	 *	Do not use x_system() here.  We do not care about the return
	 *	status of this command. Leave nfs or hsfs mounts.
	 */

	(void) system("umount -t 4.2 > /dev/null 2>&1");


	menu_log("Cleaning disk(s):");

#ifdef UFS_MINIROOT
/*
 *	This is a work around for the audit_event.h bug.  This bug will
 *	go away in C14.
 */
	mkdir_path("/usr/include/sys");
	(void) system("touch /usr/include/sys/audit_event.h");
#endif /* UFS_MINIROOT */

	for (mp = mount_list; mp->partition[0]; mp++) {
#ifdef SunB1
		/*
		 *	If either of the file system labels in LAB_OTHER,
		 *	then we cannot fsck the file system.
		 */
		if (mp->fs_minlab == LAB_OTHER || mp->fs_maxlab == LAB_OTHER) {
			menu_log("\tSkipping %s (/dev/%s)", mp->mount_pt,
				 mp->partition);
			continue;
		}
#endif /* SunB1 */

		menu_log("\t%s (/dev/%s)", mp->mount_pt, mp->partition);

		/*
		 *	No output redirection here.  We want the installer
		 *	to see the output.
		 */
		(void) sprintf(cmd, "fsck -p /dev/r%s", mp->partition);
		x_system(cmd);
	}
} /* end clean_filesys() */




/*
 *	Name:		fix_term_type()
 *
 *	Description:	Fix the terminal type in TTYTAB on the root mount
 *		point for the console.
 */

static	void
fix_term_type(sys_p)
	sys_info *	sys_p;
{
	char		cmd[MAXPATHLEN];	/* command buffer */
	char		*getenv();
	char		*terminal;


        /*
	 *       set ttytab
	 */
	terminal = getenv("TERM");
	if (terminal == NULL)
		terminal = sys_p->termtype;
	
	(void) sprintf(cmd,
    "sed \"s/sun/%s/\" < %s > /tmp/ttytab;cp /tmp/ttytab %s;rm /tmp/ttytab",
		       terminal, RMP_TTYTAB, RMP_TTYTAB);
	
	(void) system(cmd);

} /* end fix_term_type() */




/*
 *	Name:		generic_disk_name()
 *
 *	Description:	Get a generic disk name from the disk name.  A generic
 *		disk name is assumed to be the first alpha string in a disk
 *		name, e.g. "sd0" => "sd"
 */

static	char *
generic_disk_name(disk)
	char *		disk;
{
	char *		name_p;			/* ptr to name_buf */
	char *		end;			/* ptr to send of name_buf */
						/* the name_buf to return */
	static	char		name_buf[SMALL_STR];


	name_p = name_buf;
	end = name_buf + sizeof(name_buf) - 1;

	while (*disk && isalpha(*disk)) {
		if (name_p < end)
			*(name_p++) = *disk;

		disk++;
	}
	*name_p = NULL;

	return(name_buf);
} /* end generic_disk_name() */




/*
 *	Name:		init_filesys()
 *
 *	Description:	Dispatch point for initializing the filesystems.
 *		The following tasks are performed:
 *
 *			- label the disks
 *			- make the partitions on the disks
 */

static	void
init_filesys(mount_list, sys_p)
	mnt_ent *	mount_list;
	sys_info *	sys_p;
{
	label_disks();
	mk_partitions(mount_list, sys_p);
} /* end init_filesys() */




/*
 *	Name:		install_boot()
 *
 *	Description:	Install the boot program on the disk.
 */

static	void
install_boot(sys_p)
	sys_info *	sys_p;
{
	char		cmd[MAXPATHLEN * 2];	/* command buffer */
	char		cwd[MAXPATHLEN];	/* current working directory */
	char		buf[MEDIUM_STR];	/* aprid */


	(void) getwd(cwd);

	/*
	 *	Install the boot program
	 */
	(void) sprintf(cmd, "cp %s/usr/stand/boot.%s  %s/boot 2>> %s",
		       dir_prefix(),
		       aprid_to_iid(sys_p->arch_str, buf),
		       dir_prefix(), LOGFILE);

	x_system(cmd);

	/*
	 *	These sync() calls must be here or installboot may not work.
	 */
	sync(); sync();

	x_chdir("/usr/mdec");

	(void) sprintf(cmd, "./installboot %s/boot boot%s /dev/r%s 2>> %s",
		       dir_prefix(), generic_disk_name(sys_p->root),
		       sys_p->root, LOGFILE);
	(void) system(cmd);

	/*
	 *	These sync() calls must be here or installboot may not work.
	 */
	sync(); sync();

	x_chdir(cwd);
} /* end install_boot() */




/*
 *	Name:		label_disks()
 *
 *	Description:	Label the disks.  Uses the data in CMDFILE to label
 *		the disks.
 */

static	void
label_disks()
{
	char		buf[BUFSIZ];		/* input buffer */
	char		cmd[MAXPATHLEN * 2];	/* command buffer */
	FILE *		fp;			/* pointer to disk_list */
	char		pathname[MAXPATHLEN];	/* general pathname buffer */

	x_chdir("/");

	/*
	 *	Do not use x_system() here.  The command will fail on /usr
	 *	and we do not want to abort. Leave nfs or hsfs mounts.
	 */

	(void) system("umount -t 4.2 > /dev/null 2>&1");


	fp = fopen(DISK_LIST, "r");
	if (fp == NULL) {
		menu_log("%s: %s: %s.", progname, DISK_LIST, err_mesg(errno));
		menu_abort(1);
	}

	/*
	 *	Ignore keyboard signals while format is running.
	 */
	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGQUIT, SIG_IGN);
	(void) signal(SIGTSTP, SIG_IGN);

	menu_log("Label disk(s):");

	while (fgets(buf, sizeof(buf), fp)) {	/* read each disk name */
		delete_blanks(buf);

		(void) sprintf(pathname, "%s.%s", CMDFILE, buf);
		if (access(pathname, F_OK) != 0)
			continue;

		menu_log("\t%s", buf);

		(void) sprintf(cmd,
	     "/usr/etc/format -f %s -l %s/format.%s.log %s > /dev/null 2>> %s",
			       pathname, FILE_DIR, buf, buf, LOGFILE);

		x_system(cmd);
	}

	/*
	 *	Keyboard signals are okay here
	 */
	(void) signal(SIGINT, SIG_DFL);
	(void) signal(SIGQUIT, SIG_DFL);
	(void) signal(SIGTSTP, SIG_DFL);

	(void) fclose(fp);
} /* end label_disks() */




/*
 *	Name:		load_filesys()
 *
 *	Description:	This is function interface to add_services.  The
 *		task is to load the software for the system's architecture
 *		onto the disk.
 */

static	void
load_filesys(sys_p)
	sys_info *	sys_p;
{
	char *		argv[4];		/* arguments for execute */
	char		irid[MEDIUM_STR];	/* irid buffer */

	argv[0] = "add_services";
	argv[1] = "-b";
	argv[2] = aprid_to_irid(sys_p->arch_str, irid);
	argv[3] = NULL;

	if (execute(argv) != 1) {
		menu_log("%s: add_services failed.", progname);
		menu_log("\tCould not add %s software.", irid);
		menu_abort(1);
	}
} /* end load_filesys() */




/*
 *	Name:		load_server()
 *
 *	Description:	Load any additional software that is needed for
 *		being a server onto the disk and create the clients for
 *		each supported architecture.  Calls add_services for
 *		each required software architecture.  Calls add_client
 *		for each client.
 */

static	void
load_server(arch, sys_p)
	arch_info *	arch;
	sys_info *	sys_p;
{
	char		arch_str[MEDIUM_STR];	/* architecture string */
	char *		argv[4];		/* arguments for execute() */
	char		buf[BUFSIZ];		/* I/O buffer */
	FILE *		fp;			/* ptr to client_list.<arch> */
	char		pathname[MAXPATHLEN];	/* path to software_info */
	arch_info *	ap;			/* scratch poiter */
	char		irid[MEDIUM_STR];	/* irid buffer */

	/*
	 *	Load the software for each additional architecture
	 */
	argv[0] = "add_services";
	argv[1] = "-b";
	argv[3] = NULL;

	for (ap = arch; ap ; ap = ap->next) {
		if (strcmp(ap->arch_str, sys_p->arch_str) != 0) {

			argv[2] = aprid_to_irid(ap->arch_str, irid);

			menu_log("Becoming a server for %s's.", irid);

			if (execute(argv) != 1) {
				menu_log("%s: add_services failed.", progname);
				menu_log("\tCould not add %s software.", irid);
				menu_abort(1);
			}
		}
	} 


	/*
	 *	Add the clients for each architecture
	 */
	argv[0] = "add_client";
	argv[1] = "-b";
	argv[3] = NULL;

	for (ap = arch; ap ; ap = ap->next) {
		(void) strcpy(arch_str, ap->arch_str);
		(void) sprintf(pathname, "%s.%s", CLIENT_LIST, arch_str);

		fp = fopen(pathname, "r");
		if (fp == NULL) {
			menu_log("No %s clients to add.", os_name(arch_str));
			continue;
		}

		menu_log("Adding %s clients:", os_name(arch_str));

		while (fgets(buf, sizeof(buf), fp)) {
			delete_blanks(buf);

			menu_log("\t%s", buf);

			argv[2] = buf;

			if (execute(argv) != 1) {
				menu_log("%s: add_client failed.", progname);
				menu_log("\tCould not add %s software.",
					 os_name(arch_str));
				menu_abort(1);
			}
		}

		(void) fclose(fp);
	} 
} /* end load_server() */




/*
 *	Name:		mk_disk_devs()
 *
 *	Description:	Make the disk devices on the mounted root.  The
 *		following tasks are performed:
 *
 *			- make disk devices with MAKEDEV()
 *			- set device labels (SunB1)
 *			- fix DEVICE_GROUPS entries (SunB1)
 */

static	void
mk_disk_devs()
{
	char		buf[BUFSIZ];		/* input buffer */
#ifdef SunB1
	char		cmd[MAXPATHLEN * 2];	/* command buffer */
	disk_info	disk;			/* buffer for disk info */
#endif /* SunB1 */
	FILE *		fp;			/* pointer to disk_list */
#ifdef SunB1
	int		i;			/* index variable */
	char		label_str[TINY_STR];	/* buffer for label string */
	char		pathname[MAXPATHLEN];	/* path to disk info */
	int		ret_code;		/* return code */
#endif /* SunB1 */


	fp = fopen(DISK_LIST, "r");
	if (fp == NULL) {
		menu_log("%s: %s: %s.", progname, DISK_LIST, err_mesg(errno));
		menu_abort(1);
	}

	while (fgets(buf, sizeof(buf), fp)) {	/* read each disk name */
		delete_blanks(buf);

		MAKEDEV(dir_prefix(), buf);

#ifdef SunB1
		bzero((char *) &disk, sizeof(disk));

		(void) sprintf(pathname, "%s.%s", DISK_INFO, buf);
		ret_code = read_disk_info(pathname, &disk);
		if (ret_code != 1) {
			/*
			 *	Need to add error message for ret_code == 0
			 */
			if (ret_code == 0)
				menu_log("%s: %s: cannot read file.", progname,
					 pathname);

			menu_abort(1);
		}

		for (i = 0; i < NDKMAP; i++) {
			if (disk.partitions[i].mount_pt[0] &&
			    disk.partitions[i].fs_minlab != LAB_OTHER) {
				(void) sprintf(cmd,
				         "setlabel %s %s/dev/r%s%c >> %s 2>&1",
				  cv_lab_to_str(&disk.partitions[i].fs_minlab),
					       dir_prefix(), disk.disk_name,
					       i + 'a', LOGFILE);

				x_system(cmd);

				(void) sprintf(cmd,
				          "setlabel %s %s/dev/%s%c >> %s 2>&1",
				  cv_lab_to_str(&disk.partitions[i].fs_minlab),
					       dir_prefix(), disk.disk_name,
					       i + 'a', LOGFILE);

				x_system(cmd);
			}
		}

		/*
		 *	Must copy the minimum label since cv_lab_to_str()
		 *	uses a static buffer.
		 */
		if (disk.disk_minlab != LAB_OTHER &&
		    disk.disk_maxlab != LAB_OTHER) {
			(void) strcpy(label_str,
				      cv_lab_to_str(&disk.disk_minlab));
			fix_devgroup(disk.disk_name, label_str,
				     cv_lab_to_str(&disk.disk_maxlab),
				     DEVICE_CLEAN, dir_prefix());
		}
#endif /* SunB1 */
	}

	(void) fclose(fp);
} /* end mk_disk_devs() */




/*
 *	Name:		mk_media_devs()
 *
 *	Description:	Make all the local media devices used by each
 *		software architecture.
 */

static	void
mk_media_devs(arch)
	arch_info *	arch;
{
	char		pathname[MAXPATHLEN];	/* path to software_info */
	int		ret_code;		/* return code */
	soft_info	soft;			/* software info */
	arch_info *	ap;			/* scratch pointer */

	/*
	 *	Make the media device for each architecture.
	 */
	for (ap = arch; ap ; ap = ap->next) {
		bzero((char *) &soft, sizeof(soft));

		(void) sprintf(pathname, "%s.%s", SOFT_INFO,
			       ap->arch_str);

		ret_code = read_soft_info(pathname, &soft);
		if (ret_code != 1) {
			/*
			 *	Need to add error message for ret_code == 0
			 */
			if (ret_code == 0)
				menu_log("%s: %s: cannot read file.", progname,
					 pathname);

			menu_abort(1);
		}

		if (soft.media_loc == LOC_LOCAL)
			MAKEDEV(dir_prefix(), media_dev_name(soft.media_dev));
	} 
} /* end mk_media_devs() */




/*
 *	Name:		mk_partitions()
 *
 *	Description:	Create the partitions on the disks.  The following
 *		tasks are performed:
 *
 *			- craft an FSTAB entry and add it FSTAB_TMP
 *			- set the label of the disk device on the miniroot
 *			  so newfs or fsck can talk to it (SunB1)
 *			- run newfs or fsck as appropriate
 *			- make the mount point
 *			- set the label on the mount point (SunB1)
 *			- mount the file system
 *			- add FSTAB entries for dataless if necessary
 */

char	*fd_partition = "fd0";
char	*pcfs_mount_pt = "/pcfs";

static	void
mk_partitions(mount_list, sys_p)
	mnt_ent *	mount_list;
	sys_info *	sys_p;
{
	char		cmd[MAXPATHLEN * 2];	/* command buffer */
	int		dev_width = 0;		/* width of device field */
	FILE *		fp;			/* ptr to FSTAB_TMP */
	int		len;			/* scratch length variable */
	int		mnt_width = 0;		/* width of mount_pt field */
	mnt_ent *	mp;			/* mount entry pointer */
	int		pass = 3;		/* fsck pass number */
	char		pathname[MAXPATHLEN];	/* path to mount_list */
	char *		prefix = "";		/* prefix for FSTAB entry */
	char		sharepath[MAXPATHLEN];	/* path to share */
	char		release[MEDIUM_STR];	/* release string */


	fp = fopen(FSTAB_TMP, "w");
	if (fp == NULL) {
		menu_log("%s: %s: %s.", progname, MOUNT_LIST, err_mesg(errno));
		menu_log("\tCannot open file for writing.");
		menu_abort(1);
	}

	menu_log("Create/check filesystems:");


	/*
	 *	Setup the field widths for a dataless system
	 */
	if (sys_p->sys_type == SYS_DATALESS) {
		/*
		 *	The mystical "- 4" in this conditional is a "+ 1" for
		 *	the ":" and a "- 5" for "/dev/" thus "-4".
		 */
		dev_width =
			  strlen(sys_p->server) + strlen(sys_p->kvm_path) - 4;

		(void) sprintf(sharepath, "/export/share/%s",
			       aprid_to_rid(sys_p->arch_str, release));
		len = strlen(sys_p->server) + strlen(sharepath) - 4;
		if (len > dev_width)
			dev_width = len;

		mnt_width = strlen("/usr/share");
	}

	for (mp = mount_list; mp->partition[0]; mp++) {
		len = strlen(mp->partition);
		if (len > dev_width)
			dev_width = len;
		
		len = strlen(mp->mount_pt);
		if (len > mnt_width)
			mnt_width = len;
	}
	len = strlen(pcfs_mount_pt);
	if (len > mnt_width)
		mnt_width = len;

	/*
	 *	Now, go through all the partitions and newfs or fsck them
	 *	appropriately.
	 */
	for (mp = mount_list; mp->partition[0]; mp++) {
#ifdef SunB1
		prefix = "";
		
		/*
		 *	If either of the file system labels in LAB_OTHER,
		 *	then we cannot do much with them other than make
		 *	FSTAB entries.
		 */
		if (mp->fs_minlab == LAB_OTHER || mp->fs_maxlab == LAB_OTHER)
			prefix = "#";
#endif /* SunB1 */

		if (strcmp(mp->mount_pt, "/") == 0)
			(void) fprintf(fp, "/dev/%-*s  %-*s  %s rw 1 1\n",
				       dev_width, mp->partition, mnt_width,
				       mp->mount_pt, DEFAULT_FS);
		
		else if (strcmp(mp->mount_pt, "/usr") == 0)
			(void) fprintf(fp, "/dev/%-*s  %-*s  %s rw 1 2\n",
				       dev_width, mp->partition, mnt_width,
				       mp->mount_pt, DEFAULT_FS);
		else
			(void) fprintf(fp, "%s/dev/%-*s  %-*s  %s rw 1 %d\n",
				       prefix, dev_width, mp->partition,
				       mnt_width, mp->mount_pt, DEFAULT_FS,
				       pass++);

#ifdef SunB1
		if (*prefix == '#') {
			menu_log("\tSkipping %s (/dev/%s)", mp->mount_pt,
				 mp->partition);
			continue;
		}
		else
			(void) sprintf(cmd, "setlabel %s /dev/r%s >> %s 2>&1",
				       cv_lab_to_str(&mp->fs_minlab),
				       mp->partition,
				       LOGFILE);
		
#ifdef UFS_MINIROOT
		(void) system(cmd);
#else
		x_system(cmd);
#endif /* UFS_MINIROOT */
		
		(void) sprintf(cmd, "setlabel %s /dev/%s >> %s 2>&1",
			       cv_lab_to_str(&mp->fs_minlab), mp->partition,
			       LOGFILE);
		
#ifdef UFS_MINIROOT
		(void) system(cmd);
#else
		x_system(cmd);
#endif /* UFS_MINIROOT */
#endif /* SunB1 */
		
		if (mp->preserve == 'n') {
#ifdef SunB1
			(void) sprintf(cmd,
				      "newfs -Z \"%s:%s\" /dev/r%s >> %s 2>&1",
			               cv_lab_to_str(&mp->fs_minlab),
			               cv_lab_to_str(&mp->fs_maxlab),
#else
				       (void) sprintf(cmd,
						   "newfs /dev/r%s >> %s 2>&1",
#endif /* SunB1 */
						      mp->partition, LOGFILE);
			menu_log("Creating new filesystem for %s on %s",
						mp->mount_pt, mp->partition);
		} else {
			(void) sprintf(cmd, "fsck -p /dev/r%s >> %s 2>&1",
				       mp->partition, LOGFILE);
 			menu_log("Checking existing filesystem %s on %s",
 				mp->mount_pt, mp->partition);
		}
		menu_log("%s\n", cmd);

		/*
		 *	Don't use x_system, to print approprate messages
		 */
		if (system(cmd) != 0) {

			menu_log("%s: '%s' failed.\n", progname, cmd);

			if (mp->preserve == 'n') {
				/*
				 *	newfs error
				 */
				menu_log("%s /dev/r%s\n%s",
				  "newfs failed on partition",
					 mp->partition,
			     "Please run newfs manually to remake filesystem");
			} else {
				/*
				 *	fsck error
				 */
				menu_log("%s /dev/r%s\n%s",
				  "fsck failed on partition",
					 mp->partition,
			     "Please run fsck manually to check filesystem");
			}				
				
			/*
			 * 	Get out NOW!!!
			 */
			menu_abort(1);
		}
			
 		menu_log("\n");
		/*
		 *	If this is the root file system, then just use
		 *	dir_prefix() as the mount point.  This keeps the
		 *	extra slashes out of the mount point name.
		 */
		if (strcmp(mp->mount_pt, "/") == 0)
			(void) strcpy(pathname, dir_prefix());
		else
			(void) sprintf(pathname, "%s%s", dir_prefix(),
				       mp->mount_pt);
		mkdir_path(pathname);
		if (strcmp(mp->mount_pt, "/tmp") == 0) {
		(void) sprintf(cmd, "chmod 3777 %s", pathname);
		(void) system(cmd);
		}

#ifdef SunB1
		(void) sprintf(cmd, "setlabel %s %s%s >> %s 2>&1",
			       cv_lab_to_str(&mp->fs_minlab), dir_prefix(),
			       mp->mount_pt, LOGFILE);
#ifdef UFS_MINIROOT
		(void) system(cmd);
#else
		x_system(cmd);
#endif /* UFS_MINIROOT */
#endif /* SunB1 */

		(void) sprintf(cmd, "mount /dev/%s %s > /dev/null 2>&1",
			       mp->partition, pathname);
		x_system(cmd);
		if (strcmp(mp->mount_pt, "/tmp") == 0) {
		(void) sprintf(cmd, "chmod 3777 %s", pathname);
		(void) system(cmd);
		(void) sprintf(cmd, "chmod g+s %s", pathname);
		(void) system(cmd);
		}
	}

	/*
	 *	Added fstab entry for PCFS
	 *	/dev/fd0	/pcfs	pcfs rw,noauto 0 0
	 */
	(void) fprintf(fp, "/dev/%-*s  %-*s  pcfs rw,noauto 0 0\n",
		       dev_width, fd_partition, mnt_width, pcfs_mount_pt);
	/*
	 *	Make the mount point for PCFS
	 */
	(void) sprintf(pathname, "%s%s", dir_prefix(), pcfs_mount_pt);
	mkdir_path(pathname);

	/*
	 *	Added fstab entries for a dataless system
	 */
	if (sys_p->sys_type == SYS_DATALESS) {
		(void) fprintf(fp, "%s:%-*s  %-*s  %s ro 0 0\n",
			       sys_p->server, dev_width, sys_p->exec_path,
			       mnt_width, "/usr", DEFAULT_NET_FS);
		(void) fprintf(fp, "%s:%-*s  %-*s  %s ro 0 0\n",
			       sys_p->server, dev_width, sys_p->kvm_path,
			       mnt_width, "/usr/kvm", DEFAULT_NET_FS);
		(void) fprintf(fp, "%s:%-*s  %-*s  %s ro 0 0\n",
			       sys_p->server, dev_width, sharepath,
			       mnt_width, "/usr/share", DEFAULT_NET_FS);
		(void) fprintf(fp, "%s:%s/%s  %s/%s  %s rw 0 0\n",
			       sys_p->server,
			       "/home", sys_p->server,
			       "/home", sys_p->server,
			       DEFAULT_NET_FS);
	}

	(void) fclose(fp);
} /* end mk_partitions() */




/*
 *	Name:		tune_filesys()
 *
 *	Description:	Tune the file system.  The following tasks are
 *		performed:
 *
 *			- make localtime link
 *			- install the boot block
 *			- copy the HOSTS file
 *			- copy the HOSTS_LABEL and NET_LABEL file (SunB1)
 *			- copy the new FSTAB file
 *			- fix the console terminal type
 *			- make RC_DOMAINNAME, RC_HOSTNAME, RC_IFCONFIG,
 *			  RC_NETMASK, and RC_RARPD files
 *			- make all the needed devices
 *			- fix the DEVICE_GROUPS entry for the console (SunB1)
 *			- make the DAC file for the console (SunB1)
 *			- make media devices
 *			- remove /var/yp if NIS is not configured
 *			- remove EXPORTS if the system is DATALESS
 *			- fix bugs for UFS_MINIROOT (SunB1)
 */

static	void
tune_filesys(arch, sys_p)
	arch_info *	arch;
	sys_info *	sys_p;
{
	char		cmd[MAXPATHLEN * 2];	/* command buffer */
	int		i;

	/*
	 *	The check for dataless cannot be moved into mk_localtime()
	 *	because the dataless miniroot needs a localtime file even
	 *	though the dataless /usr does not by definition.
	 */
	if (!re_flag) {
		if ((sys_p->sys_type != SYS_DATALESS)
			&& (*sys_p->timezone != '\0'))
			mk_localtime(sys_p, dir_prefix(), CP_NULL);
	}

	install_boot(sys_p);

	/*
	 *	Copy the HOSTS file that was hand crafted on the miniroot
	 *	to the new root.
	 */
	(void) sprintf(cmd, "cp %s %s 2>> %s", HOSTS, RMP_HOSTS, LOGFILE);
	x_system(cmd);

#ifdef SunB1
	/*
	 *	Copy the HOSTS_LABEL and NET_LABEL files that were
	 *	hand crafted on the miniroot to the new root.
	 */
	(void) sprintf(cmd, "cp %s %s 2>> %s", HOSTS_LABEL, RMP_HOSTS_LABEL,
		       LOGFILE);
	x_system(cmd);

	(void) sprintf(cmd, "cp %s %s 2>> %s", NET_LABEL, RMP_NET_LABEL,
		       LOGFILE);
	x_system(cmd);
#endif /* end SunB1 */

	/*
	 *	Copy the fstab file that was hand crafted on the miniroot
	 *	to the new root.
	 */
	(void) sprintf(cmd, "cp %s %s%s 2>> %s", FSTAB_TMP, dir_prefix(),
		       FSTAB, LOGFILE);
	x_system(cmd);

	fix_term_type(sys_p);

	/*
	 * Create hostname.interface, defaultdomain files
	 */
	if (sys_p->yp_type == YP_NONE)
		mk_rc_domainname(CP_NULL, dir_prefix());
	else
		mk_rc_domainname(sys_p->domainname, dir_prefix());

	/*
	** if there is an ethernet address, set up the proper
	** hostname.interface file
	*/
	if (*sys_p->ether_name0) {
		mk_rc_hostname(sys_p->hostname0, dir_prefix(),
			       sys_p->ether_name0);
		for (i = 1; i < MAX_ETHER_INTERFACES; i++) {
			if (*sys_p->ether_namex(i) && *sys_p->ipx(i)) {
				mk_rc_hostname(sys_p->hostnamex(i),
					       dir_prefix(),
					       sys_p->ether_namex(i));
			}
		}
	} else
		mk_rc_hostname(sys_p->hostname0, dir_prefix(), "");
	
	MAKEDEV(dir_prefix(), STD_DEVS);
	
	mk_disk_devs();
	
#ifdef SunB1
	fix_devgroup("console", "system_low", "system_high", DEVICE_CLEAN,
		     dir_prefix());
	
	mkdir_path(RMP_SEC_DEV_DIR);
	(void) sprintf(cmd, "touch %s/console >> %s 2>&1", RMP_SEC_DEV_DIR,
		       LOGFILE);
	x_system(cmd);

	(void) sprintf(cmd, "chmod 0666 %s/console >> %s 2>&1",
		       RMP_SEC_DEV_DIR, LOGFILE);
	x_system(cmd);
#endif /* SunB1 */

	mk_media_devs(arch);

	clean_yp("", sys_p->yp_type);


	/*
	 *	A dataless system does not care about exports.
	 */
	if (sys_p->sys_type == SYS_DATALESS)
		(void) unlink(RMP_EXPORTS);

#ifdef SunB1
#ifdef UFS_MINIROOT
/*
 *	Make temp directories
 */
	(void) sprintf(cmd, "rm -rfH %s/tmp %s/var/tmp", dir_prefix(),
		       dir_prefix());
	x_system(cmd);

	(void) sprintf(cmd, "mkdir %s/.No_Hid.tmp %s/var/.No_Hid.tmp",
		       dir_prefix(), dir_prefix());
	x_system(cmd);

	(void) sprintf(cmd, "chmod 0777 %s/.No_Hid.tmp %s/var/.No_Hid.tmp",
		       dir_prefix(), dir_prefix());
	x_system(cmd);

	(void) sprintf(cmd, "chmod g+s %s/.No_Hid.tmp %s/var/.No_Hid.tmp",
		       dir_prefix(), dir_prefix());
	x_system(cmd);

/*
 *	Make spool directories
 */
	(void) sprintf(cmd,
	       "mv %s/var/spool/cron/crontabs %s/var/spool/cron/crontabs.orig",
		      dir_prefix(), dir_prefix());
	x_system(cmd);

	(void) sprintf(cmd, "rm -rf %s/var/spool/cron/atjobs", dir_prefix());
	x_system(cmd);

	(void) sprintf(cmd, "rm -rf %s/var/spool/mail %s/var/spool/mqueue",
		       dir_prefix(), dir_prefix());
	x_system(cmd);

	(void) sprintf(cmd, "rm -rf %s/var/spool/cron/.No_Hid.atjobs",
		       dir_prefix());
	x_system(cmd);

	(void) sprintf(cmd,
		"rm -rf %s/var/spool/.No_Hid.mail %s/var/spool/.No_Hid.mqueue",
		       dir_prefix(), dir_prefix());
	x_system(cmd);


	(void) sprintf(cmd,
   "mkdir %s/var/spool/cron/.No_Hid.atjobs %s/var/spool/cron/.No_Hid.crontabs",
		       dir_prefix(), dir_prefix());
	x_system(cmd);

	(void) sprintf(cmd,
"chmod 0700 %s/var/spool/cron/.No_Hid.atjobs %s/var/spool/cron/.No_Hid.crontabs",
		       dir_prefix(), dir_prefix());
	x_system(cmd);

	(void) sprintf(cmd,
"chmod g+s %s/var/spool/cron/.No_Hid.atjobs %s/var/spool/cron/.No_Hid.crontabs",
		       dir_prefix(), dir_prefix());
	x_system(cmd);

	(void) sprintf(cmd,
		 "mkdir %s/var/spool/.No_Hid.mail %s/var/spool/.No_Hid.mqueue",
		       dir_prefix(), dir_prefix());
	x_system(cmd);

	(void) sprintf(cmd, "chmod 0770 %s/var/spool/.No_Hid.mqueue",
		       dir_prefix());
	x_system(cmd);

	(void) sprintf(cmd, "chmod g+s %s/var/spool/.No_Hid.mqueue",
		       dir_prefix());
	x_system(cmd);

	(void) sprintf(cmd, "chmod 1777 %s/var/spool/.No_Hid.mail",
		       dir_prefix());
	x_system(cmd);

	(void) sprintf(cmd, "chmod g+s %s/var/spool/.No_Hid.mail",
		       dir_prefix());
	x_system(cmd);

/*
 *	This copy kludge will put all crontab files in at x_system_low
 */
	(void) sprintf(cmd,
	     "cp %s/var/spool/cron/crontabs.orig/* %s/var/spool/cron/crontabs",
		       dir_prefix(), dir_prefix());
	x_system(cmd);

	(void) sprintf(cmd, "rm -rf %s/var/spool/cron/crontabs.orig",
		       dir_prefix());
	x_system(cmd);
#endif /* UFS_MINIROOT */
#endif /* SunB1 */
} /* end tune_filesys() */





/*
 *	Name:		(static boolean) is_software_loaded()
 *
 *	Description:	This function tests to see if any software has been
 *			loaded.
 *
 *	Return Value:	TRUE  : if any software has been loaded.
 *			FALSE : if no  software has been loaded.
 */
static boolean
is_software_loaded(arch_list)
	arch_info	*arch_list;
{
	arch_info	*ap;			/* architecture info */
	char		pathname[MAXPATHLEN];	/* general pathname buffer */
	media_file *	mf_p;			/* temp media * */
	soft_info	soft;			/* software info */

	for (ap = arch_list; ap ; ap = ap->next) {

		(void) bzero((char *)&soft, sizeof(soft));

		(void) sprintf(pathname, "%s.%s", SOFT_INFO, ap->arch_str);

		if (read_soft_info(pathname, &soft) != 1) {
			menu_log("%s : error reading %s", progname, pathname);
			menu_abort(1);
		}
		
		(void) sprintf(pathname, "%s.%s", MEDIA_FILE, soft.arch_str);
		if (read_media_file(pathname, &soft) != 1) {
			menu_log("%s : error reading %s", progname, pathname);
			menu_abort(1);
		}
		
		for (mf_p = soft.media_files; mf_p &&
		     mf_p < &soft.media_files[soft.media_count]; mf_p++) {
			
			if (mf_p->mf_loaded == ANS_YES) {
				free_media_file(&soft);
				return(1);
			}
			
		} /* for mf_p = */
		
		free_media_file(&soft);
		
	} /* for ap = */

	return(0);
}


/*
 *	Name:		(static void) deselect_software()
 *
 *	Description:	This function resets all the "loaded" modules to
 *			"selected", but not loaded.  This is used mostly in
 *			the case of a restart on an installation.
 *
 */
static void
deselect_software(arch_list)
	arch_info	*arch_list;
{
	arch_info	*ap;			/* architecture info */
	char		pathname[MAXPATHLEN];	/* general pathname buffer */
	char		pathname1[MAXPATHLEN];	/* general pathname buffer */
	media_file *	mf_p;			/* temp media * */
	soft_info	soft;			/* software info */


	/*
	 *	Here, let's loop through all the media_files and reset the
 	 *	loaded modules to "selected", but not "loaded".
	 */

	for (ap = arch_list; ap ; ap = ap->next) {

		(void) bzero((char *)&soft, sizeof(soft));

		(void) sprintf(pathname, "%s.%s", SOFT_INFO, ap->arch_str);

		if (read_soft_info(pathname, &soft) != 1) {
			menu_log("%s : error reading %s", progname, pathname);
			menu_abort(1);
		}

		(void) sprintf(pathname, "%s.%s", MEDIA_FILE, soft.arch_str);
		if (read_media_file(pathname, &soft) != 1) {
			menu_log("%s : error reading %s", progname, pathname);
			menu_abort(1);
		}
		
		for (mf_p = soft.media_files; mf_p &&
		     mf_p < &soft.media_files[soft.media_count]; mf_p++) {
			
			if (mf_p->mf_loaded == ANS_YES) {
				mf_p->mf_loaded = ANS_NO;
				mf_p->mf_select = ANS_YES;
			} /* if */

		} /* for mf_p = */
		
		if (save_media_file(pathname, &soft) != 1) {
			menu_log("%s : error writing %s", progname, pathname);
			menu_abort(1);
		}

		free_media_file(&soft);
		
	} /* for ap = */


	/*
	 *	Now that all the software is reset to selected and not
	 *	loaded, Let's reset the appl_media_files.
 	 *
 	 *	It is a little redundant here, if there is more than one
 	 *	architecture with the same application architecture.
 	 *	However, for simplicity of this already complicated code, we
 	 *	will leave it this way.  This will rarely be executed
 	 *	anyway, as it is only used on re-starting the installation
 	 *	of the system.
	 */
	
	for (ap = arch_list; ap ; ap = ap->next) {

		(void) bzero((char *)&soft, sizeof(soft));

		(void) sprintf(pathname, "%s.%s", SOFT_INFO, ap->arch_str);
		
		if (read_soft_info(pathname, &soft) != 1) {
			menu_log("%s : error reading %s", progname, pathname);
			menu_abort(1);
		}
		
		(void) sprintf(pathname, "%s.%s", MEDIA_FILE, soft.arch_str);
		if (read_media_file(pathname, &soft) != 1) {
			menu_log("%s : error reading %s", progname, pathname);
			menu_abort(1);
		}
		
		(void) sprintf(pathname1, "%s.%s", APPL_MEDIA_FILE, 
			       aprid_to_arid(soft.arch_str, pathname));
		(void) sprintf(pathname, "%s.%s", MEDIA_FILE,
			       soft.arch_str);
		
		if (info_split_media_file(pathname, pathname1, &soft) != 1) {
			menu_log("%s : can't remake  %s",progname, pathname1);
			menu_abort(1);
		}
		
		free_media_file(&soft);
		
	} /* for ap = */

}




/*
 * 	Name : (static int) ask_to_nuke()
 *
 * 	Description : ask if it's ok to really restart the installation.
 *
 * 	Return Value :  1 : if the user typed in a 'Y' or 'y'
 * 			0 : if the user typed anything else.
 *
 */
static int
ask_to_nuke()
{
	char	line [256];

	(void) printf("Some software has already been loaded\n");
	(void) printf(
		"Are you sure you want to restart the installation (y/n) ? ");
	
	(void) fflush (stdout);
	line [0] = '\0';
	(void) gets (line);
	if ((line [0] == 'y') || (line [0] == 'Y'))
		return (1);
	else
		return (0);
	
} /* end of ask_to_nuke() */

