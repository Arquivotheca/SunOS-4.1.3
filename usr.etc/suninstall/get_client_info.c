#ifndef lint
#ifdef SunB1
static  char    sccsid[] = 	"@(#)get_client_info.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static  char    sccsid[] = 	"@(#)get_client_info.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1990 Sun Microsystems, Inc.
 */

/*
 *	Name:		get_client_info.c
 *
 *	Description:	Use a menu based interface to get information from
 *		the user about a client.  This program is typically called
 *		from suninstall and add_client.  If invoked with a client's
 *		name, then get_client_info deals with information about
 *		that client.
 */

#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <arpa/inet.h>


#include "client_impl.h"


/*
 *	External references:
 */
extern	char *		sprintf();

static	char		last_root_base[MAXPATHLEN];
static	char		last_swap_base[MAXPATHLEN];


/*
 *	Local functions:
 */
static	void		cv_name_to_arch();
static	int		get_info();
static  void		set_client_ip();
static	int		update_sizes();




/*
 *	Global variables:
 */
#ifdef SunB1
int		old_mac_state;			/* old mac-exempt state */
#endif /* SunB1 */
char *		progname;			/* name for error messages */




main(argc, argv)
	int		argc;
        char **		argv;
{
	arch_info 	arch_list;		/* architecture info buffer */
	char *		client_name = NULL;	/* client name */
	clnt_info	clnt;			/* client info buffer */
	char		def_arch[MEDIUM_STR];	/* default architecture */
	clnt_disp	disp;			/* client display buffer */
	form *		form_p;			/* ptr to CLIENT FORM */
	clnt_info	old_client;		/* ptr to old client info */
	sys_info	sys;			/* system info buffer */


	(void) umask(UMASK);			/* set file creation mask */

	progname = basename(argv[0]);		/* get name for error mesgs */

	if (argc > 2) {
		(void) fprintf(stderr, "Usage: %s [client_name]\n", progname);
		exit(1);
	}
	if (argc == 2) {
		client_name = argv[1];
	}

	bzero((char *) &clnt, sizeof(clnt));
	bzero((char *) last_root_base, sizeof(last_root_base));
	bzero((char *) last_swap_base, sizeof(last_swap_base));

	set_menu_log(LOGFILE);
						/* get system name */
	if (read_sys_info(SYS_INFO, &sys) != 1) {
		menu_log("%s: Error in %s.", progname, SYS_INFO);
		menu_abort(1);
	}

	if (sys.sys_type != SYS_SERVER) {
		menu_mesg("Must be a server to have clients.");
		menu_abort(1);
	}

	(void) strcpy(def_arch, sys.arch_str);

	/*
	 *	Copy the client's name from the command line
	 *	if there is one.
	 */
	if (client_name) {
		(void) strcpy(clnt.hostname, client_name);
		cv_name_to_arch(client_name, def_arch);
	}

	if (init_client_info(def_arch, &arch_list, &clnt) != 1) {
		if (is_miniroot()) {
			menu_mesg(
     "%s is not supported by this server. Please go to the software screen.",
				  def_arch);
		}  else {
			menu_mesg(
	 "%s is not supported by this server. Please run add_services %s.",
				  def_arch, def_arch);
			menu_abort(1);
		}
	}

	/*
	 *	Before we start anything major, let's clean up this file, so
	 *	that we will have a clean slate.  If it's not there, we'll
	 *	get an error, but we don't really care, since we will create
	 *	it if necessary
	 */
	(void) unlink(CLIENT_LIST_ALL);

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

	form_p = create_client_form(&sys, &arch_list, &clnt, &disp,
				    &old_client);

	show_client_list(form_p, clnt.arch_str, &disp);

	/*
	 *	Ignore the 4.0 "repaint"
	 */
	(void) signal(SIGQUIT, SIG_IGN);

	if (use_form(form_p) != 1)		/* use CLIENT form */
		menu_abort(1);

	(void) signal(SIGQUIT, SIG_DFL);

	end_menu();				/* done with menu system */

	exit(0);
	/*NOTREACHED*/
} /* end main() */


/*
 *	Name:		create_client()
 *
 *	Description:	Create a client if it does not already exist.  The
 *		values in DEFAULT_CLIENT_INFO are used to initialize the
 *		client structure.
 */

int
create_client(params)
	pointer		params[];
{
	char		arch_str[MEDIUM_STR];	/* saved architecture */
	int		choice;			/* saved choice */
	clnt_info *	client_p;		/* pointer to client info */
	clnt_info 	tmp_client;		/* temp client struct */
	clnt_disp *	disp_p;			/* pointer to client display */
	form *		form_p;			/* pointer to CLIENT form */
	char		hostname[MAXHOSTNAMELEN]; /* saved hostname */
	char		pathname[MAXPATHLEN];	/* path to client.<name> */
	int		ret_code;		/* return code */
	sys_info *	sys_p;			/* pointer to system info */
	int		arch;			/* saved arch id */
	char		exec_path[MAXPATHLEN];	/* exec path of  client */
	char		kvm_path[MAXPATHLEN];	/* kvm path of  client */
	char		share_path[MAXPATHLEN];	/* share path of  client */

	sys_p = (sys_info *) params[0];
	client_p = (clnt_info *) params[2];
	disp_p = (clnt_disp *) params[3];
	form_p = (form *) params[4];

	(void) sprintf(pathname, "%s.%s", CLIENT_STR, client_p->hostname);

	ret_code = read_clnt_info(pathname, &tmp_client);

	switch (ret_code) {
	case  -1 : /*
		   **	read error of the file
		   */
		menu_mesg("%s : cannot read  file %s.",  progname, pathname);
		return(ret_code);
		break;
	case 1 :  /*
		  **	the file existed,, so let's check if it was a real
		  **	   client
		  */
		if (tmp_client.created) {
			menu_mesg("%s%s\n%s\n%s",
				  client_p->hostname, " already exists.",
				  "You cannot recreate an existing client.",
				  "You must first remove it with 'rm_client'");
			return(0);
		} else {
			menu_mesg("Data for '%s' already exists, %s",
				  client_p->hostname,
				  "Please use 'edit' or 'delete' options");
			return(0);
		}
			
			
		break;
	default :
			/*
			 **	Probably file did not exist (0), so do
			 **	nothing different
			 */
		break;
	}
	
	
	/*
	 *	Clear client list off the screen after the error check.
	 *	Turn off client info while we are updating it.
	 */
	off_client_disp(form_p);
	off_menu_file(find_menu_file((pointer) form_p, "client_list"));
	off_client_info(client_p, form_p);
	off_form_shared(form_p, find_form_yesno(form_p, "value_check"));

	/*
	 *	save client info
	 */
	(void) strcpy(arch_str, client_p->arch_str);
	choice = client_p->choice;
	(void) strcpy(hostname, client_p->hostname);
	arch = client_p->arch;
	(void) strcpy(exec_path, client_p->exec_path);
	(void) strcpy(kvm_path, client_p->kvm_path);
	(void) strcpy(share_path, client_p->share_path);
	
	ret_code = read_clnt_info(DEFAULT_CLIENT_INFO, client_p);

	/* restore client info */
	client_p->arch = arch;
	(void) strcpy(client_p->arch_str, arch_str);
	client_p->choice = choice;
	(void) strcpy(client_p->hostname, hostname);
	(void) strcpy(client_p->share_path, share_path);

	/*
	 *	Only a return code of 1 is okay here.  Must add an error
	 *	message for ret_code == 0.
	 */
	if (ret_code != 1) {
		if (ret_code == 0)
			menu_log("%s: %s: cannot read file.", progname,
				 pathname);
		return(-1);
	}

	ret_code = get_clnt_display(client_p, disp_p);
	if (ret_code != 1)
		return(ret_code);

	(void) strcpy(client_p->exec_path, exec_path);
	(void) strcpy(client_p->kvm_path, kvm_path);
	(void) strcpy(client_p->share_path, share_path);
	(void) sprintf(&client_p->home_path[strlen(client_p->home_path)],
		       "/%s", sys_p->hostname0);
	(void) sprintf(&client_p->root_path[strlen(client_p->root_path)],
		       "/%s", hostname);
	(void) sprintf(&client_p->swap_path[strlen(client_p->swap_path)],
		       "/%s", hostname);

	(void) strcpy(client_p->domainname, sys_p->domainname);

	if (sys_p->yp_type != YP_NONE)
		client_p->yp_type = YP_CLIENT;

 	set_client_ip(client_p, sys_p);

	on_client_disp(form_p);
	on_client_info(client_p, form_p);
	on_form_shared(form_p, find_form_yesno(form_p, "value_check"));

	return(1);
} /* end create_client() */




/*
 *	Name:		client_okay()
 *
 *	Description:	This function is invoked when the user says the
 *		information gathered is okay.  If there is sufficient
 *		disk space for this client, then the following tasks
 *		are performed:
 *
 *			- the client's information is saved
 *			- the client is added to the appropriate CLIENT_LIST
 *			- the list of clients for the current architecture
 *			  is displayed
 */

int
client_okay(params)
	pointer		params[];
{
	clnt_info *	client_p;		/* pointer to client info */
	clnt_disp *	disp_p;			/* pointer to client display */
	form *		form_p;			/* pointer to CLIENT form */
	clnt_info *	old_p;			/* ptr to old client info */
	char		pathname[MAXPATHLEN];	/* path to client.<name> */
	int		ret_code;		/* return code */
	char	*	cp;			/* temp char * */

	client_p = (clnt_info *) params[2];
	disp_p = (clnt_disp *) params[3];
	form_p = (form *) params[4];
	old_p = (clnt_info *) params[5];

	/*
	 *	We are editing an existing client so adjust the old values.
	 */
	if (client_p->choice == CLNT_EDIT) {
		if (update_sizes(old_p->root_path, -MIN_XROOT,
				 old_p->swap_path, 
				 -cv_swap_to_long(old_p->swap_size)) != 1)
			return(-1);
	}

	ret_code = update_sizes(client_p->root_path, MIN_XROOT,
				client_p->swap_path, 
				cv_swap_to_long(client_p->swap_size));
	if (ret_code != 1) {
		if (client_p->choice == CLNT_EDIT &&
		    update_sizes(old_p->root_path, MIN_XROOT, old_p->swap_path, 
			         cv_swap_to_long(old_p->swap_size)) != 1) {
			menu_log("%s: cannot restore bytes count.",
				 progname);
			return(-1);
		}

		return(ret_code);
	}

	off_client_disp(form_p);
	off_client_info(client_p, form_p);
	off_form_shared(form_p, find_form_yesno(form_p, "value_check"));

	(void) sprintf(pathname, "%s.%s", CLIENT_STR, client_p->hostname);
	if (save_clnt_info(pathname, client_p) != 1)
		return(-1);

	/*
	 *	Add new client to client_list.<arch>
	 */
	if (add_client_to_list(client_p) != 1)
		return(-1);

	show_client_list(form_p, client_p->arch_str, disp_p);

	/*
	 *	Set up the base root and swap base paths
	 */
	(void) strcpy(last_root_base, client_p->root_path);
	(void) strcpy(last_swap_base, client_p->swap_path);

	cp = strrchr(last_root_base, '/');
	if (cp == (char *)NULL)
		*last_root_base = '\0';
	else
		*cp = '\0';

	cp = strrchr(last_swap_base, '/');
	if (cp == (char *)NULL)
		*last_swap_base = '\0';
	else
		*cp = '\0';


	/***	clear_arch_buttons(params);   ***/

	return(1);
} /* end client_okay() */



/*
 *	Name:		set_client_ip()
 *
 *	Description:	Try to get very close to the real ip, no the bogus
 *			on in DEFAULT_CLIENT_INFO, and while we're at it,
 *			grab the ethernet address, if possible.
 */
static void
set_client_ip(client_p, sys_p)
	clnt_info	*client_p;
	sys_info	*sys_p;
{
	struct	hostent		*host_p;
	struct ether_addr ether_addr;
	char			*cp;

	/*
	** 	If we can find a client registered in the NIS's, then we use
	**	it, else we just use some of the host's ip address numbers
	*/

	if ((host_p = gethostbyname(client_p->hostname)) !=
	    (struct hostent *)NULL) {
#ifdef lint
		host_p = host_p;
#else
		(void) strcpy(client_p->ip,
			inet_ntoa(*(struct in_addr *)host_p->h_addr_list[0]));
#endif lint
	} else {
		/*
		**  use all but the last byte of the address for the client
		*/ 
		(void) strcpy(client_p->ip, sys_p->ip0);
		if ((cp = strrchr(client_p->ip, '.')) != (char *)NULL)
			*(cp + 1) = '\0';
	}


	/*
	 * get ether addr (why not?)
	 * if there is none to be found, skip it 
	 */
	if (ether_hostton(client_p->hostname, &ether_addr) == 0)
		(void) strcpy(client_p->ether, (char *)ether_ntoa(&ether_addr));

}


/*
 *	Name:		cv_name_to_arch()
 *
 *	Description:	Convert the name of a client into an architecture code.
 */

static void
cv_name_to_arch(name, def_p)
	char *		name;
	char *		def_p;
{
	clnt_info	clnt;			/* client info buffer */
	char		pathname[MAXPATHLEN];	/* path to client_list files */


	(void) sprintf(pathname, "%s.%s", CLIENT_STR, name);

	if (read_clnt_info(pathname, &clnt) == 1)   {
		(void) strcpy(def_p, clnt.arch_str);
	}

} /* end cv_name_to_arch() */



/*
 *	Name:		delete_client()
 *
 *	Description:	Delete a client if it exists.  The following tasks
 *		are performed:
 *
 *			- ask if the user is sure
 *			- delete the client from the appropriate CLIENT_LIST
 *			- delete the client's information file
 *			- update the disk space allocation
 *			- display the client list
 */

int
delete_client(params)
	pointer		params[];
{
	clnt_info *	client_p;		/* pointer to client info */
	clnt_disp *	disp_p;			/* pointer to client display */
	form *		form_p;			/* pointer to CLIENT form */
	char		pathname[MAXPATHLEN];	/* path to client.<name> */
	int		ret_code;		/* return code */


	client_p = (clnt_info *) params[2];
	disp_p = (clnt_disp *) params[3];
	form_p = (form *) params[4];

	/*
	 *	Display the client
	 */
	ret_code = display_client(params);
	if (ret_code != 1)
		return(ret_code);

	/*
	**	If the client is created, you can't remove it with this
	**	option, so just tell them to use rm_client
	*/
	if (client_p->created) {
		off_client_disp(form_p);
		off_client_info(client_p, form_p);

		menu_mesg(
		"%s  already exists. Please use 'rm_client' to remove it",
			  client_p->hostname);
		show_client_list(form_p, client_p->arch_str, disp_p);
		/*
		 *	Clear client info after the question regardless of
		 *	the answer.  The shared object was turned off in
		 *	display_client().
		 */
		return(0);
	}

	ret_code = menu_ask_yesno(NOREDISPLAY,
				  "Do you really want to delete %s?",
			          client_p->hostname);

	/*
	 *	Clear client info after the question regardless of the answer.
	 *	The shared object was turned off in display_client().
	 */
	off_client_disp(form_p);
	off_client_info(client_p, form_p);

	if (ret_code == 1) {
		ret_code = delete_client_from_list(client_p);
		if (ret_code != 1) {
			show_client_list(form_p, client_p->arch_str, disp_p);
			return(ret_code);
		}

		(void) sprintf(pathname, "%s.%s", CLIENT_STR,
			       client_p->hostname);
		(void) unlink(pathname);

		if (update_sizes(client_p->root_path, -MIN_XROOT,
				 client_p->swap_path, 
				 -cv_swap_to_long(client_p->swap_size)) != 1)
			return(-1);
	}

	show_client_list(form_p, client_p->arch_str, disp_p);

	return(1);
} /* end delete_client() */



/*
 *	Name:		display_client()
 *
 *	Description:	Display information about a client if it exists.
 */

int
display_client(params)
	pointer		params[];
{
	char		arch_str[MEDIUM_STR];	/* saved architecture */
	int		choice;			/* saved choice */
	clnt_info *	client_p;		/* pointer to client info */
	clnt_disp *	disp_p;			/* pointer to client display */
	form *		form_p;			/* pointer to CLIENT form */
	char		hostname[MAXHOSTNAMELEN]; /* saved hostname */
	char		pathname[MAXPATHLEN];	/* path to client.<arch> */
	int		ret_code;		/* return code */
	int		arch;			/* saved arch id */


	client_p = (clnt_info *) params[2];
	disp_p = (clnt_disp *) params[3];
	form_p = (form *) params[4];

	(void) strcpy(arch_str, client_p->arch_str);
	choice = client_p->choice;
	(void) strcpy(hostname, client_p->hostname);
	arch = client_p->arch;

	/*
	 *	Clear client info before we update it.
	 */
	off_client_disp(form_p);
	off_menu_file(find_menu_file((pointer) form_p, "client_list"));
	off_client_info(client_p, form_p);
	off_form_shared(form_p, find_form_yesno(form_p, "value_check"));

	off_form_field(find_form_field(form_p, "name"));

	(void) sprintf(pathname, "%s.%s", CLIENT_STR, client_p->hostname);

	switch (read_clnt_info(pathname, client_p)) {
	case 1:
		client_p->choice = choice;
		client_p->arch = arch;
		on_form_field(find_form_field(form_p, "name"));
		break;

	case 0:
		(void) strcpy(client_p->arch_str, arch_str);
		client_p->choice = choice;
		client_p->arch = arch;
		(void) strcpy(client_p->hostname, hostname);
		on_form_field(find_form_field(form_p, "name"));

		menu_mesg("%s is not a client.", hostname);

		show_client_list(form_p, arch_str, disp_p);
		return(0);

	case -1:
		(void) strcpy(client_p->arch_str, arch_str);
		client_p->choice = choice;
		client_p->arch = arch;
		(void) strcpy(client_p->hostname, hostname);
		on_form_field(find_form_field(form_p, "name"));
		return(-1);
	}

	ret_code = get_clnt_display(client_p, disp_p);
	if (ret_code != 1)
		return(ret_code);

	on_client_disp(form_p);
	on_client_info(client_p, form_p);

	/*
	 *	If we are just displaying, the wait for an acknowledge
	 *	and clear the client info off the screen.
	 */
	if (client_p->choice == CLNT_DISPLAY) {
		menu_ack();
		off_client_disp(form_p);
		off_client_info(client_p, form_p);
		show_client_list(form_p, client_p->arch_str, disp_p);
	}

	return(1);
} /* end display_client() */




/*
 *	Name:		edit_client()
 *
 *	Description:	Set up to edit information about a client if it
 *		exists.  The following tasks are performed:
 *
 *			- display the client information
 *			- display disk space information
 *			- save the existing client information
 *			- turn on the new values confirmer object
 */

int
edit_client(params)
	pointer		params[];
{
	char 		arch_str[MEDIUM_STR];	/* saved architecture */
	int		choice;			/* saved choice */
	clnt_info *	client_p;		/* pointer to client info */
	clnt_disp *	disp_p;			/* pointer to client display */
	form *		form_p;			/* pointer to CLIENT form */
	char		hostname[MAXHOSTNAMELEN]; /* saved hostname */
	clnt_info *	old_p;			/* ptr to old client info */
	char		pathname[MAXPATHLEN];	/* path to client.<name> */
	int		ret_code;		/* return code */
	int		arch;			/* saved arch id */


	client_p = (clnt_info *) params[2];
	disp_p = (clnt_disp *) params[3];
	form_p = (form *) params[4];
	old_p = (clnt_info *) params[5];

	(void) strcpy(arch_str, client_p->arch_str);
	choice = client_p->choice;
	(void) strcpy(hostname, client_p->hostname);
	arch = client_p->arch;

	/*
	 *	Clear the client list and client info before we update it.
	 */
	off_client_disp(form_p);
	off_menu_file(find_menu_file((pointer) form_p, "client_list"));
	off_client_info(client_p, form_p);
	off_form_shared(form_p, find_form_yesno(form_p, "value_check"));

	(void) sprintf(pathname, "%s.%s", CLIENT_STR, client_p->hostname);

	ret_code = read_clnt_info(pathname, client_p);

	client_p->arch = arch;
	(void) strcpy(client_p->arch_str, arch_str);
	client_p->choice = choice;
	(void) strcpy(client_p->hostname, hostname);

	/*
	 *	Only a return code of 1 is okay here.  Must add an error
	 *	message for ret_code == 0.
	 */
	switch (ret_code) {
	case 1:
		break;

	case 0:
		menu_mesg("%s is not a client.", client_p->hostname);

		show_client_list(form_p, arch_str, disp_p);
		return(0);

	case -1:
		return(-1);
	} /* end switch() */

	if (client_p->created == ANS_YES)	{
		menu_mesg("%s\n%s",
  "You cannot edit an existing client. If changes are needed, you must ",
  "first remove it with 'rm_client' and then recreate it.");

		show_client_list(form_p, client_p->arch_str, disp_p);
		return(0);
	}

	ret_code = get_clnt_display(client_p, disp_p);
	if (ret_code != 1)
		return(ret_code);

	on_client_disp(form_p);
	on_client_info(client_p, form_p);
	on_form_shared(form_p, find_form_yesno(form_p, "value_check"));

	*old_p = *client_p;

	return(1);
} /* end edit_client() */




/*
 *	Name:		get_clnt_display()
 *
 *	Description:	Get disk space information about the client's root
 *		and swap partitions into the display buffers.
 */

int
get_clnt_display(client_p, disp_p)
	clnt_info *	client_p;
	clnt_disp *	disp_p;
{
	int		ret_code;		/* return code */


	if (client_p->choice == CLNT_CREATE) {
		/*
		 *	Only use the last root and swap specified, if you
		 *	are creating a new client
		 */
		if (*last_root_base != '\0')
			(void) strcpy(client_p->root_path, last_root_base);
		
		if (*last_swap_base != '\0')
			(void) strcpy(client_p->swap_path, last_swap_base);
	}

	ret_code = get_info(client_p->root_path,
			    disp_p->root_fs, disp_p->root_avail,
			    disp_p->rhog_fs, disp_p->rhog_avail);
	if (ret_code != 1)
		return(ret_code);

	ret_code = get_info(client_p->swap_path,
			    disp_p->swap_fs, disp_p->swap_avail,
			    disp_p->shog_fs, disp_p->shog_avail);
	if (ret_code != 1)
		return(ret_code);

	return(1);
} /* end get_clnt_display() */




/*
 *	Name:		get_info()
 *
 *	Description:	Get disk space information about a specific partition.
 */

static int
get_info(path, fs, avail, hog_fs, hog_avail)
	char		path[];
	char		fs[];
	char		avail[];
	char		hog_fs[];
	char		hog_avail[];
{
	disk_info	disk;			/* disk info buffer */
	char		disk_name[TINY_STR];	/* disk name */
	int		len;			/* scratch length */
	char		part;			/* which partition */
	char *		part_name;		/* ptr to partition name */
	char		pathname[MAXPATHLEN];	/* pathname buffer */
	int		ret_code;		/* return code */


	part_name = find_part(path);
	if (part_name == NULL)
		/*
		 *	Error message is done by find_part().
		 */
		return(-1);

	(void) sprintf(fs, "%s (%s)", path, part_name);

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

	(void) sprintf(avail, "%lu", disk.partitions[part - 'a'].avail_bytes);

	(void) sprintf(hog_fs, "%s%c", disk_name, disk.free_hog);
	(void) sprintf(hog_avail, "%lu",
		       disk.partitions[disk.free_hog - 'a'].avail_bytes);

	return(1);
} /* end get_info() */




/*
 *	Name:		init_client_info()
 *
 *	Description:	Initialize a client information structure.
 */

int
init_client_info(arch_str, arch_p, client_p)
	char *		arch_str;
	arch_info *	arch_p;
	clnt_info *	client_p;
{
	int		ret_code;		/* return code */


	ret_code = verify_arch(arch_str, arch_p);
	/*
	 *	Only a return code of 1 is okay here.  The error message is
	 *	handled by verify_arch()
	 */
	if (ret_code != 1)
		return(ret_code);

	(void) strcpy(client_p->arch_str, arch_str);

	return(1);
} /* end init_client_info() */




/*
 *	Name:		update_sizes()
 *
 *	Description:	Update the sizes for the root and swap partitions.
 */

static int
update_sizes(root_path, root_size, swap_path, swap_size)
	char *		root_path;
	long		root_size;
	char *		swap_path;
	long		swap_size;
{
	char *		part_p;			/* ptr to partition name */
	int		ret_code;		/* return code */


	/*
	 *	Adjust the root path
	 */
	part_p = find_part(root_path);
	if (part_p == NULL)
		return(-1);

	ret_code = update_bytes(part_p, root_size);
	if (ret_code != 1)
		return(ret_code);

	/*
	 *	Adjust the user path
	 */
	part_p = find_part(swap_path);
	if (part_p == NULL)
		return(-1);

	ret_code = update_bytes(part_p, SWAP_FUDGE_SIZE(swap_size));
	if (ret_code != 1) {
		part_p = find_part(root_path);
		if (part_p == NULL)
			return(-1);
		if (update_bytes(part_p, -root_size) != 1) {
			menu_log("%s: %s cannot restore byte count.", progname,
				 root_path);
			return(-1);
		}

		return(ret_code);
	}

	return(1);
} /* end update_sizes() */




/*
 *	Name:		verify_arch()
 *
 *	Description:	Verify that 'arch' is a supported architecture.
 *
 *	Side effect:	Copy the appropriate struct arch_info to *arch_p
 */

int
verify_arch(arch, arch_p)
	char *		arch;
	arch_info *	arch_p;
{
	arch_info *	ap;			/* scratch pointer */
	arch_info *	arch_list;		/* architecture info */
	int		ret_code;		/* return code */

	ret_code = read_arch_info(ARCH_INFO, &arch_list);
	/*
	 *	Only a return code of 1 is okay here.  Since this is a
	 *	verification function, do not add a message for non-fatal
	 *	errors (ret_code == 0).
	 */
	if (ret_code != 1)
		return(ret_code);

	ap = find_arch(arch, arch_list);
	if (ap == NULL) {
		bzero((char *) arch_p, sizeof(*arch_p));
		return(0);
	}

	*arch_p = *ap;
	free_arch_info(arch_list);
	return(1);
} /* end verify_arch() */
