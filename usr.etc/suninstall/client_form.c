#ifndef lint
#ifdef SunB1
#ident			"@(#)client_form.c 1.1 92/07/30 SMI; SunOS MLS";
#else
#ident			"@(#)client_form.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint


/*
 *	Copyright (c) 1990 Sun Microsystems, Inc.
 */

/*
 *	Name:		client_form.c
 *
 *	Description:	This file contains all the routines for dealing
 *		with a client form.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "client_impl.h"




/*
 *	External functions:
 */
extern	char *		sprintf();
extern	char *		strdup();
extern  int		check_client_terminal();
extern  int		chk_swap();
int			clear_arch_buttons();



/*
 *	Local functions:
 */
static	int		ck_choice();
static	int		no_yp();

#ifdef SunB1
static	int		set_ip_maxlab();
static	int		set_ip_minlab();
#endif /* end SunB1 */
static	int		use_arch();
static	int		use_yp();
static  int		more_archs();

static  arch_info *	archlist;	/* root of arch linked list */


/*
 *	Name:		create_client_form()
 *
 *	Description:	Create a CLIENT form.  Uses information from 'sys_p',
 *		'arch_p', and 'client_p' to make the form.  'disp_p' points
 *		to space that is used by the menu system for display, and
 *		'old_client_p' points to a buffer for old client data just
 *		in case the user wants to toss his/her changes.
 */

form *
create_client_form(sys_p, arch_p, client_p, disp_p, old_client_p)
	sys_info *	sys_p;
	arch_info *	arch_p;
	clnt_info *	client_p;
	clnt_disp *	disp_p;
	clnt_info *	old_client_p;
{
	form_button *	button_p;		/* ptr to radio button */
	form_field *	field_p;		/* ptr to form field */
	menu_file *	file_p;			/* ptr to menu file */
	form *		form_p;			/* ptr to CLIENT form */
	int		i;			/* index variable */
	form_radio *	radio_p;		/* ptr to radio panel */
	char		buf[MEDIUM_STR];	/* scratch buffer */
	char		buff[MEDIUM_STR];	/* scratch buffer */
	menu_coord	x, y;			/* scratch coordinates */
	menu_coord	x_saved;		/* saved x-coordinate */

	static	pointer		params[6];	/* parameter pointers */
	arch_info *	ap;			/* scratch poiter */
	int		dummy;			/* dummy variable */
	int		len;			/* length of button string */

        init_menu();
	/*
	 * The screen needs to be at least 24 by 80.  If we are in the
	 * miniroot we know that it is so we don't check
	 */
	if (!is_miniroot())
		chk_screen_size();

	form_p = create_form(PFI_NULL, ACTIVE, "CLIENT FORM");

	params[0] = (pointer) sys_p;
	params[1] = (pointer) arch_p;
	params[2] = (pointer) client_p;
#ifndef lint
	params[3] = (pointer) disp_p;
#endif lint
	params[4] = (pointer) form_p;
	params[5] = (pointer) old_client_p;

	(void) read_arch_info(ARCH_INFO, &archlist);

        /*
         *      Architecture type.  This section starts on line 'y'.
    	 *	The objects in this section line up on line 'x'.
         */
	y = 2;
	x_saved = x = 21;
        radio_p = add_form_radio(form_p, PFI_NULL,
                                 ACTIVE, "arch", &client_p->arch,
				 PFI_NULL, PTR_NULL,
				 PFI_NULL, PTR_NULL);
        (void) add_menu_string((pointer) radio_p, ACTIVE, y, 1,
                               ATTR_NORMAL, "Architecture Type :");
	/*
	 *	Start adding architectures:
	 */

	dummy = ACTIVE;

	for (ap = archlist, i = 1; ap ; ap = ap->next, i++) {
		(void) sprintf(buf, "[%s]", 
				aprid_to_irid(ap->arch_str,buff));
		len = strlen(buf) ;
		if (x + len >= menu_cols() - 9) {
			button_p = add_form_button(radio_p, PFI_NULL,
						   dummy, "more",
						   y, menu_cols() - 8,
						   -1, more_archs,
						   (pointer) radio_p);
			(void) add_menu_string((pointer) button_p, dummy,
					       y, menu_cols() - 7,
					       ATTR_NORMAL, "[more]");
			dummy = NOT_ACTIVE;
			x = x_saved;
		}
		button_p = add_form_button(radio_p, PFI_NULL,
					   dummy, CP_NULL, y, x, i,
                                 	   use_arch, (pointer) params);
		(void) add_menu_string((pointer) button_p, dummy, y, x + 1,
				       ATTR_NORMAL, strdup(buf));
		x += len + 2;
	}

	if (dummy == NOT_ACTIVE) {
		/*
		 *	Make sure that there is a "more" button at the
		 *	end of the list of disks so we can get back
		 *	to the beginning.
		 */
		button_p = add_form_button(radio_p, PFI_NULL,
						   dummy, "more", y,
						   menu_cols() - 8,
						   -1, more_archs,
						   (pointer) radio_p);
		(void) add_menu_string((pointer) button_p,
					       dummy,
					       y, menu_cols() - 7,
					       ATTR_NORMAL, "[more]");
	}

	x = x_saved;				/* restore column line up */

	/*
	 *	Client's name.  This section starts on line 'y'.
	 *	The objects in this section line up on line 'x'.
	 */
	y++;
	field_p = add_form_field(form_p, PFI_NULL,
				 ACTIVE, "name", y, x, 0,
				 client_p->hostname,
				 sizeof(client_p->hostname),
				 PFI_NULL,
				 PFI_NULL, PTR_NULL,
				 PFI_NULL, PTR_NULL);
	(void) add_menu_string((pointer) field_p, ACTIVE, y, 1,
			       ATTR_NORMAL, "Client name       :");


	/*
	 *	Client choice section.  This section starts on line 'y'.
	 *	The objects in this section line up on line 'x'.
	 */
	y++;
        radio_p = add_form_radio(form_p, PFI_NULL,
                                 ACTIVE, "choice", &client_p->choice,
                                 ck_choice, (pointer) params,
                                 PFI_NULL, PTR_NULL);
        (void) add_menu_string((pointer) radio_p, ACTIVE, y, 1,
                               ATTR_NORMAL, "Choice            :");

        button_p = add_form_button(radio_p, PFI_NULL, ACTIVE, CP_NULL,
                                   y, x, CLNT_CREATE,
                                   create_client, (pointer) params);
        (void) add_menu_string((pointer) button_p, ACTIVE, y, x + 1,
                               ATTR_NORMAL, "[create]");
	x += 10;

        button_p = add_form_button(radio_p, PFI_NULL, ACTIVE, CP_NULL,
                                   y, x, CLNT_DELETE,
                                   delete_client, (pointer) params);
        (void) add_menu_string((pointer) button_p, ACTIVE, y, x + 1,
                               ATTR_NORMAL, "[delete]");
	x += 10;

        button_p = add_form_button(radio_p, PFI_NULL, ACTIVE, CP_NULL,
                                   y, x, CLNT_DISPLAY,
                                   display_client, (pointer) params);
        (void) add_menu_string((pointer) button_p, ACTIVE, y, x + 1,
                               ATTR_NORMAL, "[display]");
	x += 11;

        button_p = add_form_button(radio_p, PFI_NULL, ACTIVE, CP_NULL,
                                   y, x, CLNT_EDIT,
                                   edit_client, (pointer) params);
        (void) add_menu_string((pointer) button_p, ACTIVE, y, x + 1,
                               ATTR_NORMAL, "[edit]");

	/*
	 *	Root file system info.  This section starts on line 'y'.
	 *	The objects in this section line up on line 'x'.
	 */
	y++;
	field_p = add_form_field(form_p, PFI_NULL,
				 NOT_ACTIVE, "root_fs", y, 11, 31,
				 disp_p->root_fs, sizeof(disp_p->root_fs),
				 PFI_NULL, menu_ignore_obj, PTR_NULL,
				 PFI_NULL, PTR_NULL);
	(void) add_menu_string((pointer) field_p, NOT_ACTIVE, y, 1,
			       ATTR_NORMAL, "Root fs :");

	/*
	 *	Root file system available bytes:
	 */
	field_p = add_form_field(form_p, PFI_NULL,
				 NOT_ACTIVE, "root_avail", y, 44, 10,
				 disp_p->root_avail,
				 sizeof(disp_p->root_avail),
				 PFI_NULL, menu_ignore_obj, PTR_NULL,
				 PFI_NULL, PTR_NULL);

	/*
	 *	Root file system hog info:
	 */
	field_p = add_form_field(form_p, PFI_NULL,
				 NOT_ACTIVE, "rhog_fs", y, 62, 6,
				 disp_p->rhog_fs, sizeof(disp_p->rhog_fs),
				 PFI_NULL, menu_ignore_obj, PTR_NULL,
				 PFI_NULL, PTR_NULL);
	(void) add_menu_string((pointer) field_p, NOT_ACTIVE, y, 56,
			       ATTR_NORMAL, "Hog :");

	/*
	 *	Root file system hog available bytes:
	 */
	field_p = add_form_field(form_p, PFI_NULL,
				 NOT_ACTIVE, "rhog_avail", y, 69, 10,
				 disp_p->rhog_avail,
				 sizeof(disp_p->rhog_avail),
				 PFI_NULL, menu_ignore_obj, PTR_NULL,
				 PFI_NULL, PTR_NULL);

	/*
	 *	Swap file system info.  This section starts on line 'y'.
	 *	The objects in this section line up on line 'x'.
	 */
	y++;
	field_p = add_form_field(form_p, PFI_NULL,
				 NOT_ACTIVE, "swap_fs", y, 11, 31,
				 disp_p->swap_fs, sizeof(disp_p->swap_fs),
				 PFI_NULL, menu_ignore_obj, PTR_NULL,
				 PFI_NULL, PTR_NULL);
	(void) add_menu_string((pointer) field_p, NOT_ACTIVE, y, 1,
			       ATTR_NORMAL, "Swap fs :");

	/*
	 *	Swap file system available bytes:
	 */
	field_p = add_form_field(form_p, PFI_NULL,
				 NOT_ACTIVE, "swap_avail", y, 44, 10,
				 disp_p->swap_avail,
				 sizeof(disp_p->swap_avail),
				 PFI_NULL, menu_ignore_obj, PTR_NULL,
				 PFI_NULL, PTR_NULL);

	/*
	 *	Swap file system hog info:
	 */
	field_p = add_form_field(form_p, PFI_NULL,
				 NOT_ACTIVE, "shog_fs", y, 62, 6,
				 disp_p->shog_fs, sizeof(disp_p->shog_fs),
				 PFI_NULL, menu_ignore_obj, PTR_NULL,
				 PFI_NULL, PTR_NULL);
	(void) add_menu_string((pointer) field_p, NOT_ACTIVE, y, 56,
			       ATTR_NORMAL, "Hog :");

	/*
	 *	Swap file system hog available bytes:
	 */
	field_p = add_form_field(form_p, PFI_NULL,
				 NOT_ACTIVE, "shog_avail", y, 69, 10,
				 disp_p->shog_avail,
				 sizeof(disp_p->shog_avail),
				 PFI_NULL, menu_ignore_obj, PTR_NULL,
				 PFI_NULL, PTR_NULL);


	/*
	 *	Client list:  This section shares space with Client
	 *	information starting at line 'y'
	 */
	y += 3;

	file_p = add_menu_file((pointer) form_p, PFI_NULL, NOT_ACTIVE,
			       "client_list", y, 1,
			       menu_lines() - 3, menu_cols() - 2, 5,
			       CLIENT_LIST);
	(void) add_menu_string((pointer) file_p, NOT_ACTIVE, y - 2, 1,
			       ATTR_NORMAL, disp_p->list_header);

	/*
	 *	Client information.  This section starts at line 'y'.
	 *	Each object in this section lines up on 'x'.
	 */
	x_saved = x = 6;

	/*
	 *	Client's IP address:
	 */
	field_p = add_form_field(form_p, PFI_NULL,
				 NOT_ACTIVE, "ip", y, x + 30, 0,
				 client_p->ip, sizeof(client_p->ip),
				 lex_no_ws, PFI_NULL, PTR_NULL,
				 ckf_inet_addr, PTR_NULL);
	(void) add_menu_string((pointer) field_p, NOT_ACTIVE, y - 1, 1,
			       ATTR_NORMAL, "Client Information :");
	(void) add_menu_string((pointer) field_p, NOT_ACTIVE, y, x,
			       ATTR_NORMAL, "Internet Address            :");
#ifdef SunB1
	y++;
	radio_p = add_form_radio(form_p, PFI_NULL,
				 NOT_ACTIVE, "ip_minlab", &client_p->ip_minlab,
				 PFI_NULL, PTR_NULL,
				 PFI_NULL, PTR_NULL);
	(void) add_menu_string((pointer) radio_p, NOT_ACTIVE, y, x,
			       ATTR_NORMAL, "IP Minimum Label            :");
	x += 31;
	button_p = add_form_button(radio_p, PFI_NULL,
				   NOT_ACTIVE, CP_NULL,
				   y, x, LAB_SYS_LOW,
				   set_ip_minlab, (pointer) params);
	(void) add_menu_string((pointer) button_p, NOT_ACTIVE, y, x + 1,
			       ATTR_NORMAL, "[system_low]");
	x += 14;
	button_p = add_form_button(radio_p, PFI_NULL,
				   NOT_ACTIVE, CP_NULL,
				   y, x, LAB_SYS_HIGH,
				   set_ip_minlab, (pointer) params);
	(void) add_menu_string((pointer) button_p, NOT_ACTIVE, y, x + 1,
			       ATTR_NORMAL, "[system_high]");
	x += 15;
	button_p = add_form_button(radio_p, PFI_NULL,
				   NOT_ACTIVE, CP_NULL,
				   y, x, LAB_OTHER,
				   set_ip_minlab, (pointer) params);
	(void) add_menu_string((pointer) button_p, NOT_ACTIVE, y, x + 1,
			       ATTR_NORMAL, "[other]");
	x = x_saved;				/* restore column line up */

	y++;
	radio_p = add_form_radio(form_p, PFI_NULL,
				 NOT_ACTIVE, "ip_maxlab", &client_p->ip_maxlab,
				 PFI_NULL, PTR_NULL,
				 PFI_NULL, PTR_NULL);
	(void) add_menu_string((pointer) radio_p, NOT_ACTIVE, y, x,
			       ATTR_NORMAL, "IP Maximum Label            :");
	x += 31;
	button_p = add_form_button(radio_p, PFI_NULL,
				   NOT_ACTIVE, CP_NULL,
				   y, x, LAB_SYS_LOW,
				   set_ip_maxlab, (pointer) params);
	(void) add_menu_string((pointer) button_p, NOT_ACTIVE, y, x + 1,
			       ATTR_NORMAL, "[system_low]");
	x += 14;
	button_p = add_form_button(radio_p, PFI_NULL,
				   NOT_ACTIVE, CP_NULL,
				   y, x, LAB_SYS_HIGH,
				   set_ip_maxlab, (pointer) params);
	(void) add_menu_string((pointer) button_p, NOT_ACTIVE, y, x + 1,
			       ATTR_NORMAL, "[system_high]");
	x += 15;
	button_p = add_form_button(radio_p, PFI_NULL,
				   NOT_ACTIVE, CP_NULL,
				   y, x, LAB_OTHER,
				   set_ip_maxlab, (pointer) params);
	(void) add_menu_string((pointer) button_p, NOT_ACTIVE, y, x + 1,
			       ATTR_NORMAL, "[other]");
	x = x_saved;				/* restore column line up */
#endif /* SunB1 */

	/*
	 *	Client's Ethernet address:
	 */
	++y;
	field_p = add_form_field(form_p, PFI_NULL,
				 NOT_ACTIVE, "ether", y, x + 30, 0,
				 client_p->ether, sizeof(client_p->ether),
				 lex_no_ws, PFI_NULL, PTR_NULL,
				 ckf_ether_aton, PTR_NULL);
	(void) add_menu_string((pointer) field_p, NOT_ACTIVE, y, x,
			       ATTR_NORMAL, "Ethernet Address            :");

	/*
	 *	Network Information Services:
	 */
	++y;
	radio_p = add_form_radio(form_p, PFI_NULL,
				 NOT_ACTIVE, "yp", &client_p->yp_type,
				 PFI_NULL, PTR_NULL,
				 PFI_NULL, PTR_NULL);
	(void) add_menu_string((pointer) radio_p, NOT_ACTIVE, y, x,
			       ATTR_NORMAL, "NIS Type                    :");
	x += 30;

	button_p = add_form_button(radio_p, PFI_NULL, NOT_ACTIVE, CP_NULL,
				   y, x, YP_NONE, no_yp, (pointer) params);
	(void) add_menu_string((pointer) button_p, NOT_ACTIVE, y, x + 1,
			       ATTR_NORMAL, "[none]");
	x += 8;

#ifdef NEVER
	
	button_p = add_form_button(radio_p, PFI_NULL, NOT_ACTIVE, CP_NULL,
				   y, x, YP_MASTER, use_yp, (pointer) params);
	(void) add_menu_string((pointer) button_p, NOT_ACTIVE, y, x + 1,
			       ATTR_NORMAL, "[master]");
	x += 10;

	button_p = add_form_button(radio_p, PFI_NULL, NOT_ACTIVE, CP_NULL,
				   y, x, YP_SLAVE, use_yp, (pointer) params);
	(void) add_menu_string((pointer) button_p, NOT_ACTIVE, y, x + 1,
			       ATTR_NORMAL, "[slave]");
	x += 9;

#endif /* NEVER */
	
	button_p = add_form_button(radio_p, PFI_NULL, NOT_ACTIVE, CP_NULL,
				   y, x, YP_CLIENT, use_yp, (pointer) params);
	(void) add_menu_string((pointer) button_p, NOT_ACTIVE, y, x + 1,
			       ATTR_NORMAL, "[client]");
	x = x_saved;				/* restore column line up */

	/*
	 *	Domain name:
	 */
	++y;
	field_p = add_form_field(form_p, PFI_NULL, NOT_ACTIVE, "domainname",
				 y, x + 30, 0,
				 client_p->domainname,
				 sizeof(client_p->domainname),
				 lex_no_ws, PFI_NULL, PTR_NULL,
				 ckf_empty, PTR_NULL);
	(void) add_menu_string((pointer) field_p, NOT_ACTIVE, y, x,
			       ATTR_NORMAL, "Domain name                 :");

	/*
	 *	Swap size:
	 */
	++y;
	field_p = add_form_field(form_p, PFI_NULL, NOT_ACTIVE, "swap_size",
				 y, x + 30, 0,
				 client_p->swap_size,
				 sizeof(client_p->swap_size),
				 lex_no_ws, PFI_NULL, PTR_NULL,
				 chk_swap, PTR_NULL);
	(void) add_menu_string((pointer) field_p, NOT_ACTIVE, y, x,
			       ATTR_NORMAL, "Swap size (e.g. 8B, 8K, 8M) :");

	/*
	 *	Path to Root:
	 */
	++y;
	field_p = add_form_field(form_p, PFI_NULL, NOT_ACTIVE, "root_path",
				 y, x + 30, 0,
				 client_p->root_path,
				 sizeof(client_p->root_path),
				 lex_no_ws, PFI_NULL, PTR_NULL,
				 ckf_abspath, PTR_NULL);
	(void) add_menu_string((pointer) field_p, NOT_ACTIVE, y, x,
			       ATTR_NORMAL, "Path to Root                :");

	/*
	 *	Path to Swap:
	 */
	++y;
	field_p = add_form_field(form_p, PFI_NULL, NOT_ACTIVE, "swap_path",
				 y, x + 30, 0,
				 client_p->swap_path,
				 sizeof(client_p->swap_path),
				 lex_no_ws, PFI_NULL, PTR_NULL,
				 ckf_abspath, PTR_NULL);
	(void) add_menu_string((pointer) field_p, NOT_ACTIVE, y, x,
			       ATTR_NORMAL, "Path to Swap                :");

	/*
	 *	Path to Executables:
	 */
	++y;
	field_p = add_form_field(form_p, PFI_NULL, NOT_ACTIVE, "exec_path",
				 y, x + 30, 0,
				 client_p->exec_path,
				 sizeof(client_p->exec_path),
				 lex_no_ws, PFI_NULL, PTR_NULL,
				 ckf_abspath, PTR_NULL);
	(void) add_menu_string((pointer) field_p, NOT_ACTIVE, y, x,
			       ATTR_NORMAL, "Path to Executables         :");

	/*
	 *	Path to Kernel Executables:
	 */
	++y;
	field_p = add_form_field(form_p, PFI_NULL, NOT_ACTIVE, "kvm_path",
				 y, x + 30, 0,
				 client_p->kvm_path,
				 sizeof(client_p->kvm_path),
				 lex_no_ws, PFI_NULL, PTR_NULL,
				 ckf_abspath, PTR_NULL);
	(void) add_menu_string((pointer) field_p, NOT_ACTIVE, y, x,
			       ATTR_NORMAL, "Path to Kernel Executables  :");

	/*
	 *	Path to Home:
	 */
	++y;
	field_p = add_form_field(form_p, PFI_NULL, NOT_ACTIVE, "home_path",
				 y, x + 30, 0,
				 client_p->home_path,
				 sizeof(client_p->home_path),
				 lex_no_ws, PFI_NULL, PTR_NULL,
				 ckf_abspath, PTR_NULL);
	(void) add_menu_string((pointer) field_p, NOT_ACTIVE, y, x,
			       ATTR_NORMAL, "Path to Home                :");

	/*
	 *	Path to Home:
	 */
	++y;
	field_p = add_form_field(form_p, PFI_NULL, NOT_ACTIVE, "termtype",
				 y, x + 30, 0,
				 client_p->termtype,
				 sizeof(client_p->termtype),
				 lex_no_ws, PFI_NULL, PTR_NULL,
#ifndef lint
				 check_client_terminal, (pointer) client_p->termtype);
#else
				 check_client_terminal, (pointer) 0);
#endif lint
	(void) add_menu_string((pointer) field_p, NOT_ACTIVE, y, x,
			       ATTR_NORMAL, "Terminal type               :");

	/*
	 *	Value confirmation object
	 */
	(void) add_confirm_obj(form_p, NOT_ACTIVE,
			       "value_check", CP_NULL,
                               ck_choice, (pointer) params,
			       PFI_NULL, PTR_NULL,
			       client_okay, (pointer) params,
			       "Ok to use these values [y/n] ?");

	/*
	 *	Finish object
	 */
	(void) add_finish_obj((pointer) form_p,
			      PFI_NULL, PTR_NULL,
			      clear_arch_buttons, (pointer) params,
			      PFI_NULL, PTR_NULL);


	return(form_p);
} /* end create_client_form() */




/*
 *	Name:		ck_choice()
 *
 *	Description:	Check the button selected in the choice radio panel.
 */

static	int
ck_choice(params)
	pointer		params[];
{
	clnt_info *	client_p;		/* pointer to client info */
	form *		form_p;			/* pointer to CLIENT form */


	client_p = (clnt_info *) params[2];
	form_p = (form *) params[4];

	if (client_p->arch == 0) {
		menu_mesg("Must select an architecture.");
		set_form_map(form_p, (pointer)find_form_radio(form_p, "arch"));
		return(menu_repeat_obj());
	}

	if (strlen(client_p->hostname) == 0) {
		menu_mesg("Must enter a client name.");
		set_form_map(form_p, (pointer)find_form_field(form_p, "name"));
		return(menu_repeat_obj());
	}

	return(1);
} /* end ck_choice() */




/*
 *	Name:		no_yp()
 *
 *	Description:	Turn off the domainname field if there is no NIS.
 */

static	int
no_yp(params)
	pointer		params[];
{
	form *		form_p;			/* pointer to CLIENT form */


	form_p = (form *) params[4];

	off_form_field(find_form_field(form_p, "domainname"));

	return(1);
} /* end no_yp() */




/*
 *	Name:		off_client_disp()
 *
 *	Description:	Turn off the client disk space display.
 */

void
off_client_disp(form_p)
	form *		form_p;
{
	off_form_field(find_form_field(form_p, "root_fs"));
	off_form_field(find_form_field(form_p, "root_avail"));
	off_form_field(find_form_field(form_p, "rhog_fs"));
	off_form_field(find_form_field(form_p, "rhog_avail"));
	off_form_field(find_form_field(form_p, "swap_fs"));
	off_form_field(find_form_field(form_p, "swap_avail"));
	off_form_field(find_form_field(form_p, "shog_fs"));
	off_form_field(find_form_field(form_p, "shog_avail"));
} /* end off_client_disp() */




/*
 *	Name:		off_client_info()
 *
 *	Description:	Turn off the client's information.
 */

void
off_client_info(client_p, form_p)
	clnt_info *	client_p;
	form *		form_p;
{
	off_form_field(find_form_field(form_p, "ip"));
#ifdef SunB1
	off_form_radio(find_form_radio(form_p, "ip_minlab"));
	off_form_radio(find_form_radio(form_p, "ip_maxlab"));
#endif /* SunB1 */
	off_form_field(find_form_field(form_p, "ether"));
	off_form_radio(find_form_radio(form_p, "yp"));
	if (client_p->yp_type != YP_NONE)
		off_form_field(find_form_field(form_p, "domainname"));
	off_form_field(find_form_field(form_p, "swap_size"));
	off_form_field(find_form_field(form_p, "root_path"));
	off_form_field(find_form_field(form_p, "swap_path"));
	off_form_field(find_form_field(form_p, "exec_path"));
	off_form_field(find_form_field(form_p, "kvm_path"));
	off_form_field(find_form_field(form_p, "home_path"));
	off_form_field(find_form_field(form_p, "termtype"));
} /* end off_client_info() */




/*
 *	Name:		on_client_disp()
 *
 *	Description:	Turn on the client's disk space display.
 */

void
on_client_disp(form_p)
	form *		form_p;
{
	on_form_field(find_form_field(form_p, "root_fs"));
	on_form_field(find_form_field(form_p, "root_avail"));
	on_form_field(find_form_field(form_p, "rhog_fs"));
	on_form_field(find_form_field(form_p, "rhog_avail"));
	on_form_field(find_form_field(form_p, "swap_fs"));
	on_form_field(find_form_field(form_p, "swap_avail"));
	on_form_field(find_form_field(form_p, "shog_fs"));
	on_form_field(find_form_field(form_p, "shog_avail"));
} /* end on_client_disp() */




/*
 *	Name:		on_client_info()
 *
 *	Description:	Turn on the client's information.
 */

void
on_client_info(client_p, form_p)
	clnt_info *	client_p;
	form *		form_p;
{
	on_form_field(find_form_field(form_p, "ip"));
#ifdef SunB1
	on_form_radio(find_form_radio(form_p, "ip_minlab"));
	on_form_radio(find_form_radio(form_p, "ip_maxlab"));
#endif /* SunB1 */
	on_form_field(find_form_field(form_p, "ether"));
	on_form_radio(find_form_radio(form_p, "yp"));
	if (client_p->yp_type != YP_NONE)
		on_form_field(find_form_field(form_p, "domainname"));
	on_form_field(find_form_field(form_p, "swap_size"));
	on_form_field(find_form_field(form_p, "root_path"));
	on_form_field(find_form_field(form_p, "swap_path"));
	on_form_field(find_form_field(form_p, "exec_path"));
	on_form_field(find_form_field(form_p, "kvm_path"));
	on_form_field(find_form_field(form_p, "home_path"));
	on_form_field(find_form_field(form_p, "termtype"));
} /* end on_client_info() */


/*
 *	Name:		clear_arch_button
 *
 *	Description:	pre-function of arch_type to clear client and
 *			choice items
 */
int
clear_arch_buttons(params)
	pointer		params[];
{
	clnt_info *	client_p;		/* pointer to client info */
	form *		form_p;			/* pointer to CLIENT form */
	client_p = (clnt_info *) params[2];
	form_p = (form *) params[4];

	clear_form_field((form_field *) find_form_field(form_p, "name"));
	clear_form_radio((form_radio *) find_form_radio(form_p, "choice"));

	(void) strcpy(client_p->hostname, "");
	client_p->choice = 0;

	display_form_field((form_field *) find_form_field(form_p, "name"));
	display_form_radio((form_radio *) find_form_radio(form_p, "choice"));

	return(1);
}



#ifdef SunB1
/*
 *	Name:		set_ip_maxlab()
 *
 *	Description:	The user wants to set the IP maximum label so do
 *		all the checks.
 */

static int
set_ip_maxlab(params)
	pointer		params[];
{
	clnt_info *	client_p;		/* pointer to client info */
	form *		form_p;			/* pointer to CLIENT form */
	form_radio *	radio_p;		/* ptr to ip_???lab radio */


	form_p = (form *) params[4];
	client_p = (clnt_info *) params[2];

	/*
	 *	Minimum label is system_high so we cannot set maxlab to
	 *	anything but system_high
	 */
	if (client_p->ip_minlab == LAB_SYS_HIGH &&
	    client_p->ip_maxlab != LAB_SYS_HIGH) {
		radio_p = find_form_radio(form_p, "ip_maxlab");

		off_form_radio(radio_p);
		client_p->ip_maxlab = LAB_SYS_HIGH;
		on_form_radio(radio_p);
	}

	/*
	 *	We set the maximum to system_low so the minimum must be set
	 *	to system_low.
	 */
	else if (client_p->ip_maxlab == LAB_SYS_LOW) {
		radio_p = find_form_radio(form_p, "ip_minlab");

		off_form_radio(radio_p);
		client_p->ip_minlab = LAB_SYS_LOW;
		on_form_radio(radio_p);
	}

	return(1);
} /* end set_ip_maxlab() */




/*
 *	Name:		set_ip_minlab()
 *
 *	Description:	The user wants to set the IP minimum label so
 *		do all the checks.
 */

static int
set_ip_minlab(params)
	pointer		params[];
{
	clnt_info *	client_p;		/* pointer to client info */
	form *		form_p;			/* pointer to CLIENT form */
	form_radio *	radio_p;		/* ptr to ip_???lab radio */


	client_p = (clnt_info *) params[2];
	form_p = (form *) params[4];

	/*
	 *	Maximum label is system_low so we cannot set minlab to
	 *	anything but system_low.
	 */
	if (client_p->ip_maxlab == LAB_SYS_LOW &&
	    client_p->ip_minlab != LAB_SYS_LOW) {
		radio_p = find_form_radio(form_p, "ip_minlab");

		off_form_radio(radio_p);
		client_p->ip_minlab = LAB_SYS_LOW;
		on_form_radio(radio_p);
	}

	/*
	 *	We set the minimum to system_high so the maximum must be set
	 *	to system_high.
	 */
	else if (client_p->ip_minlab == LAB_SYS_HIGH) {
		radio_p = find_form_radio(form_p, "ip_maxlab");

		off_form_radio(radio_p);
		client_p->ip_maxlab = LAB_SYS_HIGH;
		on_form_radio(radio_p);
	}

	return(1);
} /* end set_ip_minlab() */
#endif /* SunB1 */




/*
 *	Name:		show_client_list()
 *
 *	Description:	Show the list of clients for 'arch'.  Modifies the
 *		'list_header' field in 'disp_p' to print the right header.
 */

void
show_client_list(form_p, arch_p, disp_p)
	form *		form_p;
	char *		arch_p;
	clnt_disp *	disp_p;
{
	char		pathname[MAXPATHLEN];	/* path to client_list.<arch> */
	struct stat	stat_buf;		/* file info buffer */
	char		irid[MEDIUM_STR];	/* irid buffer */


	(void) sprintf(pathname, "%s.%s", CLIENT_LIST, arch_p);

	/*
	 *	If the client list does not exist, then don't show it.
	 */
	if (stat(pathname, &stat_buf) != 0)
		return;

	/*
	 *	The client list exists, but if it is zero length don't show it.
	 */
	if (stat_buf.st_size == 0)
		return;

	(void) sprintf(disp_p->list_header, "%s Clients:", 
			aprid_to_irid(arch_p, irid));
	(void) unlink(CLIENT_LIST);
	if (link(pathname, CLIENT_LIST) != 0) {
		menu_log("%s: %s, %s: %s.", progname, CLIENT_LIST, pathname,
			 err_mesg(errno));
		menu_log("\tCannot link files.");
		return;
	}

	on_menu_file(find_menu_file((pointer) form_p, "client_list"));
} /* end show_client_list() */




/*
 *	Name:		use_arch()
 *
 *	Description:	The user wants to select an architecture.  Make sure
 *		it is supported and print the list of clients if it is.
 */

static	int
use_arch(params)
	pointer		params[];
{
	arch_info *	arch_p;			/* ptr to architecture info */
	clnt_info *	client_p;		/* pointer to client info */
	clnt_disp *	disp_p;			/* pointer to client display */
	form *		form_p;			/* pointer to CLIENT form */
	int		i;			/* button index */
	arch_info *	ap;			/* scratch pointer */
	soft_info 	soft;			/* soft info of a given arch */
	char		pathname[MAXPATHLEN];	/* path to data file */

	arch_p = (arch_info *) params[1];
	client_p = (clnt_info *) params[2];
	disp_p = (clnt_disp *) params[3];
	form_p = (form *) params[4];

	off_client_disp(form_p);
	off_client_info(client_p, form_p);
	off_form_shared(form_p, find_form_yesno(form_p, "value_check"));
	off_menu_file(find_menu_file((pointer) form_p, "client_list"));

	/*
	*	Convert button index back to arch string
	*/
	for (i = 1, ap = archlist; i < client_p->arch && ap 
				; i++, ap = ap->next)	;

	if ( ap == NULL)    {
		menu_mesg("unmatched bottun found");
		return(0);
	}

	if (init_client_info(ap->arch_str, arch_p, client_p) != 1) {
		if (is_miniroot()) {
			menu_mesg(
           "%s is not supported by this server. Please goto software form.",
				  client_p->arch_str);
		}  else {
			menu_mesg(
	  "%s is not supported by this server. Please run add_services %s.",
				  client_p->arch_str, client_p->arch_str);
		}
		return(0);
	}

	show_client_list(form_p, client_p->arch_str, disp_p);

	(void) sprintf(pathname, "%s.%s", SOFT_INFO, client_p->arch_str);
	if (read_soft_info(pathname, &soft) != 1)
		return(0);

	(void) strcpy(client_p->exec_path, soft.exec_path);
	(void) strcpy(client_p->kvm_path, soft.kvm_path);	
	(void) strcpy(client_p->share_path, soft.share_path);	

	return(1);
} /* end use_arch() */




/*
 *	Name:		use_yp()
 *
 *	Description:	Turn on the domainname field since there is NIS.
 */

static	int
use_yp(params)
	pointer		params[];
{
	form *		form_p;			/* pointer to CLIENT form */


	form_p = (form *) params[4];

	on_form_field(find_form_field(form_p, "domainname"));

	return(1);
} /* end use_yp() */

/*
 *	Name:		more_archs()
 *
 *	Description:	Trap function for configurations that have more archs
 *		than will fit in one line.
 *
 *	Note:		The menu system should be modified to handle this
 *		type of behavior.
 */

static int
more_archs(radio_p)
	form_radio *	radio_p;
{
	form_button *	button_p;		/* ptr to button */


	/*
	 *	Find first active button
	 */
	for (button_p = radio_p->fr_buttons;
	     button_p && button_p->fb_active == NOT_ACTIVE;
	     button_p = button_p->fb_next)
		/* NULL statement */ ;

	/*
	 *	Turn off all active buttons
	 */
	for (; button_p && button_p->fb_active != NOT_ACTIVE;
	     button_p = button_p->fb_next)
		off_form_button(button_p);

	/*
	 *	If we hit the end, then wrap around
	 */
	if (button_p == NULL)
		button_p = radio_p->fr_buttons;

	/*
	 *	Turn on all buttons until the "more" button
	 */
	for (; button_p && button_p->fb_name == NULL;
	     button_p = button_p->fb_next)
		on_form_button(button_p);

	/*
	 *	Turn on the more button.
	 */
	on_form_button(button_p);

	/*
	 *	We return 0 here so that we don't leave the arch panel.
	 */
	return(0);
} /* end more_archs() */


