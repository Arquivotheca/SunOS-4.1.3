#ifndef lint
#ifdef SunB1
static  char    sccsid[] = 	"@(#)unpack.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static  char    sccsid[] = 	"@(#)unpack.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 * 	Copyright (c) 1990 by Sun Microsystems, Inc.
 */

/*
 * 	"quickinstall" -- alternative to customized suninstall.
 * 
 * 	This is the one and only "quickinstall" that everyone is talking
 * 	about. It prompts for the minimum information needed to do the
 * 	simplest sort of installation (a standalone) and goes.
 *
 * 	There's a file called CATEGORY_FILE (defined in unpack.h), in which
 * 	there are various pre-defined Categories to from which the uses can
 * 	select.  Each Category is for a different type of user and is
 * 	described in the help menus that the user can select. Each Catgory
 * 	will load a different set of software from the release media.
 * 	
 * 	This program also doubles as the sys-config program used in setting
 * 	up pre-installed systems to get them up quickly, asking many of the
 * 	same minimal questions.
 * 	
 */

#include <stdio.h>
#include <ctype.h>
#include <curses.h>
#include <signal.h>
#include "install.h"
#include "unpack.h"
#include "unpack_help.h"
#include "media.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/reboot.h>
#include <dirent.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/file.h>
#include <string.h>
#include <strings.h>
#include "setjmp.h"

static char info[BUFSIZ*4];
struct dk_geom disk_geom;
struct dk_allmap dk;
static char help_msg[1024];
jmp_buf sjbuf_hostname; 
static jmp_buf sjbuf_timezone,sjbuf_time,sjbuf_net,sjbuf_ip;
static jmp_buf sjbuf_nis,sjbuf_domain,sjbuf_useracct,sjbuf_userfn;
static jmp_buf sjbuf_userun;

/*
 *	return code definitions
 */
#define RC_DONE		0
#define RC_DEAD		1
#define RC_ABORT	2
#define RC_INSTALL	3
#define RC_CONFIG	4
#define RC_RETRY	5

/*
 * 	global junk
 */
extern char *getenv(), *sprintf();
extern long module_size();

char *progname;
static char *suninstall = "/usr/etc/install/suninstall";

/*
 * 	various command line option flags
 */
int rebootflag   = 1;	/* if 1, we do an automatic reboot */
int forceflag    = 0;	/* if 1, we don't ask for confirmation */
int quietflag    = 0;	/* if 1, we don't print anything at all */
int dateflag     = 0;	/* if 1, the date is OK; can be forced on cmd line */
int preserveflag = 0;	/* if 1, no disk relabling is performed */
int nocurses     = 0;	/* if 1, we're not using curses at all */

/*
 *  for Re-Preinstalled
 */
int re_flag      =0;    /* if 1, re-preinstalled is selected */

/* 
 * 	Global preserve string
 */

char preserve[TINY_STR];	/* set to "all" if all but a & g partitions*/
				/* are preserved			   */


/*	EXTERN Functions	    */

int	verify_and_load_toc();
int	int_media_info();


/*	Local Functions	    */
int	setup_env_variable();
short	check_need();
void	do_environment_variables();
void	update_info_string();
int	get_confirmation();
void	outtahere();
int	print_help() ;
void	get_info();
void	set_time();
void	menu_window();
int	get_pw() ;
int	get_yn() ;
void	get_config_info() ;
void	init_curses() ;
int	match() ;
int	get_a_key() ;
struct default_desc *get_default_choice() ;
struct default_desc *find_disc_label() ;
char *	fillin_defaults_menu() ;
void	info_window() ;
void	unpack_init() ;
int	execute() ;
void	check_time() ;
int	getnam() ;
char *	first_token() ;
int	read_modules() ;
struct namelist *in_extractlist() ;
void	fill_module_size() ;
long	add_module_size() ;
int	init_tapemap() ;
int	init_dkmap() ;
char *	remove_quote() ;
struct default_desc *parse_header() ;
int	find_category() ;
int	match_category() ;
int	find_category() ;
int	match_category() ;
int	set_variables() ;
struct default_desc *free_desc() ;
void	skip_block() ;
int	check_disk_space() ;
struct default_desc *find_matches() ;
int	create_files() ;
void	create_cmdfile() ;
void	create_disklist() ;
void	create_mountlist() ;
int	check_ip() ;
void	do_reboot();
int	xlate();
void	show_template();
static	void	update_hosts();

/*
 * 	put "si_key=datum" into the environment
 */
int
setup_env_variable(key,datum)
	char *key;
	char *datum;
{
	char *ep;
	char *malloc();

	if ((ep = malloc((unsigned)(strlen(key)+strlen(datum)+2))) == NULL) {
		return(0);
	}
	(void) sprintf(ep,"%s=%s",key,datum);
	return(putenv(ep));
	
}

main(argc,argv)
	int argc;
	char *argv[];
{

    char *cmdptr;
    char *strrchr();
	FILE *in, *popen();
	sys_info sys;
	struct default_desc *find_matches();
	struct default_desc *get_default_choice();
	struct default_desc *mymatches;
	struct default_desc *mychoice = NULL;
	char buf[BUFSIZ];	/* char buf for misc. purposes */
	extern void sig_trap();
	short need;	/* mask of which pieces of info we need */
	short check_need();
	char disk_device[DEVICENAME];
	soft_info soft;
	int ret_code, i;
	char *impl_str, appl_str[MEDIUM_STR];	/* architecture strings */
	char            pathname[MAXPATHLEN];   /* pathname buffer */
	int jumpto=0; /* where does it go if CONTROL_B is hit from conformation screen */


	/*
	 * these are for dealing with command-line options
	 */
	extern char *optarg;
	extern int optind;
	char opt;
	int opterror = 0;
	char *cp;
	static struct default_desc cmdline_choice = { NULL, NULL, NULL};
    	
        /*
         * get program name
         */
	progname = basename(argv[0]);
	
	bzero(soft,sizeof(soft_info));
	bzero(sys,sizeof(sys_info));
	
	/*
	 * make sure we're root here; exit if we're not
	 */
	if (getuid() != 0) {
		(void)fprintf(stderr,"%s: must be superuser\n",progname);
		exit(1);
	}
	
	set_menu_log(LOGFILE);
	
	/*
	 * For now we hardwire the code only for tapes
	 */
	soft.media_type = MEDIAT_TAPE;		
	
	/*
	 * process arguments
	 */
	while ((opt = getopt(argc, argv, "rpqnfds:")) != -1) {
		switch (opt) {
#ifdef REPREINSTALLED
		case 'r':
			/*
			 * Re-preinstalled has been selected
		     */
			re_flag=1;
			break;
#endif REPREINSTALLED
		case 'n':
			/*
			 * 	don't even show the reboot screen
			 */
			rebootflag = 0;
			break;
		case 's':
			/*
			 * 	set an environment variable using the
			 * 	argument to -s, e.g., '-s x=hikids' would
			 * 	result in an environment variable called
			 * 	'si_x' being set with the value 'hikids'.
			 */ 
			if ((cp = index(optarg,'=')) == NULL) {
				opterror++;
			} else {
				char savechar;
				
				/*
				 * split it into two, prepend 'si_', and
				 * put it in the environment.
				 */
				savechar = *cp;
				*cp = '\0';
				(void) sprintf(buf,"si_%s",optarg);
				(void) setup_env_variable(buf,cp+1);
				*cp = savechar;
			}
			break;
		case 'f':
			/*
			 * 	'force'--don't ask the user for confirmation
			 */
			forceflag++;
			break;
		case 'q':
			/*
			 * 	'quiet'--don't show the information while
			 * 	install is running
			 */
			quietflag++;
			break;
		case 'd':
			/*
			 * 	dateflag--if set, don't ask to check the date
			 */
			dateflag++;
			break;
		case 'p':
			/*
			 * 	preserve_flag--if set, don't change any 
			 * 	partition except / and /usr
			 */
			preserveflag++;
			break;
		case '?':
		default:
			opterror++;
			break;
		}
	}
	if (optind != argc) {
		/*
		 * 	if there's a final argument, it should be the name of
		 * 	the installation template directory
		 */
		cmdline_choice.def_name = argv[optind];
		cmdline_choice.def_desc = "<cmdline choice>";

		/*
		 * 	here we probably ought to check it out a bit ...
		 */
		optind++;
	}

	/*
	 * 	any leftover arguments are a mistake
	 */
	if (optind != argc) {
		opterror++;
	}

	if (opterror) {
		(void)fprintf(stderr,USAGE,progname);
		exit(1);
	}

/*
 * check to see if this is re-preinstalled
 */ 
    if ((cmdptr = strrchr(*argv,'/')) != (char *)NULL) {
        cmdptr++;
    }   else    {
        cmdptr = argv[0];
    }  
    if (*cmdptr=='r')   {
		in = popen("arch -k", "r");
		fscanf(in,"%s",buf);
		pclose(in);
		if ((strcmp(buf, "sun4c") != 0) &&
                    (strcmp(buf, "sun4m") != 0)) {
			printf("Re-preinstalled does not run on non-sun4c or non-sun4m machines.\n");
				exit(1);
		}
        re_flag=1;
        printf("This program will erase all data from your disk.\n");
        printf("You still wish to continue? (answer yes or no)\n");
        get_stdin(buf);
        if (buf[0] != 'y') {
            exit(1);
        }
    }
        /*
	 *	fill in system_info struct from user defined info or default
         *	file. If this is a preinstalled system, this information had
         *	better all be correct except for the info we'll fill in
         *	shortly.
         */
	
        if ((read_sys_info(SYS_INFO, &sys) != 1) &&
            (read_sys_info(DEFAULT_SYS_INFO, &sys) != 1)) {
                menu_log("%s:\tError in %s;\n", progname, DEFAULT_SYS_INFO);
		menu_abort(1);
	}
	
	(void) umask(UMASK);			/* set file creation mask */
	get_ethertypes(&sys);

	/*
	 *  Easyinstall should only recognize ethernet interface 0.
	 */
	for (i = 1; i < MAX_ETHER_INTERFACES; i++)
		*sys.ether_namex(i) = NULL;
	
	(void) strcpy(soft.exec_path, "/usr");
	(void) strcpy(soft.kvm_path, "/usr/kvm");
	(void) strcpy(soft.share_path, "/usr/share");
	
	/*
	 * 	set up some other stuff
	 */
	do_environment_variables(&sys);
	need = check_need(&cmdline_choice,&sys);   

	if (is_miniroot())	{
		if (init_dkmap(disk_device) < 0)    {
			menu_log("%s:\tCan't find disk information\n",progname);
			outtahere(RC_DEAD);
		}
		(void) printf(
	"\n\nPlease wait while the media contents are being checked ...\n");
		if (init_media_info(&soft) < 0)    {
			menu_log(
"%s:\tCan't find media device information.  Please make sure the device is on line\n",
				 progname);
			outtahere(RC_DEAD);
		}
		
		while ((ret_code = verify_and_load_toc(&soft, &sys)) != 1) {
			if (ret_code != 0)
				return(ret_code);
			menu_ack();
		}
	}
	
	soft.choice = SOFT_OWN ;
	
	/*
	 *	get the information we need to configure a machine.  Do it
	 *	over and over if we have to.  The first time through,
	 *	however, we suppress screens, so we pay attention to "need"
	 */
	do {
		if (!re_flag) {
                        if (is_miniroot()) {
                                /* Assume we have a network for easyinstall */
                                (void) setup_env_variable("si_network","yes");
                        }
			get_config_info(&sys,need,jumpto);    
		}

		if (is_miniroot()) {
	
			if (need & NEED_TEMPLATE) {
				/*
			 	 *	find the default suninstall files
			 	 *	that are potential matches for us
			 	 *	and get the user to choose one.
			 	 */
				if ((mymatches= find_matches(&soft)) == NULL) {
					/*
					 *	no matches, we can't use
					 *	unpack. The user will have
					 *	to use suninstall.
					 */
					info_window(NO_MATCH_MSG, LINES,
			    "Press 'a' to abort or any other key to continue",
						    'a',outtahere,RC_DEAD);
					outtahere(RC_DEAD);
				}
				
			if (re_flag) {
					if ((mychoice = find_disc_label(mymatches,disk_device))
								== NULL)	{
							printf("no disc_label found\n");
							exit(1);
					}
                } else {
				    while ((mychoice =
	    				get_default_choice(mymatches, &soft))
				           == NULL) {
				    	info_window(NO_CHOICE_MSG, LINES,
			            "Press 'a' to abort or any other key to continue",
						    'a', outtahere, RC_RETRY);
				    }
                 }
			} else {
				mychoice = &cmdline_choice;
			}
		}
		
		if (re_flag) {
			if (get_yn(NULL,CONF_BANNER," ",
				"Start the Re-preinstalled (THIS WILL DESTROY ALL DATA ON DISK)? [Y/N] : ",
						YES_NO,"n",NULL,NOMENU))
				break;
			else
				exit(1);	
		}
		/*
		 * 	reset 'need' so that get_config_info asks all
		 * 	questions next time around
		 */
			need = NEED_ALL_MASK | ((*sys.ether_name0) 
					? GOT_NETWORK_Y
					: GOT_NETWORK_N);
		if ((jumpto=get_confirmation(&sys,mychoice)) == CONFIRM_B) 
			if (sys.yp_type == YP_CLIENT) 
				jumpto++;	/*jump to domain name screen*/	
	 }while (jumpto != 1); 
	
	/*
	 * 	move any existing sys_info file to sys_info.old and make
	 * 	a new sys_info file.
	 */
	(void) sprintf(buf,"mv %s %s.old >/dev/null 2>&1",
		       SYS_INFO, SYS_INFO);
	(void) system(buf);
	
	/*
	 * set the screen up for the last thing we'll see
	 */
	if (!quietflag) {
		(void) clear();
		if (is_miniroot())
		(void)print_help(NULL,INST_BANNER,info,DONT_CENTER);
		else
		(void)print_help(INST_SCREEN,INST_BANNER,info,DONT_CENTER);
		refresh();
	}
	
	if (is_miniroot()) {
		/*
		 * create the default installation files to the right spot
		 */
		if (create_files(mychoice,&sys, disk_device, &soft) != 1)
			outtahere(RC_DEAD);
	}
	
	if (!is_miniroot())
		update_hosts(&sys);			/* update HOSTS */
	
	/*
	 * write it out to where the configuration and installation utilties
	 * expect to find it.
	 */
	if (save_sys_info(SYS_INFO, &sys) != 1)
		menu_abort(1);
	
	if (is_miniroot()) {
		
		/*
		 * exec "install" and/or "host_config" (currently, "install"
		 * does the job of "host_config", too)
		 */
		if (!execute(INSTALLATION_PROG)) {

			(void) signal(SIGQUIT,SIG_IGN);
#ifdef CATCH_SIGNALS
			(void) signal(SIGINT,sig_trap);
#endif

			menu_log("%s: Installation program failed.\n",
				 progname);
			outtahere(RC_INSTALL);
		}

		/*               
		 *  Touch /etc/.UNCONFIGURED so sys-config will be run
		 *  from rc.boot.
		 */
		sprintf (buf, "mount /dev/%sa %s > /dev/null 2>&1",
			disk_device, dir_prefix());
		if ( system(buf) != 0 ) {
			menu_log("%s:\tCan't mount root fs\n",progname);
			outtahere(RC_DEAD);
		}
 
		(void) sprintf(buf,"> %s/etc/.UNCONFIGURED",dir_prefix());
		system(buf);
		(void) sprintf(buf, "umount %s > /dev/null 2>&1", dir_prefix());
		system(buf);
 

	} else {
		/*
		 * just exec "host_config" here to change relevant files
		 */
		if (!execute(CONFIGURATION_PROG)) {
			(void) signal(SIGQUIT,SIG_IGN);
#ifdef CATCH_SIGNALS
			(void) signal(SIGINT,sig_trap);
#endif
			menu_log("%s: Host configuration failed.\n",progname);
			outtahere(RC_CONFIG);
		}
	}

	if (partitions() == TWO_PARTS) {
		(void) sprintf(pathname, "%s/usr/export/home/%s",
			       dir_prefix(), sys.hostname0);
		mkdir_path(pathname);

		(void) sprintf(pathname, "%s/home", dir_prefix());
		mklink("/usr/export/home", pathname);

	} else {
		(void) sprintf(pathname, "%s/home/%s",
			       dir_prefix(), sys.hostname0);
		mkdir_path(pathname);
		(void) sprintf(pathname, "%s/usr/export", dir_prefix());
		mkdir_path(pathname);
		(void) sprintf(pathname, "%s/usr/export/home", dir_prefix());
		mklink("/home", pathname);
	}

    	/*
	 *	This is a hack here. If we used quickinstall, add_services
	 *	will attempt to make the correct links also, but use the
	 *	default "noname" hostname, because the sys_info was not made
	 *	yet. If it does not exist, forget it.
	 */
    	(void) sprintf(pathname, "%s/home/noname", dir_prefix());
    	if (rmdir(pathname) != 0) {
		if (errno  != ENOENT)
			menu_log("failed to remove %s\n", pathname);
	}
    
    

   /*
    * Do UNCONFIGURE if processing re-preinstalled
    */ 
	if (re_flag) {
		printf("Start UNCONFIGURE.\n");

		(void) sprintf(buf,"echo \"#\" > %s/etc/hosts",dir_prefix());
		system(buf);
		(void) sprintf(buf,"echo \"#\" Sun Host Database >> %s/etc/hosts",
						dir_prefix());
		system(buf);
		(void) sprintf(buf,"echo \"#\" >> %s/etc/hosts",dir_prefix());
		system(buf);
		(void) sprintf(buf,"echo \"#\" If NIS is running, this file is only consulted when booting >> %s/etc/hosts",dir_prefix());
		system(buf);
		(void) sprintf(buf,"echo \"#\" >> %s/etc/hosts",dir_prefix());
		system(buf);
		(void) sprintf(buf,"echo 127.0.0.1 localhost loghost >> %s/etc/hosts",
						dir_prefix());
		system(buf);
		(void) sprintf(buf,"echo \"#\" >> %s/etc/hosts",dir_prefix());
		system(buf);

		(void) sprintf(buf,"rm -f %s/etc/hostname.??[0-9]",dir_prefix());
		system(buf);
		(void) sprintf(buf,"rm -f %s/etc/defaultdomain",dir_prefix());
		system(buf);
		(void) sprintf(buf,"rm -rf %s/var/yp",dir_prefix());
		system(buf);
		(void) sprintf(buf,"rm -f %s/usr/lib/zoneinfo/localtime",
				dir_prefix());
		system(buf);
		(void) sprintf(buf,"echo  > %s/etc/.UNCONFIGURED",dir_prefix());
		system(buf);
		(void) sprintf(buf,"cp %s/usr/kvm/stand/vmunix_small %s/vmunix",
						dir_prefix(),dir_prefix());
		system(buf);
		printf("UNCONFIGURE completed.\n");
	}
	/*
	 * 	give the user the choice of rebooting (if the -n option wasn't
	 * 	specified)
	 */
	if (rebootflag) {
		do_reboot();
	}

	/*
	 * if doing a sys-config, then ask for root passwd, user account
	 */
	/* while is used as "if", so "break" works */
	while (progname[0] == 's') {
		char	uname[128];
		char	ufullname[128];
		char	uid[16];
		char	gid[16];
		char	*cp;
		int	i;

		/*
		 * ask for root passwd
		 */
		get_pw(ROOTPW_SCREEN,ROOTPW_BANNER, ROOTPW_HELP, ROOTPW_HELPSCREEN, "root");

		/*
		 * if !NIS, then we ask if user wants an account
		 * this is slightly bogus - he may want a local account, but
		 * that is what EZnet is for
		 */
/*XXX TODO, add asking even tho NIS... */
		if (sys.yp_type != YP_NONE)
			break;
		setjmp(sjbuf_useracct);
		if (! get_yn(USERACCT_SCREEN,USERACCT_BANNER, USERACCT_HELP,
		    "Do you want to set up a user account(y/n)? ", YES_NO,
		    " ", USERACCT_HELPSCREEN,JUSTHELP)) {
			break;
		}

/* XXX  add asking all the questions for uname, ufullname, uid */
/* XXX get_info(screen,banner,help,prompt,val,extra_help) ***/
		setjmp(sjbuf_userfn);
		bzero(uname, sizeof(uname));
		bzero(ufullname, sizeof(ufullname));
		bzero(uid, sizeof(uid));
		
		get_info(USERACCT_FN_SCREEN,USERACCT_FN_BANNER, USERACCT_FN_HELP,
		    "   User full name => ", ufullname, USERACCT_FN_HELPSCREEN,USERFN_PAGE);

		/* default username is 1st initial - lastname */
		cp = ufullname;
		if (*cp != NULL) {
			uname[0] = tolower(*cp++);
			cp = strrchr(cp, ' ');
			if (cp != NULL) {
				cp++;		/* skip space */
				for( i = 1; *cp && i < 8; i++) {
					uname[i] = tolower(*cp++);
				}
			}
		}
	
		setjmp(sjbuf_userun);	
		get_info(USERACCT_UN_SCREEN,USERACCT_UN_BANNER, USERACCT_UN_HELP,
		    "   User name => ", uname, USERACCT_UN_HELPSCREEN,USERUN_PAGE);
	
	while(1) {	
		get_info(USERACCT_UID_SCREEN,USERACCT_UID_BANNER, USERACCT_UID_HELP,
		    "   User id => ", uid, USERACCT_UID_HELPSCREEN,USERUID_PAGE);
		if ((atoi(uid) >= 10) && (atoi(uid) <= 60000))
			break;
		info_window("Invalid user id.", LINES, "Press any key to continue", RETURN, NULL, (int)0);
		bzero(uid,sizeof(uid));	
		}

		/* staff is 10 */
		strcpy(gid, "10");		/* HARDCODE XXX */
		/*
		 * some things are just so much easier in shell,
		 * such as adding a new user
		 */
		(void)sprintf(buf,
	"/usr/etc/install/add_user %s %s %s \"%s\" /home/%s/%s /bin/csh",
		    uname, uid, gid, ufullname, sys.hostname0, uname );
		if (system(buf)) {
			/* warn user of error? */
			menu_log(
			    "%s:\tWarning, error during adding user account ",
			    progname);
			break;
		}
		/*
		 * ask for user's passwd 
		 */
		get_pw(USERPW_SCREEN,USERPW_BANNER, USERPW_HELP, USERPW_HELPSCREEN, uname);

		break;
	}

	outtahere(RC_DONE);
	/*NOTREACHED*/
}

/*
 *	return a bitfield showing which fields are still needed (if some
 *	fields were specified on the command line).  Also initializes curses
 *	unless everything's been specified on the command line.
 */

short
check_need(ch,sysp)
	struct default_desc *ch;
	sys_info *sysp;
{
	short rval = 0;
	char *ep;

	if ((ep = getenv("si_hostname")) != NULL) {
		(void) strcpy(sysp->hostname0,ep);
	} else {
		rval |= NEED_HOSTNAME;
	}

	if ((ep = getenv("si_timezone")) != NULL) {
		(void) strcpy(sysp->timezone,ep);
	} else {
		rval |= NEED_TIMEZONE;
	}

	if ((ep = getenv("si_network")) != NULL) {
		switch (*ep) {
		case 'y':
			rval |= GOT_NETWORK_Y;
			break;
		case 'n':
			rval |= GOT_NETWORK_N;
			break;
		default:
			rval |= NEED_NETWORK;
			break;
		}
	} else {
		rval |= NEED_NETWORK;
	}

	if ((ep = getenv("si_ip0")) != NULL) {
		(void) strcpy(sysp->ip0,ep);
	} else {
		rval |= NEED_IP0;
	}

	if ((ep = getenv("si_yp_type")) != NULL) {
		if (strcmp(ep, "client") == 0)	{
			sysp->yp_type = YP_CLIENT;
			rval |= GOT_YPTYPE_C;
			rval &= ~GOT_YPTYPE_N;
		}
		else if (strcmp(ep, "none") == 0)   {
			rval |= GOT_YPTYPE_N;
			rval &= ~GOT_YPTYPE_C;
			sysp->yp_type = YP_NONE;
		}
		else
			rval |= NEED_YP_TYPE;
	} else {
		rval |= NEED_YP_TYPE;
	}

	if ((ep = getenv("si_domainname")) != NULL) {
		(void) strcpy(sysp->domainname,ep);
	} else {
		rval |= NEED_DOMAINNAME;
	}

	if ((ep = getenv("si_preserve")) != NULL && !is_miniroot()) {
		if (!strcmp("yes",ep))
			(void) strcpy(preserve, "all");
		else
			(void) strcpy(preserve, "");
	} else {
		if (is_miniroot())
			rval |= NEED_PRESERVE;
	}

	if ((ch->def_name == NULL)  && is_miniroot()) {
		rval |= NEED_TEMPLATE;
	}

	if (rval & GOT_NETWORK_N) {
		rval |= GOT_YPTYPE_N;
		rval &= ~NEED_IP0;
		rval &= ~NEED_YP_TYPE;
		rval &= ~NEED_DOMAINNAME;
	}

	if (rval & GOT_YPTYPE_N) {
		rval &= ~NEED_DOMAINNAME;
	}

	/*
	 *	In theory, if all the flags are set and all the information
	 *	has been gathered, we needn't initialize curses (and
	 *	we'd avoid asking the user to identify his terminal type if
	 *	we're not going to use curses)
	 */

	switch (rval & 0xffff) {
	case OK_1:
	case OK_2:
	case OK_3:
	case OK_4:
	case OK_5:
	case OK_6:
	case OK_7:
	case OK_8:
	case OK_9:
	case OK_10:
	case OK_11:
	case OK_12:
	case OK_13:
	case OK_14:
		if (!rebootflag && quietflag && forceflag && dateflag) {
			/* don't init curses */
			nocurses++;
			break;
		}
		/* else fall through */
	default:
		init_curses(sysp->termtype);
		break;
	}
	
	return(rval);
}


/*
 * 	Determine system architecture and set various si_ environment
 * 	variables (used later by match).
 */
void
do_environment_variables(sysp)
	sys_info *sysp;
{
	char *impl_arch;
	char appl_str[SMALL_STR];

	impl_arch = get_arch();
	(void) appl_arch(impl_arch, appl_str);

	/*
	 *	If we are in multi-user mode, the only way we can be running
	 *	is being "sys-config", but at this point in time, we already
	 *	know the arch_str because sys-unconfig left the arch_str in
	 *	the SYS_INFO file there, so we can know our full identitity.
	 *	So, we just don't nuke the arch_str here.
	 */
	if (is_miniroot())
		(void) sprintf(sysp->arch_str, "%s.%s", appl_str, impl_arch);

	(void) setup_env_variable("si_arch", impl_arch);
}


/*
 * 	offer the user an automatic reboot, and do it if he so chooses
 */
void
do_reboot()
{
	int line_no;

	if (get_yn(NULL,REBOOT_BANNER,REBOOT_HELP,"Reboot now? [Y/N] : ",YES_NO,"y",NULL,NOMENU)) {
		line_no = print_help(NULL,REBOOT_BANNER,REBOOT_HELP,0);
		mvaddstr(line_no+2,2,"Rebooting ...");
		reboot(RB_AUTOBOOT);
	}
	(void) system("sync");
}

/*
 * update the static "info" string, which contains the current
 * info all nicely formatted
 */
void
update_info_string(sysp,choicep)
	sys_info *sysp;
	struct default_desc *choicep;
{
	char *cp;

	if (!is_miniroot()) {
		/*
	 	 * Hostname and timezone
	 	 */
		(void) sprintf(info,
			"\n  Hostname         : %s\n  Time zone        : %s\n",
			sysp->hostname0,sysp->timezone);
		cp = &info[strlen(info)];

		/*
	 	* network junk
	 	*/
		if (strlen(sysp->ether_name0)) {
			(void) sprintf(cp,"  Network address  : %s\n",sysp->ip0);
			cp = &info[strlen(info)];
			if (sysp->yp_type == YP_CLIENT) {
				(void) sprintf(cp,
					"  NIS domain       : %s\n",
			    		sysp->domainname);
			} else {
				(void) sprintf(cp,"  <no NIS name service>\n");
			}
		} else {
			(void) sprintf(cp,"  <non-networked workstation>\n");
		}

	} else {	/* is miniroot */

		/*
	 	 * Preservation information
	 	 */
		cp = &info[0];
		sprintf(cp,"  Preserve data partitions:  %s\n",
			strlen(preserve)?"Yes":"No");

		/*
	 	 * installation options
	 	 */
		if (choicep) {
			cp = &info[strlen(info)];
			(void) sprintf(cp,"\n  Standard Installation: %s\n\t(%s)\n\n",
		    		choicep->def_name,choicep->def_desc);
		}
	}
}

/*
 * 	show the user what he's chosen so far and ask if it's cool to use it
 */
int
get_confirmation(sysp,choicep)
	sys_info *sysp;
	struct default_desc *choicep;
{
	int val;
	char *cp;

	update_info_string(sysp,choicep);

	cp = &info[strlen(info)];

	/*
	 * if forceflag was set, just return true here
	 */
	if (forceflag)
		return(1);

	if (is_miniroot()) {
		(void) sprintf(cp,"%s",CONFIRM_QUESTION);
		val = get_yn(NULL,CONF_BANNER,info,"Start the installation? [Y/N] : ",YES_NO,"y",NULL,NOMENU);
		/*
		 *	Now delete what we just wrote
		 */
		bzero(cp,strlen(CONFIRM_QUESTION));
		return(val);

	}
	else
		return(get_yn(CONF_SCREEN,CONF_BANNER,info,"  Accept the information as correct (y/n)? ",
			YES_NO, " ",CONFIRM_HELPSCREEN,CONFIRM_PAGE));

}

/*
*   exit handler
*/	
void
outtahere(rc)
	int rc; 	/* return code */
{
	if (nocurses) {
		exit(rc);
	}
	
	if (quietflag) {
		(void) printf("\n");
	} else {
		move(LINES-2,0);
		(void) refresh();
	}
	endwin();
	resetty();
	if (rc == RC_RETRY) {
		execl(suninstall, suninstall, 0);
		(void) menu_log("%s:\tUnable to execute %s\n",
				progname, suninstall);
	}
		
	exit(rc);
}

/*
 * print a paragraph; return the next line number; try to more or less
 * center the whole mess vertically based on the number of lines in 'help' and
 * the fudge factor in 'extra_nn'
 */
int
print_help(screen,banner,help,extra_nn)
	char *screen;
	char *banner;
	char *help;
	int extra_nn;
{
	char *malloc();
	char *to, *from;
	char *text_line;
	int cur_line;
	int nn = 10;	/* 10 is our centering 'overhead' */
	int i = 0;

	/*
	 * malloc a line's worth of memory (don't need to initialize)
	 */
	if ((text_line = malloc((unsigned)COLS + 1)) == NULL) {
		/* now what?? */
	}

	/*
	 * we want to approximately center the screen horizontally, so
	 * we count the newlines in "help" and add some overhead
	 */
	for (to = help; *to != NULL; to++)
		if (*to == '\n')
			nn++;

	nn += extra_nn;

	if ((cur_line = ((LINES/2) - ((nn)/2))) < 1)
		cur_line = 1;
	
	/*
	 * print a centered banner, if provided, at the top of the screen
	 */
	if (screen) 
		mvaddstr(0,1,screen); 
	if (banner) {
		mvaddstr(1,(COLS/2)-(strlen(banner)/2),banner);
		for (i = 0; i < COLS; i++) {
			mvaddch(2,i,'_');
		}
		cur_line += 2; 
	}

	for (i = 0, from = help, to = text_line; *from != NULL; from++) {
		if (*from == '\n') {
			/* put the line out */
			to[i] = '\0';
			mvaddstr(cur_line++,0,text_line);
			i = 0;
			to = text_line;
		} else {
			/*
			 * copy from help to cur_line unless we're out of
			 * room, in which case we just truncate
			 */
			if (i < COLS) {
				to[i++] = *from;
			} 
		}
	}

	/* don't forget the last line!! */
	to[i] = '\0';
	mvaddstr(cur_line,0,text_line);

	free(text_line);
	return(cur_line);
}

/*
 * create and print a form that asks for one piece of information
 */
void
get_info(screen,banner,help,prompt,val,extra_help,page)
	char *screen;
	char *banner;
	char *help;   /* paragraph that gets put at top of screen */
	char *prompt; /* the prompt string */
	char *val;    /* ptr to where the info goes */
	char *extra_help; /* paragraph that is displayed for '?' */
	int  page;
{
	int cur_line;
	int promptline;
	int display = REDISPLAY;


	do {
		(void) clear();
		menu_window(page); 
		cur_line = print_help(screen,banner,help,0);

		/*
		 * now skip a couple lines and put the prompt string in
		 */
		cur_line += 2;
		promptline = cur_line;
		mvaddstr(promptline,2,prompt);
		
		/*
		 * now print the current value (if any) and put the cursor
		 * at the end of this value
		 */
		mvaddstr(promptline,2+strlen(prompt),val);
		refresh();
		/*
		 * now we get input
		 */
		display = getnam(val,2+strlen(prompt)+strlen(val),promptline, extra_help,page);
		if (*(val+strlen(val)-1) == '?') {
			*(val+strlen(val)-1) = '\0';  /* eat it if user inputs a '?'  */
		}
		(void) clear();
	} while (display == REDISPLAY);
}

void
menu_window(page)
int page;
{
	switch(page) {
	case NOMENU : break;
	case JUSTHELP : 
			mvaddstr(23,1,"[?] = Help          ");
			break;
	case CONFIRM_PAGE : case USERFN_PAGE : case USERUN_PAGE :
	case USERUID_PAGE :
		mvaddstr(23,1,"[?] = Help          [Control-B] = Previous Screen");
		break;
	default :	
	mvaddstr(23,1,"[?] = Help      [Control-B] = Previous Screen       [Control-F] = First Screen");
	break;
	}
}	

/*
 * create and print a form that asks for a password,
 * running the /bin/passwd program (via a pty) to actually to the work.
 *
 * ASSUMES: quiescent system - once we grabbed a pty we can continue to use it
 */
get_pw(screen,banner, help, extra_help, uname)
	char *screen;
	char	*banner;
	char *help;   /* paragraph that gets put at top of screen */
	char *extra_help; /* paragraph that is displayed for '?' */
	char *uname;	/* the user name to get the passwd for */
{
	char	c;
	int	i;
	char	*line;
	int	p;		/* pty file des. */
	int	t;		/* tty file des. */
	int	pid;
	int	status;
	int	first;
	int	cur_line;	/* cursor line number */
	int	x;		/* cursor column number */
	int n_flag=0;		/* 1=passwd doesn't like user's input */
	int	pwlinestate;	/* state variable for passwd output handling */
#define PWS_NEWPROG	0	/* a new passwd program has been invoked */
#define PWS_EAT1ST	1	/* eat the first line, ie. "Changing ..." */
#define PWS_EAT2ND	2	/* eat second line, ie. "Enter..." */
#define PWS_PASSTHRU	3	/* pass thru everthing passwd outputs */

	/* grab a pty pair */
	for (c = 'p'; c <= 's'; c++) {
		struct stat stb;
		int pgrp;

		line = "/dev/ptyXX";
		line[strlen("/dev/pty")] = c;
		line[strlen("/dev/ptyp")] = '0';
		if (stat(line, &stb) < 0)
			break;
		for (i = 0; i < 16; i++) {
			line[strlen("/dev/ptyp")] = "0123456789abcdef"[i];
			line[strlen("/dev/")] = 't';
			/*
			 * Lock the slave side so that no one else can
			 * open it after this.
			 */
			if (chmod (line, 0600) == -1)
				continue;
			line[strlen("/dev/")] = 'p';
			if ((p = open(line, O_RDWR)) == -1) {
				restore_pty(line,p);
				continue;
			}
			/*
			 * XXX - Using a side effect of TIOCGPGRP on ptys.
			 * May not work as we expect in anything other than
			 * SunOS 4.1.
			 */
			if ((ioctl(p, TIOCGPGRP, &pgrp) == -1) &&
			    (errno == EIO))
				goto gotpty;
			else {
				restore_pty(line,p);
			}
		}
	}
	fprintf(stderr, "out of ptys");
	return(1);

gotpty:

	pwlinestate = PWS_NEWPROG;

	/* fork, the child then runs /bin/passwd */
	pid = fork();
	if (pid < 0) {
		restore_pty(line,p);
		perror("pwspawn: error during fork");
		return(1);
	}
	if (pid == 0) {
		int tt;
 
		/*
		 * The child process needs to be the session leader
		 * and have the pty as its controlling tty.
		 */
		(void) setpgrp(0, 0);		/* setsid */
		line[strlen("/dev/")] = 't';
		tt = open(line, O_RDWR);
		if (tt < 0) {
			perror("can't open pty");
			exit(1);
		}
		(void) close(p);
		if (tt != 0)
			(void) dup2(tt, 0);
		if (tt != 1)
			(void) dup2(tt, 1);
		if (tt != 2)
			(void) dup2(tt, 2);
		if (tt > 2)
			(void) close(tt);
		execl("/bin/passwd", "passwd", uname, (char *)0);
		perror("can't exec /bin/passwd");
		exit(1);
		/*NOTREACHED*/
	}

re_draw:
	/* put up the initial banner */
	(void) clear();
	cur_line = print_help(screen,banner, help, 0);
	cur_line += 2;

	/* skip a couple of lines, and print prompt */
#define PASSWORD_PROMPT "   Password => "
	menu_window(JUSTHELP);
	mvaddstr(cur_line, 2, PASSWORD_PROMPT);
	x = 2 + strlen(PASSWORD_PROMPT);
	(void) refresh();
	noecho();
/***	(void) fflush(stdin); ***/


	/*
	 * now do a protocol that
	 * 1. ignores first line of output from passwd program
	 * 2. feeds characters to it (except a leading ?)
	 */
	first = 1;
	while (1) {
		int	ibits;

		ibits = (1 << 0);		/* stdin */
		ibits |= (1 << p);		/* pty */

		if (select(32, &ibits, (int *)0, (int *)0, 0) < 0) {
			perror("select");
			echo();
			restore_pty(line,p);
			return(2);
		}
		if (ibits & (1 << 0)) {		/* stdin */
			if (read(0, &c, 1) != 1) {
				echo();
				restore_pty(line,p);
				return(2);
			}
			/* if 1st, and "?", print help msg instead */
			/* if 1st, and <CR>, don't set password */
			if (first) {
				if (c == '?') {
					if (extra_help != NULL) {
						info_window(extra_help,
						    6,
						    "Press any key to continue",
						    '?', NULL, (int)0);
					}
					goto re_draw;
					/****/
				} else
				if ((c == '\n') || (c == '\r')) {
					restore_pty(line,p);
					return(0);
				} else
					first = 0;
			}
			if (write(p, &c, 1) != 1) {
				echo();
				restore_pty(line,p);
				return(2);
			}
		}
		if (ibits & (1 << p)) {		/* pty */
			i = read(p, &c, 1);
			if (i < 0) {
				if ((errno == EIO) || (errno == EINVAL))
					break;
				echo();
				restore_pty(line,p);
				return(2);
			}
			if (i == 0) {		/* gone away */
				break;
			}
			/* what to do? */
			switch (pwlinestate) {
			case PWS_NEWPROG:
			case PWS_EAT1ST:
				/* first line ends in newline */
				if (c != '\n')
					continue;	/* throw away */
				pwlinestate = PWS_EAT2ND;
				break;

			case PWS_EAT2ND:
				/* second line ends in colon */
				if (c != ':')
					continue;	/* throw away */
				pwlinestate = PWS_PASSTHRU;
				break;

			case PWS_PASSTHRU:
				switch(c) {
				case '\n':
					x = 2;	/* start another line */
					break;
				case '\r':
					break;
				default:
					if (c == 'N' ) {
						n_flag=1;
						cur_line += 1;
					}
					/* clear out for new line */
					if (x == 2) {
						move(cur_line, x);
						clrtoeol();
					}
					mvaddch(cur_line, x++, c);
					refresh();
					if (n_flag && c == ':') {
						move(cur_line,2);
						clrtoeol();
						cur_line -= 1;
						n_flag=0;
					}
				}
			}
		}
	}
	echo();
	if (wait(&status) < 0) {
		perror("pwspawn: wait on child");
		restore_pty(line,p);
		exit(2);
	}
	if (WIFEXITED(status)) {
		if (WEXITSTATUS(status) != 0) {
			/* spawn off another passwd program */
			mvaddstr(cur_line + 2, 2, "Press any key to continue");
			refresh();
			(void)getch();
			goto gotpty;
		}
		restore_pty(line,p);
		return(0);
	}
	restore_pty(line,p);
	return(3);
}

/*
 * Restore the permissions on the pty and close it
 */
restore_pty(pty_name, pty_fd)
        char *pty_name;
        int   pty_fd;
{

        pty_name[strlen("/dev/")] = 't';
        (void) chmod(pty_name, 0666);
        pty_name[strlen("/dev/")] = 'p';

        if(pty_fd != -1)
                close(pty_fd);
}

 /* prints helpful paragraph, asks a question, returns TRUE for one answer
 * and FALSE for the other.  (also can allow for an 'abort')
 */

int
get_yn(screen,banner,help,question,tag,def_answer,extra_help,page)
	char *screen;
	char *banner;		/* name of screen form */
	char *help;		/* preformatted help paragraph to stick at top */
	char *question;		/* question to ask */
	int tag;		/* tag line to use */
	char *def_answer;	/* default answer */
	char *extra_help;	/* more information about the question */
	int  page;
{
	int cur_line;
	int promptline;
	char answer[SMALL_STR];
	int cpos;
	   int answered = 0;
	char *tagstr;
	int rval = 0;
	int display;

	do {
		(void) clear();

		cur_line = print_help(screen,banner,help,0);

		/*
		 * now skip a couple lines and print the question string
		 */
		cur_line += 2;

		promptline = cur_line;
		cpos = 2+strlen(question);
		menu_window(page);
		mvaddstr(promptline,2,question);
		mvaddstr(promptline,2+strlen(question),def_answer);
		refresh();

		switch (tag) {
		default:
		case ONE_TWO:
			do {
				*answer = '\0'; 
				display = getnam(answer,cpos,promptline,
						 extra_help,page);
				if (strlen(answer)==1 && ((*answer=='1') || (*answer=='2'))) {
				switch (*answer) {
				case '1':
					rval = 1;
					/* fall through ... */
				case '2':
					answered = 1;
					break;
				}
				} else if (display != REDISPLAY) {
				move(promptline,cpos);
				clrtoeol();
				refresh();
				}
			} while (!answered  && display != REDISPLAY);
			break;
		case YES_NO:
			do {
				*answer = '\0';
				display = getnam(answer,cpos,promptline,
						 extra_help,page);
				if (*answer == NULL) {
					*answer = *def_answer;
					*(answer+1) = '\0';
				}
				if (strlen(answer)<=1 && (*answer=='Y' || *answer=='y' || *answer=='N' || *answer =='n')) {
				switch (*answer) {
				case 'y': case 'Y':
					rval = 1;
					/* fall through ... */
				case 'n': case 'N': 
					answered = 1;
					break;
				}
				} else if (!(display == REDISPLAY || display == CONFIRM_B)) {
				move(promptline,cpos);
				clrtoeol();
				refresh();
				}
			} while (!answered  && display != REDISPLAY && display != CONFIRM_B);
			break;
		}
	} while (display == REDISPLAY);

	(void) clear();
	if (display == CONFIRM_B) return(display);
		else 
		return(rval);
}

/*
 * Ask "the five" (or is it six? seven?) questions, one per screen.  Only
 * ask the relevant questions.
 * The first time through, we let any environment variables suppress
 * displaying the relevant screen.  If the user asks to reenter, however,
 * he sees all.  Env variables can be set on the command line.
 * 'n' is a bitfield that has flags for all the things we're looking for.
 */
void
get_config_info(sysp,n,where)
	sys_info *sysp;
	short n; 	/* 'need' mask */
	int where;
{
	char *ep;
	int i;
	int net_page;


	if (!is_miniroot()) {
		if (where==CONFIRM_B) {
			if(n & GOT_NETWORK_Y) longjmp(sjbuf_nis);
			else longjmp(sjbuf_net);
		}
		else if (where) 
			longjmp(sjbuf_domain);
		/*
	 	* hostname
	 	*/
		setjmp(sjbuf_hostname);
		if (n & NEED_HOSTNAME) {
			while(1) {
				if (strcmp(sysp->hostname0, "noname")==0)
					*(sysp->hostname0)='\0';
				get_info(HOSTNAME_SCREEN,HOSTNAME_BANNER,HOSTNAME_HELP,"      Hostname => ",
			    	sysp->hostname0,HOSTNAME_HELPSCREEN,JUSTHELP);
				(void) setup_env_variable("si_hostname",sysp->hostname0);
				if (strlen(sysp->hostname0) && strlen(sysp->hostname0)<= 63 && isalpha(sysp->hostname0[0]))
					break;
				info_window("Invalid hostname.", LINES,
					"Press any key to continue", 
					'?', NULL,(int)0);
				*(sysp->hostname0)='\0';
			}
		}

		/*
	 	* timezone
	 	*/
		setjmp(sjbuf_timezone);
		if (n & NEED_TIMEZONE) {
			help_timezone(sysp->timezone);
#ifdef CATCH_SIGNALS
			/* help_timezone fools with signals */
			(void) signal(SIGQUIT,SIG_IGN);
#endif
			(void) setup_env_variable("si_timezone",sysp->timezone);
		}

		/*
	 	* check the date
	 	*/
		setjmp(sjbuf_time);
		if (!dateflag) {
			if (!re_flag)
				check_time(sysp);
		}

		/*
	 	* networking stuff
	 	*/

		setjmp(sjbuf_net);
		if (n & NEED_NETWORK) {
			if(dateflag) net_page = NET_PAGE - 2;
			else net_page=NET_PAGE;
			if (get_yn(NETWORK_SCREEN,NETWORK_BANNER,NETWORK_HELP,
	    	    		"      Selection => ",ONE_TWO,
	    	    		" ",NETWORK_HELPSCREEN,net_page)) {
				n |= GOT_NETWORK_Y;
				n &= ~GOT_NETWORK_N;
				(void) setup_env_variable("si_network","yes");
				get_ethertypes(sysp);
				/*
	 			 *  Easyinstall should only recognize 
				 *  ethernet interface 0.
	 			 */
				for (i = 1; i < MAX_ETHER_INTERFACES; i++)
					*sysp->ether_namex(i) = NULL;
			} else {
				n |= GOT_NETWORK_N;
				n &= ~GOT_NETWORK_Y;
				(void) setup_env_variable("si_network","no");
				bzero(sysp->ether_name0,sizeof(sysp->ether_name0));
				bzero(sysp->ip0,sizeof(sysp->ip0));
				bzero(sysp->domainname,sizeof(sysp->domainname));
			}
		}

		setjmp(sjbuf_ip);
		if (n & GOT_NETWORK_Y) {
			if ((ep = getenv("si_ether")) != NULL)
		   	(void) strcpy(sysp->ether_name0,ep);
			if (n & NEED_IP0) {
				while(1)        {
			     		if ((ep = getenv("si_ip0")) != NULL)
					(void) strcpy(sysp->ip0,ep);
					if (strcmp(sysp->ip0,"192.9.200.1") == 0)
						*(sysp->ip0) = '\0';
			     		get_info(IP_SCREEN,IP_BANNER,IPADDR_HELP,
			    		"   Network Address (e.g. 192.9.200.1) => ",sysp->ip0,
					IPADDR_HELPSCREEN,IP_PAGE);
			     		if (check_ip(sysp->ip0)) 
					break; 
					info_window("Invalid network address format.", LINES,
					"Press any key to continue", 
					'?', NULL,(int)0);
					*(sysp->ip0)='\0';
				} 
				(void) setup_env_variable("si_ip0",sysp->ip0);
			}

			setjmp(sjbuf_nis);
			if (n & NEED_YP_TYPE) {
				if (get_yn(YP_SCREEN,YP_BANNER,YP_GEN_HELP,
	    	    	    		"      Selection => ",
		    	    		ONE_TWO," ",NIS_HELPSCREEN,NIS_PAGE)) {
					sysp->yp_type = YP_CLIENT;
				} else {
					sysp->yp_type = YP_NONE;
					n &= ~NEED_DOMAINNAME;
					bzero(sysp->domainname,sizeof(sysp->domainname));
				}
			}

			setjmp(sjbuf_domain);
			if (n & NEED_DOMAINNAME) {
        		if ((ep = getenv("si_domainname")) != NULL)
					(void) strcpy(sysp->domainname,ep);
			while (1) {
				if (strcmp(sysp->domainname,"noname")==0)
					*(sysp->domainname)='\0';
				get_info(DOMAIN_SCREEN,DOMAIN_BANNER, DOMAIN_HELP, "  Domain name => ",
			    	sysp->domainname,DOMAIN_HELPSCREEN,DOMAIN_PAGE);
				(void) setup_env_variable("si_domainname",sysp->domainname);
				if (strlen(sysp->domainname))
					break;
				info_window("Field must be non-empty.", LINES,
					"Press any key to continue", 
					RETURN, NULL, (int)0);
				}
			}
		} else {
			*sysp->ether_name0 = NULL;
			sysp->yp_type = YP_NONE;
		}
	} else {	/* is_miniroot */
		/*
	 	* Preserving the Disk
	 	*
	 	* A small disk really isn't preserved since the interesting
	 	* data in home is in the g partition which is destroyed
	 	*/
		if (n & NEED_PRESERVE) {
			if (is_miniroot() &&  get_yn(NULL,PRESERVE_BANNER,PRESERVE_HELP,
	    	    	"  Do you want to preserve the existing data partitions? [Y/N] : ",
			YES_NO,
	    	    	strcmp(preserve,"all") ? "n":"y",PRESERVE_HELPSCREEN,NOMENU)) {
				n |= GOT_PRESERVE_Y;
				n &= ~GOT_PRESERVE_N;
				(void) strcpy (preserve, "all");
				(void) setup_env_variable("si_preserve","yes");
			} else {
				n |= GOT_PRESERVE_N;
				n &= ~GOT_PRESERVE_Y;;
				(void) strcpy (preserve, "");
				(void) setup_env_variable("si_preserve","no");
			}
		}
		(void) setup_env_variable("si_yp_type",cv_yp_to_str(&sysp->yp_type));
	}
}


void
init_curses(termtype)
	char *termtype;
{
	static int not_done_yet = 1;

	if (not_done_yet) {
        	terminal(termtype);
		unpack_init();
		not_done_yet = 0;
	}
}

/*
 *	Given a key=datum, see if there's a corresponding environment
 *	variable.  if there is and it matches, or if there isn't, return
 *	TRUE; if there is and it doesn't match, return FALSE.  'Datum' may
 *	consist of several items, separated by '/'; if any of the items
 *	matches, we call it a match.  We're only concerned with environment
 *	variables that begin with "si_"
 */
int
match(d)
	struct desc_datum *d;
{
	char si_name[BUFSIZ];
	char *e;
	char *start;
	char *end;

	/*
	 * prepend "si_" to the key; only variables with si_ prefixes
	 * are of interest to us.
	 */
	(void) sprintf(si_name,"si_%s",d->dat_key);

	/*
	 * if there's no such environment variable, we consider it
	 * a match
	 */
	if ((e = getenv(si_name)) == NULL) {
		return(1);
	}

	/*
	 * if there is such an environment variable, it's a match if
	 * the values match.  There may possibly be more than one
	 * '/'-separated values in the .desc file.
	 */
	for (start=d->dat_data; ; start = end+1) {
		if ((end = index(start,'/')) == NULL) {
			return( strcmp(e,start) == 0 );
		} else {
			*end = NULL;
			if ( strcmp(e,start) == 0) {
				*end = '/';
				return(1);
			}
			*end = '/';
		}
	}
}

/*
 * given an string, copy the next key/datum pair
 * (key=datum) int the provided next_datum struct; return 0 if there
 * isn't a next one, return 1 if there is;
 * skip blank lines and lines beginning with '#'
 */
int 
get_a_key(str,next_datum)
	char *str;
	struct desc_datum *next_datum;
{
	char *cp, *keyptr, *datumptr;
	char *eqptr;	/* find the equals sign */
	char *index();
	char *fromptr;
	int i;

	cp = str;
		/*
		 * get rid of leading spaces
		 */
	while (isspace(*cp))
		cp++;
		/*
		 * skip blank lines and comment lines
		 */
	if (*cp == NULL || *cp == '#') {
		return(0);
	}
		/*
		 * skip lines that aren't in the right format
		 */
	if ((eqptr = index(cp,'=')) == NULL) {
		return(0);
	}
	keyptr = cp;
	datumptr = eqptr + 1;
	while (isspace(*datumptr)) {
		datumptr++;
	}
	*eqptr = NULL;
		/*
		 * get the key, stripping any blanks from the end
		 */
	for (fromptr=keyptr, i=0;
	     *fromptr!= NULL && !isspace(*fromptr) 
		&& i<(KEYLEN-1);
	     fromptr++) {
		next_datum->dat_key[i++] = *fromptr;
	}
	next_datum->dat_key[i] = NULL;
		/*
		 * datum is a bit harder; if it starts with a quote,
		 * it must end with a quote and we include spaces
		 */

	if (*datumptr == '"') {
		for (fromptr=datumptr+1,i=0;
		     *fromptr != NULL && *fromptr != '"' && 
		        i<(DATALEN-1);
		     fromptr++) {
			next_datum->dat_data[i++] = *fromptr;
		}
	} else {
		for (fromptr=datumptr,i=0;
	     	     *fromptr!= NULL && !isspace(*fromptr)
			&& i<(DATALEN-1);
	     	     fromptr++) {
			next_datum->dat_data[i++] = *fromptr;
		}
	}
	next_datum->dat_data[i] = NULL;
	return(1);
}


/*
 * put up a menu with all the available choices and make the user
 * choose one.  If there's more than will fit on one screen, we
 * allow scrolling around a bit.
 */
struct default_desc *
get_default_choice(mp, soft_p)
	struct default_desc *mp;
	soft_info *soft_p;
{
	int start_line;
	int help_line;
	WINDOW *swp, *subwin();
	struct default_desc *tmp = mp;

	char *big_scrn_map;
	char *fillin_defaults_menu();

	char save_char;
	char *save_spot;

	int scrn_y, real_y; /* line position */

	register struct default_desc *p;
	int num_choices = 0;
	int maxdnamelen = 10; /* the "name" column must be this wide */
	int i;

	int scrn_sz;
	int cur_topline;

	int chosen = 0;				/* we got a choice */
	char c;

	/*
	 * determine how many choices we've got to print--this depends
	 * on how many "desc" keys are in the chain pointed at by mp
	 * As long as we're nosing through mp, lets find out the length
	 * of the longest directory name, which we'll need to know.
	 */
	for (p = mp; p != NULL; p = p->def_next) {

		num_choices++;
		maxdnamelen = GREATER(maxdnamelen,strlen(p->def_name));
	}

	/*
	 * print the top message on main window
	 */
	(void) clear();
	help_line = print_help(NULL,DEF_BANNER,DEF_HELP,num_choices);
	start_line = help_line;
	/*
	 * now add the column headers
	 */
#define COL1START 2
#define COLSPC	  2

	start_line += 2;	/* skip a couple lines */
	mvaddstr(1,COL1START,"INSTALLATION");
	mvaddstr(start_line++,COL1START+maxdnamelen+COLSPC,"DESCRIPTION");
	for (i = COL1START; i < (COLS-COL1START); i++) {
		mvaddch(start_line,i,'-');
	}
		
	/*
	 * this gives us a pointer to the beginning of a map
	 * holding a screen image of the whole list of choices
	 */
	big_scrn_map = fillin_defaults_menu(mp,num_choices,maxdnamelen);

	/*
	 * Then make a subwindow for the hunk of the
	 * window that we'll actually print at any given time.
	 */
	scrn_sz = LINES - (start_line+1) - 1;   
	cur_topline = 0;
	real_y = scrn_y = 0;
	save_spot = &save_char;

	swp = subwin(stdscr,scrn_sz,COLS,start_line+1,0);
	noecho();

	if (num_choices > scrn_sz - 2)  {
		mvaddstr(start_line+scrn_sz,COL1START,"< MORE >");		
	}

	refresh();

	while (!chosen) {
		do {
			/*
			 * handle the ends of the real map
			 */
			if (real_y > num_choices) {
				/*
				 * go back to the beginning if we try to
				 * scroll off the end
				 */
				real_y = 0;
				cur_topline = 0;
				scrn_y = 0;
			} else if (real_y < 0) {
				/*
				 * put the last choice and the cursor on
				 * the last line of the screen
				 */
				real_y = num_choices;
				cur_topline
				    = GREATER((num_choices-(scrn_sz-2)),0);
				scrn_y = SMALLER((scrn_sz - 2),num_choices);
			/*
			 * then the ends of the visible window
			 */
			} else if (scrn_y >= (scrn_sz - 1 )) {
				/*
				 * move it all down one line and leave the
				 * cursor on the last line
				 */
				cur_topline++;
				scrn_y = scrn_sz - 2;
			} else if (scrn_y < 0) {
				/*
				 * move it up one line
				 */
				cur_topline--;
				scrn_y = 0;
			}

			/*
			 * we want waddstr's argument to be NULL-terminated
			 * at the right spot, so we do that
			 */
			*save_spot = save_char;
			save_spot = &big_scrn_map[(cur_topline*COLS)+((scrn_sz-1)*COLS)];
			save_char = *save_spot;
			*save_spot = '\0';

			/*
			 * put the cursor at the upper-left corner, copy
			 * the section of the map we want, then move the
			 * cursor to where we want the user to see it
			 * and refresh.
			 */
			(void) wmove(swp,0,0);
			(void) waddstr(swp,&big_scrn_map[cur_topline*COLS]);
			(void) wmove(swp,scrn_y,1);
			(void) wrefresh(swp);
	
			/*
			 * get a char and process it
			 */
	                c = wgetch(swp);
	                switch (c) {
	                case CONTROL_B:
	                case CONTROL_P:
	                       real_y--;
	                       scrn_y--;
	                       break;
	                case CONTROL_F:
	                case CONTROL_N:
	                case RETURN:
	                case SPACE:
	                        real_y++;
	                        scrn_y++;
	                        break;
			case '?':
				for (p = mp, i = real_y; p != NULL && i;
					i--, p = p->def_next) ;
				if (p)	{
					show_template(p, soft_p, help_line+4);
					help_line = print_help(NULL,DEF_BANNER,DEF_HELP,num_choices);
					refresh();
				}
				break;
	                }
	        } while ( c == CONTROL_B || c == CONTROL_P ||
	                  c == CONTROL_F || c == CONTROL_N ||
	                  c == RETURN || c == SPACE );
		if ( c == 'x' || c == 'X' ) {
	                mvwaddch(swp,scrn_y,1,'x');
	                (void) wrefresh(swp);
			/*
			 * now figure out which one we actually chose and
			 * return it.  If we chose the last entry, we
			 * return NULL, which is good.
			 */
			while (real_y--)
				mp = mp->def_next;
			if (mp != NULL) {
				if (!check_disk_space(mp->def_size,preserve)) {
			  info_window("Not enough disk space for this choice.",
                                       LINES,
                          "Press 'a' to abort or any other key to continue",
                                       'a', outtahere ,RC_ABORT);
				mp = tmp; /* reset its value */
				refresh();
				continue;
				}
			}
			chosen++;
		}
	}

	free(big_scrn_map);
	(void) wclear(swp);
	delwin(swp);
	echo();
	return(mp);
}

char *
fillin_defaults_menu(mp,n,clen)
	struct default_desc *mp;
	int n;
	int clen;
{
	int i;
	char *map;
	char *malloc();
	char *memset();

	/*
	 * this is sick.  We're going to make up a gigantic screen map
	 * of all the choices, and print parts of it into a subwindow.
	 * We really ought to be using "pads", part of System V curses.
	 * (but not part of 4.2 curses)
	 */

	if ((map = malloc((unsigned)((n + 1) * COLS) + 1)) == NULL) {
		/* oops */
		menu_log("fillin_defaults_menu: couldn't malloc");
		outtahere(RC_DEAD);
	}

	/* initialize map to blanks */
	(void) memset(map,' ',(n + 1) * COLS);

	/*
	 * for each selection, print out the directory name (last
	 * pathname component only) and the description string
	 */
		
	for (i = 0; i < n; i++, mp = mp->def_next) {
		char *cp, *fromcp;
		int j;
		fromcp = mp->def_name;
		/* position cp at beginning of current line in our 'map' */
		cp = map + (COLS * i);

		/* set up the first part of the string */
		(void) sprintf(cp,"  %s",fromcp);
		j = strlen(cp);
		cp[j++] = ' '; /* no NULLs allowed!! */
		
		/* skip to the beginning of the 2nd column */
		while (j < (clen+COL1START+COLSPC))
			j++;

		/* copy as much of the description string as will fit */
		for (fromcp = mp->def_desc; *fromcp != NULL && j < COLS; j++) {
			cp[j] = *fromcp++;
		}

	}
	
	/* now the last line */
#define NONE_OF_THE_ABOVE "  <none of the above>"

	(void)strncpy(&map[n*COLS],NONE_OF_THE_ABOVE,
		      sizeof(NONE_OF_THE_ABOVE)-1);

	/* NULL only in the last character of the whole, entire 'map'!! */
	map[(n+1)*COLS] = '\0';

	return(map);
}

/*
 * put up a window with (hopefully informative) text, then wait
 * for acknowledgment before going back to whatever we were doing
 * before.  text must be "preformatted", i.e., already broken into
 * newline-separated lines of fewer than 60 chars each.
 */
void
info_window(text,line,last_msg,key,handler,rc)
	char *text;
	int line;
	char *last_msg;
	char key;
	void (*handler)();
	int rc;
{
	WINDOW *wp, *subwin();
	char dotted_line[80];
	char cur_text[80];
	char *cur_char;
	int nlines = 4;
	int curline = 1;
	int i;
	char c;
	int start;

	/*
	 * figure out how many lines there are
	 */
	for (cur_char = text; *cur_char != NULL; cur_char++) {
		if (*cur_char == '\n')
			nlines++;
	}

	for (i = 0; i < 80; i++) {
		dotted_line[i] = '_';
		dotted_line[79] = '\0';
	}

	/* 
	 * If help is longer than message then start it on line 2
	 */
	start = (line > (nlines + 2)) ? (line - (nlines + 2)) : 2;
	wp = subwin(stdscr,nlines,80,start,((COLS-80)/2));
	(void) wclear(wp);

	cur_text[0] = '|';
	for (i = 1; i < 78; i++)
		cur_text[i] = ' ';
	cur_text[78] = '|';
	cur_text[79] = '\0';

	mvwaddstr(wp,0,0,dotted_line);
	i = 2;
	for (cur_char = text; *cur_char != NULL; cur_char++) {
		if (*cur_char == '\n') {
			while (i < 77) {
				cur_text[i++] = ' ';
			}
			mvwaddstr(wp,curline++,0,cur_text);
			i = 2;
		} else {
			if (i < 77) {
				cur_text[i++] = *cur_char;
			} /* else truncate it */
		}
	}
	while (i < 77) {
		cur_text[i++] = ' ';
	}
	mvwaddstr(wp,curline++,0,cur_text);

	mvwaddstr(wp,curline,0,"|");
	(void) wstandout(wp);
	mvwaddstr(wp,curline,7,last_msg);
	(void) wstandend(wp);
	mvwaddstr(wp,curline++,67,"           |");
	mvwaddstr(wp,curline,0,dotted_line);
	(void) wmove(wp,curline-1,6);
	(void) wrefresh(wp);

	c = wgetch(wp);

	(void) wclear(wp);
	(void) wrefresh(wp);
	delwin(wp);

	if (c == key && handler)
		(*handler)(rc);
	
}

void
unpack_init()
{
	extern void sig_trap();

	/*
	 * initialize screen
	 */
	(void) savetty();
	(void) initscr();
	(void) clear();


	/*
	 * trap some signals
	 */
	(void) signal(SIGINT,sig_trap);
	(void) signal(SIGQUIT,SIG_IGN);
#ifdef CATCH_SIGNALS
	(void) signal(SIGHUP,SIG_IGN);
        (void) signal(SIGTSTP,SIG_IGN);
#endif



	/*
	 * set some terminal modes..
	 */
	nl();			/* turn on CR-LF remap	*/
	crmode();		/* set cbreak...	*/
	echo();			/* turn on echoing     */
	leaveok(stdscr,FALSE);
	scrollok(stdscr,FALSE);
}

/*
 * execute the named program and wait around for it to finish
 */
static int
execute(name)
	char *name;
{
        register int ok;
        int id, child;
        union wait stat;
        char filename[MAXPATHLEN];

        (void) sprintf(filename,"%s/%s",TOOL_DIR,name);
        if (( child = fork()) == 0) { /* child */
                if ((re_flag) && (strcmp(name,INSTALLATION_PROG)==NULL))
                   execl(filename,filename,"r",0);
                else
                   execl(filename,filename,0);
                    
                menu_log("%s:\tUnable to execute %s\n",progname, filename);
                exit(1); /* no "outtahere" for the child */
        } else if ( child != -1 ) { /* parent */
                if((id = wait(&stat)) == -1) {
                        menu_log("%s:\tWait system call fails, returns -1\n",
                            progname);
                        perror("wait");
                        outtahere(RC_DEAD);
                } else if ( id == child )
                        ok = stat.w_status ? 0 : 1;
        } else {
                menu_log("%s:\tFork system call fails, returns -1\n", progname);
                perror("fork");
                outtahere(RC_DEAD);
        }
        return(ok);
}

/*
 * 	Display the current date according to the timezone just set, and
 * 	give the user a chance to fix it.
 *
 * 	Most of this is stolen from suninstall, so if it changes there, it
 * 	ought to change here, too
 */

extern int errno; 
void
check_time(sysp)
	sys_info *sysp;
{

	struct tm *tmp;
	register char *p;
	char zonepath[MAXPATHLEN];
	char saved_dir[MAXPATHLEN];
	int us_canada = 0;
	time_t getdate();
	struct timeval newtime;
	char qbuf[BUFSIZ];
	char abuf[BUFSIZ];
	char pbuf[BUFSIZ];
	char dbuf[BUFSIZ];
	char tbuf[BUFSIZ];
	char sbuf[BUFSIZ];
	char a1buf[BUFSIZ];
	char a2buf[BUFSIZ];
	int date_flag;
	int i;
	/*
	 * link timezone to localtime
	 */

	(void) sprintf(zonepath, "/usr/share/lib/zoneinfo/%s", sysp->timezone);
	if (access(zonepath, 0) != 0) {
		menu_log("\n%s not a valid timezone\n");
		bzero(sysp->timezone, MEDIUM_STR);
	}

	/*
	 * Save current directory - make it NULL if getwd() call fails.  A 
	 * failure here is not fatal.  We save and restore the working
	 * directory just to be good citizens.   (tpc)
	 */
	if (getwd(saved_dir) == 0) {
		menu_log("%s:%s:%s: non-fatal error - getwd() call failed with message:\n\t\t%s\n",
			progname, __FILE__, __LINE__, saved_dir);
		saved_dir[0] = '\0';
	}

	if (chdir("/usr/share/lib/zoneinfo") == 0) {
		(void) system("rm -f localtime");
		if (link(zonepath,"/usr/share/lib/zoneinfo/localtime") !=0) {
			if (errno == EACCES) {
				menu_log("%s:\t/usr/share/lib/zoneinfo/localtime - permission denied\n", progname);
			} else if (errno == ENOENT) {
				menu_log("%s:\t%s not a valid timezone\n",
				    progname, sysp->timezone);
			}
		}               
	}

	/*
	 * Restore the previous working dir.  If it's NULL, we don't have it.
	 * A failure here is not fatal.  We save and restore the working
	 * directory just to be good citizens.   (tpc) 
	 */
	if ((saved_dir[0] != '\0') && (chdir(saved_dir) != 0)) {
		menu_log("%s:%s:%s: non-fatal error - chdir() failed with errno = %d\n",
			progname, __FILE__, __LINE__, errno);
	}

	/*
	 * See if the time zone name is a two-component
	 * pathname.  If so, see if the first component is "US"
	 * or "Canada".  If so, we're in North America, so we
	 * use mm/dd/yy format for dates; otherwise, we use
	 * dd/mm/yy format.  (These rules may need further
	 * augmentation.)  Yes, this is a hack; it will have to
	 * do until we provide multi-country support, in which
	 * case "suninstall" will have to ask you what your
	 * locale is (which it probably should anyway, so it
	 * can talk to you in your native language).  (As hacks
	 * go, however, it is, no worse than the RSX-11M hack
	 * of saying "tea break" rather than "coffee break" if
	 * you're configuring your machine with a 50 hZ clock!)
	 *
	 */
	if (strncmp(sysp->timezone, "US/",3) == 0 || 
	    strncmp(sysp->timezone, "Canada/",7) == 0 ||
	    strncmp(sysp->timezone, "EST",3) == 0 ||
	    strncmp(sysp->timezone, "CST",3) == 0 ||
	    strncmp(sysp->timezone, "MST",3) == 0 ||
	    strncmp(sysp->timezone, "PST",3) == 0)
		us_canada = 1;
	tzsetwall();

	*abuf = '\0'; 	/* better set this to a NULL string */

	
	(void) signal(SIGQUIT,SIG_IGN);
	/*
	 * put the time into the question
	 */
	date_flag = dateflag;
	while (!date_flag) {
		(void) gettimeofday(&newtime, (struct timezone *)NULL);
		tmp = localtime(&newtime.tv_sec);
		p = asctime(tmp);
		p[24] = '\0'; /* replace newline with NULL */

	(void) sprintf(sbuf,"The system time is \"%s %s\".\n %s",p,tmp->tm_zone != NULL ? tmp->tm_zone : "",TIME_HELP);
	(void) sprintf(qbuf,"        Selection => ");
		if (get_yn(TIME_SCREEN,TIME_BANNER,sbuf,qbuf,ONE_TWO," ",
			SYSTIME_HELPSCR,TIME_PAGE)) {
			/*
			 * it's OK
			 */
			date_flag++;	
		 } else {
		
			/*
			 * we need to change it
			 */
		if (us_canada) 
		sprintf(dbuf,"  Current date (mm/dd/yy = month/day/year) => ");
		else 
		sprintf(dbuf,"  Current date (dd/mm/yy = day/month/year) => ");

		sprintf(tbuf,"  Local time (hh:mm:ss = hours:minutes:seconds) => ");
			do {
				a1buf[0]='\0';
				a2buf[0]='\0';
				set_time(SET_TIME_SCREEN,SET_TIME_BANNER, 
					SETTIME_HELP,  
					dbuf,tbuf,a1buf,a2buf);
				sprintf(abuf,"%s %s",a1buf,a2buf);
				newtime.tv_sec = getdate(abuf, us_canada);
				if (newtime.tv_sec < 0) {
					info_window("Invalid date or time format.",
						LINES,
			       "Press any key to continue",
						RETURN,NULL,(int)0);
				} else {
					newtime.tv_usec = 0;
					if (settimeofday(&newtime,
					    (struct timezone *)NULL) < 0) {
						menu_log("Could not set the date and time.\n");
					}
				}
			} while (newtime.tv_sec < 0);	
		}
	}
}

void
set_time(screen,banner,help,prompt1,prompt2,val1,val2)
	char *screen;
	char *banner;
	char *help;   /* paragraph that gets put at top of screen */
	char *prompt1; /* the prompt string */
	char *prompt2;
	char *val1;    /* ptr to where the info goes */
	char *val2;    /* ptr to where the info goes */
{
	int cur_line;
	int promptline;
	int display1 = REDISPLAY;
	int display2 = REDISPLAY;
	int ch;

	char c;

	do {
	     do {
		(void) clear();
		menu_window(STIME_PAGE);
		cur_line = print_help(screen,banner,help,0);

		/*
		 * now skip a couple lines and put the prompt string in
		 */
		cur_line += 2;
		promptline = cur_line;
		mvaddstr(promptline,2,prompt1);
		mvaddstr(promptline+2,2,prompt2);
		
		/*
		 * now print the current value (if any) and put the cursor
		 * at the end of this value
		 */
		mvaddstr(promptline,2+strlen(prompt1),val1);
		refresh();
		if (display1 == REDISPLAY) {
		/*
		 * now we get input
		 */
		display1 = getnam(val1,2+strlen(prompt1)+strlen(val1),promptline, SETTIME_HELPSCR,STIME_PAGE);
		if (*(val1+strlen(val1)-1) == '?') {
			*(val1+strlen(val1)-1) = '\0';  /* eat it if user inputs a '?'  */
		}
		}
	      } while (display1 == REDISPLAY);
	      promptline += 2;
	      mvaddstr(promptline,2+strlen(prompt2),val2);
	      refresh();
	      display2 = getnam(val2,2+strlen(prompt2)+strlen(val2),promptline, SETTIME_HELPSCR,STIME_PAGE);
	      if (*(val2+strlen(val2)-1) == '?') {
	          *(val2+strlen(val2)-1) = '\0';  /* eat it if user inputs a '?'  */
	      }
	} while (display2 == REDISPLAY);
}
/*
 * this is the same as suninstall's getnam() except that it allows
 * spaces 
 */
int
getnam(str,x,y,help,page)
	char str[];
	int x, y;
	char *help;
	int page;
{
	int xlate();
	int FORWARD, BACKWARD;
	register c, id, done;
	register char *cp = &str[strlen(str)];

	noecho();
	(void) move(y,x);
	for(id=x, done=0; !done ;) {
		(void) refresh();
		c = getch();
		switch(xlate(c)) {
			case '\n':
			case '\r':
				*cp = 0;
				done = 1;
				break;
			case CERASE:
				if(cp >= &str[0]) {
					mvaddch(y,--id,' ');
					if (cp == &str[0]) {
						move(y,++id);
					} else {
						move(y,id);
						*(cp--) = '\0';
					}
				}
				break;
			case CONTROL_U:
                                for (;cp >= &str[0];cp--,--id) {
                                        if (cp == &str[0]) {
                                                move(y,id);
                                                clrtoeol();
                                                refresh();
                                                break;
                                        }
                                }
                                break;
			case QUESTION:
				if (help != NULL) {
					info_window(help, 9,
						    "Press any key to continue",
						    '?', NULL, (int) 0);
					*cp++ = c;
					*cp = '\0';
					echo();
					return(REDISPLAY);
				}
				break;
			case CONTROL_B: switch(page) {
					case 2 :
						longjmp(sjbuf_timezone);
						 break;
					case 3 : 
						longjmp(sjbuf_time);
						 break;
					case 4 :
						longjmp(sjbuf_time);
						 break;
					case 5 :
						longjmp(sjbuf_net);
						 break;
					case 6 :
						longjmp(sjbuf_ip);
						 break;
					case 7 :
						longjmp(sjbuf_nis);
						 break;
					case 8 :
						return(CONFIRM_B); 
						 break;
					case 9 :
						longjmp(sjbuf_useracct);
						break;
					case 10:
						longjmp(sjbuf_userfn);
						break;
					case 11: 
						longjmp(sjbuf_userun);
						break;
				  }
				  break;

			case CONTROL_F: switch(page) {
				  case 2: case 3: case 4: case 5:case 6: case 7: 
					longjmp(sjbuf_hostname);
					  break;
				  }
				  break;
				
			default:
				if (c >= ' ' && c < '\177') {
					*cp++ = c;
					*cp = '\0';
					mvaddch(y,id++,c); 
				}
				break;
			}
	}
	echo();
#ifdef lint
	FORWARD = BACKWARD;
	BACKWARD = FORWARD;
#endif lint
	return(NOREDISPLAY);
}


/* This function compares the first token with a given key.
 * If they match, return a pointer to the 2nd token, else
 * return null pointer.
 */
char *
first_token(key,str)
	char *key,*str;
{
	char *cp1,*cp2;

	for (cp2 = str; isspace(*cp2); cp2++) ;
	for (cp1=key; *cp1 == *cp2 ; cp1++, cp2++)	
		if (*cp1 == '\0') 
		    return(cp2);
	if (*cp1 == '\0' && isspace(*cp2))
	    return(cp2);
	return(NULL);
}


int
read_modules(fp, soft_p, template)
	FILE *fp;
	soft_info * soft_p;
	char *template;
{
	char buf[BUFSIZ];
	char name[SMALL_STR], load_point[TINY_STR];
	int count = 0;

	while(fgets(buf,sizeof(buf),fp) != NULL) {
	    if (first_token("\}",buf))
		break;
	    if (sscanf(buf,"%s%s",name,load_point) != 2)    {
		    menu_log("%s:undefined category contents: %s\n",progname,buf) ;
		    outtahere(RC_DEAD);
	    }
	    if (select_module(soft_p, name, load_point))    
		    count ++;
	    else    {
		    (void) sprintf(buf, "\n%s of template \"%s\" is not found in tape contents", 
				name, template) ;
		    /*
		     * the following hard code 14 is to prevent overwriting
		     * choice screen when help is displayed.
		     */
		    info_window(buf, 14,
			    "Press 'a' to abort or any other key to continue",
			    'a', outtahere, RC_ABORT);

	    }
/***		    menu_log("\n%s of template \"%s\" is not found in tape contents", name, template) ;
***/
	}
	return(count);
}


#define DISKTYPES 4
#define MAXDISKNUM 32

int 
init_dkmap(device_name)
	char *device_name;
{
	char filename[MAXPATHLEN];
	char filename1[MAXPATHLEN];
	struct stat buf;
	int i,j;
	int tt;
        static char *disk_dev[DISKTYPES] = {
	    "sd","xy","xd", "id"
	};
	/* 
	 * Get current disk label 
	 */


	for (j=0; j < MAXDISKNUM; j++)	
	  for (i=0; i < DISKTYPES; i++)	{
	    if (*disk_dev[i] != 'i') {
		    /* Miniroot */
		(void) sprintf(filename, "/dev/%s%db", disk_dev[i],j);
		    /* Multiuser for debug only */
		(void) sprintf(filename1, "/dev/%s%da", disk_dev[i],j);
	    } else {
		    /* Miniroot */
		(void) sprintf(filename, "/dev/%s0%x%xb", disk_dev[i],j/8,j%8);
		    /* Multiuser for debug only */
		(void) sprintf(filename1, "/dev/%s0%x%xa", disk_dev[i],j/8,j%8);
	    }
	    if (( stat(filename,&buf) == -1
		    || (buf.st_dev) != (buf.st_rdev))
		&& (( stat(filename1,&buf) == -1
		    || (buf.st_dev) != (buf.st_rdev))))
		    continue;

	    if (*disk_dev[i] != 'i')
		(void) sprintf(device_name,"%s%d",disk_dev[i],j);
	    else
		(void) sprintf(device_name,"%s0%x%x",disk_dev[i],j/8,j%8);
	    (void) sprintf(filename,"/dev/r%sc",device_name);
/*	    
	    The disk having miniroot is the one will have 
	    root partition.
*/
            if ((tt = open(filename, 0)) != -1
              	 && ioctl(tt, DKIOCGAPART, &dk) == 0
		 && ioctl(tt, DKIOCGGEOM, &disk_geom) == 0) {
		    (void) close(tt);
		    return(0);
	    }
	    else {
		    menu_log("%s:could't open device %s\n",progname,filename);
		    return(-1);
	    }
	  }
	menu_log("%s:could't find disk device\n",progname);
	return(-1);
}

char *
remove_quote(str,c)
	char *str;
	char c;
{
	char *strrchr(),*strtok();
	char *endp;
        char *cp = str;
	while (isspace(*cp))
		cp++ ;
	if (*cp == c)  
		cp++ ;
	if ((endp = strrchr(cp,c)) != NULL)
		*endp = '\0';
	return(cp);
}

struct default_desc *
parse_header(fp)
	FILE *fp;
{
	char buf[LARGE_STR];
	char *cp, *strtok();
	struct default_desc *ddp;
	
	while (fgets(buf,LARGE_STR,fp) != NULL) {
	    if (strcmp("CATEGORY:", strtok(buf," ")) == 0) {
		if ((cp = strtok((char *)NULL," ")) != NULL)	{
		    if ((ddp = (struct default_desc *)
			malloc(sizeof(struct default_desc))) == NULL
			|| (ddp->def_name =
			    (char*)malloc((unsigned) strlen(cp))) == NULL ) {
			    menu_log("parse_header: out of memory");
			    outtahere(RC_DEAD);
		    }
		    (void) sscanf(cp,"%s",ddp->def_name);
		    if ((cp = strtok((char *)NULL,"\n")) != NULL)   {
			 cp = remove_quote(cp,'"');
			if ((ddp->def_desc =
			     (char*)malloc((unsigned)strlen(cp))) == NULL ) {
			    menu_log("parse_header: out of memory");
			    outtahere(RC_DEAD);
			}
			(void) strcpy(ddp->def_desc,cp);
		    }
		    else 
			    ddp->def_desc = "";
		    return(ddp);
		}
	    }
	}
	return(NULL);
}


/*  Advance file pointer to the beginning of a given category */

int
find_category(fp,catname)
	FILE *fp;
	char *catname;
{
	struct default_desc *ddp,*parse_header();
	
	while ((ddp = parse_header(fp)) != NULL)
		if (strcmp(catname,ddp->def_name) == 0)
			return(1);
	return(0);
}



/* This function parses the match statement with the following 
 * format:
 *	    match  variable=value
 * and match it with environment variables. 
 * return 1 if matched, and 0 otherwise.  In either case the
 * file pointer will advance to the beginning of next block
 */
int
match_category(fp)
	FILE *fp;
{
	struct desc_datum datum;
	char buf[LARGE_STR];
	
	while (fgets(buf,LARGE_STR,fp) != NULL) {
		if (first_token("}", buf)) 
			break;
		else   {
			if (get_a_key(buf,&datum) && match(&datum)) {
				continue;
			}
			/* Not a match, then search for end of match block */
			while (fgets(buf,LARGE_STR,fp) != NULL) {
				if (first_token("}", buf)) 
					return(0);
			}
		}
	}
	return(1);
}

int 
set_variables(fp,slackp)
	FILE *fp;
	int *slackp;
{
	struct desc_datum datum;
	char buf[LARGE_STR];
	
	*slackp = 0;
	while (fgets(buf,LARGE_STR,fp) != NULL) {
		if (first_token("}", buf)) 
			break;		
		if (get_a_key(buf,&datum))	{
			if (strcmp(datum.dat_key,"slack") == 0) {
				(void)sscanf(datum.dat_data,"%d",slackp);
			}
		}
	}
}


struct default_desc *
free_desc(dp)
	struct default_desc *dp;
{
	free(dp->def_name);    
	free(dp->def_desc);    
	free((char *)dp);    
	return((struct default_desc *)NULL);
}

void
skip_block(fp)
	FILE *fp;
{
	char buf[LARGE_STR];

	while (fgets(buf,LARGE_STR,fp) != NULL) {
	    if (first_token("}", buf)) 
		break;
	}
}

int
check_disk_space(size,preservep)
	unsigned long size;
	char *preservep;
{
	unsigned long total_size,avail_size;
	
	avail_size = C_NBLOCKS - A_NBLOCKS - B_NBLOCKS;
	if (strcmp(preservep,PRESERVE_ALL) == 0)
		avail_size -= (D_NBLOCKS + E_NBLOCKS + F_NBLOCKS + H_NBLOCKS);
	return (size < avail_size);
}

unsigned long
get_disk_size(fp, soft_p, template)
	FILE *fp;
	soft_info * soft_p;
	char *template;
{
	unsigned long total_size,avail_size;
	
	if ( read_modules(fp, soft_p, template) == 0)
		return(1);	/* no software module */
	
	return (FUDGE_SIZE(add_module_size(soft_p)) / BLOCK_SIZE);	
}

struct default_desc *
find_matches(soft_p)
	soft_info * soft_p;
{
	FILE *fp;
	struct default_desc *descp,*dp,*root = NULL;
	int slack;
	char buf[LARGE_STR];
	int matched;
	
        if (re_flag)
             fp = fopen(RE_PREINSTALLED_FILE,"r");
        else
             fp = fopen(CATEGORY_FILE,"r");
        if (fp == NULL)  {
		menu_log("find_match: No category file found\n");
		return(NULL);
	    }
	while ((descp = parse_header(fp)) != NULL)	    {
		reset_select_flag(soft_p);	    
		/* category with no match statement is a matched one */
		matched = 1;
		while (fgets(buf,LARGE_STR,fp) != NULL)	{
			if (first_token("match", buf))	    
				matched = match_category(fp);
			else if (first_token("set", buf))	{
				if (matched) 		    {
					set_variables(fp,&slack);
					if (preserveflag)
						(void) strcpy(preserve,
							      PRESERVE_ALL);
				}
				else 
					skip_block(fp);
			}
			else if (first_token("modules", buf)) {
				if (matched) {	
					descp->def_size =
						get_disk_size(fp,soft_p,
							      descp->def_name);
				} else	
					skip_block(fp);
				break;
			}
		}
		if (!matched) 
			descp = free_desc(descp);
		else {
			descp->def_next = NULL;
			if (root == NULL)   {
				root = descp;
			}
			else	{
				for (dp = root; dp->def_next;
				     dp = dp->def_next);
				
				dp->def_next = descp;
			}
		}
	}
	return(root);
}

int
load_template(choicep, soft_p, slackp, preservep)
	struct default_desc *choicep;
	soft_info *soft_p;
	int *slackp;
	char  *preservep;
{
	FILE *fp;
	char buf[LARGE_STR];

        if (re_flag)
            fp = fopen(RE_PREINSTALLED_FILE,"r");
        else
            fp = fopen(CATEGORY_FILE,"r");
        if (fp == NULL)  {
	    menu_log("load_template: No category file is found\n");
	    return(-1);
	}
	/*
	 * Search for a category that has the same name that still
	 * satifies the match requirements.  This allows multiple
	 * entries to have the same name
	 */
	
repeat:
	reset_select_flag(soft_p);	    
	if (find_category(fp,choicep->def_name))    {
	    while (fgets(buf,LARGE_STR,fp) != NULL)	{
		if (first_token("match", buf)) {	    
			if (!match_category(fp))
				goto repeat;
		}
		else if (first_token("set", buf))	{
			set_variables(fp,slackp);
			if (preserveflag)
			    (void) strcpy(preservep,PRESERVE_ALL);
		}
		else if (first_token("modules", buf)) {
			(void) read_modules(fp, soft_p, choicep->def_name); 
			break;
		}
	    }
	    return(0);
	}
	menu_log("category %s is not found\n",choicep->def_name);
	return(-1);
}
        	    
int
create_files(choicep, sysp, disk, soft_p)
	struct default_desc *choicep;
	sys_info *sysp;
	char  *disk;
	soft_info * soft_p;
{
	char pathname[MAXPATHLEN];
	char pathname1[MAXPATHLEN];
	int slack;
	int  ret_code;
	
	if (load_template(choicep, soft_p, &slack, preserve) < 0)
		return(-1);
	
        (void) sprintf(pathname1, "%s.%s", APPL_MEDIA_FILE, 
		       aprid_to_arid(soft_p->arch_str, pathname));
        (void) sprintf(pathname, "%s.%s", MEDIA_FILE, soft_p->arch_str);
        if (split_media_file(pathname, pathname1, soft_p) != 1)
		return(-1);
	
	if (update_arch(soft_p->arch_str, ARCH_INFO) < 1)
		return(-1);
	
	if (update_arch(soft_p->arch_str, ARCH_LIST) < 1)
		return(-1);
	
        (void) sprintf(pathname, "%s.%s", SOFT_INFO, soft_p->arch_str);
        if (save_soft_info(pathname, soft_p) != 1)
		return(-1);
	
	if (create_disk_files(preserve, disk, slack, soft_p) != 1)
		return(-1);
	
	create_disklist(disk);
	
	ret_code = make_mount_list(sysp);
	if (ret_code != 1)
		return(ret_code);
	
	return(1);
}

void
create_disklist(disk)
	char *disk;
{
	FILE *ofp;
	char buf[BUFSIZ];
	
	(void) sprintf(buf,"%s",DISK_LIST);
	if ((ofp = fopen(buf,"w")) == NULL)	{
		menu_log("create_disklist :could't open %s\n",buf);
		outtahere(RC_DEAD);
	}
	(void) fprintf(ofp,"%s\n",disk);
	(void) fclose(ofp);
}



int
check_ip(ipaddr)
	char *ipaddr;
{
	char buf[SMALL_STR];
	char *cp,*ep;
	int  ndots;

	(void) strcpy(buf,ipaddr);
	for(cp = buf; isspace(*cp); cp++) ;
	for(ep = cp, ndots = 0; *ep ; ep++)  {
		if (*ep == '.')	    {
		    ndots++;
		    *ep = '\0';

		if ((ndots == 2 || ndots == 3) && (ep == cp || (atoi(cp) > 255)))
			return(0);
		else if ((ndots == 1) && (ep == cp || (!atoi(cp)) || (atoi(cp) >= 255)))
			return(0);
		cp = ep+1;
		}

		else if (!isdigit(*ep))
		    return(0);
	}
        if (ndots != 3 || ep == cp || (!atoi(cp)) || (atoi(cp) >= 255))
		return(0);
	else	return(1);
}

int
xlate(c)
	register c;
{
	auto struct sgttyb local;
	auto struct tchars tlocal;
	extern int _tty_ch;	/* terminal file desc.	*/

	(void) ioctl(_tty_ch,TIOCGETP,&local);
	(void) ioctl(_tty_ch,TIOCGETC,&tlocal);

	if (c == local.sg_erase || c == CTRL(h))
		return CERASE;
	else if (c == local.sg_kill)
		return CKILL;
	else if (c == tlocal.t_eofc)
		return CEOT;
	else
		return c;
}

/*
 *	Name:		update_hosts()
 *
 *	Description:	Update the HOSTS file with the IP and hostname info
 *		from sys_info.
 */

static	void
update_hosts(sys_p)
	sys_info *	sys_p;
{
	char		value[MEDIUM_STR];	/* the value to add */


	if (strlen(sys_p->ether_name0) == 0) {
		(void) sprintf(value, "%s localhost loghost",
			       sys_p->hostname0);

		add_key_entry("127.0.0.1", value, HOSTS, KEY_OR_VALUE);
	}
	else {
		(void) sprintf(value, "%s loghost", sys_p->hostname0);

		add_key_entry(sys_p->ip0, value, HOSTS, KEY_OR_VALUE);

		if (strlen(sys_p->ip1))
			add_key_entry(sys_p->ip1, sys_p->hostname1, HOSTS, KEY_OR_VALUE);

		add_key_entry("127.0.0.1", "localhost", HOSTS, KEY_OR_VALUE);
	}

} /* end update_hosts() */

void
show_template(template, soft_p, line)
	struct default_desc *template;
	soft_info *soft_p;
	int line;
{
	int slack, count;
	media_file *	mf_p;			/* media file pointer */
	char *cp;
	int capa;

	if (load_template(template, soft_p, &slack, preserve) < 0)
		return;
	capa = 10000 / (100 + slack) ;
	(void) sprintf(help_msg, " Template Name: %s\n Preserve disk partitions : %s\n Capacity of /usr partition : %d percent\n Software Modules:\n",
		template->def_name, (strlen(preserve)?"Yes":"No"), capa);

#define MODULES_PER_ROW	4
#define MODULE_LEN  15
	for (mf_p = soft_p->media_files, count = 0, 
	     cp = &help_msg[strlen(help_msg)]; mf_p &&
	     mf_p < &soft_p->media_files[soft_p->media_count]; mf_p++) {
		if (mf_p->mf_select == ANS_YES)	{
		    count++;
		    if (strlen(mf_p->mf_name) > MODULE_LEN-1)	    {
			    if (count == MODULES_PER_ROW)	{
				    count = 2;
				    (void) sprintf(cp, "\n%30s",mf_p->mf_name);
			    }
			    else    {
				    count++ ;
				    (void) sprintf(cp, "%30s",mf_p->mf_name);
			    }
		    }
		    else    {
			    (void) sprintf(cp, "%15s",mf_p->mf_name);
		    }
		    cp = &help_msg[strlen(help_msg)];
		    if (count % MODULES_PER_ROW == 0)	    {
			    count = 0;
			    (void) sprintf(cp,"\n");
			    cp = &help_msg[strlen(help_msg)];
		    }
		}
	     }

	info_window(help_msg, line,
		    "Press any key to continue",
		    '?', NULL, (int) 0);	
}

struct default_desc * 
find_disc_label(mp,boot_disk)
	struct default_desc *mp;
	char *boot_disk;
{
	FILE *in, *fp, *fopen(), *popen();
	char label[SMALL_STR];
	long size;

#define MEDIUM ( 130 * MEGABYTE )
#define LARGE  ( 300 * MEGABYTE )

    fp = fopen(DISK_FILE,"w");
    fprintf(fp,"%s\n",boot_disk);
    fclose(fp);
	in = popen(LABEL_SCRIPT,"r");
	fscanf(in,"%s",label);
	pclose(in);
	size = atol (label);
	sprintf(label,"small");
    if (size >= LARGE) {
	 	sprintf(label,"large");
	}
	if (size >= MEDIUM) {
		sprintf(label,"medium");
	}
	(void)strcpy(preserve,"");
	(void)setup_env_variable("si-preserve","no");
	while ( mp != (struct default_desc *) NULL)	{
			if (strcmp(label,mp->def_name) == 0)	{
				if (!check_disk_space(mp->def_size,preserve)) {
					printf("Not enough disk space to start re-preinstalled.\n");
					exit(1);
				}	
				break;
			} else {
				mp = mp->def_next;
			}
	}
	return(mp);
}
			
