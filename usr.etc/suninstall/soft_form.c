#ifndef lint
#ifdef SunB1
static  char    sccsid[] = 	"@(#)soft_form.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static  char    sccsid[] = 	"@(#)soft_form.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1990 Sun Microsystems, Inc.
 */

/*
 *	Name:		soft_form.c
 *
 *	Description:	This file contains all the routines for dealing
 *		with a software form.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include "soft_impl.h"
#include "media.h"		/* for media type codes */
#include "menu_impl.h"


/*
 *	External functions:
 */
extern	char *		sprintf();
extern	char *		strdup();

extern	char		INSTALL_TAR_DIR[];


/*
 *	Local functions:
 */
static	void		arch_type_scr();
static	int		ck_choice();
static	int		ck_operation();
static	int		clear_media_loc();
static	int		config_not_ok();
static	int		config_ok();
static	int		display_media_file();
static	int		max_releases();
static	int		mf_trap_backward();
static	int		mf_trap_forward();
static	int		more();
static	int		native_appl();
static	int		native_os();
static	void		off_mf_display();
static	int		op_add();
static	int		op_edit();
static	int		op_remove();
static	void		operation_scr();
static	void		on_mf_display();
static	void		reset_media_str();
static	int		select_media_file();
static	int		setup_special_devs();
static	void		show_dep_list();
static	void		show_mf_names();
static	int		toc_ok();
static	int		use_arch();
static	int		use_data_file();
static	int		use_local_media();
static	int		use_remote_media();
static	int		values_not_ok();
static	int		values_ok();




/*
 *	Local variables:
 */
static	form_field *	disp_fields[8];		/* media file display ptrs */
static	media_file *	mf_p;			/* temp media file pointer */
static	form_yesno *	shared_p;		/* ptr to shared object */

/*
 *	Local variable aiding in the special compressed devices
 */
static 	int		special_flag = 0;	/* set when spec. dev made */
static 	int		media_flag;		/* set when spec. dev made */
static 	char *		media_num_ptr;		/* points to st_ string */
static 	int		media_x, media_y;	/* location of special media
						 *  dev.s
						 */


/*
 *	Name:		create_soft_form()
 *
 *	Description:	Create the SOFTWARE form.  Uses information in
 *		'soft_p', 'sys_p' and 'disp_p' to make the form.
 */

form *
create_soft_form(soft_p, sys_p, disp_p)
	soft_info *	soft_p;
	sys_info *	sys_p;
	mf_disp *	disp_p;
{
	char		buf[TINY_STR];		/* scratch name buffer */
	form_button *	button_p;		/* ptr to radio button */
	form_field *	field_p;		/* ptr to form field */
	menu_file *	file_p;			/* ptr to menu file object */
	form *		form_p;			/* pointer to SOFTWARE form */
	int		i,j;			/* scratch counter */
	form_radio *	radio_p;		/* ptr to radio panel */
	menu_coord	x, y;			/* scratch coordinates */

	static	pointer		params[4];	/* standard pointers */


	init_menu();
	/*
	 * The screen needs to be at least 24 by 80.  If we are in the
	 * miniroot we know that it is so we don't check
	 */
	if (!is_miniroot())
		chk_screen_size();

	form_p = create_form(PFI_NULL, ACTIVE, "SOFTWARE FORM");


	/*
	 *	Setup standard pointers:
	 */
	params[0] = (pointer) form_p;
	params[1] = (pointer) soft_p;
	params[2] = (pointer) sys_p;
#ifndef lint
	params[3] = (pointer) disp_p;
#endif lint

	y = 2;
	x = 6;

	/*
	 *	Operation section.  This section starts on line 'y'.
	 *	The objects in this section line up on 'x'.
	 */
        radio_p = add_form_radio(form_p, PFI_NULL, ACTIVE, CP_NULL,
				 &soft_p->operation,
				 update_arch_buttons, (pointer) params,
                                 PFI_NULL, PTR_NULL);
        (void) add_menu_string((pointer) radio_p, ACTIVE, y, 1,
			       ATTR_NORMAL,
			       "Software Architecture Operations :");
	y++;

        button_p = add_form_button(radio_p, PFI_NULL, ACTIVE, CP_NULL,
				   y, x, SOFT_ADD, op_add, (pointer) params);
        (void) add_menu_string((pointer) button_p, ACTIVE, y, x + 1,
                               ATTR_NORMAL, "[add new release]");
	x += 19;

        button_p = add_form_button(radio_p, PFI_NULL, ACTIVE, CP_NULL,
                                   y, x, SOFT_EDIT, op_edit, (pointer) params);
        (void) add_menu_string((pointer) button_p, ACTIVE, y, x + 1,
                               ATTR_NORMAL, "[edit existing release]");
	x += 25;


#ifdef NOT_QUITE_YET
        button_p = add_form_button(radio_p, PFI_NULL, ACTIVE, CP_NULL,
                                   y, x, SOFT_REMOVE, op_remove,
				   (pointer) params);
        (void) add_menu_string((pointer) button_p, ACTIVE, y, x + 1,
                               ATTR_NORMAL, "[remove existing release]");
#endif

	x = 6;					/* restore column line up */

	/*
	 *	Architecture information section.  This section starts on
	 *	line 'y'.  The objects in this section line up on 'x'.
	 */
	y += 2;
        radio_p = add_form_radio(form_p, PFI_NULL,
                                 NOT_ACTIVE, "arch_types", &soft_p->arch,
				 ck_operation, (pointer) params,
                                 PFI_NULL, PTR_NULL);
	(void) add_menu_string((pointer) radio_p, NOT_ACTIVE, y, 1,
			       ATTR_NORMAL, "Architecture types :");

	/*
	 *	Media information.  This section starts on line 'y'.
	 *	The objects in this section line up on 'x'.
	 */
	y += 3;

	/*
	 *	Media device type:
	 */
        radio_p = add_form_radio(form_p, PFI_NULL,
                                 NOT_ACTIVE, "media_dev", &soft_p->media_dev,
                                 PFI_NULL, PTR_NULL,
                                 PFI_NULL, PTR_NULL);
	(void) add_menu_string((pointer) radio_p, NOT_ACTIVE, y, 1,
			       ATTR_NORMAL, "Media Information :");
	y++;
        (void) add_menu_string((pointer) radio_p, NOT_ACTIVE, y, x,
                               ATTR_NORMAL, "Media Device   :");

	/*
	 *	Start adding devices:
	 */
	x = 23;
	for (i = 1; cv_media_to_str(&i) != NULL; i++) {
		(void) sprintf(buf, "[%s]", cv_media_to_str(&i));

		/*	Here is the trick part.  If the flag is not zero,
		 *	then we handle it as special, showing all of the
		 *	devices of that kind as a single unit with an
		 *	underbar as the last element.
	 	 *
	 	 *	This is currently only good for 1 specially
	 	 *	displayed device.  To make it multi device, one
	 	 *	would have to make an array of media_num_ptr's and
	 	 *	keep track of them either through the media_flag's,
	 	 *	but must maintain the sequentality of them in the
	 	 *	media.h file
		 */
		if (j = get_media_flag(i)) {
			if (special_flag)
				continue;

			media_flag = j;
			special_flag++;
			media_num_ptr = strdup(buf);
			reset_media_str();

			/*
			 *	Unfortunatly, this 4 is for the off set
		 	 *	number for the location of where to ask for
		 	 *	the number (i.e. only 1 digit numbers are
	 	 	 *	currently supported (thank god).
		 	 */
			media_x = x + 4;
			media_y = y;

			/*
			 *	make the button for it
			 */
			button_p = add_form_button(radio_p, PFI_NULL,
				   NOT_ACTIVE, CP_NULL, y, x, i,
				   setup_special_devs, (pointer) params);
			(void) add_menu_string((pointer) button_p,
			       NOT_ACTIVE, y,
			       x + 1, ATTR_NORMAL, media_num_ptr);
		} else {
			
			special_flag = 0;	/* end of one special device */
					   
			button_p = add_form_button(radio_p, PFI_NULL,
					NOT_ACTIVE, CP_NULL, y, x, i,
					clear_media_loc, (pointer) params);
			(void) add_menu_string((pointer) button_p,
					NOT_ACTIVE, y,
				       	x + 1, ATTR_NORMAL, strdup(buf));

		}

		x += strlen(buf) + 2;

	}
	x = 6;					/* restore column line up */

	/*
	 *	Media location:
	 */
	y++;
        radio_p = add_form_radio(form_p, PFI_NULL,
                                 NOT_ACTIVE, "media_loc", &soft_p->media_loc,
                                 PFI_NULL, PTR_NULL,
                                 PFI_NULL, PTR_NULL);
        (void) add_menu_string((pointer) radio_p, NOT_ACTIVE, y, x,
                               ATTR_NORMAL, "Media Location :");
	x += 17;

        button_p = add_form_button(radio_p, PFI_NULL, NOT_ACTIVE, CP_NULL,
                                   y, x, LOC_LOCAL,
                                   use_local_media, (pointer) params);
        (void) add_menu_string((pointer) button_p, NOT_ACTIVE, y, x + 1,
                               ATTR_NORMAL, "[local]");
	x += 9;

        button_p = add_form_button(radio_p, PFI_NULL, NOT_ACTIVE, CP_NULL,
                                   y, x, LOC_REMOTE,
                                   use_remote_media, (pointer) params);
        (void) add_menu_string((pointer) button_p, NOT_ACTIVE, y, x + 1,
                               ATTR_NORMAL, "[remote]");
	x = 6;					/* restore column line up */

	/*
	 *	Remote mediahost's name:
	 */
	y++;
        field_p = add_form_field(form_p, PFI_NULL,
                                 NOT_ACTIVE, "media_host", y, x + 31, 0,
                                 soft_p->media_host,sizeof(soft_p->media_host),
                                 lex_no_ws, PFI_NULL, PTR_NULL,
                                 ckf_empty, PTR_NULL);
        (void) add_menu_string((pointer) field_p, NOT_ACTIVE, y, x,
                               ATTR_NORMAL, "Mediahost                    :");

	/*
	 *	Remote mediahost's IP addr:
	 */
	y++;
        field_p = add_form_field(form_p, PFI_NULL,
                                 NOT_ACTIVE, "media_ip", y, x + 31, 0,
                                 soft_p->media_ip, sizeof(soft_p->media_ip),
                                 lex_no_ws, PFI_NULL, PTR_NULL,
                                 ckf_inet_addr, PTR_NULL);
        (void) add_menu_string((pointer) field_p, NOT_ACTIVE, y, x,
                               ATTR_NORMAL, "Mediahost's Internet Address :");

	/*
	 *	Read table of contents confirmation object:
	 */
        (void) add_confirm_obj(form_p, NOT_ACTIVE, "toc_confirm", CP_NULL,
			       PFI_NULL, PTR_NULL,
			       PFI_NULL, PTR_NULL,
			       toc_ok, (pointer) params,
	       "Ok to use these values to read the table of contents [y/n] ?");

	/*
	 *	Choice section.  This section starts on line 'y'.
	 *	The objects in this section line up on 'x'.
	 */
	y++;
        radio_p = add_form_radio(form_p, PFI_NULL,
                                 NOT_ACTIVE, "choice", &soft_p->choice,
                                 PFI_NULL, PTR_NULL,
                                 ck_choice, (pointer) params);
        (void) add_menu_string((pointer) radio_p, NOT_ACTIVE, y, 1,
                               ATTR_NORMAL, "Choice :");
	x += 4;

        button_p = add_form_button(radio_p, PFI_NULL, NOT_ACTIVE, CP_NULL,
				   y, x, SOFT_ALL, PFI_NULL, PTR_NULL);
        (void) add_menu_string((pointer) button_p, NOT_ACTIVE, y, x + 1,
                               ATTR_NORMAL, "[all]");
	x += 7;

        button_p = add_form_button(radio_p, PFI_NULL, NOT_ACTIVE, CP_NULL,
                                   y, x, SOFT_DEF, PFI_NULL, PTR_NULL);
        (void) add_menu_string((pointer) button_p, NOT_ACTIVE, y, x + 1,
                               ATTR_NORMAL, "[default]");
	x += 11;

        button_p = add_form_button(radio_p, PFI_NULL, NOT_ACTIVE, CP_NULL,
                                   y, x, SOFT_REQ,
                                   PFI_NULL, PTR_NULL);
        (void) add_menu_string((pointer) button_p, NOT_ACTIVE, y, x + 1,
                               ATTR_NORMAL, "[required]");
	x += 12;

        button_p = add_form_button(radio_p, PFI_NULL, NOT_ACTIVE, CP_NULL,
                                   y, x, SOFT_OWN, PFI_NULL,
				   PTR_NULL);
        (void) add_menu_string((pointer) button_p, NOT_ACTIVE, y, x + 1,
                               ATTR_NORMAL, "[own choice]");
	x += 14;

        button_p = add_form_button(radio_p, PFI_NULL, NOT_ACTIVE, "data_file",
                                   y, x, SOFT_SAVED,
                                   use_data_file, (pointer) params);
        (void) add_menu_string((pointer) button_p, NOT_ACTIVE, y, x + 1,
                               ATTR_NORMAL, "[existing choice]");
	x = 6;					/* restore column line up */

	if (sys_p->sys_type == SYS_DATALESS)
		soft_p->choice = SOFT_REQ;


	/*
	 *	Default path for executables:
	 */
	y++;
        field_p = add_form_field(form_p, PFI_NULL,
                                 NOT_ACTIVE, "exec_path", y, x + 20, 0,
                                 soft_p->exec_path, sizeof(soft_p->exec_path),
                                 lex_no_ws, PFI_NULL, PTR_NULL,
                                 ckf_abspath, PTR_NULL);
        (void) add_menu_string((pointer) field_p, NOT_ACTIVE, y, x,
                               ATTR_NORMAL, "Executables path :");

	/*
	 *	Default path for kernel executables:
	 */
	y++;
        field_p = add_form_field(form_p, PFI_NULL,
                                 NOT_ACTIVE, "kvm_path", y, x + 27, 0,
                                 soft_p->kvm_path, sizeof(soft_p->kvm_path),
                                 lex_no_ws, PFI_NULL, PTR_NULL,
                                 ckf_abspath, PTR_NULL);
        (void) add_menu_string((pointer) field_p, NOT_ACTIVE, y, x,
                               ATTR_NORMAL,
			       "Kernel executables path :");


	/*
	 *	The media list object.  This object starts on line 'y'.
	 */
	y += 3;
	file_p = add_menu_file((pointer) form_p, PFI_NULL, NOT_ACTIVE,
			       "media_list", y, 1, menu_lines() - 4,
			       menu_cols() - 2, 5, MEDIA_LIST);
        (void) add_menu_string((pointer) file_p, NOT_ACTIVE, y - 1, 1,
			       ATTR_NORMAL, "Media Filenames:");


	/*
	 *	Value confirmation object:
	 */
        (void) add_confirm_obj(form_p, NOT_ACTIVE, "values_confirm",
			       CP_NULL,
			       mf_trap_backward, (pointer) params,
			       values_not_ok, (pointer) params,
			       values_ok, (pointer) params,
	       "Ok to use these values to select Software Categories [y/n] ?");

	/*
	 *	Destination file system info.  This section starts on line 'y'.
	 *	The objects in this section line up on 'x'.
	 */
	y--;
        field_p = add_form_field(form_p, PFI_NULL,
                                 NOT_ACTIVE, "dest_str", y, x + 12, 45,
				 disp_p->dest_str, sizeof(disp_p->dest_str),
                                 PFI_NULL, menu_ignore_obj, PTR_NULL,
                                 PFI_NULL, PTR_NULL);
        (void) add_menu_string((pointer) field_p, NOT_ACTIVE, y, 1,
                               ATTR_NORMAL, "Destination fs :");

        field_p = add_form_field(form_p, PFI_NULL,
                                 NOT_ACTIVE, "dest_avail", y, x + 63, 10,
				 disp_p->dest_avail,
				 sizeof(disp_p->dest_avail),
                                 PFI_NULL, menu_ignore_obj, PTR_NULL,
                                 PFI_NULL, PTR_NULL);

	/*
	 *	Free hog file system info.  This section starts on line 'y'.
	 *	The objects in this section line up on 'x'.
	 */
	y++ ;
        field_p = add_form_field(form_p, PFI_NULL,
                                 NOT_ACTIVE, "hog_fs", y, x + 56, 6,
				 disp_p->hog_fs, sizeof(disp_p->hog_fs),
                                 PFI_NULL, menu_ignore_obj, PTR_NULL,
                                 PFI_NULL, PTR_NULL);
        (void) add_menu_string((pointer) field_p, NOT_ACTIVE, y, x + 50,
                               ATTR_NORMAL, "Hog :");

        field_p = add_form_field(form_p, PFI_NULL,
                                 NOT_ACTIVE, "hog_avail", y, x + 63, 0,
				 disp_p->hog_avail, sizeof(disp_p->hog_avail),
                                 PFI_NULL, menu_ignore_obj, PTR_NULL,
                                 PFI_NULL, PTR_NULL);


	/*
	 *	Media file info.  This section starts on line 'y'.
	 *	The objects in this section line up on 'x'.
	 */
	y++ ;
        field_p = add_form_field(form_p, PFI_NULL,
                                 NOT_ACTIVE, "media_name", y, x + 2, 52,
				 disp_p->media_name, sizeof(disp_p->media_name),
                                 PFI_NULL, menu_ignore_obj, PTR_NULL,
                                 PFI_NULL, PTR_NULL);
        (void) add_menu_string((pointer) field_p, NOT_ACTIVE, y, 1,
                               ATTR_NORMAL, "Name :");

        field_p = add_form_field(form_p, PFI_NULL,
                                 NOT_ACTIVE, "media_size", y, x + 63, 0,
				 disp_p->media_size, sizeof(disp_p->media_size),
                                 PFI_NULL, menu_ignore_obj, PTR_NULL,
                                 PFI_NULL, PTR_NULL);
        (void) add_menu_string((pointer) field_p, NOT_ACTIVE, y, x + 56,
                               ATTR_NORMAL, "Size :");

        field_p = add_form_field(form_p, PFI_NULL,
                                 NOT_ACTIVE, "media_status",
				 menu_lines() - 2, 67, 12,
				 disp_p->media_status,
				 sizeof(disp_p->media_status),
                                 PFI_NULL, menu_ignore_obj, PTR_NULL,
                                 PFI_NULL, PTR_NULL);
        (void) add_menu_string((pointer) field_p, NOT_ACTIVE,
			       menu_lines() - 2, 59,
                               ATTR_NORMAL, "status: ");

        (void) add_confirm_obj(form_p, NOT_ACTIVE, "media_select",
			       &disp_p->media_select,
			       display_media_file, (pointer) params,
			       select_media_file, (pointer) params,
			       select_media_file, (pointer) params,
			       "Select this media file [y/n] ?");

	/*
	 *	The dependent list object.  This section starts on line 'y'.
	 */
	y += 2;
	file_p = add_menu_file((pointer) form_p, PFI_NULL, NOT_ACTIVE,
			       "dep_list", y, 1, menu_lines() - 3,
			       menu_cols() - 2, 5, DEPENDENT_LIST);
        (void) add_menu_string((pointer) file_p, NOT_ACTIVE, y - 1, 1,
			       ATTR_NORMAL, "Dependents:");

	/*
	 *	Architecture configuration confirmation object:
	 */
        (void) add_confirm_obj(form_p, NOT_ACTIVE, "arch_confirm",
			       CP_NULL,
			       mf_trap_forward, (pointer) params,
			       config_not_ok, (pointer) params,
			       config_ok, (pointer) params,
		          "Ok to use this architecture configuration [y/n] ?");

        (void) add_finish_obj((pointer) form_p,
			      max_releases, PTR_NULL,
                       	      redisplay, MENU_CLEAR,
                       	      PFI_NULL, PTR_NULL);

	disp_fields[0] = find_form_field(form_p, "dest_str");
	disp_fields[1] = find_form_field(form_p, "dest_avail");
	disp_fields[2] = find_form_field(form_p, "hog_fs");
	disp_fields[3] = find_form_field(form_p, "hog_avail");
	disp_fields[4] = find_form_field(form_p, "media_name");
	disp_fields[5] = find_form_field(form_p, "media_size");
	disp_fields[6] = find_form_field(form_p, "media_status");
	disp_fields[7] = 0;
	shared_p = find_form_yesno(form_p, "media_select");

	return(form_p);
} /* end create_soft_form() */



/*
 *	Name:		setup_special_devs()
 *
 *	Description:	This is the proc function for the special devices,
 *			that might have been selected by the user.  The
 *			special flag right now is basically used for
 *			shrinking the size taken up on the screen by the
 *			numerous devices supported by all the platforms.
 *
 *			Here, the user just hit the compressed devices and
 *			we ask it to enter a number and hit a <return>. Then
 *			we do some minimal checking for supported devics,
 *			and the and then pass it on as OK.
 *
 */

static int
setup_special_devs(params)
	pointer		params[];
{
	char		string[MEDIUM_STR];
	form_radio *	radio_p;
	form *		form_p;			/* form pointer */
	soft_info *	soft_p;			/* software info pointer */
	char		number;			/* temp storage */

	form_p = (form *) params[0];
	soft_p = (soft_info *) params[1];

	(void) bzero(string, sizeof(string));

	/*
	 *	Fetch the devices number
	 */

	number = menu_ask_num(media_x, media_y);

	/*
	 *	Not a particularly nice hack, but this is localized to this
	 *	file and care has been taken to make the upgrade to more
	 *	specially handled devices on the screen, through the use of
	 *	the media_flag, in the structure defined in the file
	 *	lib/cv_media.c.
	 */
	switch(media_flag) {
	case SPECIAL_ST_FORM:
		(void) sprintf(string, "st%c", number);
		break;
	}

        if (!cv_str_to_media(string, &(soft_p->media_dev))) {
		menu_mesg("%s : not a supported device", string);
		reset_media_str();
		return(0);
	}

	/*
	 *	This is a good devices, now let's display it and change the
	 *	menu string associated with this special device
	 */

	(void) sprintf(media_num_ptr, "[%s]", string);

	/*
	 *	Now that all is good, get all the info possible and put it
	 *	in the *soft_p structure and set up for the next field on
	 *	the screen
	 */
	media_set_soft(soft_p, soft_p->media_dev);
	
	radio_p = find_form_radio(form_p, "media_loc");
	
	/*
	 *	Toggle the radio panel off, clear the media location and
	 *	toggle the button back on.
	 */
	off_form_radio(radio_p);
	soft_p->media_loc = 0;
	on_form_radio(radio_p);
	
	return(1);
}



/*
 *	Name:		clear_media_loc()
 *
 *	Description:	Unpush the button selected in the media loc radio
 *		panel.  This routine is invoked when the media device and
 *		media local pair is invalid. (ie when a button in the
 *		devices radio form has been pressed).
 */

static int
clear_media_loc(params)
	pointer		params[];
{
	form *		form_p;			/* form pointer */
	form_radio *	radio_p;		/* ptr to media_loc radio */
	soft_info *	soft_p;			/* software info pointer */


	form_p = (form *) params[0];
	soft_p = (soft_info *) params[1];

	/*
	 * find our own radio panel; then using the button number/media token,
	 * call a function to setup the soft_info structure with media info.
	 */
	radio_p = find_form_radio(form_p, "media_dev");
	media_set_soft(soft_p, *(radio_p->fr_codeptr));

	reset_media_str();

	radio_p = find_form_radio(form_p, "media_loc");

	/*
	 *	Toggle the radio panel off, clear the media location and
	 *	toggle the button back on.
	 */
	off_form_radio(radio_p);
	soft_p->media_loc = 0;
	on_form_radio(radio_p);

	return(1);
} /* end clear_media_loc() */




/*
 *	Name:		ck_choice()
 *
 *	Description:	Check the choice of how to select the software.
 *		A dataless system can only have REQUIRED software.
 */

static int
ck_choice(params)
	pointer		params[];
{
	form *		form_p;			/* form pointer */
	soft_info *	soft_p;			/* software info pointer */
	sys_info *	sys_p;			/* system info pointer */


	form_p = (form *) params[0];
	soft_p = (soft_info *) params[1];
	sys_p = (sys_info *) params[2];


	/*
	 *	Clear the media file names regardless of choice
	 */
	off_menu_file(find_menu_file((pointer) form_p, "media_list"));

	/*
	 *	Dataless system must get required software.
	 */
	if (sys_p->sys_type == SYS_DATALESS && soft_p->choice != SOFT_REQ) {
		clear_form_radio(find_form_radio(form_p, "choice"));
		soft_p->choice = SOFT_REQ;
		display_form_radio(find_form_radio(form_p, "choice"));
	}

	return(1);
} /* end ck_choice() */




/*
 *	Name:		config_not_ok()
 *
 *	Description:	The function is invoked if the user says the
 *		displayed configuration is not okay.  Turns off the
 *		media list file object and the architecture confirmer.
 */

static int
config_not_ok(params)
	pointer		params[];
{
	form *		form_p;			/* form pointer */


	form_p = (form *) params[0];


	/*
	 *	Always clear the media file list.
	 */
	off_menu_file(find_menu_file((pointer) form_p, "media_list"));

	off_form_shared(form_p, find_form_yesno(form_p, "arch_confirm"));

	on_form_shared(form_p, find_form_yesno(form_p, "values_confirm"));

	return(1);
} /* end config_not_ok() */




/*
 *	Name:		config_ok()
 *
 *	Description:	The user says the configuration is okay.  The
 *		following steps are performed:
 *
 *			- update path values associated with the architecture
 *			- save information in SOFT_INFO and MEDIA_FILE files
 */

static int
config_ok(params)
	pointer		params[];
{
	form *		form_p;			/* form pointer */
	char		pathname[MAXPATHLEN];	/* pathname buffer */
	char		pathname1[MAXPATHLEN];	/* pathname buffer */
	int		ret_code;		/* return code */
	soft_info *	soft_p;			/* software info pointer */
	sys_info *	sys_p;			/* system info pointer */
	extern int 	info_split_media_file();/* special split routine */

	form_p = (form *) params[0];
	soft_p = (soft_info *) params[1];
	sys_p = (sys_info *) params[2];


	/*
	 *	Always clear the media file list.
	 */
	off_menu_file(find_menu_file((pointer) form_p, "media_list"));

	off_form_shared(form_p, find_form_yesno(form_p, "arch_confirm"));

	on_form_shared(form_p, find_form_yesno(form_p, "values_confirm"));

	ret_code = update_arch(soft_p->arch_str, ARCH_INFO);
    	if (ret_code != 1)
		return(ret_code);
	ret_code = update_arch(soft_p->arch_str, ARCH_LIST);
    	if (ret_code != 1)
		return(ret_code);

	(void) sprintf(pathname, "%s.%s", SOFT_INFO,soft_p->arch_str);

	ret_code = save_soft_info(pathname, soft_p);
	if (ret_code != 1)
		return(ret_code);

	(void) sprintf(pathname1, "%s.%s", APPL_MEDIA_FILE,
			aprid_to_arid(soft_p->arch_str, pathname));
	(void) sprintf(pathname, "%s.%s", MEDIA_FILE, soft_p->arch_str);

	ret_code = info_split_media_file(pathname, pathname1, soft_p);
	if (ret_code != 1)
		return(ret_code);

	ret_code = save_sys_info(SYS_INFO, sys_p);
	if (ret_code != 1)
		return(ret_code);

	return(1);
} /* end config_ok() */




/*
 *	Name:		display_media_file()
 *
 *	Description:	Display the media file information and determine
 *		what choice to make.  If ask_mf_choice() denies a choice
 *		then menu_skip_io() is returned and we don't have a choice.
 *		Otherwise we return to the menu system and get the answer.
 */

static int
display_media_file(params)
	pointer		params[];
{
	mf_disp *	disp_p;			/* media file display ptr */
	form *		form_p;			/* form pointer */
	int		i;			/* counter variable */
	int		ret_code;		/* return code */
	soft_info *	soft_p;			/* software info pointer */
	sys_info *	sys_p;			/* system info pointer */


	form_p = (form *) params[0];
	soft_p = (soft_info *) params[1];
	sys_p = (sys_info *) params[2];
	disp_p = (mf_disp *) params[3];

	if (mf_p == NULL)
		mf_p = soft_p->media_files;

	for (i = 0; disp_fields[i]; i++)
		clear_form_field(disp_fields[i]);

	(void) strcpy(disp_p->media_name, mf_p->mf_name);
	(void) sprintf(&disp_p->media_name[strlen(disp_p->media_name)],
		       " (%s)", cv_iflag_to_str(&mf_p->mf_iflag));
	(void) sprintf(disp_p->media_size, "%ld", FUDGE_SIZE(mf_p->mf_size));

	/*
	 *	Status of selected takes priority over status of loaded
	 *	and status of not selected.
	 */
	if (mf_p->mf_select == ANS_YES)
		(void) strcpy(disp_p->media_status, "selected    ");

	else if (mf_p->mf_loaded == ANS_YES)
		(void) strcpy(disp_p->media_status, "loaded      ");
	else
		(void) strcpy(disp_p->media_status, "not selected");

	ret_code = get_mf_disp_info(sys_p, soft_p, mf_p, disp_p);
	if (ret_code != 1)
		return(ret_code);

	for (i = 0; disp_fields[i]; i++)
		display_form_field(disp_fields[i]);

	show_dep_list(form_p, mf_p);

	if (!ask_mf_choice(disp_p, mf_p, soft_p, sys_p)) {
		return(menu_skip_io());
	}

	paint_menu();

	return(1);
} /* end display_media_file() */




/*
 *	Name:		mf_trap_backwards()
 *
 *	Description:	Media file trap backwards.  This is a trap to allow
 *		us to scroll through the media file list with a menu backward
 *		command.
 */

static int
mf_trap_backward(params)
	pointer		params[];
{
	form *		form_p;			/* form pointer */
	soft_info *	soft_p;			/* software info pointer */


	form_p = (form *) params[0];
	soft_p = (soft_info *) params[1];


	/*
	 *	Menu backward key moves to the previous media file entry.
	 *	This action wraps to the end of the list.
	 */
	if (_menu_backward && shared_p->fyn_active == ACTIVE) {
		if (mf_p <= soft_p->media_files)
			mf_p = &soft_p->media_files[soft_p->media_count];
		mf_p--;

		set_form_map(form_p, (pointer) shared_p);
		return(menu_goto_obj());
	}

	return(1);
} /* end mf_trap_backward() */




/*
 *	Name:		mf_trap_forward()
 *
 *	Description:	Media file trap forward.  This is a trap for allowing
 *		us to scroll forward through the media file list with menu
 *		forward commands.
 */

static int
mf_trap_forward(params)
	pointer		params[];
{
	form *		form_p;			/* form pointer */
	soft_info *	soft_p;			/* software info pointer */


	form_p = (form *) params[0];
	soft_p = (soft_info *) params[1];

	/*
	 *	If no software was selected, then don't show this selected
	 *	files, of course.
	 */
	if (none_selected(soft_p)) {
		off_mf_display(form_p);
		off_form_yesno(find_form_yesno(form_p, "arch_confirm"));
		return(menu_skip_io());
	}


	/*
	 *	Menu forward key moves to the next media file entry.
	 *	This action wraps to the beginning of the list.
	 */
	if (_menu_forward && shared_p->fyn_active == ACTIVE) {
		mf_p++;
		if (mf_p >= &soft_p->media_files[soft_p->media_count])
			mf_p = soft_p->media_files;

		set_form_map(form_p, (pointer) shared_p);
		return(menu_goto_obj());
	}

	/*
	 *	At this point we are preprocessing for the configuration
	 *	confirmation object so get rid of the media file display
	 *	and display the media file names.
	 */
	off_mf_display(form_p);
	show_mf_names(form_p, soft_p);

	return(1);
} /* end mf_trap_forward() */




/*
 *	Name:		more()
 *
 *	Description:	Trap function for radio_panels that have more buttons
 *		than will fit on one line.
 *
 *	Note:		The menu system should be modified to handle this
 *		type of behavior.
 */

static int
more(radio_p)
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
	 *	We return 0 here so that we don't leave the radio panel.
	 */
	return(0);
} /* end more() */




/*
 *	Name:		off_mf_display()
 *
 *	Description:	Turn off the media file display.
 */

static void
off_mf_display(form_p)
	form *		form_p;
{
	int		i;			/* counter variable */


	for (i = 0; disp_fields[i]; i++)
		off_form_field(disp_fields[i]);

	off_form_shared(form_p, shared_p);

	off_menu_file(find_menu_file((pointer) form_p, "dep_list"));
} /* end off_mf_display() */




/*
 *	Name:		on_mf_display()
 *
 *	Description:	Turn on the media file display.
 */

static void
on_mf_display(form_p)
	form *		form_p;
{
	int		i;			/* counter variable */


	off_menu_file(find_menu_file((pointer) form_p, "media_list"));

	for (i = 0; disp_fields[i]; i++)
		on_form_field(disp_fields[i]);

	on_form_shared(form_p, shared_p);
} /* end on_mf_display() */




/*
 *	Name:		op_add()
 *
 *	Description:	Setup for an add software architecture operation.
 */

static int
op_add(params)
	pointer		params[];
{
	form *		form_p;			/* form pointer */
	soft_info *	soft_p;			/* software info pointer */


	form_p = (form *) params[0];
	soft_p = (soft_info *) params[1];

	/*
	 *	Reset the media device and media location and turn on
	 *	the table of contents confirmer.
	 */
	clear_form_radio(find_form_radio(form_p, "media_dev"));
	clear_form_radio(find_form_radio(form_p, "media_loc"));

	(void) bzero(soft_p->arch_str, sizeof(soft_p->arch_str));
	soft_p->media_dev = 0;
	reset_media_str();
	soft_p->media_type = 0;
	soft_p->media_loc = 0;

	on_form_radio(find_form_radio(form_p, "media_dev"));
	on_form_radio(find_form_radio(form_p, "media_loc"));
	on_form_shared(form_p, find_form_yesno(form_p, "toc_confirm"));

	menu_mesg(
	     "Please insert the release media that you are going to install");

	return(1);
} /* end op_add() */




/*
 *	Name:		op_edit()
 *
 *	Description:	Setup for an edit software architecture operation.
 */

static int
op_edit(params)
	pointer		params[];
{
	form *		form_p;			/* form pointer */

	form_p = (form *) params[0];

	/*
	 *	Everything should be off
	 */
	off_form_radio(find_form_radio(form_p, "media_dev"));
	off_form_radio(find_form_radio(form_p, "media_loc"));
	off_form_shared(form_p, find_form_yesno(form_p, "toc_confirm"));
	off_form_field(find_form_field(form_p, "media_host"));
	off_form_field(find_form_field(form_p, "media_ip"));
	off_form_field(find_form_field(form_p, "exec_path"));
	off_form_field(find_form_field(form_p, "kvm_path"));
	off_form_radio(find_form_radio(form_p, "choice"));
	off_form_shared(form_p, find_form_yesno(form_p, "values_confirm"));

	return(1);
} /* end op_edit() */




#ifdef NOT_QUITE_YET
/*
 *	Name:		op_remove()
 *
 *	Description:	Setup for an remove software architecture operation.
 */

static int
op_remove(params)
	pointer		params[];
{
	form *		form_p;			/* form pointer */

	form_p = (form *) params[0];
#ifdef lint
	form_p = form_p;
#endif

	menu_mesg("Operation not implemented (yet).");
	return(0);
} /* end op_remove() */
#endif




/*
 *	Name:		select_media_file()
 *
 *	Description:	Select or deselect a media file.  If the media file
 *		is selected and was not selected before or is not loaded, then
 *		we try to get disk space for it.  If we cannot get disk space
 *		for it, then update_bytes() will complain for us.  If we are
 *		deselecting a media file that was previously selected and is
 *		not loaded, then we give back the disk space.  Returns 1 if
 *		everything works, 0 if there is a non-fatal error, and -1 if
 *		there is a fatal error.
 */

static int
select_media_file(params)
	pointer		params[];
{
	mf_disp *	disp_p;			/* media file display ptr */
	soft_info *	soft_p;			/* software info pointer */
	sys_info *	sys_p;			/* system info */


	soft_p = (soft_info *) params[1];
	sys_p = (sys_info *) params[2];
	disp_p = (mf_disp *) params[3];

	switch (disp_p->media_select) {
	case 'Y':
	case 'y':
		/*
		 *	Media file is already loaded so just mark is as
		 *	selected.  No need to update byte count.
		 */
		if (mf_p->mf_loaded == ANS_YES) {
			if (soft_p->choice != SOFT_OWN)
				mf_p->mf_select = ANS_NO;
			else	{
				menu_mesg(
				   "Media file '%s' has already been loaded.",
					  mf_p->mf_name);
				if (menu_ask_yesno(REDISPLAY,
				  "Do you want to load it again anyway?") == 0)
					mf_p->mf_select = ANS_NO;
				else
					mf_p->mf_select = ANS_YES;
			}
		}
		else if (mf_p->mf_select == ANS_NO) {
			/*
			 *	Decrement the available byte count.
			 */

			switch (update_bytes(disp_p->dest_fs,
				(long) FUDGE_SIZE(mf_p->mf_size))) {
			case 1:
				mf_p->mf_select = ANS_YES;
				break;

			case 0:
				/*
				 *	If there is no space, then we just skip
				 *	to the next entry,  if all required
				 *	have been selected or loaded.
				 */
				if (required_software(soft_p, sys_p) != 1) {
					menu_mesg("%s\n%s",
"Not all the required software can be selected because of lack of disk space",
"You can't load this architecture's software into the current paths.");

					mf_p = 0;
					return(1);
				}
				break;

			case -1:
				return(-1);
			} /* end switch */
		}
		break;

	case 'N':
	case 'n':
		/*
		 *	Media file is already loaded so just mark is as
		 *	not selected.  No need to update byte count.
		 */
		if (mf_p->mf_loaded == ANS_YES)
			mf_p->mf_select = ANS_NO;

		else if (mf_p->mf_select == ANS_YES) {
			/*
			 *	The user changed her/his mind.  We should
			 *	always be able to put back bytes.
			 */
			if (update_bytes(disp_p->dest_fs, -mf_p->mf_size) != 1)
				return(-1);

			mf_p->mf_select = ANS_NO;
		}
		break;
	}

	/*
	 *	If we are doing a menu backward or menu forward command,
	 *	then return to the caller and let mf_trap_backward() or
	 *	mf_trap_forward() do their thing.
	 */
	if (_menu_backward || _menu_forward)
		return(1);

	mf_p++;
	/*
	 *	If we have not gone past the end of the list,
	 *	then set the form map to bring us in here again.
	 *	Otherwise clear the pointer and we move on.
	 */
	if (mf_p < &soft_p->media_files[soft_p->media_count]) {
		return(menu_goto_obj());
	}

	mf_p = 0;

	return(1);
} /* end select_media_file() */




/*
 *	Name:		show_dep_list()
 *
 *	Description:	Show the list of dependents associated with the
 *		media file, 'mf_p'.
 */

static void
show_dep_list(form_p, mf_p)
	form *		form_p;
	media_file *	mf_p;
{
	menu_file *	file_p;			/* ptr to dep_list object */
	FILE *		fp;			/* ptr to dependent_list */
	int		i;			/* index variable */


	file_p = find_menu_file((pointer) form_p, "dep_list");

	off_menu_file(file_p);

	if (mf_p->mf_depcount == 0)
		return;

	fp = fopen(DEPENDENT_LIST, "w");
	if (fp == NULL) {
		menu_log("%s: %s: %s.", progname, DEPENDENT_LIST,
			 err_mesg(errno));
		menu_log("\tCannot open file for writing.");
		menu_abort(1);
	}

	for (i = 0; i < mf_p->mf_depcount; i++) {
		(void) fprintf(fp, "%s\n", mf_p->mf_deplist[i]);
	}

	(void) fclose(fp);

	on_menu_file(file_p);
} /* end show_dep_list() */




/*
 *	Name:		show_mf_names()
 *
 *	Description:	Show the media filenames that have been selected
 *		or are already loaded in this configuration.
 */

static void
show_mf_names(form_p, soft_p)
	form *		form_p;
	soft_info *	soft_p;
{
	menu_file *	file_p;			/* ptr to dep_list object */
	FILE *		fp;			/* ptr to dependent_list */
	media_file *	mf;			/* media file pointer */


	file_p = find_menu_file((pointer) form_p, "media_list");

	off_menu_file(file_p);


	if (soft_p->media_files == NULL)	/* nothing to display */
		return;

	fp = fopen(MEDIA_LIST, "w");
	if (fp == NULL) {
		menu_log("%s: %s: %s.", progname, MEDIA_LIST, err_mesg(errno));
		menu_log("\tCannot open file for writing.");
		menu_abort(1);
	}

	for (mf = soft_p->media_files;
	     mf < &soft_p->media_files[soft_p->media_count] &&
	     mf->mf_name && mf->mf_name[0]; mf++) {
		if (mf->mf_select == ANS_NO && mf->mf_loaded == ANS_NO)
			continue;

		/*
		 *	Status of selected takes precedence over status of
		 *	loaded.
		 */
		if (mf->mf_select == ANS_YES)
			(void) fprintf(fp, "%s\n", mf->mf_name);
		else
			(void) fprintf(fp, "%s (loaded)\n", mf->mf_name);
	}

	(void) fclose(fp);

	on_menu_file(file_p);
} /* end show_mf_names() */




/*
 *	Name:		toc_ok()
 *
 *	Description:	The user says the values selected are okay to use
 *		for reading the table of contents.  The following checks are
 *		performed:
 *
 *			- a media device must be selected
 *			- a media location must be selected
 *			- if the location is remote then ifconfig the dev
 *			- if the location is remote then start golabeld (SunB1)
 *			- get the path to the media device
 *			- enable the media file objects
 *			- clear the existing software configuration for
 *			  this architecture
 *			- recalculate the disk space
 */

static int
toc_ok(params)
	pointer		params[];
{
	form *		form_p;			/* form pointer */
	char		pathname[MAXPATHLEN];	/* pathname buffer */
	char		pathname1[MAXPATHLEN];	/* pathname buffer */
	int		ret_code;		/* return code */
	soft_info *	soft_p;			/* software info pointer */
	sys_info *	sys_p;			/* system info pointer */
	soft_info 	tmp_soft;		/* temp soft structure */

#ifdef SunB1
	char		value[MEDIUM_STR];	/* value for HOTS_LABEL */
#endif /* SunB1 */
	Os_ident	sys_os, soft_os;

	form_p = (form *) params[0];
	soft_p = (soft_info *) params[1];
	sys_p = (sys_info *) params[2];


	/*
	 *	Save a copy of the soft_p we want to edit. Copying a
	 *	structure here is not alway portable for all C compilers,
	 *	but since most of this is Sun specific code, it doesn't
	 *	matter.
	 */
	if (soft_p->operation == SOFT_EDIT)
		tmp_soft = *soft_p;

	off_form_shared(form_p, find_form_yesno(form_p, "toc_confirm"));

	if (soft_p->media_dev == 0) {
		clear_form_radio(find_form_radio(form_p, "media_loc"));
		soft_p->media_loc = 0;
		display_form_radio(find_form_radio(form_p, "media_loc"));

		menu_mesg("Must specify Media Device.");
		set_form_map(form_p,
			     (pointer) find_form_radio(form_p, "media_dev"));
		return(menu_repeat_obj());
	}

	if (soft_p->media_loc == 0) {
		menu_mesg("Must specify Media Location.");
		set_form_map(form_p,
			     (pointer) find_form_radio(form_p, "media_loc"));
		return(menu_repeat_obj());
	}

	if (soft_p->media_loc == LOC_REMOTE) {
		add_key_entry(soft_p->media_ip, soft_p->media_host, HOSTS,
			      KEY_OR_VALUE);
#ifdef SunB1
		(void) sprintf(value, "%s Labeled", soft_p->media_host);
		add_key_entry(soft_p->media_ip, value, HOSTS_LABEL,
			      KEY_OR_VALUE);
#endif /* SunB1 */

		ifconfig(sys_p, soft_p, IFCONFIG_RSH);
#ifdef SunB1
		golabeld(sys_p);
#endif /* SunB1 */
	}

	ret_code = get_media_path(soft_p);
	if (ret_code != 1) {
		clear_form_radio(find_form_radio(form_p, "media_dev"));
		clear_form_radio(find_form_radio(form_p, "media_loc"));
		soft_p->media_dev = 0;
		reset_media_str();
		soft_p->media_type = 0;
		soft_p->media_loc = 0;
		display_form_radio(find_form_radio(form_p, "media_dev"));
		display_form_radio(find_form_radio(form_p, "media_loc"));
		off_form_field(find_form_field(form_p, "media_host"));
		off_form_field(find_form_field(form_p, "media_ip"));

		on_form_shared(form_p, find_form_yesno(form_p, "toc_confirm"));

		set_form_map(form_p,
			     (pointer) find_form_radio(form_p, "media_dev"));
		return(menu_repeat_obj());
	}

	if ((ret_code = read_file(soft_p, 1, XDRTOC)) != 1 ||
	    (ret_code = toc_xlat(sys_p, XDRTOC, soft_p)) != 1)	{
		on_form_shared(form_p, find_form_yesno(form_p, "toc_confirm"));
		/*
		 *	Let's make these errors non-fatal and don't exit
		 */
		return(0);
	}

	display_form(form_p);

	switch (soft_p->operation) {
	case SOFT_ADD :
		if (!sys_has_release(sys_p)) {
			/*
			 *	Since the system software must be loaded
			 *	first for proper installation, we check here
			 *	to make sure that this happens.
			 */
			if (read_sys_release(tmp_soft.arch_str) != 0)
				return(-1);

			if (strcmp(tmp_soft.arch_str, soft_p->arch_str) != 0) {
				(void) strcpy(pathname,
					      os_name(soft_p->arch_str));
				(void) strcpy(pathname1,
					      os_name(tmp_soft.arch_str));
				menu_mesg(
  "You have the %s media loaded.\nPlease load the system software release %s",
					  pathname, pathname1);
				return(menu_skip_io());
			}
		}

		/*
		 * See if the tape has a release that we support on it
		 */
		if ((ret_code = check_valid_release(soft_p->arch_str)) != 1) {
			menu_mesg("The %s media loaded is not supported.",
				os_name(soft_p->arch_str));
			return(ret_code);
		}

		if ((ret_code = menu_ask_yesno(NOREDISPLAY,
			       "You have the %s media loaded.  Is this ok?",
				       os_name(soft_p->arch_str))) != 1)
			return(ret_code);
		break;
	case SOFT_EDIT :

		if (strcmp(tmp_soft.arch_str, soft_p->arch_str) != 0) {
			/*
			 *	If it's not the release that we expect, then
			 *	let's bag it and ask if the user are
			 *	finished with the form.
			 */
			(void)strcpy(pathname, os_name(tmp_soft.arch_str));

			menu_mesg(
	     "You have the %s media loaded.\nPlease load the %s release media",
				  os_name(soft_p->arch_str),
				  pathname);
			return(menu_skip_io());
		}
		break;
	}


	/*
	 *	Reset the media_file list for the architecture that
	 *	we are about to specify/respecify.
	 */
	(void) sprintf(pathname1, "%s.%s", APPL_MEDIA_FILE,
			aprid_to_arid(soft_p->arch_str, pathname));
	(void) sprintf(pathname, "%s.%s", MEDIA_FILE, soft_p->arch_str);

	ret_code = replace_media_file(pathname, soft_p);
	if (ret_code != 1)
		return(ret_code);
	/*
	 *	merge with appl. media file if there is one.
	 *	othewise create one.
	 */
        if (access(pathname1, F_OK) == 0)   {
		ret_code = merge_media_file(pathname, pathname1, soft_p);
		if (ret_code != 1)
			return(ret_code);
	}
	else	{
		ret_code = save_media_file(pathname1, soft_p);
		if (ret_code != 1)
			return(ret_code);
	}

	/*
	 *	check if the os tape is going to be installed as the native os
	 *	of the mechine
	 */
	if (native_os(sys_p, soft_p))	{
		(void) strcpy(sys_p->arch_str, soft_p->arch_str);
		(void) strcpy(soft_p->exec_path, "/usr");
		(void) strcpy(soft_p->kvm_path, "/usr/kvm");
		if (sys_p->sys_type == SYS_DATALESS)	{
		  	(void) sprintf(sys_p->exec_path, "/export/exec/%s",
			      aprid_to_arid(soft_p->arch_str, pathname));
			(void) sprintf(sys_p->kvm_path, "/export/exec/kvm/%s",
			      aprid_to_irid(soft_p->arch_str, pathname));
		}
	}
	else	{
		if (native_appl(sys_p, soft_p))
		    	(void) strcpy(soft_p->exec_path, "/usr");
		else if (aprid_to_execpath(soft_p->arch_str, pathname))
			(void) strcpy(soft_p->exec_path, pathname);
		else
			return(-1);
		if (aprid_to_kvmpath(soft_p->arch_str, pathname))
			(void) strcpy(soft_p->kvm_path,pathname);
		else
			return(-1);
	}
	if (fill_os_ident(&sys_os, sys_p->arch_str) > 0 &&
	    fill_os_ident(&soft_os, soft_p->arch_str) > 0 &&
	    same_release(&sys_os, &soft_os))
		(void) sprintf(soft_p->share_path, "/usr/share");
	else
		(void) strcpy(soft_p->share_path,
			aprid_to_sharepath(soft_p->arch_str, pathname));

	on_form_field(find_form_field(form_p, "exec_path"));
	on_form_field(find_form_field(form_p, "kvm_path"));
	on_form_radio(find_form_radio(form_p, "choice"));
	off_form_button(find_form_button(form_p, "data_file"));
	on_form_shared(form_p, find_form_yesno(form_p, "values_confirm"));


	return(1);
} /* end toc_ok() */




/*
 *	Name:		update_arch_buttons()
 *
 *	Description:	Update the architecture radio panel's buttons.
 */

int
update_arch_buttons(params)
	pointer		params[];
{
	form_radio *	radio_p;		/* prt to radio panel */
	char		buf[BUFSIZ];		/* buffer for input */
	form_button *	button_p;		/* ptr to radio button */
	int		dummy;			/* dummy active state */
	FILE *		fp;			/* pointer to ARCH_INFO */
	int		i;			/* dummy button value */
	menu_coord	len;			/* length of button string */
	char *		next_p;			/* ptr to next avail space */
	menu_string *	string_p;		/* scratch string pointer */
	menu_coord	x, y;			/* scratch menu coordinates */

	static	char	saved_aprs[BUFSIZ];	/* saved APRs buffer */
	char		irid[BUFSIZ];		/* irid buffer */
	form *		form_p;			/* form pointer */

	form_p = (form *) params[0];
	radio_p = find_form_radio(form_p, "arch_types");

	/*
	 *	Turn off the architecture radio panel so it will disappear
	 *	if ARCH_INFO is removed.
	 */
	off_form_radio(radio_p);
	clear_form_radio(radio_p);
	operation_scr(form_p);

	fp = fopen(ARCH_INFO, "r");
	if (fp == NULL)
		return(1);

	/*
	 *	Free up the existing buttons.
	 *
	 *	Note: this code depends on menu system internals.
	 */
	while (radio_p->fr_buttons)
		free_form_button(radio_p, radio_p->fr_buttons);

	/*
	 *	Turn on the architecture radio panel after all the old
	 *	buttons have been freed.
	 */
	on_form_radio(radio_p);

	/*
	 *	Initialize coordinates for button placement.  The y-coordinate
	 *	is derived from the last string assigned to the radio panel.
	 *	This assumes that there are strings assigned to the radio
	 *	panel, but this is a safe assumption.
	 *
	 *	Note: this code depends on menu system internals.
	 */
	for (string_p = radio_p->fr_mstrings; string_p->ms_next;)
		string_p = string_p->ms_next;
	y = string_p->ms_y + 1;
	x = 6;

	/*
	 *	Read the APRs from ARCH_INFO, copy the APRs into saved_aprs
	 *	and create a button for each APR.
	 */

	next_p = saved_aprs;
	dummy = ACTIVE;

	for (i = 1; fgets(buf, sizeof(buf), fp);) {
		delete_blanks(buf);

		(void) sprintf(next_p, "[%s]", aprid_to_irid(buf,irid));
		len = strlen(next_p);

		if (x + len >= menu_cols() - 9) {
			button_p = add_form_button(radio_p, PFI_NULL, dummy,
						   "more", y, menu_cols() - 8,
						   -1, more,
						   (pointer) radio_p);
			(void) add_menu_string((pointer) button_p, dummy,
					       y, menu_cols() - 7, ATTR_NORMAL,
					       "[more]");

			if (dummy == ACTIVE)	{
				on_form_button(button_p);
				dummy = NOT_ACTIVE;
			}
			x = 6;
		}

		button_p = add_form_button(radio_p, PFI_NULL, dummy, CP_NULL,
					   y, x, i++, use_arch,
					   (pointer) params);
		(void) add_menu_string((pointer) button_p, dummy, y, x + 1,
				       ATTR_NORMAL, next_p);

		if (dummy == ACTIVE)	on_form_button(button_p);

		x += len + 2;
		next_p += len + 1;
	}

	/*
	 *	If dummy is not active, then we had more architectures than
	 *	would fit on one line so we must add a "more" button to
	 *	get back to the beginning.
	 */
	if (dummy == NOT_ACTIVE) {
		button_p = add_form_button(radio_p, PFI_NULL, dummy,
					   "more", y, menu_cols() - 8,
					   -1, more,
					   (pointer) radio_p);
		(void) add_menu_string((pointer) button_p, dummy,
				       y, menu_cols() - 7, ATTR_NORMAL,
				       "[more]");
	}

	return(1);
} /* end update_arch_buttons() */




/*
 *	Name:		use_arch()
 *
 *	Description:	Select a software architecture to operate on.
 */

static int
use_arch(params)
	pointer		params[];
{
	form *		form_p;			/* form pointer */
	form_radio *	radio_p;		/* ptr to architecture panel */
	soft_info *	soft_p;			/* software info pointer */
	char		pathname[MAXPATHLEN];	/* path to data file */
	char		aprid[MEDIUM_STR];	/* aprid buffer */
	int		ret_code;		/* return code */
	int		op_code;		/* operation code */


	form_p = (form *) params[0];
	soft_p = (soft_info *) params[1];

	/*
	 *	Save the old op  code
	 */
	op_code = soft_p->operation;

	radio_p = find_form_radio(form_p, "arch_types");

	/*
	 *      Extract the architecture from the string on the screen
	 */
	(void) strcpy(soft_p->arch_str,
		      &radio_p->fr_pressed->fb_mstrings->ms_data[1]);
	soft_p->arch_str[strlen(soft_p->arch_str) - 1] = '\0';

	(void) sprintf(pathname, "%s.%s", SOFT_INFO,
			irid_to_aprid(soft_p->arch_str, aprid));
	ret_code = read_soft_info(pathname, soft_p);
	if (ret_code != 1)
		return(ret_code);


	/*
	 *	restore the op code after the read from the file
	 */
	soft_p->operation = op_code;

	on_form_radio(find_form_radio(form_p, "media_dev"));
	on_form_radio(find_form_radio(form_p, "media_loc"));

#if 0
	on_form_field(find_form_field(form_p, "exec_path"));
	on_form_field(find_form_field(form_p, "kvm_path"));
	on_form_radio(find_form_radio(form_p, "choice"));
	on_form_shared(form_p, find_form_yesno(form_p, "values_confirm"));
#endif

	on_form_shared(form_p, find_form_yesno(form_p, "toc_confirm"));

	return(1);
} /* end use_arch() */




/*
 *	Name:		use_data_file()
 *
 *	Description:	Load saved values in from the disk and use them.
 */

static int
use_data_file(params)
	pointer		params[];
{
	form *		form_p;			/* form pointer */
	soft_info *	soft_p;			/* software info pointer */


	form_p = (form *) params[0];
	soft_p = (soft_info *) params[1];
#ifdef lint
	form_p = form_p;
	soft_p = soft_p;
#endif lint

	return(1);
} /* end use_data_file() */




/*
 *	Name:		use_local_media()
 *
 *	Description:	Select a local media device.  Make sure that a
 *		device type is selected first.  Call MAKEDEV to make
 *		the device files.
 */

static int
use_local_media(params)
	pointer		params[];
{
	form *		form_p;			/* form pointer */
	soft_info *	soft_p;			/* software info pointer */


	form_p = (form *) params[0];
	soft_p = (soft_info *) params[1];


	if (soft_p->media_dev == 0) {
		menu_mesg("Must select a device type first.");
		set_form_map(form_p,
			     (pointer) find_form_radio(form_p, "media_dev"));
		return(menu_repeat_obj());
	}

	off_form_field(find_form_field(form_p, "media_host"));
	off_form_field(find_form_field(form_p, "media_ip"));

	MAKEDEV(CP_NULL, media_dev_name(soft_p->media_dev));

	return(1);
} /* end use_local_media() */




/*
 *	Name:		use_remote_media()
 *
 *	Description:	Select a remote media device.  Make sure a device
 *		type is selected first.
 */

static int
use_remote_media(params)
	pointer		params[];
{
	form *		form_p;			/* form pointer */
	soft_info *	soft_p;			/* software info pointer */


	form_p = (form *) params[0];
	soft_p = (soft_info *) params[1];


	if (soft_p->media_dev == 0) {
		menu_mesg("Must select a device type first.");
		set_form_map(form_p,
			     (pointer) find_form_radio(form_p, "media_dev"));
		return(menu_repeat_obj());
	}

	on_form_field(find_form_field(form_p, "media_host"));
	on_form_field(find_form_field(form_p, "media_ip"));

	return(1);
} /* end use_remote_media() */




/*
 *	Name:		values_not_ok()
 *
 *	Description:	The values selected for reading the media are not ok.
 *		Turn off the confirmer and try again.
 */

static int
values_not_ok(params)
	pointer		params[];
{
	form *		form_p;			/* form pointer */


	form_p = (form *) params[0];

	off_form_shared(form_p, find_form_yesno(form_p, "arch_confirm"));

	return(1);
} /* end values_not_ok() */




/*
 *	Name:		values_ok()
 *
 *	Description:	The user says the values selected are okay to use.
 *		The following checks are performed:
 *
 *			- a media device must be selected
 *			- a media location must be selected
 *			- a software choice must be selected
 *			- if the location is remote then ifconfig the dev
 *			- if the location is remote then start golabeld (SunB1)
 *			- get the path to the media device
 *			- if loading a saved file just display it
 *			  and recalculate the disk space.  Otherwise
 *			- enable the media file objects
 *			- clear the existing software configuration for
 *			  this architecture
 *			- recalculate the disk space
 */

static int
values_ok(params)
	pointer		params[];
{
	form *		form_p;			/* form pointer */
	char		pathname[MAXPATHLEN];	/* pathname buffer */
	char		pathname1[MAXPATHLEN];	/* pathname buffer */
	int		ret_code;		/* return code */
	soft_info *	soft_p;			/* software info pointer */
	sys_info *	sys_p;			/* system info pointer */
#ifdef SunB1
	char		value[MEDIUM_STR];	/* value for HOSTS_LABEL */
#endif /* SunB1 */
	soft_info 	soft;			/* scratch software info */


	form_p = (form *) params[0];
	soft_p = (soft_info *) params[1];
	sys_p = (sys_info *) params[2];

	off_form_shared(form_p, find_form_yesno(form_p, "values_confirm"));

	if (soft_p->media_dev == 0) {
		clear_form_radio(find_form_radio(form_p, "media_loc"));
		soft_p->media_loc = 0;
		display_form_radio(find_form_radio(form_p, "media_loc"));

		menu_mesg("Must specify Media Device.");
		set_form_map(form_p,
			     (pointer) find_form_radio(form_p, "media_dev"));
		return(menu_repeat_obj());
	}

	if (soft_p->media_loc == 0) {
		menu_mesg("Must specify Media Location.");
		set_form_map(form_p,
			     (pointer) find_form_radio(form_p, "media_loc"));
		return(menu_repeat_obj());
	}

	if (soft_p->choice == 0) {
		menu_mesg("Must specify Software Choice.");
		set_form_map(form_p,
			     (pointer) find_form_radio(form_p, "choice"));
		return(menu_repeat_obj());
	}

	if (soft_p->media_loc == LOC_REMOTE) {
		add_key_entry(soft_p->media_ip, soft_p->media_host, HOSTS,
			      KEY_OR_VALUE);
#ifdef SunB1
		(void) sprintf(value, "%s Labeled", soft_p->media_host);
		add_key_entry(soft_p->media_ip, value, HOSTS_LABEL,
			      KEY_OR_VALUE);
#endif /* SunB1 */

		ifconfig(sys_p, soft_p, IFCONFIG_RSH);
#ifdef SunB1
		golabeld(sys_p);
#endif /* SunB1 */
	}

	ret_code = get_media_path(soft_p);
	if (ret_code != 1) {
		clear_form_radio(find_form_radio(form_p, "media_dev"));
		clear_form_radio(find_form_radio(form_p, "media_loc"));
		soft_p->media_dev = 0;
		reset_media_str();
		soft_p->media_type = 0;
		soft_p->media_loc = 0;
		display_form_radio(find_form_radio(form_p, "media_dev"));
		display_form_radio(find_form_radio(form_p, "media_loc"));

		set_form_map(form_p,
			     (pointer) find_form_radio(form_p, "media_dev"));
		return(menu_repeat_obj());
	}

	/*
	 *	If the operation is add os release, then read toc from the
	 *	tape,  else read from media file.
	 */
	if (soft_p->operation == SOFT_ADD) {
		while (1)   {
			soft = *soft_p;
			ret_code = read_file(&soft, 1, XDRTOC);
			if (ret_code != 1)
				return(ret_code);

			ret_code = toc_xlat(sys_p, XDRTOC, &soft);
			if (ret_code != 1)
				return(ret_code);

			if (strcmp(soft.arch_str, soft_p->arch_str) == 0)
				break;

			menu_log(
			  "Please load the selected release and hit <Return>");
			menu_ack();
		}

		*soft_p = soft;

		/*
		 *	Reset the media_file list for the architecture that
		 *	we are about to specify/respecify.
		 */
                (void) sprintf(pathname1, "%s.%s", APPL_MEDIA_FILE,
				aprid_to_arid(soft_p->arch_str, pathname));
		(void) sprintf(pathname,"%s.%s",MEDIA_FILE,soft_p->arch_str);
		ret_code = replace_media_file(pathname, soft_p);
		if (ret_code != 1)
			return(ret_code);
		/*
		 *	merge with appl. media file if there is one.
		 *	othewise create one.
		 */
	        if (access(pathname1, F_OK) == 0)   {
			ret_code = merge_media_file(pathname,
						    pathname1, soft_p);
			if (ret_code != 1)
				return(ret_code);
		}
		else	{
			ret_code = save_media_file(pathname1, soft_p);
			if (ret_code != 1)
				return(ret_code);
		}
	}
	else	{
                (void) sprintf(pathname1, "%s.%s", APPL_MEDIA_FILE,
				aprid_to_arid(soft_p->arch_str, pathname));
                (void) sprintf(pathname, "%s.%s", MEDIA_FILE, soft_p->arch_str);
		ret_code = merge_media_file(pathname, pathname1, soft_p);
		if (ret_code != 1)
			return(ret_code);
	}

	on_mf_display(form_p);

	/*
	 *	Calculate the disk space that was used.  This call is
	 *	necessary to undo any unverified changes.
	 */
	ret_code = calc_disk(sys_p);
	if (ret_code != 1)
		return(ret_code);

	on_form_shared(form_p, find_form_yesno(form_p, "arch_confirm"));
	clear_form_yesno(find_form_yesno(form_p, "arch_confirm"));

	return(1);
} /* end values_ok() */



/*
 *	Name:		ck_operation
 *
 *	Description:	Check the operation will be performed,
 *			If it is adding a new release, then ship the
 *			architecture penel
 */
static int
ck_operation(params)
	pointer	    params[];
{
	soft_info *	soft_p;			/* software info pointer */
	form *		form_p;			/* form pointer */
	form_radio *	radio_p;		/* ptr to architecture panel */

	form_p = (form *) params[0];
	soft_p = (soft_info *) params[1];
	radio_p = find_form_radio(form_p, "arch_types");
#ifdef lint
	radio_p = radio_p;
#endif lint

	arch_type_scr(form_p);

	clear_form_radio(find_form_radio(form_p, "media_dev"));
	clear_form_radio(find_form_radio(form_p, "media_loc"));

	if (soft_p->operation != SOFT_ADD)
		return(1);

	on_form_radio(find_form_radio(form_p, "media_dev"));
	on_form_radio(find_form_radio(form_p, "media_loc"));
	return(menu_ignore_obj());
}

/*
 *	Name:		operation_scr
 *
 *	Description:	clean up the screen below the operation field
 */
static void
operation_scr(form_p)
	form *		form_p;
{
	off_form_radio(find_form_radio(form_p, "media_dev"));
	off_form_radio(find_form_radio(form_p, "media_loc"));
	off_form_shared(form_p, find_form_yesno(form_p, "toc_confirm"));
	off_form_field(find_form_field(form_p, "media_host"));
	off_form_field(find_form_field(form_p, "media_ip"));
	off_form_field(find_form_field(form_p, "exec_path"));
	off_form_field(find_form_field(form_p, "kvm_path"));
	off_form_radio(find_form_radio(form_p, "choice"));
}

/*
 *	Name:		arch_type_scr
 *
 *	Description:	clean up the screen below the arch. type field
 */
static void
arch_type_scr(form_p)
	form *		form_p;
{
	off_form_radio(find_form_radio(form_p, "media_dev"));
	off_form_radio(find_form_radio(form_p, "media_loc"));
	off_form_field(find_form_field(form_p, "media_host"));
	off_form_field(find_form_field(form_p, "media_ip"));
	off_form_field(find_form_field(form_p, "exec_path"));
	off_form_field(find_form_field(form_p, "kvm_path"));
	off_form_radio(find_form_radio(form_p, "choice"));
	off_form_shared(form_p, find_form_yesno(form_p, "values_confirm"));
}


/*
 *	Name:		native_os
 *
 *	Description:	determine if the os from the given tape is a
 *			native os of the machine
 */
static int
native_os(sys_p, soft_p)
	sys_info *	sys_p;
	soft_info *	soft_p;
{
	Os_ident    os_tape, os_machine;
	if (fill_os_ident(&os_tape, soft_p->arch_str) > 0)	{
		if (fill_os_ident(&os_machine, sys_p->arch_str) > 0
			&& same_os(&os_tape, &os_machine))
			return(1);
		else if (fill_os_ident(&os_machine, sys_p->arch_str) == 0
			&& same_arch_pair(&os_tape, &os_machine))
			return(1);
	}
	return(0);
}

/*
 *	Name:		native_appl
 *
 *	Description:	determine if the os from the given tape is a
 *			native application architecture of the machine
 */
static int
native_appl(sys_p, soft_p)
	sys_info *	sys_p;
	soft_info *	soft_p;
{
	Os_ident    os_tape, os_machine;
	if (fill_os_ident(&os_tape, soft_p->arch_str) > 0)	{
		if (fill_os_ident(&os_machine, sys_p->arch_str) > 0
			&& same_appl_arch(&os_tape, &os_machine)
		    	&& same_release(&os_tape, &os_machine))
			return(1);
	}
	return(0);
}


/*
 *	Name:		max_releases
 *
 *	Description:	if we are not in miniroot then we can only get
 *			software information for one release at a time
 *
 */
static int
max_releases()
{
	arch_info *     arch_list;      /* architecture info */

	if (is_miniroot() || read_arch_info(ARCH_LIST, &arch_list) != 1)
		return(1);
	if (arch_list == NULL)
		return(1);
	/*
	 * There is a valid architecture selected so we're outa here
	 */
	return(MENU_EXIT_MENU);
}


/*
 *	Name:		reset_media_str()
 *
 *	Description:	This function sets the special media dev to null
 *			again (a blank for a number). 
 *
 */
static void
reset_media_str()
{
	switch(media_flag) {
	case SPECIAL_ST_FORM:
		(void) sprintf(media_num_ptr, "[%s]", SPECIAL_ST_NULL);
	}
}
