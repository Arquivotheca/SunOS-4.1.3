#ifndef lint
#ifdef SunB1
static  char    sccsid[] = 	"@(#)main.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static  char    sccsid[] = 	"@(#)main.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */
 
/*
 *	Name:		main.c
 *
 *	Description:	This is the main driver for the system installation
 *		tool.  This program uses a menu based interface for getting
 *		information from the user about the system's configuration.
 *
 *	Overview :
 *
 *                              main driver
 *                                   |
 *			       get_host_info
 *				     |
 *			       get_disk_info
 *                                   |
 *			     get_software_info
 *                                   |
 *			      get_client_info
 *                                   |
 *			        get_sec_info
 *                                   |
 *                              installation
 */
 
#include <sys/file.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sysexits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "install.h"
#include "main_impl.h"


/*
 *	External functions:
 */
extern	time_t		getdate();
extern	char *		sprintf();


/*
 *	Local functions:
 */
static	void		ask_for_time();
static	void		get_timezone();
static	int		is_northam();
static	void		print_ddmmyy();
static	void		print_mmddyy();


/*
 *	Global variables:
 */
#ifdef SunB1
int		old_mac_state;			/* old mac-exempt state */
#endif /* SunB1 */
char *		progname;

main(argc, argv)
	int		argc;
	char **		argv;
{
	menu *		menu_p;			/* pointer to MAIN menu */
	sys_info	sys;			/* system information */

    char *new_argv[3];


#ifdef lint
	argc = argc;
#endif

	(void) umask(UMASK);			/* set file creation mask */

	progname = basename(argv[0]);		/* get name for error mesgs */

	/*
	 *	Only superuser can do damage with this command
	 */
	if (suser() != 1) {
		(void) fprintf(stderr, "%s: must be superuser\n", progname);
		exit(EX_NOPERM);
	}

	/*
	 *	Ignore the 4.0 "repaint"
	 */
	(void) signal(SIGQUIT, SIG_IGN);

	/*
	**  get installation method
	*/
	switch(get_install_method()) {
#ifdef REPREINSTALLED
	case RE_PREINSTALLED :
        *new_argv=*argv;
        *(new_argv+1)="-r";
        *(new_argv+2)=(char *) 0;
		if (execv (EASYINSTALL, new_argv) != 1) {
			(void) menu_log("%s:\tUnable to execute %s\n",
					progname, "easyinstall");
			exit(1);
        }
#endif REPREINSTALLED
	case EASY_INSTALL :
               	argv[0] = EASYINSTALL;
		*(argv+1)=(char *) NULL;
		if (execv (EASYINSTALL, argv) != 1) {
			(void) menu_log("%s:\tUnable to execute %s\n",
					progname, "easyinstall");
			exit(1);
		}
		break;
	case EXIT_INSTALL :
		exit(0);
	}

	set_menu_log(LOGFILE);

	/*
	 *      Read sys_info from user defined info or default file
	 */
	if (read_sys_info(SYS_INFO, &sys) != 1 &&
	    read_sys_info(DEFAULT_SYS_INFO, &sys) != 1) {
		menu_log("%s: Error in %s.", progname, DEFAULT_SYS_INFO);
		menu_abort(1);
	}

	get_terminal(sys.termtype);		/* get terminal type */

	if (is_miniroot()) {
		get_timezone(&sys);		/* get timezone */

		ask_for_time(sys.timezone);	/* ask for the time */

		if (save_sys_info(SYS_INFO, &sys) != 1)
			menu_abort(1);
	}

	menu_p = create_main_menu(&sys);	/* create MAIN menu */

	set_items(menu_p, &sys);		/* set up menu items */

	/*
	 *	Ignore the 4.0 "repaint"
	 */
	(void) signal(SIGQUIT, SIG_IGN);

	if (use_menu(menu_p) != 1)		/* use MAIN menu */
		menu_abort(1);

	(void) signal(SIGQUIT, SIG_DFL);

	end_menu();				/* done with menu system */

	exit(0);
	/*NOTREACHED*/
} /* end main() */




/*
 *	Name:		ask_for_time()
 *
 *	Description:	Ask the user what time it is and set the time if
 *		the user changes it.
 */

static	void
ask_for_time(timezone)
	char *		timezone;
{
	char *		ascii_p;		/* ptr to time in ASCII */
	char		buf[BUFSIZ];		/* input buffer */
	int		done;			/* done with input? */
	int		need_set = 0;		/* need to set the time? */
	struct timeval	now;			/* buffer for the time */
	struct tm *	local_p;		/* pointer to local time */


	(void) gettimeofday(&now, (struct timezone *) NULL);

	while (1) {
		local_p = localtime(&now.tv_sec);
		ascii_p = asctime(local_p);

		(void) printf(
		   "\n\nIs this the correct date/time: %.20s%s%s\n[y/n] >> ",
			      ascii_p,
			      local_p->tm_zone ? local_p->tm_zone : "",
			      ascii_p + 19);
		(void) fflush(stdout);

		get_stdin(buf);

		switch (buf[0]) {
		case 'N':
		case 'n':
			need_set = 1;
			break;

		case 'Y':
		case 'y':
			if (need_set &&
			    settimeofday(&now, (struct timezone *) NULL) < 0) {
#ifndef TEST_JIG
				menu_log("%s: cannot set date and time.",
					 progname);
				menu_abort(1);
#endif TEST_JIG
			}
			return;
		}

		done = 0;
		while (!done) {
			if (is_northam(timezone))
				print_mmddyy();
			else
				print_ddmmyy();

			get_stdin(buf);

			now.tv_sec = getdate(buf, is_northam(timezone));
			if (now.tv_sec >= 0)
				done = 1;
			else {
				(void) printf(
					 "That date and time is not valid.\n");
				(void) fflush(stdout);
				continue;
			}
		}
	} /* end while(1) */
} /* end ask_for_time() */




/*
 *	Name:		get_timezone()
 *
 *	Description:	Ask the user if the timezone is valid or ask the
 *		user for a timezone.  If the user enters a '?', then the
 *		timezone menu is invoked.  Setup the localtime link once
 *		the timezone has been established.
 */

static	void
get_timezone(sys_p)
	sys_info *	sys_p;
{
	char		buf[BUFSIZ];		/* input buffer */
	int		done;			/* done with input? */
	char		pathname[MAXPATHLEN];	/* path to timezone file */
	menu *		tz_p = NULL;		/* pointer to timezone menu */
	struct stat	stat_buf;		/* stat buffer for dir test */

	if (strlen(sys_p->timezone)) {
		/*
		 *	Confirm the timezone with the user.
		 */
		done = 0;
		while (!done) {
			(void) printf(
			    "\nIs this the correct time zone: %s\n\n[y/n] >> ",
				      sys_p->timezone);
			(void) fflush(stdout);

			get_stdin(buf);

			switch (buf[0]) {
			case 'N':
			case 'n':
				bzero(sys_p->timezone, sizeof(sys_p->timezone));
				/*
				 *	Fall through here
				 */
			case 'Y':
			case 'y':
				done = 1;
				break;
			}
		}
	}

	while (!strlen(sys_p->timezone)) {
		bzero(buf, sizeof(buf));

		(void) printf(
		"\nEnter the local time zone name (enter ? for help):\n\n>> ");
		(void) fflush(stdout);

		get_stdin(buf);

		if (buf[0] == '?') {
			if (tz_p == NULL)
				tz_p = create_tz_menu(buf);
			else
				init_menu();
			
			/*
			 *	Ignore the 4.0 "repaint"
			 */
			(void) signal(SIGQUIT, SIG_IGN);
			
			(void) use_menu(tz_p);

			end_menu();
		}
		
		(void) sprintf(pathname, "%s/%s", ZONEINFO, buf);
		
		if ((stat(pathname, &stat_buf) == -1) ||
		    (S_ISDIR(stat_buf.st_mode)))  {
			(void) printf("'%s': is not a valid time zone.\n",
				      buf);
			(void) fflush(stdout);
			continue;
		}
		
		(void) strcpy(sys_p->timezone, buf);
	}
	
	mk_localtime(sys_p, CP_NULL, CP_NULL);

	tzsetwall();
	
} /* end get_timezone() */




/*
 *	Name:		is_client_ok()
 *
 *	Description:	Is the information about a client ok?
 */

int
is_client_ok(sys_p)
	sys_info *	sys_p;
{
	arch_info	*arch_list, *ap;	/* architecture info */
	char		buf[BUFSIZ];		/* input buffer */
	clnt_info	clnt;			/* client info buffer */
	FILE *		fp;			/* ptr to client_list.<arch> */
	char		pathname[MAXPATHLEN];	/* general pathname buffer */
	int		ret_code;		/* return code */


	/*
	 *	System is not a server so the client stuff cannot be okay.
	 */
	if (sys_p->sys_type != SYS_SERVER) {
		return(0);
	}

	/*
	 *	If there are no architectures, then client stuff is not ok.
	 */
	ret_code = read_arch_info(ARCH_INFO, &arch_list);
	if (ret_code != 1) {
		return(0);
	}

	for (ap = arch_list; ap ; ap = ap->next) {

		(void) sprintf(pathname, "%s.%s", CLIENT_LIST, 
			    ap->arch_str);

		fp = fopen(pathname, "r");
		/*
		 *	It is okay if there are no clients for this
		 *	architecture.
		 */
		if (fp == NULL)
			continue;
						/* read each client name */
		while (fgets(buf, sizeof(buf), fp)) {
			delete_blanks(buf);

			(void) sprintf(pathname, "%s.%s", CLIENT_STR, buf);

			/*
			 *	Make sure we can read the client info.  Treat
			 *	fatal and non-fatal errors the same.
			 */
			if (read_clnt_info(pathname, &clnt) != 1) {
				(void) fclose(fp);
				free_arch_info(arch_list);
				return(0);
			}
		}

		(void) fclose(fp);
	}

	free_arch_info(arch_list);

	/*
	 *	It is okay if there are no clients at all.
	 */

	return(1);
} /* end is_client_ok() */




/*
 *	Name:		is_disk_ok()
 *
 *	Description:	Is the information about a disk ok?
 */

int
is_disk_ok()
{
	char		buf[BUFSIZ];		/* input buffer */
	disk_info	disk;			/* disk info buffer */
	int		disk_count = 0;		/* number of valid disks */
	FILE *		fp;			/* pointer to disk_list */
	char		pathname[MAXPATHLEN];	/* general pathname buffer */


	fp = fopen(DISK_LIST, "r");
	if (fp == NULL)
		return(0);

	while (fgets(buf, sizeof(buf), fp)) {	/* read each disk name */
		delete_blanks(buf);

		(void) sprintf(pathname, "%s.%s", DISK_INFO, buf);
		if (read_disk_info(pathname, &disk) == 1)
			disk_count++;
		else {
			(void) fclose(fp);
			return(0);
		}
	}

	(void) fclose(fp);

	/*
	 *	If there are no disks, then the disk stuff is not ok.
	 */
	if (disk_count == 0)
		return(0);

	return(1);
} /* end is_disk_ok() */





/*
 *	Name:		is_northam()
 *
 *	Description:	Determine if the timezone is in North America.
 */

static	char *		northam_prefixes[] = {
	"US/",
	"Canada/",
	"EST",
	"CST",
	"MST",
	"PST",

	CP_NULL
};

static	int
is_northam(timezone)
	char *		timezone;
{
	char **		cpp;			/* ptr to prefix pointer */


	for (cpp = northam_prefixes; *cpp; cpp++)
		if (strncmp(timezone, *cpp, strlen(*cpp)) == 0)
			return(1);

	return(0);
} /* end is_northam() */




/*
 *	Name:		is_sec_ok()
 *
 *	Description:	Is the information about security ok?
 */

int
is_sec_ok(sys_p)
	sys_info *	sys_p;
{
	arch_info	*arch_list, *ap;	/* architecture info */
	char		buf[BUFSIZ];		/* input buffer */
	FILE *		fp;			/* ptr to client_list.<arch> */
	char		pathname[MAXPATHLEN];	/* general pathname buffer */
	int		ret_code;		/* return code */
	sec_info	sec;			/* security info buffer */


	(void) sprintf(pathname, "%s.%s", SEC_INFO, sys_p->hostname0);
	if (read_sec_info(pathname, &sec) != 1)
		return(0);

	/*
	 *	System is not a server no need to check client security.
	 */
	if (sys_p->sys_type != SYS_SERVER) {
		return(1);
	}

	/*
	 *	If there are no architectures, then client security
	 *	stuff is not ok.
	 */
	ret_code = read_arch_info(ARCH_INFO, &arch_list);
	if (ret_code != 1) {
		return(0);
	}

	for (ap = arch_list; ap ; ap = ap->next) {
		(void) sprintf(pathname, "%s.%s", CLIENT_LIST, 
			    ap->arch_str);

		fp = fopen(pathname, "r");
		/*
		 *	It is okay if there are no clients for this
		 *	architecture.
		 */
		if (fp == NULL)
			continue;
						/* read each client name */
		while (fgets(buf, sizeof(buf), fp)) {
			delete_blanks(buf);

			(void) sprintf(pathname, "%s.%s", SEC_INFO, buf);

			/*
			 *	Make sure we can read the client's security
			 *	info.  Treat fatal and non-fatal errors the
			 *	same.
			 */
			if (read_sec_info(pathname, &sec) != 1) {
				(void) fclose(fp);
				free_arch_info(arch_list);
				return(0);
			}
		}

		(void) fclose(fp);
	}

	free_arch_info(arch_list);
	return(1);
} /* end is_sec_ok() */




/*
 *	Name:		is_soft_ok()
 *
 *	Description:	Is the information about the software ok?
 */

int
is_soft_ok()
{
	arch_info	*arch_list, *ap;	/* architecture info */
	char		pathname[MAXPATHLEN];	/* general pathname buffer */
	int		soft_count = 0;		/* # of valid software */

	/*
	 *	Software info structure must be declared static since
	 *	read_soft_info() allocates and frees its own run-time memory.
	 */
	static	soft_info	soft;


	/*
	 *	If there are no architectures, then software stuff is not ok.
	 */
	if (read_arch_info(ARCH_INFO, &arch_list) != 1)
		return(0);

	for (ap = arch_list; ap ; ap = ap->next) {
		/*
		 *	The software for this architecture is not valid unless
		 *	both soft_info and media_file can be read.
		 */
		(void) sprintf(pathname, "%s.%s", SOFT_INFO,
			       ap->arch_str);
		if (read_soft_info(pathname, &soft) != 1) {
			free_arch_info(arch_list);
			return(0);
		}
		(void) sprintf(pathname, "%s.%s", MEDIA_FILE,
			       ap->arch_str);
		if (read_media_file(pathname, &soft) == 1)
			soft_count++;
		else	{
			free_arch_info(arch_list);
			return(0);
		}
	}

	/*
	 *	If there is no software, then the software stuff is not ok.
	 */
	free_arch_info(arch_list);

	if (soft_count == 0)
		return(0);

	return(1);
} /* end is_soft_ok() */




/*
 *	Name:		is_sys_ok()
 *
 *	Description:	Is the information about the system ok?
 */

int
is_sys_ok(sys_p)
	sys_info *	sys_p;
{
	if (read_sys_info(SYS_INFO, sys_p) != 1)
		return(0);

	if (strlen(sys_p->hostname0) == 0)
		return(0);

	if (strcmp(sys_p->hostname0, "noname") == 0)
		return(0);

	if (sys_p->yp_type != YP_NONE &&
	    strcmp(sys_p->domainname, "noname") == 0)
		return(0);

	return(1);
} /* end is_sys_ok() */




static	char *		ddmmyy_lines[] = {
"",
"Enter the current date and local time (e.g. 09/03/88 12:20:30); the date",
"may be in one of the following formats:",
"",
"        dd/mm/yy",
"        dd/mm/yyyy",
"        dd.mm.yyyy",
"        dd-mm-yyyy",
"        dd-mm-yy",
"        month dd, yyyy",
"        dd month yyyy",
"",
"and the time may be in one of the following formats:",
"",
"        hh am/pm",
"        hh:mm am/pm",
"        hh.mm",
"        hh:mm am/pm",
"        hh.mm",
"        hh:mm:ss am/pm",
"        hh:mm:ss",
"        hh.mm.ss am/pm",
"        hh.mm.ss",
"",

CP_NULL
};


/*
 *	Name:		print_ddmmyy()
 *
 *	Description:	Print a date and time prompt in non-North
 *		American style.
 */

static	void
print_ddmmyy()
{
	char **		cpp;			/* ptr to string to print */


	for (cpp = ddmmyy_lines; *cpp; cpp++)
		(void) printf("%s\n", *cpp);

	(void) printf(">> ");
	(void) fflush(stdout);
} /* end print_ddmmyy() */




static	char *		mmddyy_lines[] = {
"",
"Enter the current date and local time (e.g. 03/09/88 12:20:30); the date",
"may be in one of the following formats:",
"",
"        mm/dd/yy",
"        mm/dd/yyyy",
"        dd.mm.yyyy",
"        dd-mm-yyyy",
"        dd-mm-yy",
"        month dd, yyyy",
"        dd month yyyy",
"",
"and the time may be in one of the following formats:",
"",
"        hh am/pm",
"        hh:mm am/pm",
"        hh.mm",
"        hh:mm am/pm",
"        hh.mm",
"        hh:mm:ss am/pm",
"        hh:mm:ss",
"        hh.mm.ss am/pm",
"        hh.mm.ss",
"",

CP_NULL
};


/*
 *	Name:		print_mmddyy()
 *
 *	Description:	Print a date and time prompt in North American style.
 */

static	void
print_mmddyy()
{
	char **		cpp;			/* ptr to string to print */


	for (cpp = mmddyy_lines; *cpp; cpp++)
		(void) printf("%s\n", *cpp);

	(void) printf(">> ");
	(void) fflush(stdout);
} /* end print_mmddyy() */
