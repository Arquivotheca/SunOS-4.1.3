#ifndef lint
#ifdef SunB1
#ident			"@(#)main_menu.c 1.1 92/07/30 SMI; SunOS MLS";
#else
#ident			"@(#)main_menu.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		main_menu.c
 *
 *	Description:	This file contains all the routines for dealing
 *		with the main driver menu.
 */

#include <stdio.h>
#include <signal.h>
#include "main_impl.h"
#include "menu_impl.h"


/*
 *	External functions:
 */
extern	char *		sprintf();


static	int		assign_client();
static	int		assign_disk();
static	int		assign_host();
static	int		assign_sec();
static	int		assign_software();
static	int		get_sec_info();
static	int		start_install();
static	int		exit_menu();




/*
 *	Name:		create_main_menu()
 *
 *	Description:	Create the MAIN menu.  Uses information in 'sys_p'
 *		to create the menu.
 */

menu *
create_main_menu(sys_p)
	sys_info *	sys_p;
{
	menu_item *	item_p;			/* pointer to menu item */
	menu *		menu_p;			/* pointer to MAIN menu */
	menu_coord	x, y;			/* scratch coordinates */

	static	pointer		params[2];	/* parameter pointers */


        init_menu();

	menu_p = create_menu(PFI_NULL, ACTIVE, "MAIN MENU");

	params[0] = (pointer) sys_p;
	params[1] = (pointer) menu_p;

	/*
	 *	Header strings start on line 'y':
	 */
	y = 2;
	(void) add_menu_string((pointer) menu_p, ACTIVE, y, 19, ATTR_NORMAL,
			       "Sun Microsystems System Installation Tool");
	y += 2;
	(void) add_menu_string((pointer) menu_p, ACTIVE, y, 21, ATTR_NORMAL,
			       "( + means the data file(s) exist(s) )");

	y += 3;
	x = 24;
	item_p = add_menu_item(menu_p, PFI_NULL, ACTIVE, "assign_host",
			       y, x, PFI_NULL, PTR_NULL,
			       assign_host, (pointer) params,
			       is_sys_ok, (pointer) sys_p);
	(void) add_menu_string((pointer) item_p, ACTIVE, y, x + 2, ATTR_NORMAL,
			       "assign host information");

	y += 2;
	item_p = add_menu_item(menu_p, PFI_NULL, ACTIVE, "assign_disk",
			       y, x, PFI_NULL, PTR_NULL,
			       assign_disk, (pointer) params,
			       is_disk_ok, PTR_NULL);
	(void) add_menu_string((pointer) item_p, ACTIVE, y, x + 2, ATTR_NORMAL,
			       "assign disk information");

	y += 2;
	item_p = add_menu_item(menu_p, PFI_NULL, ACTIVE, "assign_software",
			       y, x, PFI_NULL, PTR_NULL,
			       assign_software, (pointer) params,
			       is_soft_ok, PTR_NULL);
	(void) add_menu_string((pointer) item_p, ACTIVE, y, x + 2, ATTR_NORMAL,
			       "assign software information");

	y += 2;
	item_p = add_menu_item(menu_p, PFI_NULL, ACTIVE, "assign_client",
			       y, x, PFI_NULL, PTR_NULL,
			       assign_client, (pointer) params,
			       is_client_ok, (pointer) sys_p);
	(void) add_menu_string((pointer) item_p, ACTIVE, y, x + 2, ATTR_NORMAL,
			       "assign client information");

	y += 2;
	item_p = add_menu_item(menu_p, PFI_NULL, ACTIVE, "assign_sec",
			       y, x, PFI_NULL, PTR_NULL,
			       assign_sec, (pointer) params,
			       is_sec_ok, (pointer) sys_p);
	(void) add_menu_string((pointer) item_p, ACTIVE, y, x + 2, ATTR_NORMAL,
			       "assign security information");

	y += 2;
	item_p = add_menu_item(menu_p, PFI_NULL, ACTIVE, "start_install",
			       y, x, PFI_NULL, PTR_NULL,
			       start_install, PTR_NULL,
			       PFI_NULL, PTR_NULL);
	(void) add_menu_string((pointer) item_p, ACTIVE, y, x + 2, ATTR_NORMAL,
			       "start the installation");

	y += 2;
	item_p = add_menu_item(menu_p, PFI_NULL, ACTIVE, PTR_NULL,
			       y, x, PFI_NULL, PTR_NULL,
			       exit_menu, PTR_NULL,
			       PFI_NULL, PTR_NULL);
	(void) add_menu_string((pointer) item_p, ACTIVE, y, x + 2, ATTR_NORMAL,
			       "exit suninstall");

	return(menu_p);
} /* end create_main_menu() */



/*
 *	Name:		assign_client()
 *
 *	Description:	Invoke get_client_info to gather client information.
 */

static int
assign_client(params)
	pointer		params[];
{
	char *		argv[2];		/* arguments for execute() */
	menu *		menu_p;			/* pointer to MAIN menu */
	int		ret_code;		/* return code */
	sys_info *	sys_p;			/* pointer to system info */


	sys_p = (sys_info *) params[0];
	menu_p = (menu *) params[1];

	end_menu();

	argv[0] = "get_client_info";
	argv[1] = NULL;
	ret_code = main_execute(argv);

	init_menu();
	if (menu_p)
		display_menu(menu_p);

	if (ret_code != 1 || is_client_ok(sys_p) != 1) {
		set_items(menu_p, sys_p);

		menu_log("Failed to assign client information.");
		return(0);
	}

	set_items(menu_p, sys_p);

	return(1);
} /* end assign_client() */




/*
 *	Name:		assign_disk()
 *
 *	Description:	Invoke get_disk_info to gather disk information.
 */

static int
assign_disk(params)
	pointer		params[];
{
	char *		argv[2];		/* arguments for execute() */
	menu *		menu_p;			/* pointer to MAIN menu */
	int		ret_code;		/* return code */
	sys_info *	sys_p;			/* pointer to system info */


	sys_p = (sys_info *) params[0];
	menu_p = (menu *) params[1];

	end_menu();

	argv[0] = "get_disk_info";
	argv[1] = NULL;
	ret_code = main_execute(argv);

	init_menu();
	if (menu_p)
		display_menu(menu_p);

	if (ret_code != 1 || is_disk_ok() != 1) {
		set_items(menu_p, sys_p);

		menu_log("Failed to assign disk information.");
		return(0);
	}

	set_items(menu_p, sys_p);

	return(1);
} /* end assign_disk() */




/*
 *	Name:		assign_host()
 *
 *	Description:	Invoke get_host_info() to gather system information.
 */

static int
assign_host(params)
	pointer		params[];
{
	char *		argv[2];		/* arguments for execute() */
	menu *		menu_p;			/* pointer to MAIN menu */
	int		ret_code;		/* return code */
	sys_info *	sys_p;			/* pointer to system info */


	sys_p = (sys_info *) params[0];
	menu_p = (menu *) params[1];

	end_menu();

	argv[0] = "get_host_info";
	argv[1] = NULL;
	ret_code = main_execute(argv);

	init_menu();
	if (menu_p)
		display_menu(menu_p);

	if (ret_code != 1 || is_sys_ok(sys_p) != 1) {
		set_items(menu_p, sys_p);

		menu_log("Failed to assign host information.");
		return(0);
	}

	set_items(menu_p, sys_p);

	return(1);
} /* end assign_host() */




/*
 *	Name:		assign_sec()
 *
 *	Description:	Call get_sec_info() to gather security information
 */

static int
assign_sec(params)
	pointer		params[];
{
	menu *		menu_p;			/* pointer to MAIN menu */
	int		ret_code;		/* return code */
	sys_info *	sys_p;			/* pointer to system info */


	sys_p = (sys_info *) params[0];
	menu_p = (menu *) params[1];

	end_menu();

	ret_code = get_sec_info(sys_p);

	init_menu();
	if (menu_p)
		display_menu(menu_p);

	set_items(menu_p, sys_p);

	if (ret_code != 1 || is_sec_ok(sys_p) != 1) {
		menu_log("Failed to assign security information.");
		return(0);
	}

	return(1);
} /* end assign_sec() */




/*
 *	Name:		assign_software()
 *
 *	Description:	Invoke get_software_info to gather software
 *		information.
 */

static int
assign_software(params)
	pointer		params[];
{
	char *		argv[2];		/* arguments for execute() */
	menu *		menu_p;			/* pointer to MAIN menu */
	int		ret_code;		/* return code */
	sys_info *	sys_p;			/* pointer to system info */


	sys_p = (sys_info *) params[0];
	menu_p = (menu *) params[1];

	end_menu();

	argv[0] = "get_software_info";
	argv[1] = NULL;
	ret_code = main_execute(argv);

	init_menu();
	if (menu_p)
		display_menu(menu_p);

	if (ret_code != 1 || is_soft_ok() != 1) {
		set_items(menu_p, sys_p);

		menu_log("Failed to assign software information.");
		return(0);
	}

	set_items(menu_p, sys_p);

	return(1);
} /* end assign_software() */




/*
 *	Name:		get_sec_info()
 *
 *	Description:	This is the dispatch point for gathering security
 *		information.  This routine invokes get_sec_info to gather
 *		security information about the system and any clients.
 */

static	int
get_sec_info(sys_p)
	sys_info *	sys_p;
{
	arch_info	*arch_list, *ap;	/* architecture info */
	char *		argv[3];		/* arguments for execute() */
	char		buf[BUFSIZ];		/* input buffer */
	FILE *		fp;			/* ptr to client_list.<arch> */
	char		pathname[MAXPATHLEN];	/* general pathname buffer */
	int		ret_code;		/* return code */


	argv[0] = "get_sec_info";
	argv[1] = NULL;
	ret_code = main_execute(argv);
	if (ret_code != 1)
		return(ret_code);

	if (sys_p->sys_type != SYS_SERVER)
		return(1);

	/*
	 *	If there are no architectures, then we cannot get sec_info
	 */
	switch(read_arch_info(ARCH_INFO, &arch_list)) {
	case 1:
		break;

	case 0:
		(void) fprintf(stderr, "%s: %s: cannot read file.\n", progname,
			       ARCH_INFO);
		return(0);

	default:
		/*
		 *	Error message is done by read_arch_info()
		 */
		return(-1);
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

			argv[1] = buf;
			argv[2] = NULL;
			ret_code = main_execute(argv);
			if (ret_code != 1) {
				(void) fclose(fp);
				free_arch_info(arch_list);
				return(ret_code);
			}
		}

		(void) fclose(fp);
	}

	free_arch_info(arch_list);

	return(1);
} /* end get_sec_info() */




/*
 *	Name:		set_items()
 *
 *	Description:	Enable and disable menu items as information is
 *		validated.  The general philosophy is that a menu item
 *		is only turned on if its dependents are valid.  This
 *		allows the assign* routines to skip the validity checks.
 */

void
set_items(menu_p, sys_p)
	menu *		menu_p;
	sys_info *	sys_p;
{
	int		client_ok;		/* are client_info files ok? */
	menu_item *	client_p;		/* ptr to client menu item */
	menu_item *	disk_p;			/* ptr to disk menu item */
	menu_item *	host_p;			/* ptr to sys_info menu item */
	int		sec_ok;			/* is sec_info ok? */
	menu_item *	sec_p;			/* ptr to sec_info menu item */
	menu_item *	soft_p;			/* ptr to software menu item */
	menu_item *	start_p;		/* ptr to start install item */


	host_p = find_menu_item(menu_p, "assign_host");
	disk_p = find_menu_item(menu_p, "assign_disk");
	soft_p = find_menu_item(menu_p, "assign_software");
	client_p = find_menu_item(menu_p, "assign_client");
	sec_p = find_menu_item(menu_p, "assign_sec");
	start_p = find_menu_item(menu_p, "start_install");

	on_menu_item(host_p);
	off_menu_item(disk_p);
	off_menu_item(soft_p);
	off_menu_item(client_p);
	off_menu_item(sec_p);
	off_menu_item(start_p);

	if (!is_sys_ok(sys_p))
		return;

	/*
	 *	sys_info is okay so turn on disk item
	 */
	on_menu_item(disk_p);

	if (!is_disk_ok())
		return;

	/*
	 *	disk info is okay so turn on software item
	 */
	on_menu_item(soft_p);

	if (!is_soft_ok())
		return;

	/*
	 *	If the system is not a server, then set client_ok to 1 so
	 *	we can turn on the installation item
	 */
	if (sys_p->sys_type != SYS_SERVER)
		client_ok = 1;
	else {
		/*
		 *	soft_info is okay so turn on client item
		 */
		on_menu_item(client_p);
		client_ok = is_client_ok(sys_p);
	}

	/*
	 *	If Security is not loaded, then set sec_ok to 1 so
	 *	we can turn on the installation item
	 */
	if (is_sec_loaded(sys_p->arch_str) == 0)
		sec_ok = 1;
	else {
		/*
		 *	client info is okay so turn on security item
		 */
		if (client_ok) {
			on_menu_item(sec_p);
			sec_ok = is_sec_ok(sys_p);
		}
		else
			sec_ok = 0;
	}

	if (client_ok && sec_ok && is_miniroot())
		on_menu_item(start_p);
} /* end set_items() */




/*
 *	Name:		start_install()
 *
 *	Description:	Invoke installation to install the system.
 */

static	int
start_install()
{
	char *		argv[2];		/* arguments for execute() */


	end_menu();

	argv[0] = "installation";
	argv[1] = NULL;

	if (execute(argv) != 1) {
		menu_log("Installation aborted.");
		menu_abort(1);
	}

	exit(0);
} /* end start_install() */

/*
 *	Name:		exit_menu()
 *
 *	Description:	Return the exit from menu token
 */
static int
exit_menu()
{
	return(MENU_EXIT_MENU);
}


/*
 * 	Name: main_execute()
 *
 *	Description: this is the only place that we really want to catch
 *			signals
 */
int
main_execute(argv)
	char	**argv;
{
	int	ret_code;

	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGQUIT, SIG_IGN);

	ret_code = execute(argv);

	(void) signal(SIGINT, SIG_DFL);
	(void) signal(SIGQUIT, SIG_DFL);

	return(ret_code);
}
	

