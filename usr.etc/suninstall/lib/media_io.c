#ifndef lint
#ifdef SunB1
static  char    sccsid[] = 	"@(#)media_io.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static  char    sccsid[] = 	"@(#)media_io.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		media_io.c
 *
 *	Description:	This file contains all the routines necessary for
 *		manipulating the media device.  This includes some hairy
 *		manipulations for CD_ROM.
 */

#include <stdio.h>
#include "install.h"
#include "menu.h"
#include "media.h"
#include <sys/stat.h>
#include "mktp.h"
#include <unistd.h>

/*
 *	External functions:
 */
extern	char *		sprintf();

/*
 *	Local functions:
 */
static 	int		setup_cd_rom();
static 	int		mount_cd_rom();

/*
 *	Name:		media_extract()
 *
 *	Description:	Extract the given media file.  Uses 'soft_p' to
 *		determine attributes about the media device, 'sys_p' to
 *		determine if the soft arch is the same as the machine
 *		arch, and 'mf_p' to get information about the media
 *		file itself.
 *
 *		Returns 1 if everything is okay, and 0 if there was a
 *		non-fatal error.
 */

int
media_extract(soft_p, sys_p, mf_p)
	soft_info *	soft_p;
	sys_info *	sys_p;
	media_file *	mf_p;
{
	char		cmd[MAXPATHLEN * 3];	/* command buffer */
	char		blocks[MEDIUM_STR];	/* # blocks for the media */
	Os_ident	soft_os;		/* software os */
	
#ifdef lint
	sys_p = sys_p;
#endif lint

	/*
	 *	XXX put in media specific place!
	 */
	
	/*
	 * 	skip to the correct file
	 */
	if (media_fsf(soft_p, mf_p->file_no, REDISPLAY) != 1)
		return(0);

	(void) fill_os_ident(&soft_os, soft_p->arch_str);
	
	menu_log("Extracting the %s '%s' media file.",
		 os_name(soft_p->arch_str),
		 mf_p->mf_name);

	bzero(cmd, sizeof(cmd));

	/*
	 *	XXX probably ought to split the code out into separate
	 *	functions too
	 */
	switch (soft_p->media_type) {
	case MEDIAT_CD_ROM:
		/*
		 * 	If this is a CD_ROM or any disk for that matter,
		 * 	implement the file naming scheme
		 * 	for the modules.
		 */
		(void) sprintf(soft_p->media_path, "%s%s",
			       INSTALL_TAR_DIR,
			       std_cd_path(mf_p->mf_loadpt,
					   soft_p->arch_str, mf_p->mf_name));

		/*
		 *      Note that this is  k bytes
		 */
		(void) sprintf(blocks, "%dk", media_block_size(soft_p));

		goto use_dd;
		/* NOTREACHED */

	case MEDIAT_TAPE:
		/*
		 *      Note that this is blocks, not k
		 */
		(void) sprintf(blocks, "%db", media_block_size(soft_p));

use_dd:
		if (soft_p->media_loc == LOC_REMOTE) {
			/*
			 * The redirect from /dev/null is necessary to prevent
			 * the child of rsh from hanging around.
			 */
			(void)sprintf(cmd,
			    "rsh -n %s exec dd bs=%s if=%s 2>> %s | ",
			    soft_p->media_host, blocks,
			    soft_p->media_path, LOGFILE);
			/*
			 * Compressed?
			 */
			if (mf_p->mf_type == TARZ) {
				(void)strcat(cmd, "uncompress -c | ");
			}
			/*
			 * The software and system architectures are
			 * the same so read it all in.
			 */
			(void) sprintf(&cmd[strlen(cmd)],
#ifdef SunB1
					       "tar xpfBHZ - 2>> %s",
#else
					       "tar xpfB - 2>> %s",
#endif /* SunB1 */
					       LOGFILE);
		} else {
			/*
			 * The software and system architectures are
			 * the same so read it all in.
			 *
			 * Compressed?
			 */
			if (mf_p->mf_type == TARZ) {
				(void) sprintf(&cmd[strlen(cmd)],
#ifdef SunB1
				    "tar xpfbHZ %s %s 2>> %s",
#else
		    "dd if=%s ibs=20b | uncompress -c | tar xpfB - 2>> %s",
#endif SunB1
				    soft_p->media_path, LOGFILE);
			} else {
				(void) sprintf(&cmd[strlen(cmd)],
#ifdef SunB1
				   "tar xpfbHZ %s %s 2>> %s",
#else
			           "tar xpfb %s %s 2>> %s",
#endif SunB1
				    soft_p->media_path, blocks, LOGFILE);
			}
		}

#ifdef TEST_JIG
		menu_log("%s", cmd);
#else
		/*
		 *	Do not use x_system() here since the media device
		 *	may be off-line and we don't want to be unfriendly.
		 */
		macex_on();
		if (system(cmd) != 0) {
			macex_off();
			menu_mesg("%s: cannot extract file.",
				  soft_p->media_path);
			menu_log("%s: '%s' failed.", progname, cmd);
			return(0);
		}
		macex_off();
#endif
		break;

	case MEDIAT_FLOPPY:
/* XXX NOTE: remote floppy disallowed at selection time? ??? */
		if (soft_p->media_loc == LOC_REMOTE) {
			menu_mesg("remote floppy install not allowed");
			menu_log("remote floppy install not allowed");
			return (0);
		}
		macex_on();

		/*
		 * the usage of extract_diskette is:
		 * extract_diskette arch device volno bs name [server]
		 *	arch - machine architecture
		 *	device - full pathname of device
		 *	volno - which volume the file starts on
		 *	bs - blocksize is ignored
		 *	name - Suninstall catagory to be extracted
		 * it handles its own logging to LOGFILE
		 *
		 */
		(void) sprintf(cmd,
		    "exec /usr/etc/install/extract_diskette %s %s %d 99 %s",
		    soft_os.impl_arch, soft_p->media_path,
		    mf_p->media_no, mf_p->mf_name);
		if (system(cmd) != 0) {
			macex_off();
			menu_mesg("%s: cannot extract file.",
			    soft_p->media_path);
			menu_log("%s: '%s' failed.", progname,
			    "extract_diskette");
			return(0);
		}
		macex_off();
		break;

	default:
		menu_mesg("media_extract: unknown media type: %d.",
		    soft_p->media_type);
		return (0);
	}

	return(1);
} /* end media_extract() */




/*
 *	Name:		media_fsf()
 *
 *	Description:	Skip forward 'count' files on the media.  Uses
 *		'soft_p' to determine attributes about the media device.
 *		Returns 1 if everything is okay, and 0 if there was a
 *		non-fatal error.
 */

int
media_fsf(soft_p, count, redisplay)
	soft_info *	soft_p;
	int		count;
	int		redisplay;
{
	char		cmd[MAXPATHLEN + BUFSIZ]; /* command buffer */
	int		ret_code;
	struct stat	buf;


	switch (soft_p->media_type) {
	case MEDIAT_CD_ROM:
		/* No positioning necessary for CD_ROMS */
		return(1);

	case MEDIAT_FLOPPY:
		/* floppy handles its own re-positioning at extract time */
		return(1);

	case MEDIAT_TAPE:
		break;

	default:
		menu_mesg("%s: unknown media type %d.", progname,
		    soft_p->media_type);
		return(0);
	}

	bzero(cmd, sizeof(cmd));

	menu_flash_on("Positioning media");

	if (soft_p->media_loc == LOC_REMOTE) {
		/*
		 *	If this fails, try mt rew and then mt fsf and try
		 *	again by rewinding and then fsf'ing (in case the
		 *	remote OS does not understand the asf option.
		 *
		 *	One problem here is that even if asf is not
		 *	understood, the "rsh" might work and return a 0 exit
		 *	status, and system() will succeed.  This is why we
		 *	will not /dev/null the output, but put it in a file
		 *	/tmp/rsh.out
		 */
		(void) sprintf(cmd,
		 	"rsh -n %s exec mt -f %s asf %d 2> /tmp/rsh.out",
			    soft_p->media_host,
			    soft_p->media_path,
			    count);

		ret_code = system(cmd);
		if (stat("/tmp/rsh.out", &buf) != 0) {
			menu_log("stat failed on /tmp/rsh.out");
			return(0);
		}
		unlink("/tmp/rsh.out");	 /* clean up */
		 
		if ((ret_code == 0) && (buf.st_size == (off_t)0)) {
			menu_flash_off(REDISPLAY);
			return(1);
		} else {
			/*
			**	The sucker failed, so try again down below.
			*/
			if ((ret_code = media_rewind(soft_p, NOREDISPLAY))
			    != 1) {
				menu_flash_off(REDISPLAY);
				return(ret_code);
			}
		(void) sprintf(cmd,
		 	"rsh -n %s exec mt -f %s fsf %d 2> /dev/null",
			       soft_p->media_host,
			       soft_p->media_path,
			       count);
		}
	} else {
		/*
		 * use asf for local tapes
		 */
		(void) sprintf(cmd, "exec mt -f %s asf %d 2> /dev/null",
			       soft_p->media_path, count);
	}
		

	/*
	 *	Do not use x_system() here since we need to call
	 *	menu_flash_off() to clean up the display.  Also the
	 *	media device may be off-line and we don't want to
	 *	be unfriendly.
	 */
	if (system(cmd) != 0) {
		menu_flash_off(REDISPLAY);
		menu_mesg(
		"%s: cannot position media device %s to file number %d.",
			  progname, soft_p->media_path, count);
		return(0);
	}

	menu_flash_off(redisplay);
	return(1);
} /* end media_fsf() */




/*
 *	Name:		media_read_file()
 *
 *	Description:	Read the current media file into 'name.  Uses
 *		'soft_p' to determine attributes about the media device.
 * NOTE: this turns out the be used *ONLY* for the XDRTOC, and CD_ROM and
 *	floppy depend on that.
 *
 *	Return Value: 1 : if everything is okay,
 *		     -1 : if there was an error.
 *	
 */

int
media_read_file(soft_p, name, redisplay)
	soft_info *	soft_p;
	char *		name;
	int		redisplay;
{
	char		cmd[MAXPATHLEN + BUFSIZ]; /* command buffer */
	char		blocks[MEDIUM_STR];	/* #blocks for dumping */

	menu_flash_on("Reading media file");

	bzero(cmd, sizeof(cmd));

	switch (soft_p->media_type) {
	case MEDIAT_CD_ROM:

		if (setup_cd_rom(soft_p) != 1)
			return(-1);

		/*
		 * 	Don't bother reading the media file, just put it
		 * 	where you want it to be : "XDRTOC".
		 */
		(void) sprintf(soft_p->media_path, "%s%s",
			       INSTALL_TAR_DIR,
			       std_cd_path("impl",soft_p->arch_str, "XDRTOC"));


		/* Note that this is  k bytes */
		(void) sprintf(blocks, "%dk", media_block_size(soft_p));
		goto use_dd;
		/* NOTREACHED */

	case MEDIAT_TAPE:
		/* Note that this is blocks, not k */
		(void) sprintf(blocks, "%db", media_block_size(soft_p));

use_dd:
		if (soft_p->media_loc == LOC_REMOTE)
			(void) sprintf(cmd, "rsh -n %s ", soft_p->media_host);

		(void) sprintf(&cmd[strlen(cmd)],
			"exec dd bs=%s if=%s > %s 2>> %s", blocks,
			soft_p->media_path, name, LOGFILE);

		/*
		 *	Do not use x_system() here since we need to call
		 *	menu_flash_off() to clean up the display.  Also the
		 *	media device may be off-line and we don't want to
		 *	be unfriendly.
		 */
		if (system(cmd) != 0) {
			menu_flash_off(REDISPLAY);
			menu_mesg("%s: cannot read file %s from %s",
				  progname,
				  name,
				  soft_p->media_path);
			return(0);
		}
		break;

	case MEDIAT_FLOPPY:
		if (soft_p->media_loc == LOC_REMOTE) {
			menu_mesg("%s: remote floppy install not supported",
			    progname);
			return (0);
		}

		/*
		 * the usage of verify_diskette is:
		 * verify_diskette arch volno file/off device name [server]
		 *
		 * to just read a toc, volno = -1 and toc is on stdout
		 * verify_diskette does its own logging
		 */
		(void) sprintf(cmd,
		    "exec verify_diskette %s -1 0 %s dummy > %s ",
			       (cv_arch_to_str(&soft_p->arch) != NULL) ?
			       cv_arch_to_str(&soft_p->arch) : "-",
			       soft_p->media_path, name);
		if (system(cmd) != 0) {
			menu_flash_off(REDISPLAY);
			menu_mesg("%s: cannot read file %s from %s",
				  progname, name, soft_p->media_path);
			return(0);
		}
		break;

	default:
		menu_flash_off(REDISPLAY);
		menu_mesg("%s: unknown media type %d.",
			  progname, soft_p->media_type);
		return(0);
	}
	
	menu_flash_off(redisplay);
	return(1);
} /* end media_read_file() */




/*
 *	Name:		media_rewind()
 *
 *	Description:	Rewind the media file.  Uses 'soft_p' to determine
 *		attributes about the media device.
 *
 *		Returns 1 if everything is okay, and 0 if there was a
 *		non-fatal error.
 */

int
media_rewind(soft_p, redisplay)
	soft_info *	soft_p;
	int		redisplay;
{
	char		cmd[MAXPATHLEN + BUFSIZ]; /* command buffer */

	switch (soft_p->media_type) {
	case MEDIAT_CD_ROM:
		/* No rewinding necessary for CD_ROMS */
		return(1);

	case MEDIAT_FLOPPY:
		/* floppy handles its own re-positioning at extract time */
		return(1);

	case MEDIAT_TAPE:
		break;

	default:
		menu_mesg("%s: unknown media type %d.", progname,
		    soft_p->media_type);
		return(0);
	}

	bzero(cmd, sizeof(cmd));

	if (soft_p->media_loc == LOC_REMOTE) {
		(void) sprintf(cmd,
			"rsh -n %s exec mt -f %s rew 2> /dev/null",
			       soft_p->media_host,
			       soft_p->media_path);
	} else {
		/* don't rewind local devices (mt's "asf" will do it) */
		return(1);
	}

	menu_flash_off(redisplay);
	menu_flash_on("Rewinding media");

	/*
	 *	Do not use x_system() here since we need to call
	 *	menu_flash_off() to clean up the display.  Also the
	 *	media device may be off-line and we don't want to
	 *	be unfriendly.
	 */
	if (system(cmd) != 0) {
		menu_flash_off(REDISPLAY);
		menu_mesg("%s: cannot rewind media device %s.",
			  progname, soft_p->media_path);
		return(0);
	}

	menu_flash_off(redisplay);

	menu_flash_on("Forwarding media");

	return(1);
} /* end media_rewind() */



/*
 *	Name:		(static int) setup_cd_rom()
 *
 *	Description:	Do all that is necessary to read a CD_ROM. 
 *
 *	Return Value:	1 : if everything is okay,
 *			0 : if there was anon-fatal error.
 *
 */
static int
setup_cd_rom(soft_p)
	soft_info *	soft_p;
{
	sys_info	sys;
	char		cmd[MAXPATHLEN * 3];
	char *		argv[2];	/* for exectution of get_arch_info */
	FILE *		fp;		/* FILE * to the LOAD_ARCH */
	
	/*
	 *	get system info
	 */
	if (read_sys_info(SYS_INFO, &sys) != 1 &&
	    read_sys_info(DEFAULT_SYS_INFO, &sys) != 1) {
		menu_log("%s: Error in %s.", progname, DEFAULT_SYS_INFO);
		menu_abort(1);
	}

	/*
	 *	We must now mount the cdrom, if necessary
	 */
	if (mount_cd_rom(soft_p) != 1)
		return(0);
	
	/*
	 *	Here we only want to invoke "get_arch_info" if we
	 *	don't know what arch to read the toc from. This will
	 *	create the file LOAD_ARCH, so we know what arch to
	 *	load, or at least which XDRTOC to read.
	 */
	if (*(soft_p->arch_str) != '\0')
		return(1);
	
	/*
	 *	So, we have a good mount, let's party and copy the
	 *	AVAIL_ARCHES over to this system as well, if remote.
	 */
	if (soft_p->media_loc == LOC_REMOTE) {
		(void) sprintf(cmd,
		       "exec rcp %s:%s %s >> %s 2>&1", soft_p->media_host, 
			       AVAIL_ARCHES, AVAIL_ARCHES, LOGFILE);
		
		if (system(cmd) != 0) {
			menu_mesg("%s: could not copy %s:", progname,
				  AVAIL_ARCHES);
			return(0);
		}
	}

	/*
	 *	If a system release has not been selected, grab the release
	 *	name from the file RELEASE, and don't even bother invoking
	 *	get_arch_info to get the architecture information.
	 */
	
	if (!sys_has_release(&sys)) {
		if (read_sys_release(soft_p->arch_str) != 0)
			return(-1);
		else
			return(1);
	}

	/*
	 *	Since a release has been chosen by here, let's just go a
	 *	head and get the next release from the architecture screen
	 */
	end_menu();
	/*
	 *	No archicture specified, so invoke get_arch_info.
	 */
	argv[0] = "get_arch_info";
	argv[1] = (char *)NULL;
	
	if (execute(argv) != 1) {
		init_menu();
		return(-1);
	}
	
	init_menu();
	
	if ((fp = fopen(LOAD_ARCH, "r")) == (FILE *)NULL) {
		menu_mesg("%s: failed to open %s", progname,
			  LOAD_ARCH);
		return(-1);
	}
	

	/*
	 *	Ah, LOAD_ARCH was there, so lets read it.
	 */
	if (fgets(soft_p->arch_str, (int) sizeof(soft_p->arch_str), fp)
	    == (char *)NULL) {
		menu_mesg("%s: no architecture specified in  %s",
			  progname, LOAD_ARCH);
		(void)fclose(fp);
		return(-1);
	}
	
	(void)fclose(fp);
	(void)unlink(LOAD_ARCH);	/* 'tis no longer needed */

	return(1);

	/* NOTREACHED */
} /* end setup_cd_rom() */


/*
 *	Name:		(static int) mount_cd_rom()
 *
 *	Description:	Do all that is necessary to mount CD_ROMs. 
 *
 *	Return Value:	1 : if everything is okay,
 *			0 : if there was anon-fatal error.
 *
 */
static int
mount_cd_rom(soft_p)
	soft_info *	soft_p;
{
	char		cmd[MAXPATHLEN];	/* commands */
	struct stat	buf;
	int		ret_code;
	extern char	CD_EXPORT_DIR[];

	(void) bzero(cmd, sizeof(cmd));

	/*
	 *	Mount the cd rom if necessary (AVAIL_ARCHES exists)
	 */
	
	switch(soft_p->media_loc) {
	case LOC_REMOTE :
		if (ck_remote_path(soft_p, CD_EXPORT_DIR))
			return(1);
		/*
		 *	AVAIL_ARCHES isn't there, so mount the CD, remotely
		 */
		if (soft_p->media_loc == LOC_REMOTE)
			(void) sprintf(cmd, "rsh -n %s ", soft_p->media_host);
		break;
	default :
		if (access(CD_EXPORT_DIR, F_OK) == 0) {
			/*
			 *	Ahhh..., its alread mounted
			 */
			return(1);
		}
	}

	/*
	 *	Mount it using the hsfs file system type
	 */
	(void) sprintf(&cmd[strlen(cmd)],
		       "exec mount -rt hsfs /dev/%s %s  > %s 2>&1",
		       cv_media_to_str(&(soft_p->media_dev)),
		       INSTALL_TAR_DIR, "/tmp/rsh.out");

	/*

	 *	One problem here is that even if the mount fails,
	 *	the "rsh" might work and return a 0 exit
	 *	status, and system() will succeed.  This is why we
	 *	will not /dev/null the output, but put it in a file
	 *	/tmp/rsh.out
	 */

	ret_code = system(cmd);

	if (stat("/tmp/rsh.out", &buf) != 0) {
		menu_log("stat failed on /tmp/rsh.out");
		return(0);
	}

	unlink("/tmp/rsh.out");	 /* clean up */
	
	if ((ret_code == 0) && (buf.st_size == (off_t)0)) {
		return(1);
	} else {
		menu_flash_off(REDISPLAY);
		menu_mesg("%s: cannot mount /dev/%s on %s",
			  progname,
			  cv_media_to_str(&(soft_p->media_dev)),
			  INSTALL_TAR_DIR);
		return(0);
	}

	/* NOTREACHED */

} /* end mount_cd_rom() */

