#ifndef lint
#ifdef SunB1
static  char    sccsid[] = 	"@(#)arch_form.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static  char    sccsid[] = 	"@(#)arch_form.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		arch_form.c
 *
 *	Description:	This file contains all the routines for creating and
 *		using a menu for selecting an architecture from a CDROM
 *		device. Through the use of the file "AVAIL_ARCHES", we can
 *		tell what arches are available on the CDROM.
 *
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include "soft_impl.h"
#include "media.h"


/*
 *	External functions:
 */
extern	char *		sprintf();


/*
 *	Local functions:
 */
static	int		make_arch_items();
static	int		select_arch();
static	int		number_of_releases();
static	int		exit_arch_menu();
static	void		clear_menu_status();
static  menu *		create_arch_menu();

char *		progname;

/*
 *	This 2-D Buffer is being used to store all the menu strings used
 *	in the menu_items that display the architectures supported.  It will
 *	be a long while before we support more than 64 architectures on 1 CD.
 */
static	char	menu_strings[64][MEDIUM_STR];


int
main(argc, argv)
	int		argc;
	char ** 	argv;
{
	soft_info	soft;			/* software information */
	sys_info	sys;			/* system information */
	menu *		menu_p;			/* pointer to MAIN menu */


#ifdef lint
	argc = argc;
#endif
	
	(void) umask(UMASK);			/* set file creation mask */

	progname = basename(argv[0]);		/* get name for error mesgs */

	/*
	 *	Software info structure must be initialized before the
	 *	first call to read_software_info() since it allocates
	 *	and frees its own run-time memory.
	 */
	bzero((char *) &soft, sizeof(soft));

	/*
	 *	Since later on things depend on this being nullified, let's
 	 *	make sure here, inspite of what the standard compiler
	 *	should do.
	 */
	bzero((char *)menu_strings, sizeof(menu_strings));

	
	set_menu_log(LOGFILE);
	
	/*
	 *	get system info, as much as is there.
	 */
	if (read_sys_info(SYS_INFO, &sys) != 1) {
		menu_log("%s: Error in %s.", progname, SYS_INFO);
		menu_abort(1);
	}
	
	get_terminal(sys.termtype);		/* get terminal type */

	if ((menu_p = create_arch_menu(&soft, &sys)) == (menu *)NULL)
		menu_abort(1);

	if (use_menu(menu_p) != 1)		/* use ARCHITECTURE FORM */
		menu_abort(1);

	end_menu();				/* done with menu system */
	
	exit(0);
	/*NOTREACHED*/
} /* end main() */


/*
 *	Name:		(static menu *) create_arch_menu()
 *
 *	Description:	Create the ARCHITECTURE menu.
 *
 *	Return Value:   (menu *)     : pointer to menu created.
 *		 	(menu *)NULL : if there was an error creating menu.
 *
 */

static menu *
create_arch_menu(soft_p, sys_p)
	soft_info *	soft_p;
	sys_info *	sys_p;
{
	menu_coord	x, y;			/* scratch coordinates */
	menu *		menu_p;			/* pointer to menu */

	static	pointer		params[4];	/* standard pointers */

	init_menu();

	menu_p = create_menu(PFI_NULL, ACTIVE, "ARCHITECTURE FORM");

	/*
	 *	Setup standard pointers:
	 */
	params[0] = (pointer) menu_p;
	params[1] = (pointer) soft_p;
	params[2] = (pointer) sys_p;

	y = 3;
	x = 4;

	/*
	 *	Architecture information section.  This section starts on
	 *	line 'y'.  The objects in this section line up on 'x'.
	 */

	(void) add_menu_string((pointer) menu_p, ACTIVE, y, x, ATTR_NORMAL,
			       "Please Select An Architecture :");

	if (make_arch_items(params) != 1)
		return((menu *)NULL);

	return(menu_p);

} /* end create_soft_form() */



/*
 *	Name:		make_arche_items()
 *
 *	Description:	Make architecture menu_items.
 *
 *	Return Value:  	 1 : if all is aok
 *			-1 : if all is not aok
 */
static int
make_arch_items(params)
	pointer		params[];
{
	char		buf[BUFSIZ];		/* buffer for input */
	FILE *		fp;			/* pointer to ARCH_INFO */
	int		i;			/* dummy button value */
	menu_coord	x, y;			/* scratch menu coordinates */
	menu_coord	first_x, first_y;	/* scratch menu coordinates */
	menu_coord	skip_y;			/* skip 1 or 2 lines?       */
	char		irid[BUFSIZ];		/* irid buffer */
	menu *		menu_p;			/* pointer to arch menu */
	menu_item *	item_p;			/* pointer to an item */
	
	menu_p = (menu *) params[0];

	/*
	 *	Turn off the architecture radio panel so it will disappear
	 *	if ARCH_INFO is removed.
	 */

	fp = fopen(AVAIL_ARCHES, "r");
	if (fp == NULL)
		return(-1);

	first_y = y = 5;
	first_x = x = 6;

	/*
	 *	Some useful macros to help in screen placement.
	 */

#define	Y_MENU_LIMIT	(menu_lines() - 7)
#define	X_SEC_COLUMN	((menu_cols() / 2) - first_x)

	/*
	 *	Decide if we should skip by 2 lines or 1
	 */
	skip_y  = 2;
	if (number_of_arches(fp) > (Y_MENU_LIMIT / skip_y))
		skip_y  = 1;
		

	/*
	 *	Read the APRs from ARCH_INFO, copy the APRs into saved_aprs
	 *	and create a button for each APR.
	 */

	for (i = 0; fgets(buf, sizeof(buf), fp); i++) {
		delete_blanks(buf);
		
		(void) sprintf(menu_strings[i], "[%s]",
			       aprid_to_irid(buf, irid));

		item_p = add_menu_item(menu_p, PFI_NULL, ACTIVE,
				       menu_strings[i],
				       y, x,
				       PFI_NULL, PTR_NULL,
				       select_arch, (pointer) params,
				       PFI_NULL, PTR_NULL);

		(void) add_menu_string((pointer) menu_p, ACTIVE, y, x + 2,
				       ATTR_NORMAL, menu_strings[i]);

		y += skip_y;

		if (y > Y_MENU_LIMIT) {
			x += X_SEC_COLUMN;
			y = first_y;
		}

	}
	
	item_p = add_menu_item(menu_p, PFI_NULL, ACTIVE, "end_menu",
			       menu_lines() - 4, first_x, PFI_NULL, PTR_NULL,
			       exit_arch_menu, PTR_NULL,
			       PFI_NULL, PTR_NULL);
	(void) add_menu_string((pointer) item_p, ACTIVE,
			       menu_lines() - 4, first_x + 2, ATTR_NORMAL,
			       "[exit architecture menu]");

	return(1);
} /* end make_arch_items() */


/*
 *	Name:		select_arch()
 *
 *	Description:	Select a software architecture to operate on.  This
 *			is the proc function for the architecture menu
 *			items.
 *
 *	Return Value:	0 : for an error
 *			1 : for for no error
 *
 */
static int
select_arch(params)
	pointer		params[];
{
	soft_info *	soft_p;			/* software info pointer */
	FILE *		fp;			/* tmp file * */
	char		buf[MEDIUM_STR];	/* aprid buffer */
	menu *		menu_p;			/* menu_pointer */
	
	menu_p = (menu *) params[0];
	soft_p = (soft_info *) params[1];

	/*
	 *	If necessary, clear any past '+''s and then create the new
	 *	one
	 */
	if (menu_p->m_ptr->mi_chkvalue != 1) {
		clear_menu_status(menu_p);
		menu_p->m_ptr->mi_chkvalue = 1;
		display_item_status(menu_p->m_ptr);
	}


	/*
	 *      Extract the architecture from the string on the screen and
	 *	strip off the 2 end brackets. '[' and ']'
	 */
	(void) strcpy(soft_p->arch_str, (menu_p->m_ptr->mi_name) + 1);
	soft_p->arch_str[strlen(soft_p->arch_str) - 1] = '\0';

	/*
	 *	convert to a full architecture aprid
	 */
	(void) irid_to_aprid(soft_p->arch_str, buf); 


	/*
	 *	Finally,  create the LOAD_ARCH file, so we know what to do
	 */
	fp = fopen(LOAD_ARCH, "w");
	if (fp == (FILE *)NULL) {
		menu_mesg("%s: could not open file %s", progname, LOAD_ARCH);
		return(0);
	}
	
	/*
	 *	Put out full aprid
	 */
	(void) fputs(buf, fp);
	(void) fclose(fp);
	
	return(1);

} /* end select_arch() */


/*
 *	Name:		(int) number_of_arches()
 *
 *	Description:	return the number of arches in the AVAIL_ARCHES file.
 *
 */
static int
number_of_arches(file_p)
	FILE *		file_p;	/* open file pointer of AVAIL_ARCHES file */
{
	char	buf[BUFSIZ];
	int 	i;
	
	for (i = 0; fgets(buf, sizeof(buf), file_p); i++);
	
	/*
	 *	Put file back to offset 0, like it started out
	 */
	rewind(file_p);

	return(i);
	
} /* end number_of_arches() */



/*
 *	Name:		clear_menu_status()
 *
 *	Description:	clear all menu item status's (clear the '+');
 *
 */
static void
clear_menu_status(menu_p)
	menu *		menu_p;
{
	menu_item *	item_p;
	int		i;

	for (i = 0;  *(menu_strings[i]) != '\0';i++) {
		item_p = find_menu_item(menu_p, menu_strings[i]);
		item_p->mi_chkvalue = 0;
		display_item_status(item_p);
	}
} /* end clear_menu_status() */



/*
 *	Name:		exit_arch_menu()
 *
 *	Description:	exit the menu and all is well and good
 *
 */
static int
exit_arch_menu()
{
	end_menu();				/* done with menu system */
	exit(0);
	/*NOTREACHED*/
} /* end exit_arch_menu() */


