#ifndef lint
#ifdef SunB1
static  char    sccsid[] =	"@(#)add_services.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static  char    sccsid[] =	"@(#)add_services.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1990 Sun Microsystems, Inc.
 */

/*
 *	Name:		
 *
 *	Description: This program adds software to the system.  add_services
 *		is called from installation during suninstall time, and can
 *		be called after the system has been installed.  add_services
 *		must be called with the architecture's name.  If called after
 *		suninstall time, then get_software_info is invoked to provide
 *		a menu based interface for the information gathering.
 */

#include <sys/file.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sysexits.h>
#include "install.h"
#include "menu.h"
#include "media.h"


/*
 *	External functions:
 */
extern	char *		getwd();
extern	char *		sprintf();


/*
 *	Local functions:
 */
static	void		copy_progs();
static	void		create_bootparams();
static	void		fix_inetd_conf();
#ifndef SunB1
static	void		get_sec_info();
#endif /* not SunB1 */
static	int		is_already_server();
static	int		load_media();
static	void		read_toc();
static	void		run_progs();
static	void		run_server_progs();
static	void		setup_filesys();
static	void		tune_client_security();
static	void		tune_security();
static	void		tune_servr();
static	void		tune_server_security();
static	void		update_exports();
static	void		usage();
static	int		verify_media();
#ifdef NEVER
static char *		realpath();
#endif
static int		space_allowed();
static int		is_loaded();
static int		share_module_installed();



/*
 *	Global variables:
 */
#ifdef SunB1
int			old_mac_state;		/* old mac-exempt state */
#endif /* SunB1 */
char *			progname;		/* name for error mesgs */
int			server_flag = 0;	/* set if this is the first
						** time it is made a server
						*/

/*
 * List of items we wish to export via exportfs.  This is a pointer to
 *	malloc'ed character arrays. The "" must be there.
 */
static char *		exportlist = "";


int
main(argc, argv)
	int		argc;
	char **		argv;
{
	char		arch[MEDIUM_STR];	/* architecture */
	int		batch_mode = 0;		/* are we in batch mode? */
	char *		exec_argv[3];		/* arguments for execute() */
	char		pathname[MAXPATHLEN];	/* pathname buffer */
	char		pathname1[MAXPATHLEN];	/* pathname buffer */
	int		ret_code;		/* return code */
	soft_info	soft;			/* software info */
	sys_info	sys;			/* system information */
	arch_info *     arch_list;		/* architecture info */

	(void) umask(UMASK);			/* set file creation mask */

	/*
	 *	Software info structure must be initialized before the
	 *	first call to read_software_info() since it allocates
	 *	and frees its own run-time memory.
	 */
	(void) bzero((char *) &soft, sizeof(soft));

	progname = basename(argv[0]);		/* get name for error mesgs */

	if (argc == 3) {
		if (strcmp(argv[1], "-b") != 0)
			usage();

		batch_mode = 1;
		argc--;
		argv++;
	}

	if (argc == 2)
		(void) irid_to_aprid(argv[1], arch);
	
/*
*	Should the input be checked
*
		(void) fprintf(stderr, 
			       "%s: %s: is not a valid architecture.\n", 
			       progname, argv[1]);
		exit(1);
*/

	/*
	 *	Only superuser can do damage with this command
	 */
	if (suser() != 1) {
		(void) fprintf(stderr, "%s: must be superuser\n", progname);
		exit(EX_NOPERM);
	}

	if (!batch_mode) {
		exec_argv[0] = "get_software_info";
		exec_argv[1] = argv[1];
		exec_argv[2] = NULL;

		if (execute(exec_argv) != 1)
			exit(1);

		if (read_arch_info(ARCH_LIST, &arch_list) != 1)	{
			menu_log("%s: Cannot read arch_list file.", progname);
			menu_abort(1);
		}
		if (arch_list == NULL)	{
			menu_log("No services were added");
			menu_abort(1);
		}
		if (arch_list->next != NULL)	{
			menu_log(
     "Only one O.S release can be added at a time. Please rerun add_services");
			menu_abort(1);
		}
		(void) strcpy(arch, arch_list->arch_str);
		free_arch_info(arch_list);

	}

               /* get system name */
        ret_code = read_sys_info(SYS_INFO, &sys);
        if (ret_code != 1) {
		/*
		 *	Need to add error message for ret_code == 0
		 */
		if (ret_code == 0)
                	menu_log("%s: Error in %s.", progname, SYS_INFO);
		menu_abort(1);
        }

	(void) sprintf(pathname, "%s.%s", SOFT_INFO, arch);
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

	(void) sprintf(pathname1, "%s.%s", APPL_MEDIA_FILE, 
			aprid_to_arid(arch, pathname));
	(void) sprintf(pathname, "%s.%s", MEDIA_FILE, arch);
	ret_code = merge_media_file(pathname, pathname1, &soft);
	if (ret_code != 1) {
		/*
		 *	Need to add error message for ret_code == 0
		 */
		if (ret_code == 0)
			menu_log("%s: %s: cannot read file.", progname, 
				 pathname);
		menu_abort(1);
	}

#ifndef SunB1
	if (!batch_mode && is_sec_loaded(soft.arch_str))
		get_sec_info(&soft, &sys);
#endif /* not SunB1 */


	setup_filesys(&soft, &sys);

	if (soft.media_loc == LOC_REMOTE) {
		ifconfig(&sys, &soft, IFCONFIG_RSH);
		menu_flash_on("\n", REDISPLAY);
#ifdef SunB1
		golabeld(&sys);
#endif /* SunB1 */
	}

	if (load_media(&soft, &sys) != 1)
		exit(1);

	if (sys.sys_type == SYS_SERVER) {
		if (strcmp("/usr/share", soft.share_path) != 0)  {
			mk_localtime (&sys, dir_prefix(), soft.share_path);
		}
	}

	if (sys.sys_type == SYS_SERVER && !is_server(soft.arch_str)) {
		if (!is_already_server())
			server_flag++;
		tune_servr(&soft, &sys);
	}

#ifdef SunB1
	/*
	 *	On a SunOS MLS system you only have to tune security
	 *	when on the miniroot.
	 */
	if (is_miniroot() && is_sec_loaded(soft.arch_str))
#else
	/*
	 *	If security is loaded, then you must tune_security since
	 *	security could have been added after the miniroot.
	 */
	if (is_sec_loaded(soft.arch_str))
#endif /* SunB1 */
		tune_security(&soft, &sys);

	(void) sprintf(pathname1, "%s.%s", APPL_MEDIA_FILE, 
			aprid_to_arid(soft.arch_str, pathname));
        (void) sprintf(pathname, "%s.%s", MEDIA_FILE, soft.arch_str);
 
        ret_code = split_media_file(pathname, pathname1, &soft);
        if (ret_code != 1)
                menu_abort(1);

	exit(0);
	/*NOTREACHED*/
} /* end main() */




/*
 *	Name:		copy_progs()
 *
 *	Description:	Copy boot program into /tftpboot for a server.  Uses
 *		dir_prefix() to find the right place.  Uses 'soft_p' and
 *		'sys_p' to get the right architecture specific stuff.
 */

static	void
copy_progs(soft_p)
	soft_info *	soft_p;
{
	char		cmd[MAXPATHLEN * 2];	/* command buffer */
	char		buf[MEDIUM_STR];	/* aprid */
	char		buf2[MEDIUM_STR];	/* aprid */


	menu_mesg("Setting up tftpboot files\n");

	(void) sprintf(cmd,"cp %s%s/stand/boot.%s %s/tftpboot/boot.%s 2>> %s", 
		       dir_prefix(), soft_p->kvm_path,
		       aprid_to_iid(soft_p->arch_str, buf),
		       dir_prefix(), aprid_to_irid(soft_p->arch_str, buf2),
		       LOGFILE);

		       x_system(cmd);

} /* end copy_progs() */




/*
 *	Name:		create_bootparams()
 *
 *	Description:	Create the BOOTPARAMS file on the server.  Cannot
 *		use RMP_BOOTPARAMS here since this may not be the miniroot.
 */

static	void
create_bootparams()
{
	FILE *		fp;			/* ptr to BOOTPARAMS */
	char		pathname[MAXPATHLEN];	/* path to BOOTPARAMS */


	menu_flash_on("Updating bootparams\n");

	(void) sprintf(pathname, "%s%s", dir_prefix(), BOOTPARAMS);
	if (access(pathname, F_OK) != 0) {
#ifdef TEST_JIG
		menu_log("creating %s with just a '+'", pathname);
#else
		fp = fopen(pathname, "w");
		if (fp == NULL) {
			menu_log("%s: %s: %s", progname, pathname, 
				 err_mesg(errno));
			menu_abort(1);
		}

		(void) fprintf(fp, "+\n");
		(void) fclose(fp);
#endif
	}

	menu_flash_off(REDISPLAY);
} /* end create_bootparams() */




/*
 *	Name:		fix_inetd_conf()
 *
 *	Description:	Fix the INETD_CONF file on the server to turn on
 *		the tftp daemon.  Cannot use RMP_INETD_CONF since this
 *		may not be the miniroot.
 */

static	void
fix_inetd_conf()
{
	char		cmd[MAXPATHLEN];	/* command buffer */
	FILE *		fp_in;
	FILE *		fp_out;


	menu_mesg("Configuring inetd for tftpd operation\n");

	(void) sprintf(cmd, "%s/etc/inetd.conf", dir_prefix());

	if ((fp_in = fopen(cmd, "r")) == NULL)
		return;

	if ((fp_out = fopen("/tmp/inetd.conf", "w")) == NULL)
		return;

	bzero(cmd, sizeof(cmd));
	while (fgets(cmd, sizeof(cmd), fp_in) != NULL) {
		if (strncmp(cmd, "#tftp", 5))
			(void) fputs(cmd, fp_out);
		else
			(void) fputs(&cmd[1], fp_out);
	}
	
	(void) fclose(fp_in);
	(void) fclose(fp_out);

	(void) sprintf(cmd, "cp /tmp/inetd.conf %s/etc/inetd.conf", 
		dir_prefix());
	(void) system(cmd);
	(void) system("rm /tmp/inetd.conf");

} /* end fix_inetd_conf() */



#ifndef SunB1
/*
 *	Name:		get_sec_info()
 *
 *	Description:	Call get_sec_info to get security information for
 *		the system and any clients.  This routine is only called
 *		if we are interactive.
 */

static	void
get_sec_info(soft_p, sys_p)
	soft_info *	soft_p;
	sys_info *	sys_p;
{
	char *		argv[3];		/* arguments for execute() */
	char		buf[BUFSIZ];		/* buffer for input */
	FILE *		fp;			/* ptr to CLIENT_LIST.<arch> */
	char		pathname[MAXPATHLEN];	/* path to CLIENT_LIST */


	argv[0] = "get_sec_info";
	argv[1] = NULL;

	if (strcmp(soft_p->arch_str, sys_p->arch_str) == 0) {
		if (execute(argv) != 1)
			exit(1);
	}

	if (sys_p->sys_type == SYS_SERVER) {
		(void) sprintf(pathname, "%s.%s", CLIENT_LIST, 
			       soft_p->arch_str);

		if (fp = fopen(pathname, "r")) {
			while (fgets(buf, sizeof(buf), fp)) {
				delete_blanks(buf);

				argv[1] = buf;
				argv[2] = NULL;

				if (execute(argv) != 1)
					exit(1);
			}
			(void) fclose(fp);
		}
	}
} /* end get_sec_info() */
#endif /* not SunB1 */




/*
 *	Name:		load_media()
 *
 *	Description:	Read in the media files.  The following tasks are
 *		are performed:
 *
 *			- make the load point directory
 *			- load the media file
 *			- copy the PROTO_ROOT to the root mount point as needed
 */

static	int
load_media(soft_p, sys_p)
	soft_info *	soft_p;
	sys_info *	sys_p;
{
	char		cmd[MAXPATHLEN];	/* command buffer */
	char		cwd[MAXPATHLEN];	/* current working directory*/
	char		ext_dir[MAXPATHLEN];	/* extract directory */
	media_file *	mf_p;			/* media file pointer */
	int		ret_code;		/* return code */
	int		trys;			/* # trys to extract media */
	int		extraction_failed = 0;	/* set if the next module is
						** needed because module
						** failed to load.
						*/
	int		last_tapevol = -1;	/* last tape volume loaded */
	char		buf[MEDIUM_STR];	/* aprid */
	char		buf1[MEDIUM_STR];	/* aprid */
	Os_ident	soft_os;		/* software system's os */
	Os_ident	sys_os;			/* system's os */

	(void) getwd(cwd);
	
	(void) fill_os_ident(&sys_os,  sys_p->arch_str);
	(void) fill_os_ident(&soft_os, soft_p->arch_str);

	for (mf_p = soft_p->media_files; mf_p &&
	     mf_p < &soft_p->media_files[soft_p->media_count]; mf_p++) {
		/*
		 *	Skip media files that we are not supposed to load.
		 *	If a media file has already been loaded and it is
		 *	selected again, then we reload it.
		 */
		if (mf_p->mf_select == ANS_NO)
			continue;

		/*
		 *	Skip the root media file for any architecture
		 *	except this system's.  Cannot use RMP_PROTO_ROOT
		 *	here since this may not be the miniroot.  If the
		 *	system is dataless, then just drop the files in
		 *	the root directory.
		 */
		if (strcmp(mf_p->mf_loadpt, "root") == 0) {
 		    if (is_miniroot())  {
                        if (sys_p->sys_type == SYS_SERVER) {
 				(void) sprintf(ext_dir, "%s%s", dir_prefix(), 
					proto_root_path(soft_p->arch_str));

				/* 
				 *  If root is already loaded, set the
				 *  flags
				 */
				if (access(ext_dir, F_OK) == 0) {  
					mf_p->mf_loaded = ANS_YES;
					mf_p->mf_select = ANS_NO;
					continue;
				}
			}
			else {
				(void) strcpy(ext_dir, dir_prefix());
				if (ext_dir[0] == NULL)
					(void) strcpy(ext_dir, "/");
			}
		    }
 		    else    {
			    (void) sprintf(ext_dir, "%s%s",
					   dir_prefix(), 
					   proto_root_path(soft_p->arch_str));
			    
			if (access(ext_dir, F_OK) == 0) {  
				mf_p->mf_loaded = ANS_YES;
				mf_p->mf_select = ANS_NO;
				continue;
			}
 		    }
		}

		else if (strcmp(mf_p->mf_loadpt, "impl") == 0) {
			(void) sprintf(ext_dir, "%s%s", dir_prefix(), 
				       soft_p->kvm_path);
		}
		else if (strcmp(mf_p->mf_loadpt, "appl") == 0) {
			(void) sprintf(ext_dir, "%s%s", dir_prefix(), 
				       soft_p->exec_path);
		}
		else {
			(void) sprintf(ext_dir, "%s%s", dir_prefix(), 
				       soft_p->exec_path);
		}


		/*
		 *	Verify the toc, until you're not dead. If the
		 * 	extraction of the last module failed, check the toc
		 * 	again.  I really can't assume anything about the
		 * 	intelligence of the user. :-)
		 *
		 *	For CD_ROM devices, tape volume does not matter
		 *	because there is only 1 volume and it's already
		 *	mounted on the /usr/partition
		 */

		if (soft_p->media_type == MEDIAT_FLOPPY) {
			/*
			 * For FLOPPY disks, some program has to eject them,
			 * handle multiple diskettes for one file, etc.,
			 * so the extract handles the verify,
			 *  hence this null statement.
			 */
		} else if ((last_tapevol != mf_p->media_no) &&
		    (soft_p->media_type != MEDIAT_CD_ROM)) {
			read_toc(soft_p, sys_p, mf_p);
			last_tapevol = mf_p->media_no;
		} else {
			/*
			 *	Because of the blasted error, let's verify
			 *	that we have the correct media, still
			 */
			if (extraction_failed)
				read_toc(soft_p, sys_p, mf_p);
		}


		
		/*
		 *	Make the extract directory, if necessary
		 */
		mkdir_path(ext_dir);
		x_chdir(ext_dir);

		/*
		 *	To avoid a real hassle of aborting the install,
		 *	let's check try to extract the module twice and then
		 *	fail if we still cannot extract the media.  If we
		 *	fail on a required module, then we fail for good,
		 *	otherwise we just continue on with the next software
		 *	module.
		 */

		extraction_failed = 0;
		trys = 0;
		for (;;) {
			ret_code = media_extract(soft_p, sys_p, mf_p);
			if (ret_code == 1)
				break;
			else {
				/*
				 * 	The extraction failed
				 */
				if (trys == 0) {
					please_check(soft_p);
					menu_ack();

					/*
					 *	Because of the blasted
					 *	error, let's verify that we
					 *	have the correct media, still
					 *
					 * 	FLOPPY extract does verify!
					 */
					if (soft_p->media_type !=
					    MEDIAT_FLOPPY) {
						read_toc(soft_p, sys_p, mf_p);
					}

					trys++;
				} else {
					/*
					 *	We have already tried once,
		 			 *	so if the module was
		 			 *	required, abort the program
		 			 *	by returning to the caller,
		 			 *	otherwise go to the next
		 			 *	software category.
					 */
					if (mf_p->mf_iflag == IFLAG_REQ) {
						menu_mesg(
     "Extraction failed for required '%s' module.. aborting installation\n",
							  mf_p->mf_name);
						return(ret_code);
					} else {
						menu_mesg(
     "Extraction failed for optional '%s' module..continuing to next module\n",
							  mf_p->mf_name);
						extraction_failed++;
						break;
					}
				}  /*  if (trys == 0)  ( else )*/
			}  /* if (ret_code == 1)  ( else )*/
		} /* for(;;) */

		/*
		 * 	If extraction failed, go on the the next module
		 */
		if (extraction_failed)
			continue;

		/* 
		 *	If the loaded module is an appl(ication) module and
		 *	not a native release, then post_install the share
		 *	files to the share location, if possible.
		 *
		 *	The arch_str's are validated by now to be real.
		 */

		if ((strcmp(mf_p->mf_loadpt, "appl") == 0)  &&
		    !(strcmp(os_arid(&sys_os, buf),
			     os_arid(&soft_os, buf1)) == 0)) {

			if (!share_module_installed(soft_p, mf_p))  {
				if (space_allowed(soft_p) != 1)  {
				    menu_log("Not enough space in %s", 
					     soft_p->share_path);
				    menu_abort(1);
			    	}
				(void)sprintf(cmd, 
				      "cp -rp %s/share/* %s/%s 2>> %s", 
					      ext_dir, dir_prefix(), 
					      soft_p->share_path, LOGFILE);
				(void) system(cmd);
			}
			else if (strcmp(mf_p->mf_name, "usr") != 0)	{
				(void)sprintf(cmd, 
				      "cp -rp %s/share/* %s/%s 2>> %s", 
					      ext_dir, dir_prefix(), 
					      soft_p->share_path, LOGFILE);
				(void) system(cmd);
			}
		
			(void) sprintf(cmd, "rm -r %s/share/* 2>> %s ", 
					ext_dir, LOGFILE);
			(void) system(cmd);
		}

		x_chdir(cwd);
		mf_p->mf_loaded = ANS_YES;
		mf_p->mf_select = ANS_NO;
	}

	/* eject last of the media */
	/* XXX this is where MEDIA flags would be handy */
	if (soft_p->media_type == MEDIAT_FLOPPY) {
		(void) system("eject floppy");
	}

	/*
	 *	Create mount point of kvm module for heterogeneous server
	 */
	if (strcmp(sys_p->arch_str, soft_p->arch_str)) {
		(void) sprintf(cwd, "%s%s/kvm", 
			       dir_prefix(),
			       aprid_to_execpath(soft_p->arch_str, cmd));
		mkdir_path(cwd);
	}		
		
	/*
	 *	Copy the proto root to the right place.  This is only done
	 *	if this is the miniroot, the system is not DATALESS and the
	 *	software being loaded is for the system's architecture.
	 */
	if (is_miniroot() && sys_p->sys_type == SYS_SERVER &&
	    strcmp(sys_p->arch_str, soft_p->arch_str) == 0) {
 		(void) sprintf(cwd, "%s%s", dir_prefix(), 
			       proto_root_path(soft_p->arch_str));
					       
		(void) sprintf(ext_dir, "%s/", dir_prefix());

		copy_tree(cwd, ext_dir);
	}

	/*
	 *	Copy the files in the INSTALL_BIN list.  This is only done
	 *	if this is the miniroot and if the software being loaded is
	 *	for the system's architecture.
	 */
	if (is_miniroot() && 
		strcmp(sys_p->arch_str, soft_p->arch_str) == 0) {
		/*
		 *	If this is a DATALESS system, then we have to mount
		 *	the server's exec_path and kvm_path for the copy to
		 *	work.
		 */
		if (sys_p->sys_type == SYS_DATALESS) {
			ifconfig(sys_p, soft_p, IFCONFIG_MOUNT);
			menu_flash_on("\n", REDISPLAY);
#ifdef SunB1
			golabeld(sys_p);
#endif /* SunB1 */

			(void) sprintf(cwd, "%s/usr", dir_prefix());
			mkdir_path(cwd);

			/*
			 *	Mount the user file system from the server.
			 *	We use system() here since we care about the
			 *	return code.
			 */
			(void) sprintf(cmd, "mount -r -o soft %s:%s %s/usr", 
				       sys_p->server, sys_p->exec_path, 
				       dir_prefix());

			while (system(cmd) != 0) {
				menu_log("%s: '%s' failed.", progname, cmd);
				menu_mesg(
#ifdef SunB1
		    		  "Please check %s and %s file entries on %s", 
					  HOSTS, HOSTS_LABEL, sys_p->server);
#else
			     		  "Please check %s file entry on %s", 
					  HOSTS, sys_p->server);
#endif /* SunB1 */
			}

			/*
			 *	Mount the kernel exec file system from the
			 *	server.  We use system() here since we care
			 *	about the return code.
			 */
			(void) sprintf(cmd, 
				       "mount -r -o soft %s:%s %s/usr/kvm", 
				       sys_p->server, sys_p->kvm_path, 
				       dir_prefix());

			while (system(cmd) != 0) {
				menu_log("%s: '%s' failed.", progname, cmd);
				menu_mesg(
#ifdef SunB1
		    		  "Please check %s and %s file entries on %s", 
					  HOSTS, HOSTS_LABEL, sys_p->server);
#else
			     		  "Please check %s file entry on %s", 
					  HOSTS, sys_p->server);
#endif /* SunB1 */
			}
		}

		copy_install_bin("/usr", "/usr/kvm", "/");
	}

	return(1);
} /* end load_media() */



/*
 *	Name:		read_toc()
 *
 *	Description:	Endless loop until the correct toc is read.
 */
static void
read_toc(soft_p, sys_p, mf_p)
	soft_info	*soft_p;
	sys_info	*sys_p;
	media_file	*mf_p;
{
	char		question[128 + MAXHOSTNAMELEN + MAXPATHLEN];
	int		ret_code;
	int		tmp_vol;	/* keep old soft_p volume number */
	
	/*
	 *	For mount_string purposes, since soft_p does not change
	 */
	tmp_vol = soft_p->media_vol;
	soft_p->media_vol = mf_p->media_no;

	/*
	 *	Verify that we have the right media, 
	 *      but not for every media file, just the
	 *      different volumes.
	 */
	while ((ret_code = verify_media(soft_p, sys_p, mf_p)) != 1) {
		if (ret_code != 0) {
			/*
			 *	Fatal error, but keep trying
			 */
			menu_mesg("Error reading the table of contents.");
			please_check(soft_p);
		}
		/*
		 *	This is the wrong tape
		 */
		(void) sprintf(question, "Please mount %s",
			       mount_string(soft_p));
		menu_mesg(question);
		menu_ack();
	}
	/*
	 *	Yeh, it's the correct tape.
	 */
	soft_p->media_vol = tmp_vol;
}


/*
 *	Name:		run_progs()
 *
 *	Description:	Run any programs that need to be run for a server.
 */

static	void
run_progs(sys_p)
	sys_info *	sys_p;
{

	char *	cmd;	/* command buffer */

	if (is_miniroot())
		return;

	cmd = malloc((unsigned)(strlen(exportlist) + MAXPATHLEN + 15));

	menu_flash_on("Starting daemons\n"); 
	
	/*
	** we don't want multiples of theses processes around, so, 
	** we check if they are running first
	*/
	if (!is_running("rpc.bootparamd")) 
		x_system("/usr/etc/rpc.bootparamd");
	
	if (!is_running("rarpd")) 
		rarpd(sys_p);
	
	run_server_progs();
	
	menu_flash_on("Exporting file systems\n");
	
	(void) sprintf(cmd, "exportfs %s 2>> %s", exportlist, LOGFILE);
	x_system(cmd);

	free(cmd);
	
} /* end run_progs() */



/*
 *	Name:		run_server_progs()
 *
 *	Description:	Run any programs that need to be run for a server.
 *			this function is separate to hide some information
 *			from the other function.
 */

static	void
run_server_progs()
{
	if (!is_miniroot()) {
		/*
		** just in case you are wondering. These are in the rc.local
		** file and are usually started at boot time
		*/

		if (server_flag) {
			x_system("nfsd 8 &");
			if (access("/etc/security/passwd.adjunct", 0) == 0)
				x_system("rpc.mountd");
			else
				x_system("rpc.mountd -n");
		}

		/*
		** restart inetd, so that inetd.conf is re-read, so we can
		** be a server.
		*/
		(void) system(
	       "kill -HUP `ps aux|grep inetd|grep -v grep|awk '{print $2}'`");
		
	}
	
} /* end run_server_progs() */



/*
 *	Name:		setup_filesys()
 *
 *	Description:	Setup the file system.  The following tasks are
 *		performed:
 *
 *			- if this is a server make the appropriate
 *			  directories in /export
 *			- make links to /usr and /usr/kvm as appropriate
 *			- make directories for additional architectures
 *			  as appropriate
 */

static	void
setup_filesys(soft_p, sys_p)
	soft_info *	soft_p;
	sys_info *	sys_p;
{
	char		pathname[MAXPATHLEN];	/* pathname buffer */
	char		buf[MEDIUM_STR];	/* aprid */
	Os_ident	sys_os, soft_os;

	menu_flash_on("Setting up server file system for services\n");

	if (fill_os_ident(&sys_os, sys_p->arch_str) < 0 ||
	    fill_os_ident(&soft_os, soft_p->arch_str) < 0)    {
		menu_log("%s: undefined architecture %s or %s", 
			    progname, sys_p->arch_str, soft_p->arch_str);
		menu_abort(1);
	}		

    
	/*
	 *	Set up the /usr/export/home links.
	 */
	if (is_miniroot() )	{
 		if (partitions() == TWO_PARTS &&
		    sys_p->sys_type != SYS_DATALESS) {
			(void) sprintf(pathname, "%s/usr/export/home/%s",
				       dir_prefix(), 
				       sys_p->hostname0);
			mkdir_path(pathname);
			
			(void) sprintf(pathname, "%s/home", dir_prefix());
			mklink("/usr/export/home", pathname);
		} else {
			if (sys_p->sys_type == SYS_DATALESS)
				(void) sprintf(pathname, "%s/home/%s", 
					       dir_prefix(), sys_p->server);
			else
				(void) sprintf(pathname, "%s/home/%s", 
					       dir_prefix(), sys_p->hostname0);
			
			mkdir_path(pathname);
			(void) sprintf(pathname, "%s/usr/export",
				       dir_prefix());
			mkdir_path(pathname);
			(void) sprintf(pathname, "%s/usr/export/home",
				       dir_prefix());
			mklink("/home", pathname);
 		}
	}


	(void) sprintf(pathname, "%s%s", dir_prefix(), 
		       aprid_to_execpath(sys_p->arch_str, buf));
	mklink("/usr", pathname);
	(void) sprintf(pathname, "%s%s", dir_prefix(), 
		       aprid_to_kvmpath(sys_p->arch_str, buf));
	mklink("/usr/kvm", pathname);
	
	/*
	 *	Set up share path and symbolic link
	 */
	if (strcmp("/usr/share", soft_p->share_path) == 0)  {
		(void) sprintf(pathname, "%s/%s", dir_prefix(), 
			       aprid_to_sharepath(soft_p->arch_str, buf));
		mklink("/usr/share", pathname);
	} else	{
		(void) sprintf(pathname, "%s/%s", 
			       dir_prefix(), soft_p->share_path);
		mkdir_path(pathname);	    
		/*
		 *	recreate symbolic link for the
		 *	<share_path>/sys if not the native release.
		 */
		(void) sprintf(pathname, "%s/%s/sys", 
			       dir_prefix(), soft_p->share_path);
		mklink("../kvm/sys", pathname);
	}

	/*
	 * 	Now that we read the Os_ident struct for sys...
	 * 	Make the default executable link
	 */
	(void) sprintf(pathname, "%s%s/%s", dir_prefix(),
		       STD_EXEC_PATH,
		       sys_os.appl_arch);
	mklink("/usr",pathname);

	if (!same_appl_arch(&sys_os, &soft_os)  &&
	    same_release(&sys_os, &soft_os)) {
		(void) sprintf(pathname, "%s%s/%s", dir_prefix(),
			       STD_EXEC_PATH,
			       soft_os.appl_arch);
		mklink(aprid_to_arid(soft_p->arch_str, buf), pathname);
	}
	
	/*
	 *	Set up kvm path
	(void) sprintf(pathname, "%s%s", dir_prefix(),
		aprid_to_kvmpath(soft_p->arch_str, cmd));

	if (strcmp(sys_p->arch_str, soft_p->arch_str) == 0) 
		mklink("/usr/kvm", pathname);
	else
		mkdir_path(pathname);
	 */

	/*
	 *	Set up default /usr/kvm link
	 */
	(void) sprintf(pathname, "%s%s/%s", dir_prefix(),
		       STD_KVM_PATH,
		       sys_os.impl_arch);
	mklink(aprid_to_irid(sys_p->arch_str, buf), pathname);

	if (!same_impl_arch(&sys_os, &soft_os)  &&
	    same_release(&sys_os, &soft_os)) {
		(void) sprintf(pathname, "%s%s/%s",
			       dir_prefix(),
			       STD_KVM_PATH,
			       soft_os.impl_arch);
		mklink(aprid_to_irid(soft_p->arch_str, buf), pathname);
	}
		

	if (sys_p->sys_type == SYS_SERVER) {
		(void) sprintf(pathname, "%s/export/swap", dir_prefix());
		mkdir_path(pathname);

		(void) sprintf(pathname, "%s/tftpboot", dir_prefix());
		mkdir_path(pathname);

		(void) sprintf(pathname, "%s/tftpboot/tftpboot", dir_prefix());
		mklink(".",pathname);

	}

	/*
	 *	Set up custom exec/kvm paths
	 */
	if (strcmp(sys_p->arch_str, soft_p->arch_str) == 0) {
		if (strcmp(soft_p->exec_path, "/usr") != 0) {
			(void) sprintf(pathname, "%s%s", dir_prefix(), 
				       soft_p->exec_path);
			mklink("/usr", pathname);
			(void) strcpy(soft_p->exec_path, "/usr");
		}

		if (strcmp(soft_p->kvm_path, "/usr/kvm") != 0) {
			(void) sprintf(pathname, "%s%s", dir_prefix(), 
				       soft_p->kvm_path);
			mklink("/usr/kvm", pathname);
			(void) strcpy(soft_p->kvm_path, "/usr/kvm");
		}
	}
	else {
		(void) sprintf(pathname, "%s%s", dir_prefix(), 
			       soft_p->exec_path);
		mkdir_path(pathname);
		if (strcmp(soft_p->exec_path, 
			aprid_to_execpath(soft_p->arch_str, buf)) != 0)
			mklink(pathname, buf);

		(void) sprintf(pathname, "%s%s", dir_prefix(), 
			       soft_p->kvm_path);
		mkdir_path(pathname);
		if (strcmp(soft_p->kvm_path, 
			aprid_to_kvmpath(soft_p->arch_str, buf)) != 0)
			mklink(pathname, buf);
	}

} /* end setup_filesys() */




/*
 *	Name:		tune_client_security()
 *
 *	Description:	Tune security for existing clients.  This routine
 *		is needed to setup security for existing clients if the
 *		Security media file is loaded after suninstall time.  This
 *		routine should not modify file system objects outside of
 *		the client's domain.  EXCEPTION: the tune_audit() routine
 *		modifies the server's EXPORTS file and makes audit directories
 *		for clients while processing the audit directory list.  The
 *		following tasks are performed:
 *
 *			- tune auditing configuration
 *			- fix PASSWD file entries
 */

static	void
tune_client_security(name)
	char *		name;
{
	clnt_info	clnt;			/* client info */
	char		pathname[MAXPATHLEN];	/* scratch path */
	sec_info	sec;			/* security info */


	(void) sprintf(pathname, "%s.%s", CLIENT_STR, name);
	switch (read_clnt_info(pathname, &clnt)) {
	case 0:
		menu_log("%s: %s: cannot read file.", progname, pathname);
		menu_abort(1);

	case 1:
		break;

	case -1:
		menu_abort(1);
	}

	(void) sprintf(pathname, "%s%s", dir_prefix(), clnt.root_path);
	if (access(pathname, F_OK) != 0)
		return;

	(void) sprintf(pathname, "%s.%s", SEC_INFO, name);
	switch (read_sec_info(pathname, &sec)) {
	case 0:
		menu_log("%s: %s: cannot read file.", progname, pathname);
		menu_abort(1);

	case 1:
		break;

	case -1:
		 menu_abort(1);
	}

	(void) sprintf(pathname, "%s%s", dir_prefix(), clnt.root_path);

	tune_audit(&clnt, pathname, &sec);

	fix_passwd(pathname, &sec);
} /* end tune_client_security() */




/*
 *	Name:		tune_security()
 *
 *	Description:	Tune security for the system (non SunB1).  The
 *		following tasks are performed:
 *
 *			- tune audit configuration
 *			- fix PASSWD file entries
 *
 *		Client's are tuned via the call to tune_server_security().
 */

static	void
tune_security(soft_p, sys_p)
	soft_info *	soft_p;
	sys_info *	sys_p;
{
	char		pathname[MAXPATHLEN];	/* path to SEC_INFO */
	sec_info	sec;			/* security info */


	if (strcmp(soft_p->arch_str, sys_p->arch_str) == 0) {
		(void) sprintf(pathname, "%s.%s", SEC_INFO, sys_p->hostname0);
		switch (read_sec_info(pathname, &sec)) {
		case 0:
			menu_log("%s: %s: cannot read file.", progname, 
				 pathname);
			menu_abort(1);

		case 1:
			break;

		case -1:
			menu_abort(1);
		}

		tune_audit((clnt_info *) NULL, dir_prefix(), &sec);

		fix_passwd(dir_prefix(), &sec);
	}

	if (sys_p->sys_type == SYS_SERVER)
		tune_server_security(soft_p);
} /* end tune_security() */




/*
 *	Name:		tune_servr()
 *
 *	Description:	Tune the files on the server.  This routine should
 *		not modify file system objects in the client's domain.  The
 *		following tasks are performed:
 *
 *			- copy programs that the server needs
 *			- create the BOOTPARAMS file
 *			- update the EXPORTS file
 *			- fix INETD_CONF
 *			- run server related programs
 */

static	void
tune_servr(soft_p, sys_p)
	soft_info *	soft_p;
	sys_info *	sys_p;
{
	copy_progs(soft_p);

	create_bootparams();

	update_exports(soft_p);

	fix_inetd_conf();

	run_progs(sys_p);
} /* end tune_servr() */




/*
 *	Name:		tune_server_security()
 *
 *	Description:	This routine is a dispatch point for tuning all the
 *		client's security via tune_client_security().  This routine
 *		exists only for reading the CLIENT_LIST files.
 */

static	void
tune_server_security(soft_p)
	soft_info *	soft_p;
{
	char		buf[BUFSIZ];		/* buffer for input */
	FILE *		fp;			/* ptr to CLIENT_LIST.<arch> */
	char		pathname[MAXPATHLEN];	/* scratch path */


	(void) sprintf(pathname, "%s.%s", CLIENT_LIST, 
		       soft_p->arch_str);

	if (fp = fopen(pathname, "r")) {
		while (fgets(buf, sizeof(buf), fp)) {
			delete_blanks(buf);

			tune_client_security(buf);
		}
		(void) fclose(fp);
	}
} /* end tune_server_security() */


/*
 *	Name:		export_it()
 *
 *	Description:	This function adds the "path" to the exportlist
 *			string for later exporting
 */
static void
export_it(path)
	char *	path;
{
	char *	cp;

	/*
	 * 	Save the list of pathnames we wish to export. The (+ 4) is
	 * 	for  spaces and the null termination of the strings.
	 */
	cp = malloc((unsigned) (strlen(path) + 4 + strlen(exportlist)));

	(void) sprintf(cp, "%s %s", exportlist, path);

	free(exportlist);

	exportlist = cp;	/* assign the new export list */
}



/*
 *	Name:		update_exports()
 *
 *	Description:	Update the EXPORTS file on the server.  Cannot use
 *		RMP_EXPORTS since this may not be the miniroot.
 */

static	void
update_exports(soft_p)
	soft_info *	soft_p;
{
	char		pathname[MAXPATHLEN];	/* path to EXPORTS file */

	menu_flash_on("Updating server's exports file\n");

	(void) sprintf(pathname, "%s%s", dir_prefix(), EXPORTS);

	/*
	**	For a server, /usr should always be exported. Just to make
	** 	sure that /usr and /usr/kvm are not exported already, we
	**	make this check here. If we are in multi-user mode, we are
	**	assuming that if /usr is not in /etc/exports, then they
	**	don't want it there, so we will not add it, unless this is a
	**	conversion to a server.
	*/
	if (server_flag || is_miniroot()) {
		add_key_entry("/usr", "", pathname, KEY_ONLY);
		(void) export_it("/usr");
	}

	if (strcmp("/usr", soft_p->exec_path) != 0) {	
		add_key_entry(soft_p->exec_path, "", pathname, KEY_ONLY);
		(void) export_it(soft_p->exec_path);
	}
			      
	if (strcmp("/usr/kvm", soft_p->kvm_path) != 0) {
		add_key_entry(soft_p->kvm_path, "", pathname, KEY_ONLY);
		(void) export_it(soft_p->kvm_path);
	}
	
	if (strcmp("/usr/share", soft_p->share_path) != 0) {
		add_key_entry(soft_p->share_path, "", pathname, KEY_ONLY);
		export_it(soft_p->share_path);
	}

	if ((partitions() != TWO_PARTS) || server_flag && is_miniroot()) {
		add_key_entry("/home", "", pathname, KEY_ONLY);
		export_it("/home");
	}

	if (is_miniroot() || server_flag) {
		add_key_entry("/var/spool/mail", "", pathname, KEY_ONLY);
		export_it("/var/spool/mail");
	}

} /* end update_exports() */




/*
 *	Name:		verify_media()
 *
 *	Description:	Verify that media file we want is loaded in the
 *		media device.
 */

static	int
verify_media(soft_p, sys_p, mf_p)
	soft_info *	soft_p;
	sys_info *	sys_p;
	media_file *	mf_p;
{
	int		ret_code;		/* return code */
	soft_info	tmp;			/* scratch software info */
	char		mount_str[512];

	/*
	 *	Software info structure must be initialized before the
	 *	first call to read_software_info() since it allocates
	 *	and frees its own run-time memory.
	 */
	bzero((char *) &tmp, sizeof(tmp));

	ret_code = read_file(soft_p, 1, XDRTOC);
	if (ret_code != 1)
		return(ret_code);

	(void) strcpy(tmp.arch_str, soft_p->arch_str);
	tmp.media_vol = mf_p->media_no;

 	ret_code = toc_xlat(sys_p, XDRTOC, &tmp);
	free_media_file(&tmp);
	if (ret_code != 1)
		return(ret_code);

	if (strcmp(tmp.arch_str, soft_p->arch_str) ||
		mf_p->media_no != tmp.media_vol ) {
		(void) sprintf(mount_str, "You have %s mounted",
			       mount_string(&tmp));
		menu_mesg(mount_str);
		return(0);
	}
	
	
	return(1);
} /* end verify_media() */



/****************************************************************************
** Function : (static int) is_aleady_server()
**
** Return value : 1 : if host is already a server
**		  0 : if host is not a server
**
** Description : If the file /etc/exports exists, then it is a server of
** 		some sort and the appropriate processes are assumed to be
**		running at then, but now won't be invoked.
**
*****************************************************************************
*/
static int
is_already_server()
{
	if (access("/etc/exports", 0) == 0)
		return(1);
	else
		return(0);
}
			

/*
 *	Name:		usage()
 *
 *	Description:	Print a usage message and die.
 */

static	void
usage()
{
	(void) fprintf(stderr, "Usage: %s [architecture]\n", progname);
	exit(1);
} /* end usage() */



/*
 *	Name:		share_module_installed()
 *
 *	Description:	check to see if the share portion of the given medule
 *			has been installed or not.
 */
static int
share_module_installed(soft_p, mf_p)
	soft_info *	soft_p;
	media_file *	mf_p;			/* media file pointer */
{
	arch_info *	arch_list;
    	arch_info *	ap;
	Os_ident	tape_os;

	if (fill_os_ident(&tape_os, soft_p->arch_str) < 0)	{
		menu_log("Undefined architecture string %s.", 
			 soft_p->arch_str);
		menu_abort(1);
	}
		
	if (read_arch_info(ARCH_INFO, &arch_list) != 1)	{
		menu_log("Cannot read arch_list file.");
		menu_abort(1);
	}
	for (ap = arch_list; ap ; ap = ap->next)    {
		/*
		 *	Here, assume share the /usr/share stuff is the same
		 *	even if their releases are the same, including
		 *	realization.
		 */

		if (same_os(&ap->os, &tape_os))
			continue;

		if (same_release(&ap->os, &tape_os) &&
		    is_loaded(ap->arch_str, mf_p->mf_name) == ANS_YES) {
			free_arch_info(arch_list);
			return(1);
		}
	}
	free_arch_info(arch_list);
	return(0);
}


/*
 *	Name:		is_loaded()
 *
 *	Description:	check to see the given medule
 *			has been installed or not.
 */
static int
is_loaded(arch_str, name)
	char *	    arch_str;
	char *	    name;
{
	soft_info	soft;			/* software info */
	char		pathname[MAXPATHLEN];	/* pathname buffer */
	char		pathname1[MAXPATHLEN];	/* pathname buffer */
	int		ret_code;		/* return code */
	media_file *	mf_p;			/* media file pointer */

	(void) sprintf(pathname1, "%s.%s", APPL_MEDIA_FILE, 
			aprid_to_arid(arch_str, pathname));
	(void) sprintf(pathname, "%s.%s", MEDIA_FILE, arch_str);
	ret_code = merge_media_file(pathname, pathname1, &soft);
	if (ret_code != 1) {
		/*
		 *	Need to add error message for ret_code == 0
		 */
		if (ret_code == 0)
			menu_log("%s: %s: cannot read file.", progname, 
				 pathname);
		menu_abort(1);
	}
	for (mf_p = soft.media_files; mf_p &&
	     mf_p < &soft.media_files[soft.media_count]; mf_p++) {
		if (strcmp(mf_p->mf_name, name) == 0)	
			return(mf_p->mf_loaded);
		}
	return(-1);
}


/*
 *	Name:		space_allowed()
 *
 *	Description:	check to see the destination of the share portion of 
 *			a given module has enough space or not.
 */
static int
space_allowed(soft_p)
	soft_info *	soft_p;
{
	char		cmd[MAXPATHLEN];	/* command buffer */
	char *		part_p;
	FILE *		pp;
	long		size;
    
	(void) sprintf(cmd, 
	    "%s/usr/bin/du -s ./share | /bin/sed 's/.\\/share//'", 
		       dir_prefix());
	if ((pp = popen(cmd, "r")) == NULL ||
		fscanf(pp, "%d", &size) != 1)   {
		menu_log("'%s' failed.", progname, cmd);
		(void) pclose(pp);
		return(-1);		
	}
	(void) pclose(pp);

	size *= 1000;

	if ((part_p = find_part(soft_p->share_path)) != NULL)	
		return(update_bytes(part_p, size));
	return(-1);
}

/*
 *	Name:		realpath()
 *
 *	Description:	find the real directory if the given dir is a
 *			symlink.
 */
#ifdef NEVER
static char *
realpath(dir, realdir)
	char *	dir;	    /* directory to be checked */
	char *	realdir;    /* buffer thar teh real directory to be sved */
{
	char buf[MAXPATHLEN];
	int len;
	char *cp;

	strcpy(realdir, dir);
	while ((len = readlink(realdir, buf, sizeof(buf))) > 0)   {
		buf[len] = '\0';
		if (buf[0] != '/')  {
		    if ((cp = strrchr(realdir, '/')) != NULL)
			 *(cp+1) = '\0';
		    else 
			*realdir = '\0';
		    strcat(realdir, buf);
		}
		else	{
		    strcpy(realdir, buf);
		}
	}
	return(realdir);
}

#endif
