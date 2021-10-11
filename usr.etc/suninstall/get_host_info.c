#ifndef lint
#ifdef SunB1
static  char    sccsid[] = 	"@(#)get_host_info.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static  char    sccsid[] = 	"@(#)get_host_info.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1990 Sun Microsystems, Inc.
 */

/*
 *	Name:		get_host_info()
 *
 *	Description:	Use a menu based interface to get information from
 *		the user about system.  This program is typically called
 *		from suninstall.
 */

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "install.h"
#include "host_impl.h"

/*
 *	External functions:
 */
extern	char *		sprintf();


/*
 *	Global variables:
 */
#ifdef SunB1
int		old_mac_state;			/* old mac-exempt state */
#endif /* SunB1 */
char *		progname;			/* name for error messages */


static	void		update_hosts();
#ifdef SunB1
static	void		update_hosts_label();
static	void		update_net_label();
#endif /* SunB1 */


main(argc, argv)
int	argc;
char	**argv;
{
	char		appl_str[MEDIUM_STR];	/* application arch string */
	form *		form_p;			/* pointer to HOST FORM */
	char *		impl_str;		/* ptr to impl architecture */
	sys_info	sys;			/* system information */


#ifdef lint
	argc = argc;
#endif lint

	(void) umask(UMASK);			/* set file creation mask */

	progname = basename(argv[0]);		/* get name for error mesgs */

	set_menu_log(LOGFILE);

	/*
	 *	Read sys_info from user defined info or default file
	 */
	if (read_sys_info(SYS_INFO, &sys) != 1 &&
	    read_sys_info(DEFAULT_SYS_INFO, &sys) != 1) {
		menu_log("%s: Error in %s.", progname, DEFAULT_SYS_INFO);
		menu_abort(1);
	}

	/*
	 *	If the system's software architecture is unknown, then
	 *	handcraft a partial APR id.  The APR id created here is
	 *	only a partial so it can be used in strncmp() calls in
	 *	get_software_info.  If os_aprid() is used, then there
	 *	would be extra 'dots' from the empty fields.
	 */
	if (strlen(sys.arch_str) == 0) {
		impl_str = get_arch();
		(void) appl_arch(impl_str, appl_str);
		(void) sprintf(sys.arch_str, "%s.%s", appl_str, impl_str);
	}

	if (*sys.ether_name0 == 0)		/* get ethernet interfaces */
        	get_ethertypes(&sys);

	if (*sys.ether_name0 != 0)
		sys.ether_type = ETHER_0;

	get_terminal(sys.termtype);		/* get terminal type */

        form_p = create_host_form(&sys);	/* create HOST FORM */

	/*
	 *	Ignore the 4.0 "repaint"
	 */
	(void) signal(SIGQUIT, SIG_IGN);

	if (use_form(form_p) != 1)		/* use HOST FORM */
		menu_abort(1);

	(void) signal(SIGQUIT, SIG_DFL);
	
	menu_flash_on("Updating databases");

	update_hosts(&sys);			/* update HOSTS */

#ifdef SunB1
	update_hosts_label(&sys);		/* update HOSTS_LABEL */
	update_net_label(&sys);			/* update NET_LABEL */
#endif /* SunB1 */

	menu_flash_off(REDISPLAY);

						/* save the new sys_info */
	if (save_sys_info(SYS_INFO, &sys) != 1)
		menu_abort(1);

        end_menu();				/* done with menu system */

	exit(0);
	/*NOTREACHED*/
} /* end main() */


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


	if (sys_p->ether_type == ETHER_NONE) {
		(void) sprintf(value, "%s localhost loghost",
			       sys_p->hostname0);

		add_key_entry("127.0.0.1", value, HOSTS, KEY_OR_VALUE);
	} else {
		int i;
		for (i = 0; i < MAX_ETHER_INTERFACES; i++)
			if (*sys_p->ipx(i)) {
				(void) sprintf(value, "%s loghost",
					       sys_p->hostnamex(i));
				add_key_entry(sys_p->ipx(i), value,
					      HOSTS, KEY_OR_VALUE);
		}

		add_key_entry("127.0.0.1", "localhost", HOSTS, KEY_OR_VALUE);
	}

	/*
	 *	If this is a DATALESS system, then add the server to
	 *	the HOSTS file.
	 */
	if (sys_p->sys_type == SYS_DATALESS) {
		add_key_entry(sys_p->server_ip, sys_p->server, HOSTS,
			      KEY_OR_VALUE);
	}
} /* end update_hosts() */




#ifdef SunB1
/*
 *	Name:		update_hosts_label()
 *
 *	Description:	Update the HOSTS_LABEL file with the IP and hostname
 *		info from sys_info.
 */

static	void
update_hosts_label(sys_p)
	sys_info *	sys_p;
{
	char		value[MEDIUM_STR];	/* the value to add */


	if (sys_p->ether_type == ETHER_NONE) {
		add_key_entry("127.0.0.1", "localhost Labeled", HOSTS_LABEL, KEY_OR_VALUE);
	}
	else {
		(void) sprintf(value, "%s Labeled", sys_p->hostname0);

		add_key_entry(sys_p->ip0, value, HOSTS_LABEL, KEY_OR_VALUE);

		if (strlen(sys_p->ip1)) {
			(void) sprintf(value, "%s Labeled", sys_p->hostname1);
			add_key_entry(sys_p->ip1, value, HOSTS_LABEL,
				      KEY_OR_VALUE);
		}

		add_key_entry("127.0.0.1", "localhost Labeled",
			      HOSTS_LABEL, KEY_OR_VALUE);
	}

	/*
	 *	If this is a DATALESS system, then add the server to
	 *	the HOSTS_LABEL file.
	 */
	if (sys_p->sys_type == SYS_DATALESS) {
		(void) sprintf(value, "%s Labeled", sys_p->server);

		add_key_entry(sys_p->server_ip, value, HOSTS_LABEL,
			      KEY_OR_VALUE);
	}
} /* end update_hosts_label() */




/*
 *	Name:		update_net_label()
 *
 *	Description:	Update the NET_LABEL file with the IP and label
 *		info from sys_info.
 */

static	void
update_net_label(sys_p)
	sys_info *	sys_p;
{
	/*
	 *	May have to put in an entry for the loopback here.
	 */
	if (sys_p->ether_type == ETHER_NONE)
		return;

	add_net_label(sys_p->ip0, sys_p->ip0_minlab, sys_p->ip0_maxlab, "");

	if (strlen(sys_p->ip1))
		add_net_label(sys_p->ip1, sys_p->ip1_minlab,
			      sys_p->ip1_maxlab, "");

	/*
	 *	If this is a DATALESS system, then add the server to
	 *	the NET_LABEL file.
	 */
	if (sys_p->sys_type == SYS_DATALESS) {
		add_net_label(sys_p->server_ip, LAB_SYS_LOW, LAB_SYS_HIGH, "");
	}
} /* end update_net_label() */
#endif /* SunB1 */
