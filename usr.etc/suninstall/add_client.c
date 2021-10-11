#ifndef lint
#ifdef SunB1
#ident                  "@(#)add_client.c 1.1 92/07/30 SMI; SunOS MLS";
#else
#ident                  "@(#)add_client.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		add_client.c
 *
 *	Description:	This program adds clients to a server.  add_client
 *		is called from the "installation" program during miniroot
 *		time, and can be run in multiuser mode.  add_client must be
 *		called with the client's name if not ivoked with the "-i"
 *		option.  Only with the "-i" option specified, will
 *		get_client_info be invoked to provide a menu based interface
 *		for the information gathering.
 */


#include <ctype.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sysexits.h>


#include "install.h"
#include "menu.h"
#include "client_impl.h"


/*
 *	The well know usage message.
 */


#define USAGE \
"usage: %s [options] clients\n \
      %s -i|-p [options] [clients]\n \
   where 'clients' is the name of the any number of clients to add and\n \
     [options] are one or more of the following for each client:\n \
	-a arch \tarchitecture type (e.g. sun3, sun4, etc.) \n \
	-e path	\tpath to client's executables \n \
	-f path	\tpath to client's share location \n \
	-h path	\tpath to client's home directory \n \
	-i	\tinteractive mode - invoke full-screen mode \n \
	-k path	\tpath to client's kernel executables \n \
	-m path	\tpath to client's mail \n \
	-p	\tprint information of existing client\n \
	-r path	\tpath to client's root \n \
	-s path	\tpath to client's swap \n \
	-t termtype \tterminal to be used as console on client\n \
	-v	\tverbose mode - reports progress while running\n \
	-y type	\tclient's NIS type (client, or none)\n \
	-z size	\tsize of swap file (e.g. 16M, 16000K, 32768b  etc.)\n \
	-n	\tprints parameter settings and exits w/o adding client\n\n"


/*
 *	Macro to make sure  a full pathname is specified
 */

#define	fullpath(x,y)	if (index(x,'/') != x)	{	\
				(void) fprintf(stderr,	\
				   "%s: %s arg must begin with /\n",	\
					progname, x);	\
				y++;	\
			}

/*
 *	External functions and variables
 */
extern	void		clean_yp();
extern	char *		getwd();
extern	unsigned long	inet_addr();
extern	char *		inet_ntoa();
extern	char *		sprintf();
extern	char *		strstr();
extern	char *		index();

extern	char *		optarg;
extern	int		optind;
extern	int		opterr;


/*
 *	Local functions:
 */
static	int		check_out_client();
static	int		check_out_host();
static	void		clear_flags();
static	int		get_media_paths();
static	int		make_client();
static	void		make_info_clients();
static	void		mk_tftp_links();
static	void		print_all_clients();
static	void		print_client_info();
static	void		run_progs();
static	int		select_media_file();
static	void		setup_client_fs();
static	void		setup_info();
static	void		setup_server_fs();
static	void		setup_client_ttytab();
static	void		tune_client();
static	void		tune_client_security();
static	void		tune_server();
static	void		update_bootparams();
static	void		update_exports();
static	void		update_fstab();
static	void		update_clnt_hosts();
#ifdef SunB1
static	void		update_clnt_hosts_label();
static	void		update_clnt_net_label();
#endif /* SunB1 */
static	void		update_serv_hosts();
#ifdef SunB1
static	void		update_serv_hosts_label();
#endif /* SunB1 */
static	void		usage();
static	int		which_aprid();



/*
 *	Global variables:
 */

#ifdef SunB1
int		old_mac_state;			/* old mac-exempt state */
#endif /* SunB1 */

char *		progname;			/* name for error mesgs */

/*
** local variables
**
** NOTE : if more flags are added, one must update the function clear_flags()
**	  to make sure each client is initialized properly.
*/
static int debug = 0;	/* -d true for debugging mode			*/
static int aflag = 0;	/* -a true when architecure is specified	*/
static int eflag = 0;	/* -e true when the exec path was specified	*/
static int fflag = 0;	/* -f true when share path was specified	*/
static int hflag = 0;	/* -h true when the home path was specified	*/
static int iflag = 0;	/* -i true when interactive mode  was specified */
static int kflag = 0;	/* -k true when kvm path  was specified 	*/
static int mflag = 0;	/* -m true when mail path specified		*/
static int nflag = 0;	/* -n true when no-execute was specified	*/
static int pflag = 0;	/* -p true when client info is to be printed	*/
static int rflag = 0;	/* -r true when root path was specified		*/
static int sflag = 0;	/* -s true when swap path was specified		*/
static int tflag = 0;	/* -t true when terminal type was specified	*/
static int vflag = 0;	/* -v true when verbose specified		*/
static int zflag = 0;	/* -z true when swap size was specified (-z)	*/
/* Note : no -y flag is needed because of the initialized variable yptype,
**	  but the function clear_flags() does reset it to YP_CLIENT
*/


static int	was_not_server = 0;		/* set if it was not a sever
						** to begin with
						*/
						   
/*
 *	Variable placeholers for the command line arguments
 */
static	char		arch[MEDIUM_STR];	/* client architecture	*/   
static	char *		exec;			/* client's exec path 	*/
static	char *		home;			/* client's home path 	*/
static	char *		kvm;			/* client's kvm path 	*/
static	char *		mail;			/* client's mail path 	*/
static	char *		root;			/* client's root path 	*/
static	char *		share;			/* client's share path 	*/
static	char *		swap;			/* client's swap path 	*/
static	char *		swapsize;		/* client's swap size	*/
static	char *		terminal;		/* client's termtype	*/
static	int		yptype = YP_CLIENT;	/* client NIS type	*/

static	char *		exec_argv[5];		/* arguments for execute() */

/*
 * List of items we wish to export via exportfs.  This is a pointer to
 *	malloc'ed character arrays. The "" must be there.
 */
static char *		exportlist = "";


main(argc, argv)
	int		argc;
	char **		argv;
{
	sys_info	sys;			/* system information */

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
	**	Before we really do anything, let's check for any arguments
	*/
	if (argc == 1)
		(void) usage();

	if (read_sys_info(SYS_INFO, &sys) != 1) {
		menu_log("%s: Error in %s.", progname, SYS_INFO);
		menu_abort(1);
	}

	/*
	**	If this host is not a server, then we make it a server by
	**	changing the the SYS_INFO file to say it is a server. A
	**	problem can occur here if the Networking/NIS has not been
	**	loaded. The proper NIS and networking information may not be
	**	in the sys_info file.  If you are going to add a client, we
	**	are assuming the Networking has been intalled, if not, too
	**	bad, you screwed up.
	*/
	if (!is_miniroot() && (sys.sys_type != SYS_SERVER)) {
		was_not_server++;
		sys.sys_type = SYS_SERVER;
		if (save_sys_info(SYS_INFO, &sys) != 1) {
			menu_log("%s: could not save %s", progname, SYS_INFO);
			menu_abort(1);
		}
	}

	
	/*
	**	This is the main loop for going through all the clients.  It
	**	will keep adding the clients, until there is a bad command
	**	line option, or the end of the command line. If adding a
	**	one client failed, it will continue on to the next client.
	*/
	while (optind < argc) {

		/*
		**	Nullify all flags from previous client
		*/
		clear_flags();

		/*
		**	Set up all command line arguments
		*/
		if (parse_args(argc, argv) != 0)
			exit(EX_DATAERR);

		/*
		**	If we are at the end of the command line, then let's
		**	check if we were really expecting it after the
		**	command line switches.
		*/
		if ((argv[optind] == CP_NULL)) {
			if (pflag) {
				/*
				**	Print info on all the clients
				*/
				print_all_clients(&sys);
				break;		/* end it all */
			} else  if (!iflag) {
				/*
				**	If you don't want interactive mode
				**	finally, then give an error
				*/
				menu_mesg(
				  "%s : no client specified after argument %s",
					  progname, argv[optind - 1]);
				exit(1);
			}
		}
		
		if (iflag && !is_miniroot()) {
			/*
			**	Ivoke get_client_info to give a menu-based
			**	add_client.
			*/
			
			(void) fprintf(stderr,
		"\nInteractive mode uses no command line arguments\n");
			sleep(2);
			
			if (argv[optind] == CP_NULL) {
				exec_argv[0] = "get_client_info";
				exec_argv[1] = CP_NULL;
			} else {
				exec_argv[0] = "get_client_info";
				exec_argv[1] = optarg;
				exec_argv[2] = CP_NULL;
			}
			
			if (execute(exec_argv) != 1)
				exit(1);

			/*
			 * 	Now, let's make all of them, shall we.
			 *	They are listed in CLIENT_LIST_ALL
			 */
			make_info_clients(&sys);

			iflag = 0;	/* Don't invoke it the next time */

		} else {
			if (make_client(argv[optind], &sys) != 0) {
				menu_mesg("%s : failed to add client '%s'",
					  progname, argv[optind]);
			}
		}
			

		/*
		**	Go on to next client.
		*/
		optarg++;
		optind++;
	}

	exit(0);

	/* NOTREACHED */
} /* end main() */



/****************************************************************************
** Function : (static int)  make_client()
**
** Return Value : 0	if all is ok
**		  1	if an error occurred
**
** Purpose : This function does all that is necessary to add the client
**		passed to it as an argument.
**
****************************************************************************
*/
static int
make_client(client_name, sys_p)
	char		*client_name;
	sys_info	*sys_p;
{
	clnt_info	clnt;			/* client information */
	int		new_client = 1;		/* is this a new client? */
	char		pathname[MAXPATHLEN];	/* pathname buffer */
	char		buf[MEDIUM_STR];

	/*
	**	Make sure everything is cleaned out :-)
	*/
	(void) bzero((char *)&clnt, sizeof(clnt));

	(void) sprintf(pathname, "%s.%s", CLIENT_STR, client_name);
	switch (read_clnt_info(pathname, &clnt)) {
	case 1:
		/*	
		**	The client already exists, say so and continue,
		**	or just print out the info.
		*/
		if (pflag) {
			print_client_info(&clnt);
			return(0);
		}
		
		if (clnt.created) {
			(void) fprintf(stderr,
				       "%s: client %s is already a client\n",
				       progname, clnt.hostname);
			return(0);
		}
		break;

	case 0:
		/*
		**	So, the client does not actually exist.	 But now we
		**	want to go merrily along and start the creation
		**	process by reading in all the default information,
		**	unless the "-p" option was specified.
		*/
		if (pflag) {
			menu_mesg("%s : %s is not a client",
				  progname, client_name);
			return(0);
		}
		
		if (read_clnt_info(DEFAULT_CLIENT_INFO, &clnt) != 1) {
			menu_log( "%s: error in reading %s.\n",
				       progname, DEFAULT_CLIENT_INFO);
			return(1);
		}
		break;

	default:
		/*
		 *	Error message is done by read_clnt_info()
		 */
		return(1);
	}

	

	/*
	**      Don't do these things if you chose the interactive screen.
	**	These are more for the "-n" option than anything else.
	*/
	if (!iflag)  {

		if (!clnt.created)
			(void) strcpy(clnt.hostname, client_name);
		
		/*
		**      Assign	some information to the clinet structure and
		**      any  of those paths passed as arguments.
		*/
		if (aflag)
			(void) strcpy(clnt.arch_str, arch);
		if (zflag)
			(void) strcpy(clnt.swap_size, swapsize);
	}
	
	
	/*
	**	Let's do some checking of the host and client now, with
	**	respect to NIS, ip and domain stuff and then go get all the
	**	other information needed.  We are assuming that in
	**	miniroot, all the information had already been entered.
	*/
	if (!is_miniroot()) {
		if (check_out_host(sys_p->hostname0, sys_p->ip0, &clnt) != 0)
			return(EX_NOHOST);
		if (check_out_client(&clnt) != 0)
			return(1);

	
		/*
		**  All the information is stored now, so lets get the true
		**  architecture/release strings from ARCH_INFO and SOFT_INFO
		*/
		if (!iflag) {
			if (which_aprid(clnt.hostname, clnt.arch_str)
			    != 0) {
				menu_log(
			"%s : No release was chosen, aborting\n", progname);
				return(1);  /* exit program */
			}
			
			/*
			**	now we must get the exec and kvm paths from
			**	the appropriate media_file
			*/
			if (get_media_paths(&clnt) != 0) {
				menu_log(
			   "%s : Failed to get exec and kvm paths, aborting\n",
					 progname);
				return(1);		/* exit program */
			}
			
			/*
			**	Only get the paths if get_client_info was
			**	not called
			*/
			(void) setup_info(sys_p, &clnt);
			
		}  /* if (!iflag) */
	}
	
	
	/*
	**	Now, let's take a look at the information, before all the
	**	real work is done creating the monster.
	*/
	if (vflag)
		print_client_info(&clnt);
	
	if (nflag || debug)
		return(0);
	

	menu_mesg("\nCreating client '%s'\n", client_name);


	/*
	**	Let's save everything to disk, just to make sure we don't
	**	loose the information during the addition
	*/
	clnt.created = ANS_NO;
	if (save_all_client_info(&clnt) != 0) {
		menu_log("%s: could not save client information",
			 progname);
		return(1);
	}


	/*
	**	Only if this host was not a server of the client's
	**	architecture, will add_services be invoked to make it one.
	**	This is also definitely not a miniroot thing to do.
	*/
	if (!is_miniroot() &&
	    (was_not_server ||	
	     !is_server(clnt.arch_str)))  {
		(void) printf("Becoming a server for %s's\n",
			os_name(clnt.arch_str));

		if (select_media_file(clnt.arch_str, "root") != 1)  {
			menu_log("%s: could not select media file root",
				 progname);
			menu_abort(1);
		}
		
		exec_argv[0] = "add_services";
		exec_argv[1] = "-b";
		exec_argv[2] = aprid_to_irid(clnt.arch_str, buf);
		exec_argv[3] = CP_NULL;

		if (execute(exec_argv) != 1)
			exit(1);
	}

	if (iflag && is_sec_loaded(clnt.arch_str)) {
		exec_argv[0] = "get_sec_info";
		exec_argv[1] = clnt.hostname;
		exec_argv[2] = CP_NULL;

		if (execute(exec_argv) != 1)
			exit(1);
	}

	tune_server(&clnt, sys_p);

	setup_server_fs(&clnt, &new_client, sys_p);

	setup_client_fs(&clnt, new_client);

	tune_client(&clnt, sys_p);

	if (is_sec_loaded(clnt.arch_str))
		tune_client_security(&clnt);

	/*
	**	set the created field to yes and re-save the client files
	*/
	clnt.created = ANS_YES;
	if (save_all_client_info(&clnt) != 0) {
		menu_log("%s: could not save client information",
			 progname);
		return(1);
	}

	run_progs();

	/*
	 *	Now  do the necessary NIS updates, if appropriate
	 */
	update_yp(&clnt);

	return(0);
}



/****************************************************************************
** Function : (static void)  make_info_clients()
**
** Return Value : none
**
** Purpose : This function does is called after the invocation of
**		get_client_info to make all the clients listed from that
**		program, which are all listed in CLIENT_LIST_ALL.  If one of
**		the clients should happen to fail to make, we just give a
**		warning as usual.
**
****************************************************************************
*/
static void
make_info_clients(sys_p)
	sys_info	*sys_p;
{
	char		client_name[MAXHOSTNAMELEN];
	FILE *		fp;

	
	if ((fp = fopen(CLIENT_LIST_ALL, "r")) == (FILE *)NULL) {
		/*
		 *	No clients were entered, so forget it.
		 */
		return;
	}
	
	while (fscanf(fp, "%s\n", client_name) != EOF) {

		if (make_client(client_name, sys_p) != 0) {
			menu_mesg("%s : failed to add client '%s'",
				  progname,
				  client_name);
		}
	}   
}




/****************************************************************************
** Function : (static void)  print_all_clients()
**
** Return Value : none
**
** Purpose : If "add_client -p" is invoked, at the end of the command line,
**		all the clients following the -p will have their information
**		printed on the screen, if they exist.  No client will be
**		made that is listed after "-p", even if it does not exist.
**
****************************************************************************
*/
static void
print_all_clients(sys_p)
	sys_info	*sys_p;
{
	arch_info *	arch_list, *ap;	/* root of arch linked list */
	char		client_name[MAXHOSTNAMELEN];
	char		pathname[MAXPATHLEN];
	FILE *		fp;

	
	if (read_arch_info(ARCH_INFO, &arch_list) != 1) {
		menu_mesg("%s : error reading %s", progname, ARCH_INFO);
		return;
	}

	/*
	**	Let's see how many matches there are.  If there is only one,
	**	then we don't need to ask the question
	*/
	for (ap = arch_list; ap ; ap = ap->next) {

		(void) sprintf(pathname, "%s.%s", CLIENT_LIST, ap->arch_str);
		
		if ((fp = fopen(pathname, "r")) == (FILE *)NULL) {
			/*
			**	No clients were entered for this
			**	architecture, so forget it and go to the
			**	next architecture.
			*/

			continue;
		}
			
 		while (fscanf(fp, "%s\n", client_name) != EOF){
			/*
			**	Since the -p is specified, the client will
			**	not be made, but only printed out
			*/	
			if (make_client(client_name, sys_p) != 0) {
				menu_mesg("%s : failed to add client '%s'",
					  progname,
					  client_name);
			}
		}   
	}   /* for */
}



/****************************************************************************
** Function : (static void) clear_flags()
**
** Return Value : none
**
** Purpose : This function clears all flags so that each client starts anew.
**
****************************************************************************
*/

static void
clear_flags()
{
	aflag = 0;   /* -a true when architecure is specified	     	*/
	eflag = 0;   /* -e true when the exec path was specified     	*/
	fflag = 0;   /* -f true when share path was specified	     	*/
	hflag = 0;   /* -h true when the home path was specified     	*/
	iflag = 0;   /* -i true when interactive mode  was specified 	*/
	kflag = 0;   /* -k true when kvm path  was specified	     	*/
	mflag = 0;   /* -m true when mail path specified	     	*/
	rflag = 0;   /* -r true when root path was specified	     	*/
	sflag = 0;   /* -s true when swap path was specified	     	*/
	tflag = 0;   /* -t true when terminal type was specified     	*/
	vflag = 0;   /* -v true when verbose specified		     	*/
	zflag = 0;   /* -z true when swap size was specified (-z)    	*/
	yptype= YP_CLIENT;  /* The default NIS type			*/
}

/****************************************************************************
** Function : (static int) parse_args()
**
** Return Value : 0	      if all is ok
**		  EX_DATAERR  if an error occured
**
** Purpose : this function parses and checks the arguments from the command
*		line.
**
****************************************************************************
*/
static int
parse_args(argc, argv)
	int		argc;
	char		**argv;
{
	char	*cp;
	int	dataerr = 0;
	int	err = 0;
	int	c;
	
	while ((c = getopt(argc, argv, "a:bde:f:h:ik:m:npr:s:t:vy:z:"))
	       != -1) {
		switch (c) {
		case 'a':	/* architecture */
			/*
			**	Let's assume here that no one is going to
			**	type in the totally correct architecture, so
			**	we just handle it as a string here.
			*/
			aflag++;
			cp = optarg;
			(void) strcpy(arch, cp);
			break;
		case 'b':	
			/* eat the -b, but do nothing. */
			break;
		case 'd':	/* debuggin messages */
			debug++;
			vflag++;
			break;
		case 'e':	/* exec path */
			eflag++;
			exec = optarg;
			fullpath(exec, dataerr);	
			break;
		case 'f':	/* share path */
			fflag++;
			share = optarg;
			fullpath(share, dataerr);	
			break;
		case 'h':	/* home path */
			hflag++;
			home = optarg;
			fullpath(home, dataerr);	
			break;
		case 'i':	/* interactive mode */
			iflag++;
			break;
		case 'k':	/* home path */
			kflag++;
			kvm = optarg;
			fullpath(kvm, dataerr);
			break;
		case 'm':	/* mail path */
			mflag++;
			mail = optarg;
			fullpath(mail, dataerr);
			break;
		case 'n':	/*
				** prints parameter settings without making it
				*/
			nflag++;
			vflag++;
			break;
		case 'p':	/* only print client info */
			pflag++;
			break;
		case 'r':	/* root path */
			rflag++;
			root = optarg;
			fullpath(root, dataerr);	
			break;
		case 's':	/* swap path */
			sflag++;
			swap = optarg;
			fullpath(swap, dataerr);
			break;
		case 't':	/* terminal type */
			tflag++;
			terminal = optarg;
			if (check_client_terminal(terminal) != 1)
				dataerr++;
			break;
		case 'v':	/* verbose */
			vflag++;
			break;
		case 'y':	/* yptype */
			cp = optarg;
			if (!cv_str_to_yp(cp, &yptype)) {
				menu_log("%s: %s illegal yptype\n", 
					       progname,
					       cp);
				dataerr++;
			}
			else if (yptype == YP_MASTER || yptype == YP_SLAVE) {
				menu_log(
				    "%s: dataless client can not be NIS %s\n", 
				       progname,
				       cp);
				dataerr++;
			}
			break;
		case 'z':	/* swap size */
			zflag++;
			swapsize = optarg;
			if (cv_swap_to_long(swapsize) <= 0)
				dataerr++;
			break;
		default:
			err++;
			break;
		}
	}
	if (err)
		(void) usage();
	else if (dataerr) {
		menu_log( "%s: data error\n", progname);
		return(EX_DATAERR);
	}

	return(0);  /* all is AOK */
}


/****************************************************************************
** Function : (static void) setup_info()
**
** Return Value : none
**
** Purpose : this function checks to see if the args were specified and puts
**	     them into the client structure
**
****************************************************************************
*/
static void
setup_info(sys_p, clnt_p)
	sys_info	*sys_p;
	clnt_info	*clnt_p;
{

	clnt_p->yp_type = yptype;

	/* 
	 * Set up directory pathnames - root, exec, swap, etc.
	 */

	/*
	**	Use the root path on the command line, if it's there.
	*/
	if (rflag) {
		(void) strcpy(clnt_p->root_path, root);
	} else {
		/*
		**	just to make sure the client name is not repeated
		**	2ice in the pathname, we check for it's existence.
		*/
		if (strstr(clnt_p->root_path, clnt_p->hostname) == CP_NULL) {
			(void) strcat(clnt_p->root_path,  "/");
			(void) strcat(clnt_p->root_path, clnt_p->hostname);
		}
	}
	
	/*
	**	Use the swap path on the command line, if it's there.
	*/
	if (sflag)
		(void) strcpy(clnt_p->swap_path, swap);
	else {
		/*
		**	just to make sure the client name is not repeated
		**	2ice in the pathname, we check for it's existence.
		*/
		if (strstr(clnt_p->swap_path, clnt_p->hostname) == CP_NULL) {
			(void) strcat(clnt_p->swap_path,  "/");
			(void) strcat(clnt_p->swap_path,  clnt_p->hostname);
		}
	}
	

	/*
	**	just to make sure the client name is not repeated
	**	2ice in the pathname, we check for it's existence.
	*/
	if (strstr(clnt_p->home_path, sys_p->hostname0) == CP_NULL) {
		(void) strcat(clnt_p->home_path, "/");
		(void) strcat(clnt_p->home_path, sys_p->hostname0);
	}

#if 0
	/*
	**	Set up the share path
	*/
	(void) sprintf(clnt_p->share_path,"%s/%s", STD_SHARE_PATH,
			aprid_to_rid(clnt_p->arch_str, rid));

#endif
	/*
	**	Assign some information to the client structure and
	**	any  of those paths passed as arguments.  Even though you
	**	should not touch the first  three, we do allow the user to
	**	shoot himself.  Those are determined by the locations as set
	**	by add_services and should not be changed by the user!!!!
	*/
	if (eflag)
		(void) strcpy(clnt_p->exec_path, exec);
	if (kflag)
		(void) strcpy(clnt_p->kvm_path, kvm);
	if (fflag)
		(void) strcpy(clnt_p->share_path, share);

	if (hflag)
		(void) strcpy(clnt_p->home_path, home);
	if (mflag)
		(void) strcpy(clnt_p->mail_path, mail);
	if (tflag)
		(void) strcpy(clnt_p->termtype, terminal);

}


/****************************************************************************
** Function (int) check_out_host()
**
** Return Value : 0 if all is fine with the host
**		  EX_NOHOST if something is amiss
**
** Purpose : This function checks on the hostname, ip addresses, and sets
**	     the domainname of the client.
**
****************************************************************************
*/
static int
check_out_host(servername, server_ip, clnt_p)
	char		*servername;
	char		*server_ip;
	clnt_info	*clnt_p;
{
	struct hostent *he;
	
	/*
	 *	get the hostname of this host.	If none can be gotten,
	 *	stop now..
	 */
	if ((gethostname(servername, MAXHOSTNAMELEN) == -1) ||
	    (!strlen(servername))) {
		(void) log( "%s: couldn't get server's hostname\n", progname);
		(void) log( "%srun 'hostname <server> and retry.\n",
			   "\t\t\t");
		return(EX_NOHOST);
	}
	
	if ((he = gethostbyname(servername)) == (struct hostent *)NULL) {
		menu_log("%s: the server's name and internet address\n",
				progname);
		menu_log("   were not found in the hosts map.\n");
		return(EX_NOHOST);
	}
#ifdef lint
	he = he;
	server_ip = (char *) 0;
	server_ip = server_ip;
#else
	(void) strcpy(server_ip,
		      inet_ntoa(*(struct in_addr *)he->h_addr_list[0]));
#endif lint
	
	/*
	 *	catch the error in which the client and the servername
	 *	are the same...
	 */
	if (!strcmp(clnt_p->hostname, servername)) {
		(void) log(
		  "%s: server and client cannot have the same name -> [%s]\n",
			   progname, servername);
		return(EX_NOHOST);
	}
	
	/*
	 *	set domain name for client (same as server's for now...)
	 */
	(void) getdomainname(clnt_p->domainname, sizeof(clnt_p->domainname));

	return(0);
}


/****************************************************************************
** Function (static int) check_out_client()
**
** Return Value : 0 if all is fine with the client
**		  -1 if something is amiss
**
** Purpose : This function checks on the hostname, ip addresses and ethernet
**		addresses of the client
**	     
**
****************************************************************************
*/
static int
check_out_client(clnt_p)
	clnt_info	*clnt_p;
{
	struct hostent *he;
	struct ether_addr ether_addr;

	if ((he = gethostbyname(clnt_p->hostname)) ==(struct hostent *)NULL) {
		(void) menu_log("%s: client %s not in hosts map\n",
			   progname, clnt_p->hostname);
		return(-1);
	} else {
#ifdef lint
		he = he;
#else
		(void) strcpy(clnt_p->ip,
			inet_ntoa(*(struct in_addr *)he->h_addr_list[0]));
#endif lint
	}

	/*
	 *	get ether addr (for warning)
	 */
	if (ether_hostton(clnt_p->hostname, &ether_addr))  {
		(void) log(
		"%s: warning: entry for %s not found in ethers map\n",
			   progname, clnt_p->hostname);
		(void) bzero(clnt_p->ether, sizeof(clnt_p->ether));
	} else
		(void) strcpy(clnt_p->ether, (char *)ether_ntoa(&ether_addr));

	return(0);
}


/****************************************************************************
** Function (static void) print_client_info()
**
** Return Value : none
**
** Purpose : Prints out info in the clnt_info structure
**
****************************************************************************
*/
static void
print_client_info(clnt_p)
	clnt_info	*clnt_p;
{
	static short	title_line = 0;
	
#define FORMAT_STRING  "%-12s%-25s%-16s%s\n"

	if (!vflag) {
		if (!title_line)
			printf(FORMAT_STRING, "NAME", "RELEASE/ARCH",
			       "IP", "ROOT");
		title_line = 1;
		printf(FORMAT_STRING,
			clnt_p->hostname, os_name(clnt_p->arch_str),
			clnt_p->ip, clnt_p->root_path);
		return;
	}


        (void) printf("\nclient name               : %s\n",
                      clnt_p->hostname);
	(void) printf("client architecture  [-a] : %s\n",
		      os_name(clnt_p->arch_str));
        (void) printf("client inet (ip) address  : %s\n",
                      clnt_p->ip);
        (void) printf("client ethernet  address  : %s\n",
                      clnt_p->ether);
        (void) printf("client domainname         : %s\n",
                      clnt_p->domainname);
        (void) printf("client exec path     [-e] : %s\n",
                      clnt_p->exec_path);
        (void) printf("client share path    [-f] : %s\n",
                      clnt_p->share_path);
        (void) printf("client home path     [-h] : %s\n",
                      clnt_p->home_path);
        (void) printf("client kvmpath       [-k] : %s\n",
                      clnt_p->kvm_path);
        (void) printf("client mail path     [-m] : %s\n",
                      clnt_p->mail_path);
        (void) printf("client root path     [-r] : %s\n",
                      clnt_p->root_path);
        (void) printf("client swap path     [-s] : %s\n",
                      clnt_p->swap_path);
        (void) printf("client terminal type [-t] : %s\n",
                      clnt_p->termtype);
        (void) printf("client NIS type      [-y] : %s\n",
                      cv_yp_to_str(&clnt_p->yp_type));
        (void) printf("client swap size     [-z] : %s bytes\n",
                      clnt_p->swap_size);
        (void) putchar('\n');
	switch(clnt_p->created) {
	case ANS_YES :
		(void) printf("`%s' has already been created\n",
			      clnt_p->hostname);
		break;
	case ANS_NO :
		(void) printf("`%s' has not been created yet\n",
			      clnt_p->hostname);
		break;
	}
        (void) putchar('\n');

        if (debug) {
                (void) printf("client choice             : %d\n",
                              clnt_p->choice);
        }

}



/****************************************************************************
** Function (int) save_all_client_info()
**
** Return Value : 0 if all is fine with the client
**		  non zero if something is amiss
**
** Purpose : This function saves all the client information for us.
**
****************************************************************************
*/
int
save_all_client_info(clnt_p)
	clnt_info	*clnt_p;
{
	char	pathname[MAXPATHLEN];
	
	(void) sprintf(pathname, "%s.%s", CLIENT_STR, clnt_p->hostname);
	if (save_clnt_info(pathname, clnt_p) != 1)
		return(-1);
	
	/*
	 *	Add new client to client_list.<arch>
	 */
	if (add_client_to_list(clnt_p) == 0)
		return(-1);

	return(0);
}


/*
 *	Name:		mk_tftp_links()
 *
 *	Description:	Make the appropriate links in /tftpboot for the
 *		named client.  Calls dir_prefix() to get to the right place.
 */

static	void
mk_tftp_links(client_p)
	clnt_info *	client_p;
{
	unsigned long	addr;			/* inet address */
	char		boot_name[SMALL_STR];	/* boot file name */
	struct hostent * host_p;		/* ptr to host entry */
	char		link_path[MAXPATHLEN];	/* path to link */
	char		buf[MEDIUM_STR];
	char		*strupr();

	host_p = gethostbyname(client_p->hostname);
	if (host_p == (struct hostent *)NULL) {
		menu_log("%s: %s: cannot get host file entry.", progname,
			 client_p->hostname);
		menu_abort(1);
	}

	(void) sprintf(boot_name, "boot.%s", 
			    aprid_to_irid(client_p->arch_str, buf));

	/*
	 *	Translate the address from the host file into an inet address
	 */
#ifdef lint
	addr = 0;
#else
	addr = inet_addr(inet_ntoa(*((struct in_addr *) host_p->h_addr)));
#endif lint

	(void) sprintf(link_path, "%s/tftpboot/%08lX", dir_prefix(), addr);

	mklink(boot_name, link_path);

	(void) sprintf(link_path, "%s/tftpboot/%08lX.%s", dir_prefix(), 
			addr, strupr(aprid_to_iid(client_p->arch_str, buf)));

	mklink(boot_name, link_path);
} /* end mk_tftp_links() */


/*
 *	Name:		setup_client_ttytab()
 *
 *	Description:	Fix the terminal type in TTYTAB on the root mount
 *		point for the console.
 */

static void
setup_client_ttytab(clnt_p)
	clnt_info	*clnt_p;
{
	char		cmd[MAXPATHLEN*3];	/* command buffer */
	char		pathname[MAXPATHLEN];

	(void) fprintf(stderr, "Setting up Client's ttytab\n");

	/*
	**	To avoid the nasty error of prehaps no entry, "sun" is hard
	**	coded here just to make sure
	*/
	if (!isalnum(*(clnt_p->termtype)))
		(void) strcpy(clnt_p->termtype, "sun");

	/*
	**	Now, set up the ttytab on the client
	*/
	(void) sprintf(pathname, "%s%s%s",
		      dir_prefix(),
		      clnt_p->root_path,
		      TTYTAB);

	(void) sprintf(cmd,
    "sed \"s/sun/%s/\" < %s > /tmp/ttytab;cp /tmp/ttytab %s;rm /tmp/ttytab",
		     clnt_p->termtype, pathname, pathname);
	
	(void) system(cmd);
}




/*
 *	Name:		run_progs()
 *
 *	Description:	Run any programs that must be run after a client
 *		is added.
 */

static	void
run_progs()
{
	char *	cmd;					/* command buffer */

	if (is_miniroot())
		return;

	cmd = malloc((unsigned)strlen(exportlist) + MAXPATHLEN + 15); 
	
	(void) sprintf(cmd, "exportfs %s 2>> %s", exportlist, LOGFILE);
	x_system(cmd);

	free(cmd);

} /* end run_progs() */




/*
 *	Name:		setup_client_fs()
 *
 *	Description:	Setup the file system for 'client_p'.  This routine
 *		only sets up file system objects in the client's domain on the
 *		server.	 If 'new_client' is true, then the following tasks
 *		are performed:
 *
 *			- copy PROTO_ROOT
 *			- copy proto.local if it exists
 *			- copy binaries listed in INSTALL_BIN
 *			- make device nodes
 *			- fix the DEVICE_GROUPS file (SunB1)
 *			- set up the console DAC file (SunB1)
 *
 *		The following tasks are always performed:
 *
 *			- make mount point for /usr
 *			- make home directory for the client
 *			- cleanup NIS
 */	

static	void
setup_client_fs(client_p, new_client)
	clnt_info *	client_p;
	int		new_client;
{
#ifdef SunB1
	char		cmd[MAXPATHLEN + SMALL_STR]; /* command buffer */
#endif SunB1	
	char		dest_path[MAXPATHLEN];	/* destination pathname */
	char		src_path[MAXPATHLEN];	/* source pathname */
	char		buf[MEDIUM_STR];	/* temp buffer */


	(void) printf("Setting up client's file system\n");

	/*
	 *	Only do some of the setup if this is a new client.
	 */
	if (new_client) {
		/*
		 *	Copy the prototype root filesystem to the client's
		 *	root.  Cannot use RMP_PROTO_ROOT here since we don't
		 *	know if this is the miniroot or not.
		 */
		(void) sprintf(dest_path, "%s%s", dir_prefix(),
			       client_p->root_path);
		(void) sprintf(src_path, "%s%s", dir_prefix(),
			       proto_root_path(client_p->arch_str));

		copy_tree(src_path, dest_path);

		/*
		 *	Copy the local prototype to the client's root.
		 */
		(void) sprintf(src_path, "%s%s/../proto.local", dir_prefix(),
			       client_p->root_path);

		if (access(src_path, F_OK) == 0)
			copy_tree(src_path, dest_path);

		copy_install_bin(client_p->exec_path, client_p->kvm_path,
			client_p->root_path);

		/*
		 *	Make device node in the client's root.
		 */
		MAKEDEV(dest_path, STD_DEVS);

#ifdef SunB1
		fix_devgroup("console", "system_low", "system_high",
			     DEVICE_CLEAN, dest_path);

		(void) sprintf(dest_path, "%s%s%s", dir_prefix(),
			       client_p->root_path, SEC_DEV_DIR);
		mkdir_path(dest_path);

		(void) sprintf(cmd, "touch %s/console >> %s 2>&1", dest_path,
			       LOGFILE);
		x_system(cmd);

		(void) sprintf(cmd, "chmod 0666 %s/console >> %s 2>&1",
			      dest_path, LOGFILE);
		x_system(cmd);
#endif /* SunB1 */
	}
	else {
		menu_log("%s: root directory for %s already exists (%s%s).\n",
			       progname, client_p->hostname, dir_prefix(),
			       client_p->root_path);
		menu_log(
			       "\tContents will not be overwritten.\n");
	}

	/*
	 *	Make mount point for /usr on the client's root.
	 */
	(void) sprintf(dest_path, "%s%s/usr", dir_prefix(),
		       client_p->root_path);
	mkdir_path(dest_path);

	/*
	**	Let's make sure the share path also exist, so a client of a
	**	hetero server can mount on the share path
	*/
	(void) sprintf(dest_path, "%s%s/share", dir_prefix(),
		       client_p->exec_path);
	mkdir_path(dest_path);
	

	/*
	 *	Make the client's home directory.
	 */
	(void) sprintf(dest_path, "%s%s%s", dir_prefix(),
		       client_p->root_path,
		       client_p->home_path);
	mkdir_path(dest_path);

	clean_yp(client_p->root_path, client_p->yp_type);

} /* end setup_client_fs() */




/*
 *	Name:		setup_server_fs()
 *
 *	Description:	Setup the server's file system.	 This routine sets
 *		up file system objects outside of the client's domain.	If 
 *		'client_p' is a new client, then *new_client_p is set to 1.
 *		The following tasks are performed:
 *
 *			- if the client's architecture is the same as the
 *			  servers, then make a symbolic link from
 *			  /export/exec/<arch> to /usr and from
 *			  /export/exec/kvm/<arch> to /usr/kvm
 *			- make tftp links
 *			- make client's root directory
 *			- make client's home directory in the server's /home
 *			- make client's swap file
 */

static	void
setup_server_fs(client_p, new_client_p, sys_p)
	clnt_info *	client_p;
	int *		new_client_p;
	sys_info *	sys_p;
{
	char		cmd[MAXPATHLEN * 2];	/* command buffer */
	char		pathname[MAXPATHLEN];	/* pathname buffer */
	char		buf[MEDIUM_STR];

	(void) printf("Setting up server file system to support client\n");

	if (strcmp(sys_p->arch_str, client_p->arch_str) == 0) {
		(void) sprintf(pathname, "%s%s", dir_prefix(),
			       aprid_to_execpath(sys_p->arch_str, cmd));

		mklink("/usr", pathname);

		(void) sprintf(pathname, "%s%s", dir_prefix(),
			       aprid_to_kvmpath(sys_p->arch_str, cmd));

		mklink("/usr/kvm", pathname);
	}

	mk_tftp_links(client_p);

	/*
	 *	Make the client's root directory
	 */
	(void) sprintf(pathname, "%s%s", dir_prefix(), client_p->root_path);
	if (access(pathname, F_OK) == 0)
		*new_client_p = 0;
	else
		mkdir_path(pathname);
	
	/*
	 *	Make the client's parent unreadable by anyone other than
	 *	the owner.  This is done to prevent a security hole of
	 *	being able to execute setuid programs on the server
	 *	from the client's root partition
	 */
	(void) sprintf(pathname, "%s%s/..", dir_prefix(), client_p->root_path);
	(void) chmod(pathname,0700);

	/*
	 *	Make the client's home directory on the server.
	 */
	(void) sprintf(pathname, "%s%s", dir_prefix(), client_p->home_path);
	mkdir_path(pathname);

	/*
	 *	Make the parent directory for the client's swap file.
	 */
	(void) sprintf(pathname, "%s%s", dir_prefix(), client_p->swap_path);
	mkdir_path(dirname(pathname));

	(void) unlink(pathname);

	/*
	 *	Make the client's swap file
	 */
	(void) printf("Making client's swap file\n");
	(void) sprintf(cmd, "mkfile %ld %s 2>> %s",
		       cv_swap_to_long(client_p->swap_size), pathname,
		       LOGFILE);
	x_system(cmd);

} /* end setup_server_fs() */




/*
 *	Name:		tune_client()
 *
 *	Description:	Tune 'client_p' after all the files have been loaded.
 *		This routine only tunes file system objects in the client's
 *		domain.	 The following tasks are performed:
 *
 *			- update FSTAB
 *			- update client's HOSTS file
 *			- update client's HOSTS_LABEL file (SunB1)
 *			- update client's NET_LABEL file (SunB1)
 *			- make the client's RC_DOMAINNAME, RC_HOSTNAME,
 *			  RC_IFCONFIG, RC_NETMASK, and RC_RARPD files
 *			- fix bugs for UFS_MINIROOT (SunB1)
 */

static	void
tune_client(client_p, sys_p)
	clnt_info *	client_p;
	sys_info *	sys_p;
{
#ifdef SunB1
#ifdef UFS_MINIROOT
	char		cmd[MAXPATHLEN * 2];	/* command buffer */
#endif /* UFS_MINIROOT */
#endif /* SunB1 */

	char		pathname[MAXPATHLEN];	/* path to hostname file */

	(void) printf("Updating client's databases\n");

	update_fstab(client_p, sys_p);

	update_clnt_hosts(client_p, sys_p);

#ifdef SunB1
	update_clnt_hosts_label(client_p, sys_p);
	update_clnt_net_label(client_p, sys_p);
#endif /* SunB1 */

	(void) sprintf(pathname, "%s%s", dir_prefix(), client_p->root_path);
	if (client_p->yp_type == YP_NONE)
		mk_rc_domainname(CP_NULL, pathname);
	else
		mk_rc_domainname(client_p->domainname, pathname);

	setup_client_ttytab(client_p);

#ifdef SunB1
#ifdef UFS_MINIROOT
/*
 *	Make temp directories
 */
	(void) sprintf(cmd, "rm -rfH %s%s/tmp %s%s/var/tmp",
		       dir_prefix(), client_p->root_path,
		       dir_prefix(), client_p->root_path);
	x_system(cmd);

	(void) sprintf(cmd, "mkdir %s%s/.No_Hid.tmp %s%s/var/.No_Hid.tmp",
		       dir_prefix(), client_p->root_path,
		       dir_prefix(), client_p->root_path);
	x_system(cmd);

	(void) sprintf(cmd,"chmod 0777 %s%s/.No_Hid.tmp %s%s/var/.No_Hid.tmp",
		       dir_prefix(), client_p->root_path,
		       dir_prefix(), client_p->root_path);
	x_system(cmd);

	(void) sprintf(cmd, "chmod g+s %s%s/.No_Hid.tmp %s%s/var/.No_Hid.tmp",
		       dir_prefix(), client_p->root_path,
		       dir_prefix(), client_p->root_path);
	x_system(cmd);

/*
 *	Make spool directories
 */
	(void) sprintf(cmd,
	  "mv %s%s/var/spool/cron/crontabs %s%s/var/spool/cron/crontabs.orig",
		       dir_prefix(), client_p->root_path,
		       dir_prefix(), client_p->root_path);
	x_system(cmd);

	(void) sprintf(cmd, "rm -rf %s%s/var/spool/cron/atjobs",
		       dir_prefix(), client_p->root_path);
	x_system(cmd);

	(void) sprintf(cmd,"rm -rf %s%s/var/spool/mail %s%s/var/spool/mqueue",
		       dir_prefix(), client_p->root_path,
		       dir_prefix(), client_p->root_path);
	x_system(cmd);

	(void) sprintf(cmd, "rm -rf %s%s/var/spool/cron/.No_Hid.atjobs",
		       dir_prefix(), client_p->root_path);
	x_system(cmd);

	(void) sprintf(cmd,
	   "rm -rf %s%s/var/spool/.No_Hid.mail %s%s/var/spool/.No_Hid.mqueue",
		       dir_prefix(), client_p->root_path,
		       dir_prefix(), client_p->root_path);
	x_system(cmd);


	(void) sprintf(cmd,
"mkdir %s%s/var/spool/cron/.No_Hid.atjobs %s%s/var/spool/cron/.No_Hid.crontabs",
		       dir_prefix(), client_p->root_path,
		       dir_prefix(), client_p->root_path);
	x_system(cmd);

	(void) sprintf(cmd,
"chmod 0700 %s%s/var/spool/cron/.No_Hid.atjobs %s%s/var/spool/cron/.No_Hid.crontabs",
		       dir_prefix(), client_p->root_path,
		       dir_prefix(), client_p->root_path);
	x_system(cmd);

	(void) sprintf(cmd,
"chmod g+s %s%s/var/spool/cron/.No_Hid.atjobs %s%s/var/spool/cron/.No_Hid.crontabs",
		       dir_prefix(), client_p->root_path,
		       dir_prefix(), client_p->root_path);
	x_system(cmd);

	(void) sprintf(cmd,
	    "mkdir %s%s/var/spool/.No_Hid.mail %s%s/var/spool/.No_Hid.mqueue",
		       dir_prefix(), client_p->root_path,
		       dir_prefix(), client_p->root_path);
	x_system(cmd);

	(void) sprintf(cmd, "chmod 0770 %s%s/var/spool/.No_Hid.mqueue",
		       dir_prefix(), client_p->root_path);
	x_system(cmd);

	(void) sprintf(cmd, "chmod g+s %s%s/var/spool/.No_Hid.mqueue",
		       dir_prefix(), client_p->root_path);
	x_system(cmd);

	(void) sprintf(cmd, "chmod 1777 %s%s/var/spool/.No_Hid.mail",
		       dir_prefix(), client_p->root_path);
	x_system(cmd);

	(void) sprintf(cmd, "chmod g+s %s%s/var/spool/.No_Hid.mail",
		       dir_prefix(), client_p->root_path);
	x_system(cmd);

/*
 *	This copy kludge will put all crontab files in at system_low
 */
	(void) sprintf(cmd,
	"cp %s%s/var/spool/cron/crontabs.orig/* %s%s/var/spool/cron/crontabs",
		       dir_prefix(), client_p->root_path,
		       dir_prefix(), client_p->root_path);
	x_system(cmd);

	(void) sprintf(cmd, "rm -rf %s%s/var/spool/cron/crontabs.orig",
		       dir_prefix(), client_p->root_path);
	x_system(cmd);
#endif /* UFS_MINIROOT */
#endif /* SunB1 */

} /* end tune_client() */




/*
 *	Name:		tune_client_security()
 *
 *	Description:	Tune the security features on the client.  This
 *		routine should only modify objects in the client's domain.
 *		EXCEPTION: the tune_audit() routine will modify the server's
 *		EXPORTS file and make audit directories for clients while
 *		processing the list of audit directories.  The following
 *		tasks are performed:
 *
 *			- tune auditing configuration
 *			- fix PASSWD file entries
 */

static	void
tune_client_security(client_p)
	clnt_info *	client_p;
{
	char		pathname[MAXPATHLEN];	/* scratch path */
	sec_info	sec;			/* security info */


	(void) sprintf(pathname, "%s.%s", SEC_INFO, client_p->hostname);
	switch (read_sec_info(pathname, &sec)) {
	case 0:
		menu_log("%s: %s: cannot read file.", progname, pathname);
		menu_abort(1);

	case 1:
		break;

	case -1:
		menu_abort(1);
	}

	(void) sprintf(pathname, "%s%s", dir_prefix(), client_p->root_path);

	tune_audit(client_p, pathname, &sec);

	fix_passwd(pathname, &sec);
} /* end tune_client_security() */




/*
 *	Name:		tune_server()
 *
 *	Description:	Tunes the server.  This routine should only modify
 *		file system objects outside of the client's domain.  The
 *		following tasks are performed:
 *
 *			- update BOOTPARAMS file
 *			- update ETHERS file
 *			- update the server's HOSTS file
 *			- update the server's HOSTS_LABEL file (SunB1)
 *			- update the server's exports file
 *			- if this is not the miniroot, then push the
 *			  NIS databases.
 */

static	void
tune_server(client_p, sys_p)
	clnt_info *	client_p;
	sys_info *	sys_p;
{
	char		pathname[MAXPATHLEN];	/* path to ETHERS files */


	(void) printf("Updating the server's databases\n");

	update_bootparams(client_p, sys_p);
	/*
	 *	Update the ETHERS file on the server.  Cannot use
	 *	RMP_ETHERS here since this may not be the miniroot.
	 */
	if (*(client_p->ether) != '\0') {
		(void) sprintf(pathname, "%s%s", dir_prefix(), ETHERS);
		add_key_entry(client_p->ether, client_p->hostname, pathname, KEY_OR_VALUE);
	}

	update_serv_hosts(client_p);

#ifdef SunB1
	update_serv_hosts_label(client_p);
#endif /* SunB1 */

	update_exports(client_p);

} /* end tune_server() */




/*
 *	Name:		update_bootparams()
 *
 *	Description:	Update the BOOTPARAMS file on the server.  Cannot
 *		use RMP_BOOTPARAMS here since this may not be the miniroot.
 *
 *	Note:		This implementation assumes that the server is always
 *		hostname0, but this is demonstrably wrong.
 */

static	void
update_bootparams(client_p, sys_p)
	clnt_info *	client_p;
	sys_info *	sys_p;
{
	char		entry[MAXPATHLEN * 3];	/* entry buffer */
	char		pathname[MAXPATHLEN];	/* path to BOOTPARAMS */
	FILE *		pp;			/* ptr to ed(1) process */


	(void) sprintf(entry, "root=%s:%s\\\n\tswap=%s:%s",
		       sys_p->hostname0, client_p->root_path,
		       sys_p->hostname0, client_p->swap_path);

	(void) sprintf(pathname, "%s%s", dir_prefix(), BOOTPARAMS);
	add_key_entry(client_p->hostname, entry, pathname, KEY_OR_VALUE);

	(void) sprintf(entry, "ed %s > /dev/null 2>&1", pathname);

	pp = popen(entry, "w");
	if (pp == (FILE *)NULL) {
		menu_log("%s: '%s' failed.", progname, entry);
		menu_abort(1);
	}

	(void) fprintf(pp, "/^+/d\n");
	(void) fprintf(pp, "$a\n+\n.\n");
	(void) fprintf(pp, "w\nq\n");
	(void) pclose(pp);
} /* end update_bootparams() */




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
 *	Description:	Update the EXPORTS file on the server.	Cannot use
 *		RMP_EXPORTS here since this may not be the miniroot.
 */

static	void
update_exports(client_p)
	clnt_info *	client_p;
{
	char		entry[MAXPATHLEN * 2];	/* entry buffer */
	char		pathname[MAXPATHLEN];	/* path to EXPORTS */

	(void) sprintf(pathname, "%s%s", dir_prefix(), EXPORTS);

	(void) sprintf(entry, "-access=%s,root=%s", client_p->hostname,
		       client_p->hostname);

	add_key_entry(client_p->root_path, entry, pathname, KEY_ONLY);
	export_it(client_p->root_path);

	add_key_entry(client_p->swap_path, entry, pathname, KEY_ONLY);
	export_it(client_p->swap_path);

} /* end update_exports() */




/*
 *	Name:		update_fstab()
 *
 *	Description:	Update the server's FSTAB with the information from
 *		'client_p'.
 *
 *	Note:		The remote mount of /var/spool/mail may not work
 *		properly for SunB1 since /var/spool/mail is a hiding dir.
 */

static	void
update_fstab(client_p, sys_p)
	clnt_info *	client_p;
	sys_info *	sys_p;
{
	char		key[MAXPATHLEN + SMALL_STR];	/* key buffer */
	char		value[LARGE_STR];	/* value buffer */
	char		pathname[MAXPATHLEN];	/* pathname to FSTAB */
	char		buf[MEDIUM_STR];	/* aprid strings */

	(void) sprintf(pathname, "%s%s%s", dir_prefix(), client_p->root_path,
		       FSTAB);
	/*
	 *	Remove the existing FSTAB so we start with a clean slate
	 */
	(void) unlink(pathname);
	
	(void) sprintf(key, "%s:%s", sys_p->hostname0, client_p->root_path);
	(void) sprintf(value, "/ %s rw 0 0", DEFAULT_NET_FS);
	add_key_entry(key, value, pathname, KEY_OR_VALUE);

	(void) sprintf(key, "%s:%s", sys_p->hostname0,
				aprid_to_execpath(client_p->arch_str, buf));
	(void) sprintf(value, "/usr %s ro 0 0", DEFAULT_NET_FS);
	add_key_entry(key, value, pathname, KEY_OR_VALUE);

	(void) sprintf(key, "%s:%s", sys_p->hostname0, 
			aprid_to_kvmpath(client_p->arch_str, buf));
	(void) sprintf(value, "/usr/kvm %s ro 0 0", DEFAULT_NET_FS);
	add_key_entry(key, value, pathname, KEY_OR_VALUE);

	(void) sprintf(key, "%s:%s", sys_p->hostname0, client_p->home_path);
	(void) sprintf(value, "%s %s rw 0 0", client_p->home_path,
		       DEFAULT_NET_FS);
	add_key_entry(key, value, pathname, KEY_OR_VALUE);

	(void) sprintf(key, "%s:%s", sys_p->hostname0, client_p->mail_path);
	(void) sprintf(value, "/var/spool/mail %s rw 0 0", DEFAULT_NET_FS);
	add_key_entry(key, value, pathname, KEY_OR_VALUE);

/*	(void) sprintf(key, "%s:%s", sys_p->hostname0, client_p->crash_path);
	(void) sprintf(value, "/var/crash %s rw 0 0", DEFAULT_NET_FS);
	add_key_entry(key, value, pathname, KEY_OR_VALUE);
*/

	(void) sprintf(key, "%s:%s", sys_p->hostname0, 
		    aprid_to_sharepath(client_p->arch_str, buf));
	(void) sprintf(value, "/usr/share %s rw 0 0", DEFAULT_NET_FS);
	add_key_entry(key, value, pathname, KEY_OR_VALUE);

} /* end update_fstab() */




/*
 *	Name:		update_clnt_hosts()
 *
 *	Description:	Update the client's HOSTS file with information
 *		from 'client_p' and 'sys_p'.
 */

static	void
update_clnt_hosts(client_p, sys_p)
	clnt_info *	client_p;
	sys_info *	sys_p;
{
	char		pathname[MAXPATHLEN];	/* path to HOSTS */


	(void) sprintf(pathname, "%s%s%s", dir_prefix(), client_p->root_path,
		       HOSTS);

	add_key_entry(client_p->ip, client_p->hostname, pathname, KEY_OR_VALUE);

	add_key_entry(sys_p->ip0, sys_p->hostname0, pathname, KEY_OR_VALUE);

	if (strlen(sys_p->ip1))
		add_key_entry(sys_p->ip1, sys_p->hostname1, pathname, KEY_OR_VALUE);
} /* end update_clnt_hosts() */




#ifdef SunB1
/*
 *	Name:		update_clnt_hosts_label()
 *
 *	Description:	Update the HOSTS_LABEL file for the client with the
 *		IP and hostname info from clnt_info and sys_info.
 */

static	void
update_clnt_hosts_label(client_p, sys_p)
	clnt_info *	client_p;
	sys_info *	sys_p;
{
	char		pathname[MAXPATHLEN];	/* path to HOSTS_LABEL */
	char		value[MEDIUM_STR];	/* value to add */


	(void) sprintf(pathname, "%s%s%s", dir_prefix(),
		       client_p->root_path,  HOSTS_LABEL);

	(void) sprintf(value, "%s Labeled", client_p->hostname);
	add_key_entry(client_p->ip, value, pathname, KEY_OR_VALUE);

	(void) sprintf(value, "%s Labeled", sys_p->hostname0);
	add_key_entry(sys_p->ip0, value, pathname, KEY_OR_VALUE);

	if (strlen(sys_p->ip1)) {
		(void) sprintf(value, "%s Labeled", sys_p->hostname1);
		add_key_entry(sys_p->ip1, value, pathname, KEY_OR_VALUE);
	}
} /* end update_clnt_hosts_label() */




/*
 *	Name:		update_clnt_net_label()
 *
 *	Description:	Update the NET_LABEL file for the client with the
 *		IP and label info from clnt_info and sys_info.
 */

static	void
update_clnt_net_label(client_p, sys_p)
	clnt_info *	client_p;
	sys_info *	sys_p;
{
	char		prefix[MAXPATHLEN];	/* prefix to NET_LABEL */


	(void) sprintf(prefix, "%s%s", dir_prefix(), client_p->root_path);
	add_net_label(client_p->ip, client_p->ip_minlab, client_p->ip_maxlab,
		      prefix);

	/*
	 *	Note: may have to put in an entry for the loopback here.
	 */
	if (sys_p->ether_type != ETHER_NONE) {
		add_net_label(sys_p->ip0, sys_p->ip0_minlab, sys_p->ip0_maxlab,
			      prefix);

		if (strlen(sys_p->ip1))
			add_net_label(sys_p->ip1, sys_p->ip1_minlab,
				      sys_p->ip1_maxlab, prefix);
	}
} /* end update_clnt_net_label() */
#endif /* SunB1 */




/*
 *	Name:		update_serv_hosts()
 *
 *	Description:	Update the server's HOSTS file with information
 *		from 'client_p'.
 */

static	void
update_serv_hosts(client_p)
	clnt_info *	client_p;
{
	add_key_entry(client_p->ip, client_p->hostname, HOSTS, KEY_OR_VALUE);

	/*
	 *	Update the HOSTS file on the miniroot too.
	 */
	if (is_miniroot())
		add_key_entry(client_p->ip, client_p->hostname, RMP_HOSTS, KEY_OR_VALUE);
} /* end update_serv_hosts() */




#ifdef SunB1
/*
 *	Name:		update_serv_hosts_label()
 *
 *	Description:	Update the HOSTS_LABEL file for the server with the
 *		IP and hostname info from clnt_info.
 */

static	void
update_serv_hosts_label(client_p)
	clnt_info *	client_p;
{
	char		value[MEDIUM_STR];	/* value to add */


	(void) sprintf(value, "%s Labeled", client_p->hostname);
	add_key_entry(client_p->ip, value, HOSTS_LABEL, KEY_OR_VALUE);

	/*
	 *	Update the HOSTS_LABEL file on the miniroot too.
	 */
	if (is_miniroot())
		add_key_entry(client_p->ip, value, RMP_HOSTS_LABEL, KEY_OR_VALUE);
} /* end update_serv_hosts_label() */
#endif /* SunB1 */





/****************************************************************************
** Function : (static int) arch_compares()
** 
** Return values :  1 : if the architectures do match upto strlen() chars
**		    0 : if the architectures do not match upto strlen() chars
**
** Description : This function compares the to architecture strings passed
**			to it in a special way.  If the aflag (-a) was
**			specified, then only the implementation
**			architectures are specified.  If no (-a), then
**			either interactive mode was used, so we match the
**			entire arch_string.
**
*****************************************************************************
*/
int
arch_compares(command_line_arch, arch_str)
	char	*command_line_arch;
	char	*arch_str;
{
	char		buffer[MEDIUM_STR];
	char		buffer2[MEDIUM_STR];

	if (aflag) {

		/*
		 *	Compare the implementation architectures
		 */
		return(strncmp(command_line_arch,
			       aprid_to_irid(arch_str, buffer),
			       strlen(command_line_arch)) == 0);
	} else {

		/*
		 *	Compare the whole implementation string
		 */
		return(strncmp(aprid_to_irid(command_line_arch, buffer2),
			       aprid_to_irid(arch_str, buffer),
			       strlen(command_line_arch)) == 0);
	}		
}



/****************************************************************************
** Function : (static char *) which_aprid()
** 
** Return values :  0 : if and an aprid was chosen.
**		    1 : indicating no pair was chosen.
**
** Description : This function goes through the architectures in the
**		ARCH_INFO file and asks the user if the architecture is the
**		one that he wants and then gets all the information needed
**		out of the SOFT_INFO.APRID files about exec and kvm paths
**
*****************************************************************************
*/
static int
which_aprid(client_name, command_line_arch)
	char	*client_name;
	char	*command_line_arch;
{
	arch_info *	arch_list, *ap;	/* root of arch linked list */
	char		question[128];
	short		only_one = 0;	/* if only 1 match, don't ask the
					** question
					*/

	if (read_arch_info(ARCH_INFO, &arch_list) != 1) {
		menu_mesg("%s : error reading %s", progname, ARCH_INFO);
		return(1);
	}


	/*
	**	Let's see how many matches there are.  If there is only one,
	**	then we don't need to ask the question later, just return.
	*/
	for (ap = arch_list; ap ; ap = ap->next) {
		if (arch_compares(command_line_arch, ap->arch_str))
			only_one++;
	}


	for (ap = arch_list; ap ; ap = ap->next) {
		/*
		**	If the user specified architecture, if any exists on
		**	the command line then compare it to the ones in
		**	ARCH_INFO  and ask him if this is the correct one.
		**
		*/
		if (arch_compares(command_line_arch, ap->arch_str)) {
			if (only_one == 1) {
				(void) strcpy(command_line_arch, ap->arch_str);
				return(0);
			}
			
			(void) sprintf(question,
	   "Is '%s' the correct release to use for '%s' ? (y/n)",
				       os_name(ap->arch_str),
				       client_name);
			
			if (ask(question)) {
				(void) strcpy(command_line_arch, ap->arch_str);
				return(0);
			} else
				continue;
		}   /* if strncmp() */
	}   /* for */

	return(1);
}

/****************************************************************************
** Function : (static int) get_media_paths()
** 
** Return values :  0  : if the paths were found
**		    -1 : indicating no paths were found
**
** Description : This function reads the information from the SOFT_INFO
**		 files, according to the aprid in clnt_p
**
*****************************************************************************
*/
static int
get_media_paths(clnt_p)
	clnt_info	*clnt_p;
{
	char		pathname[MAXPATHLEN];
	soft_info	soft;

	(void) bzero((char *)&soft, sizeof(soft));

	(void) sprintf(pathname, "%s.%s", SOFT_INFO, clnt_p->arch_str);

	if (read_soft_info(pathname, &soft) != 1)
		return(-1);
	else {
		(void) strcpy(clnt_p->exec_path, soft.exec_path);
		(void) strcpy(clnt_p->kvm_path, soft.kvm_path);
		(void) strcpy(clnt_p->share_path, soft.share_path);
		return(0);
	}
}



/****************************************************************************
** Function : (int) yes_no_question()
**
** Return Value : 0 : if the answer is no
**		  1 : if the answer is yes
**
** Description : this function just asks the use a yes or no question, which
**		 defaults to NO
**
*****************************************************************************
*/

int
ask(yes_no_question)
	char *	yes_no_question;
{
	char line [32];

	(void) printf ("%s ", yes_no_question);
	(void) fflush (stdout);
	line [0] = '\0';
	(void) gets (line);
	if ((line [0] == 'y') || (line [0] == 'Y'))
		return (1);
	else
		return (0);
	
} /* end of ask() */



/*
 *	Name:		usage()
 *
 *	Description:	Print a usage message and die.
 */


static	void
usage()
{
	(void) fprintf(stderr, USAGE, progname, progname);
	exit(1);
} /* end usage() */

static int
select_media_file(arch, mediafile)
	char *	    arch ;
	char *	    mediafile ;
{
	char		pathname[MAXPATHLEN];
	char		pathname1[MAXPATHLEN];
	soft_info	soft;
	media_file *	mf_p;			/* media file pointer */

	(void) bzero((char *)&soft, sizeof(soft));

	(void) sprintf(pathname1, "%s.%s", APPL_MEDIA_FILE, 
			aprid_to_arid(arch, pathname));
	(void) sprintf(pathname, "%s.%s", MEDIA_FILE, arch);
	if ( merge_media_file(pathname, pathname1, &soft) != 1)
		return(-1);

	for (mf_p = soft.media_files; mf_p &&
	     mf_p < &soft.media_files[soft.media_count]; mf_p++) 
		if (strcmp(mediafile, mf_p->mf_name) == 0)   {
			mf_p->mf_select = ANS_YES;
			return (split_media_file(pathname, pathname1, &soft));
		}
	return(-1);	
}
