#ifndef lint
#ifdef SunB1
static  char    sccsid[] = 	"@(#)get_software_info.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static  char    sccsid[] = 	"@(#)get_software_info.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint


/*
 *	Copyright (c) 1990 Sun Microsystems, Inc.
 */

/*
 *	Name:		get_software_info.c
 *
 *	Description:	Use a menu based interface to get information
 *		from the user about the system software.  This program is
 *		typically called from suninstall and add_services.  If
 *		invoked with an architecture name, then get_software_info
 *		deals with information about that architecture.
 */

#include <sys/file.h>
#include <stdio.h>
#include <string.h>
#include "soft_impl.h"
#include "media.h"


/*
 *	External references:
 */
extern	char *		sprintf();
extern	char		INSTALL_TAR_DIR[];



/*
 *	Local functions:
 */
static	char *		find_mf_path();



/*
 *	Global variables:
 */
#ifdef SunB1
int		old_mac_state;			/* old mac-exempt state */
#endif /* SunB1 */
char *		progname;


main(argc, argv)
        int		argc;
        char *		argv[];
{
	char *		arch_str = NULL;	/* command line architecture */
	mf_disp		disp;			/* media file display info */
	form *		form_p;			/* ptr to SOFTWARE form */
	soft_info	soft;			/* software information */
	sys_info	sys;			/* system information */


	(void) umask(UMASK);			/* set file creation mask */

	progname = basename(argv[0]);		/* get name for error mesgs */

	if (argc > 2) {
		(void) fprintf(stderr, "Usage: %s [arch]\n", progname);
		exit(1);
	}
	if (argc == 2)
		arch_str = argv[1];

	bzero((char *) &disp, sizeof(disp));
	/*
	 *	Software info structure must be initialized before the
	 *	first call to read_software_info() since it allocates
	 *	and frees its own run-time memory.
	 */
	bzero((char *) &soft, sizeof(soft));

	set_menu_log(LOGFILE);

                                                /* get system name */
	if (read_sys_info(SYS_INFO, &sys) != 1) {
		menu_log("%s: Error in %s.", progname, SYS_INFO);
		menu_abort(1);
	}

	/*
	 *	An architecture was specified on the command line so use it.
	 */
	if (arch_str)
		(void) strcpy(soft.arch_str, arch_str);

	/*
	 * Dynamically set up disk lists if not in miniroot
	 */
	init_disk_info(&sys);

	/*
	 *	Calculate the disk space up front just in case
	 *	something was changed.
	 */
    	if (calc_disk(&sys) != 1)
		menu_abort(1);

	get_terminal(sys.termtype);		/* get terminal type */

	/*
	 *	remove the temporary arch list file
	 */
	(void) unlink(ARCH_LIST);

	form_p = create_soft_form(&soft, &sys, &disp);

	/*
	 *	Ignore the 4.0 "repaint"
	 */
	(void) signal(SIGQUIT, SIG_IGN);

	if (use_form(form_p) != 1)		/* use SOFTWARE FORM */
		menu_abort(1);

	(void) signal(SIGQUIT, SIG_DFL);

	end_menu();				/* done with menu system */
	
	exit(0);
	/*NOTREACHED*/
} /* end main() */




/*
 *	Name:		ask_mf_choice()
 *
 *	Description:	Determine if we should ask the user about whether to
 *		load a media file or not.  Returns 1 if the user has a choice
 *		and 0 otherwise.  A whole series of special cases determine
 *		whether the user has a choice and what the automated choice is.
 */

int
ask_mf_choice(disp_p, mf_p, soft_p, sys_p)
	mf_disp *	disp_p;
	media_file *	mf_p;
	soft_info *	soft_p;
	sys_info *	sys_p;
{
	char **		cpp;			/* scrach char pointer */
	media_file *	sp;			/* scratch media file ptr */


	disp_p->media_select = ' ';

	/*
	 *	If the choice is "existing choice", then return 0 to disallow 
	 *	selection.
	 */
	if (soft_p->choice == SOFT_SAVED) {
		return(0);
	}

	/*
	 *	If the choice is for all the software, then mark it as
	 *	selected.
	 */
	if (soft_p->choice == SOFT_ALL) {
		disp_p->media_select = 'y';
		return(0);
	}

	/*
	 *	Special case for dataless.  Dataless only gets the root
	 *	media file.
	 */
	if (sys_p->sys_type == SYS_DATALESS) {
		if (strcmp(mf_p->mf_name, "root") == 0)
			disp_p->media_select = 'y';
		else
			disp_p->media_select = 'n';

		return(0);
	}

	/*
	 *	Special case for networking.  If we have ethernet
	 *	configured, then we will force networking, since it is a
	 *	very good idea.
	 */
	if (strcmp(mf_p->mf_name, "Networking") == 0 && 
	    sys_p->ether_type != ETHER_NONE &&
	    soft_p->choice != SOFT_OWN) {
		disp_p->media_select = 'y';
		return(0);
	}

	/*
	 *	See if this media file is a dependent of some other media
	 *	file that we have selected.
	 */
	for (sp = soft_p->media_files;
	     sp < &soft_p->media_files[soft_p->media_count]; sp++) {
		for (cpp = sp->mf_deplist;
		     cpp && cpp < &sp->mf_deplist[sp->mf_depcount]; cpp++)
			if (strcmp(mf_p->mf_name, *cpp) == 0 &&
			    sp->mf_select == ANS_YES) {
				disp_p->media_select = 'y';
				return(0);
			}
	}

	switch (soft_p->choice) {
	case SOFT_OWN:
/***	case SOFT_SAVED:    ***/
		if (!is_miniroot())
			return(1);
		break;

	case SOFT_DEF:
		if (mf_p->mf_iflag == IFLAG_COMM) {
			disp_p->media_select = 'y';
			return(0);
		}

		if (mf_p->mf_iflag == IFLAG_DES) {
			disp_p->media_select = 'y';
			return(0);
		}
		break;

	case SOFT_REQ:
		if (mf_p->mf_iflag != IFLAG_REQ)
			disp_p->media_select = 'n';
		else
			disp_p->media_select = 'y';

		return(0);
		break;
	} /* end switch */

	if (mf_p->mf_iflag == IFLAG_REQ) {
		disp_p->media_select = 'y';
		return(0);
	}

	return(1);
} /* end ask_mf_choice() */







/*
 *	Name:		find_mf_path()
 *
 *	Description:	Find the pathname associated with given media file.
 *		'soft_p' points to architecture information, 'mf_p' points
 *		to media file information and 'sys_p' points to system
 *		information.  Determination is based on mf_loadpt which
 *		may be any of root|appl|impl|share.
 */

static	char *
find_mf_path(soft_p, mf_p, sys_p)
	soft_info *	soft_p;
	media_file *	mf_p;
	sys_info *	sys_p;
{
	char *		pathname;		/* path name to return */


	if (strcmp(mf_p->mf_loadpt, "root") == 0 ||
	    strcmp(mf_p->mf_name, "root") == 0 ||	/* for 4.0.x */
	    sys_p->sys_type == SYS_DATALESS)
		pathname = "/";

	else if (strcmp(mf_p->mf_loadpt, "appl") == 0 ||
		strcmp(mf_p->mf_name, "usr") == 0)	/* for 4.0.x */
		pathname = soft_p->exec_path;

	else if (strcmp(mf_p->mf_loadpt, "impl") == 0 ||
		strcmp(mf_p->mf_name, "Kvm") == 0)	/* for 4.0.x */
		pathname = soft_p->kvm_path;

	else
		pathname = soft_p->exec_path;

	return(pathname);
} /* end find_mf_path() */




/*
 *	Name:		get_mf_disp_info()
 *
 *	Description:	Get disk space allocation information for the media
 *		file display.
 */

int
get_mf_disp_info(sys_p, soft_p, mf_p, disp_p)
	sys_info *	sys_p;
	soft_info *	soft_p;
	media_file *	mf_p;
	mf_disp *	disp_p;
{
	disk_info	disk;			/* disk info buffer */
	char		disk_name[TINY_STR];	/* disk name */
	int		len;			/* scratch length */
	char		part;			/* which partition */
	char *		part_name;		/* partition name */
	char		pathname[MAXPATHLEN];	/* pathname buffer */
	int		ret_code;		/* return code */

	part_name = find_mf_part(soft_p, mf_p, sys_p);
	/*
	 *	Can't find the partition name.  Error message provided
	 *	by find_part().
	 */
	if (part_name == NULL)
		return(-1);

	(void) strcpy(disp_p->dest_fs, part_name);
	(void) sprintf(disp_p->dest_str, "%s (%s)",
		       find_mf_path(soft_p, mf_p, sys_p), part_name);

	len = strlen(part_name) - 1;
	part = part_name[len];
	bzero(disk_name, sizeof(disk_name));
	(void) strncpy(disk_name, part_name, len);

	(void) sprintf(pathname, "%s.%s", DISK_INFO, disk_name);
	ret_code = read_disk_info(pathname, &disk);
	/*
	 *	Only a return of 1 is okay here.  This file is not
	 *	optional.  read_disk_info() prints the error
	 *	message for ret_code == -1 so we have to print one
	 *	for ret_code == 0.
	 */
	if (ret_code != 1) {
		if (ret_code == 0)
			menu_log("%s: cannot read file.", pathname);
		return(-1);
	}

	(void) sprintf(disp_p->dest_avail, "%lu",
		       disk.partitions[part - 'a'].avail_bytes);

	(void) sprintf(disp_p->hog_fs, "%s%c", disk_name, disk.free_hog);
	(void) sprintf(disp_p->hog_avail, "%lu",
		       disk.partitions[disk.free_hog - 'a'].avail_bytes);

	return(1);
} /* end get_mf_disp_info() */




/*
 *	Name:		get_media_path()
 *
 *	Description:	Get a pathname to the media device.  Returns 1 if
 *		everthing went ok, 0 if there is a non-fatal error and -1
 *		if there was a fatal error.
 */

int
get_media_path(soft_p)
	soft_info *	soft_p;
{
	char		dev[SMALL_STR];		/* device pathname */
	char		cdev[SMALL_STR];	/* basic device name */
	int		ret_code;		/* return code */


	/*
	 *      If there is a cd_rom device, don't do anything, for there is
	 *      no need to check the media paths.
	 */
	if (soft_p->media_type == MEDIAT_CD_ROM) 
		return(1);

	(void) strcpy(cdev,cv_media_to_str(&soft_p->media_dev));

	/*
	 * diskettes are at a known path, but must use the raw device
	 */
	if (soft_p->media_type == MEDIAT_FLOPPY) {
		(void) sprintf(soft_p->media_path, "/dev/r%s", cdev);
		return(1);	/* verify_diskette will check access */
	}

	/*
	 *	first check for xt/mt name conflict
	 */
	if (!strcmp(cdev,"xt0"))
		(void)strcpy(cdev,"mt0");
	/*
	 *	Hack to support 3E.  It doesn't do auto-density so
	 *	we make sure we use the QIC-24 device for st0
	 */
	if (!strcmp(cdev,"st0"))
		(void)strcpy(cdev,"st8");
tryagain:

	/*
	 *	Always check for a no-rewind device first
	 */
	(void) sprintf(dev, "/dev/nr%s", cdev);

	if (soft_p->media_loc == LOC_REMOTE)
		ret_code = ck_remote_path(soft_p, dev);
	else
		ret_code = access(dev, R_OK) == 0 ? 1 : 0;

	/*
	 *	Note: error conditions are ignored when looking for a
	 *	no-rewind device, except in the case of access denied.      
	 */
	switch(ret_code) {
	case 1 :
		(void) strcpy(soft_p->media_path, dev);
		return(1);
	case -1 : /*
		   *	Permission was denied on remote host, so return.
		   */
		return(-1);
	}

	/*
	 *	Check for regular device name if a no-rewind device
	 *	cannot be found.
	 */
	(void) sprintf(dev, "/dev/%s", cdev);

	if (soft_p->media_loc == LOC_REMOTE)
		ret_code = ck_remote_path(soft_p, dev);
	else
		ret_code = access(dev, R_OK) == 0 ? 1 : 0;

	switch(ret_code) {
	case 1:
		(void) strcpy(soft_p->media_path, dev);
		break;

	case 0:
		/*
		 * If we can't find st8 for the 3E hack then try st0
		 */
		if (!strcmp(cdev,"st8")) {
			(void)strcpy(cdev,"st0");
			goto tryagain;
		}
		menu_mesg("%s: is not a valid media device.", dev);
		break;

	case -1:
		/*
		 *	Error message is done is ck_remote_path()
		 */
		break;
	}

	return(ret_code);
} /* end get_media_path() */
 


/*
 *	Name:		required_software()
 *
 *	Description:	Check to see if all the required software for an
 *			architecture has been loaded or selected already.
 *			If it has not, we can't boot a diskless client, so
 *			we then unselect all software and screem at the
 *			user. 
 *
 *	Return Value : 1 : if all required software has been selected or
 *				loaded.
 *		      -1 : if all required software has not been selected or
 *				loaded.
 *
 */

int
required_software(soft_p, sys_p)
	soft_info *	soft_p;
	sys_info  *	sys_p;
{
	media_file *	mf_p;
	int		required_count = 0;
	int		required_sel = 0;

	/*
	 *	If this system has an ethernet interface, it needs
	 *	networking and hence we determine the "Networking" module to
	 *	be required
	 */
	for (mf_p = soft_p->media_files;
	     mf_p < &soft_p->media_files[soft_p->media_count];
	     mf_p++) {

		if (sys_p->sys_type == ETHER_NONE) {
			if (mf_p->mf_iflag == IFLAG_REQ)
				required_count++;
		} else {
			if (strcmp(mf_p->mf_name, "Networking") == 0) {
				required_count++;
				if (mf_p->mf_select || mf_p->mf_loaded)
					required_sel++;
			} else  {
				if (mf_p->mf_iflag == IFLAG_REQ)
					required_count++;
			}
		}

		if ((mf_p->mf_iflag == IFLAG_REQ) &&
		    (mf_p->mf_select || mf_p->mf_loaded))
			required_sel++;
	}

	/*
	 *	If the number of required modules is not equal to the number
 	 *	loaded and selected, then we can't load this software and
 	 *	everthing gets deselected
 	 */

	if (required_sel == required_count)
		return(1);
	else {
		reset_selected_media(soft_p);
		return(-1);
	}
			
}

