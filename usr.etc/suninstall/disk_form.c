#ifndef lint
#ifdef SunB1
#ident			"@(#)disk_form.c 1.1 92/07/30 SMI; SunOS MLS";
#else
#ident			 "@(#)disk_form.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		disk_form.c
 *
 *	Description:	This file contains all the routines for dealing
 *		with a disk form.
 */

#include <sys/file.h>
#include <stdio.h>
#include <string.h>
#include "disk_impl.h"
#include "menu_impl.h"



extern	char *		sprintf();
extern	char *		strdup();


static	int		default_label();
static	int		done_with_disk();
static	int		existing_label();
static	int		finished_with_form();
#ifdef SunB1
static	int		mls_trap();
#endif /* SunB1 */
static	int		more_disks();
static	int		mt_chk();
static	int		mt_pre_chk();
#ifdef SunB1
static	void		off_mls_table();
#endif /* SunB1 */
static	void		off_part_table();
static	void		on_disk_radios();
#ifdef SunB1
static	void		on_mls_table();
#endif /* SunB1 */
static	void		on_part_table();
static	int		only_c_part();
static	int		preserve_valid();
static	int		force_yesno();
#ifdef SunB1
static	int		part_trap();
#endif /* SunB1 */
static	int		saved_label();

#ifdef SunB1
static	int		set_disk_maxlab();
static	int		set_disk_minlab();
static	int		set_fs_maxlab();
static	int		set_fs_minlab();
#endif /* SunB1 */

static	int		size_chk();
static	int		size_pre_chk();
static	int		update_units();
static	int		use_disk();


/*
 *	Name:		create_disk_form()
 *
 *	Description:	Create a DISK form.  Uses information from 'disk_p',
 *		'disp_p' and 'sys_p' to create the form.
 */

form *
create_disk_form(disk_p, disp_p, sys_p)
	disk_info *	disk_p;
	disk_disp *	disp_p;
	sys_info *	sys_p;
{
	char		buf[BUFSIZ];		/* buffer for disk name */
	form_button *	button_p;		/* ptr to a button */
	int		dummy;			/* dummy variable */
	form_field *	field_p;		/* ptr to field */
	form *		form_p;			/* pointer to DISK form */
	FILE *		fp;			/* ptr to disk_list */
	int (*		func_no)();		/* scratch function pointer */
	int (*		func_yes)();		/* scratch function pointer */
	pointer		p_no;			/* scratch parameter pointer */
	pointer		p_yes;			/* scratch parameter pointer */
	int		i;			/* scratch counter */
	char		name_buf[TINY_STR];	/* buffer for names */
	form_radio *	radio_p;		/* ptr to a radio panel */
	menu_coord	x, y;			/* scratch menu coordinates */

	static	pointer		params[4];	/* buffer for parameter ptrs */
	static	preserve_info	p_info[NDKMAP];	/* buffer for preserve info */


	disp_p->params = params;		/* save ptr to parameters */

	fp = fopen(DISK_LIST, "r");
	if (fp == NULL) {
		menu_log("%s: %s: cannot open file.", progname, DISK_LIST);
		menu_abort(1);
	}

        init_menu();

	form_p = create_form(PFI_NULL, ACTIVE, "DISK FORM");

	params[0] = (pointer) form_p;
	params[1] = (pointer) disk_p;
	params[2] = (pointer) disp_p;
	params[3] = (pointer) sys_p;

	/*
	 *	Disk selection section.  This section starts on line 'y'.
	 *	The objects in this section line up on 'x'.
	 */
	y = 2;
	x = 3;
        radio_p = add_form_radio(form_p, PFI_NULL,
                                 ACTIVE, "select", (int *) NULL,
                                 PFI_NULL, PTR_NULL,
                                 PFI_NULL, PTR_NULL);
	(void) add_menu_string((pointer) radio_p, ACTIVE, y, 1,
			       ATTR_NORMAL, "Attached Disk Devices :");

	y++;
	dummy = ACTIVE;
	for (i = 1; fgets(buf, sizeof(buf), fp);) {
		delete_blanks(buf);

		button_p = add_form_button(radio_p, PFI_NULL, dummy, CP_NULL,
			   		   y, x, i++,
			   		   use_disk, (pointer) params);
		(void) sprintf(name_buf, "[%s]", buf);
		(void) add_menu_string((pointer) button_p, dummy, y, x + 1,
				       ATTR_NORMAL, strdup(name_buf));

		x += DISK_WIDTH;
		if (x > 3 + (DISKS_PER_COL * DISK_WIDTH)) {
			button_p = add_form_button(radio_p, PFI_NULL,
						   NOT_ACTIVE, "more",
						   y, menu_cols() - 9,
						   -1, more_disks,
						   (pointer) radio_p);
			(void) add_menu_string((pointer) button_p, dummy,
					       y, menu_cols() - 8,
					       ATTR_STAND, "[more]");
			dummy = NOT_ACTIVE;
			x = 3;
		}
	}

	(void) fclose(fp);

	/*
	 *	If there were more than DISKS_PER_COL + 1 disks,
	 *	then turn on the more button (i is one-based).
	 */
	if (i - 2 > DISKS_PER_COL) {
		on_form_button(find_form_button(form_p, "more"));

		/*
		 *	Make sure that there is a "more" button at the
		 *	end of the list of disks so we can get back
		 *	to the beginning.
		 */
		if ((i - 1) % (DISKS_PER_COL + 1)) {
			button_p = add_form_button(radio_p, PFI_NULL,
						   NOT_ACTIVE, "more", y,
						   menu_cols() - 9,
						   -1, more_disks,
						   (pointer) radio_p);
			(void) add_menu_string((pointer) button_p,
					       dummy,
					       y, menu_cols() - 8,
					       ATTR_STAND, "[more]");
		}
	}
	x = 3;					/* restore column line up */

	/*
	 *	Disk label section.  This section starts on line 'y'.
	 *	The objects in this section line up on 'x':
	 */
	y += 2;
	x = 14;
        radio_p = add_form_radio(form_p, PFI_NULL,
                                 NOT_ACTIVE, "label",
				 &disk_p->label_source,
                                 PFI_NULL, PTR_NULL,
                                 PFI_NULL, PTR_NULL);
        (void) add_menu_string((pointer) radio_p, NOT_ACTIVE, y, 1,
                               ATTR_NORMAL, "Disk Label :");
        button_p = add_form_button(radio_p, PFI_NULL,
                                   NOT_ACTIVE, CP_NULL,
                                   y, x, DKL_DEFAULT,
                                   default_label, (pointer) params);
        (void) add_menu_string((pointer) button_p, NOT_ACTIVE, y, x + 1,
                               ATTR_NORMAL, "[default]");
	x += 11;

        button_p = add_form_button(radio_p, PFI_NULL,
                                   NOT_ACTIVE, CP_NULL,
                                   y, x, DKL_EXISTING,
                                   existing_label, (pointer) params);
        (void) add_menu_string((pointer) button_p, NOT_ACTIVE, y, x + 1,
                               ATTR_NORMAL, "[use existing]");
	x += 16;

        button_p = add_form_button(radio_p, PFI_NULL,
                                   NOT_ACTIVE, CP_NULL,
                                   y, x, DKL_MODIFY,
                                   existing_label, (pointer) params);
        (void) add_menu_string((pointer) button_p, NOT_ACTIVE, y, x + 1,
                               ATTR_NORMAL, "[modify existing]");
	x += 19;

        button_p = add_form_button(radio_p, PFI_NULL,
                                   NOT_ACTIVE, "data_file",
                                   y, x, DKL_SAVED,
                                   saved_label, (pointer) params);
        (void) add_menu_string((pointer) button_p, NOT_ACTIVE, y, x + 1,
                               ATTR_NORMAL, "[data file]");

	/*
	 *	Free hog section.  This section starts on line 'y'.
	 *	The objects in this section line up on 'x'.
	 */
	y++;
	x = 27;
        radio_p = add_form_radio(form_p, PFI_NULL,
                                 NOT_ACTIVE, "hog", &disk_p->free_hog,
                                 PFI_NULL, PTR_NULL,
                                 PFI_NULL, PTR_NULL);
        (void) add_menu_string((pointer) radio_p, NOT_ACTIVE, y, 1,
                               ATTR_NORMAL, "Free Hog Disk Partition :");

	/*
	**	Don't use partitions 'a', 'b', or 'c' for the free hog
	*/
	for (i = 0; i < NDKMAP; i++) {
		if ((i + 'a' == 'a') ||
		    (i + 'a' == 'b') ||
		    (i + 'a' == 'c'))
			continue;

		button_p = add_form_button(radio_p, PFI_NULL,
					   NOT_ACTIVE, CP_NULL, y, x,
					   i + 'a',
					   PFI_NULL, PTR_NULL);
		(void) sprintf(name_buf, "[%c]", i + 'a');
		(void) add_menu_string((pointer) button_p, NOT_ACTIVE,
				       y, x + 1,
				       ATTR_NORMAL, strdup(name_buf));
		x += strlen(name_buf) + 2;
	}

	/*
	 *	Display unit section.  This section starts on line 'y'.
	 *	The objects in this section line up on 'x'.
	 */
	y++;
	x = 27;
        radio_p = add_form_radio(form_p, PFI_NULL, NOT_ACTIVE, "unit",
				 &disk_p->display_unit,
                                 PFI_NULL, PTR_NULL,
                                 PFI_NULL, PTR_NULL);
        (void) add_menu_string((pointer) radio_p, NOT_ACTIVE, y, 1,
                               ATTR_NORMAL, "Display Unit            :");
        button_p = add_form_button(radio_p, PFI_NULL,
                                   NOT_ACTIVE, CP_NULL,
                                   y, x, DU_MBYTES,
                                   update_units, (pointer) params);
        (void) add_menu_string((pointer) button_p, NOT_ACTIVE, y, x + 1,
                               ATTR_NORMAL, "[Mbytes]");
	x += 10;

        button_p = add_form_button(radio_p, PFI_NULL, NOT_ACTIVE, CP_NULL,
                                   y, x, DU_KBYTES,
                                   update_units, (pointer) params);
        (void) add_menu_string((pointer) button_p, NOT_ACTIVE, y, x + 1,
                               ATTR_NORMAL, "[Kbytes]");
	x += 10;

        button_p = add_form_button(radio_p, PFI_NULL, NOT_ACTIVE, CP_NULL,
                                   y, x, DU_BLOCKS,
                                   update_units, (pointer) params);
        (void) add_menu_string((pointer) button_p, NOT_ACTIVE, y, x + 1,
                               ATTR_NORMAL, "[blocks]");
	x += 10;

        button_p = add_form_button(radio_p, PFI_NULL,
                                   NOT_ACTIVE, CP_NULL,
                                   y, x, DU_CYLINDERS,
                                   update_units, (pointer) params);
        (void) add_menu_string((pointer) button_p, NOT_ACTIVE, y, x + 1,
                               ATTR_NORMAL, "[cylinders]");

#ifdef SunB1
	/*
	 *	Disk minimum label section.  This section starts on line 'y'.
	 *	The objects in this section line up on 'x'.
	 */
	y++;
	x = 27;
	radio_p = add_form_radio(form_p, PFI_NULL, NOT_ACTIVE, "disk_minlab",
				 &disk_p->disk_minlab,
				 PFI_NULL, PTR_NULL,
				 PFI_NULL, PTR_NULL);
	(void) add_menu_string((pointer) radio_p, NOT_ACTIVE, y, 1,
			       ATTR_NORMAL, "Disk Minimum Label      :");

	button_p = add_form_button(radio_p, PFI_NULL, NOT_ACTIVE, CP_NULL,
				   y, x, LAB_SYS_LOW,
				   set_disk_minlab, (pointer) params);
	(void) add_menu_string((pointer) button_p, NOT_ACTIVE, y, x + 1,
			       ATTR_NORMAL, "[system_low]");
	x += 14;

	button_p = add_form_button(radio_p, PFI_NULL, NOT_ACTIVE, CP_NULL,
				   y, x, LAB_SYS_HIGH,
				   set_disk_minlab, (pointer) params);
	(void) add_menu_string((pointer) button_p, NOT_ACTIVE, y, x + 1,
			       ATTR_NORMAL, "[system_high]");
	x += 15;

	button_p = add_form_button(radio_p, PFI_NULL, NOT_ACTIVE, CP_NULL,
				   y, x, LAB_OTHER,
				   set_disk_minlab, (pointer) params);
	(void) add_menu_string((pointer) button_p, NOT_ACTIVE, y, x + 1,
			       ATTR_NORMAL, "[other]");

	/*
	 *	Disk maximum label section.  This section starts on line 'y'.
	 *	The objects in this section line up on 'x'.
	 */
	y++;
	x = 27;
	radio_p = add_form_radio(form_p, PFI_NULL, NOT_ACTIVE, "disk_maxlab",
				 &disk_p->disk_maxlab,
				 PFI_NULL, PTR_NULL,
				 PFI_NULL, PTR_NULL);
	(void) add_menu_string((pointer) radio_p, NOT_ACTIVE, y, 1,
			       ATTR_NORMAL, "Disk Maximum Label      :");

	button_p = add_form_button(radio_p, PFI_NULL, NOT_ACTIVE, CP_NULL,
				   y, x, LAB_SYS_LOW,
				   set_disk_maxlab, (pointer) params);
	(void) add_menu_string((pointer) button_p, NOT_ACTIVE, y, x + 1,
			       ATTR_NORMAL, "[system_low]");
	x += 14;

	button_p = add_form_button(radio_p, PFI_NULL, NOT_ACTIVE, CP_NULL,
				   y, x, LAB_SYS_HIGH,
				   set_disk_maxlab, (pointer) params);
	(void) add_menu_string((pointer) button_p, NOT_ACTIVE, y, x + 1,
			       ATTR_NORMAL, "[system_high]");
	x += 15;

	button_p = add_form_button(radio_p, PFI_NULL, NOT_ACTIVE, CP_NULL,
				   y, x, LAB_OTHER,
				   set_disk_maxlab, (pointer) params);
	(void) add_menu_string((pointer) button_p, NOT_ACTIVE, y, x + 1,
			       ATTR_NORMAL, "[other]");
#endif /* SunB1 */

	/*
	 *	Partition table section.  This section starts on line 'y'.
	 *	The objects in this section line up on 'x'.
	 */
	y += 2;
	x = 5;
	disp_p->strings[0] = add_menu_string((pointer) form_p, NOT_ACTIVE,
					     y, 1, ATTR_NORMAL,
"PARTITION START_CYL BLOCKS    SIZE     MOUNT PT             PRESERVE(Y/N)");

#ifdef SunB1
	disp_p->mls_strings[0] = add_menu_string((pointer) form_p, NOT_ACTIVE,
					         y, 1, ATTR_NORMAL,
"PARTITION     FILE SYSTEM MINIMUM                 FILE SYSTEM MAXIMUM");
#endif /* SunB1 */
	y++;
	disp_p->strings[1] = add_menu_string((pointer) form_p, NOT_ACTIVE,
					     y, 1, ATTR_NORMAL,
"==============================================================================");
#ifdef SunB1
	disp_p->mls_strings[1] = disp_p->strings[1];
#endif /* SunB1 */
	y++;

	for (i = 0; i < NDKMAP; i++) {
		(void) sprintf(name_buf, "%c_size", i + 'a');
		field_p = add_form_field(form_p, PFI_NULL, NOT_ACTIVE,
					 strdup(name_buf),
					 y + i, x + 26, 8,
					 disk_p->partitions[i].size_str,
				        sizeof(disk_p->partitions[i].size_str),
					 lex_no_ws, size_pre_chk,
					 (pointer) &disp_p->disp_list[i],
					 size_chk,
					 (pointer) &disp_p->disp_list[i]);

        	(void) add_menu_string((pointer) field_p, NOT_ACTIVE,
				       y + i, x, ATTR_NORMAL,
				       disp_p->disp_list[i].name);
        	(void) add_menu_string((pointer) field_p, NOT_ACTIVE,
				       y + i, x + 6, ATTR_NORMAL,
				       disk_p->partitions[i].start_str);
        	(void) add_menu_string((pointer) field_p, NOT_ACTIVE,
				       y + i, x + 16, ATTR_NORMAL,
				       disk_p->partitions[i].block_str);

		(void) sprintf(name_buf, "%c_mnt", i + 'a');
		(void) add_form_field(form_p, PFI_NULL, NOT_ACTIVE,
				      strdup(name_buf),
				      y + i, x + 35, 26,
				      disk_p->partitions[i].mount_pt,
				      sizeof(disk_p->partitions[i].mount_pt),
				      lex_no_ws, mt_pre_chk,
				      (pointer) &disp_p->disp_list[i],
				      mt_chk, (pointer) &disp_p->disp_list[i]);
		(void) sprintf(name_buf, "%c_pre", i + 'a');

		p_info[i].params = (char **) params;
		p_info[i].part   = i;
		p_info[i].preserve = &disk_p->partitions[i].preserve_flag;
		func_no  = (i + 'a' == 'c') ? only_c_part : NULL;
		func_yes = (i + 'a' == 'c') ? only_c_part : preserve_valid;
		p_no     = (i + 'a' == 'c') ? (pointer) params : PTR_NULL;
		p_yes    = (i + 'a' == 'c') ? (pointer) params : 
					      (pointer)&p_info[i];
		(void) add_form_yesno(form_p, PFI_NULL, NOT_ACTIVE,
				      strdup(name_buf), y + i, x + 62,
				      &disk_p->partitions[i].preserve_flag,
				      force_yesno, PTR_NULL,
				      func_no, p_no,
				      func_yes, p_yes);

#ifdef SunB1
		/*
		 *	A dummy object for swapping between the partition
		 *	table and the MLS table.
		 */
		(void) sprintf(name_buf, "%c_mls", i + 'a');
		(void) add_form_field(form_p, PFI_NULL, NOT_ACTIVE,
				      strdup(name_buf), y + i, 0, 1, "", 0,
				      PFI_NULL, mls_trap,
				      (pointer) &disp_p->disp_list[i],
				      PFI_NULL, PTR_NULL);

		/*
		 *	Partition minimum label section.  This section
		 *	starts on line 'y + i'.  The objects in this section
		 *	line up on 'x'.
		 */
		x = 1;
		(void) sprintf(name_buf, "%c_minlab", i + 'a');
		radio_p = add_form_radio(form_p, PFI_NULL, NOT_ACTIVE,
					 strdup(name_buf),
					 &disk_p->partitions[i].fs_minlab,
					 PFI_NULL, PTR_NULL,
					 PFI_NULL, PTR_NULL);
        	(void) add_menu_string((pointer) radio_p, NOT_ACTIVE,
				       y + i, x, ATTR_NORMAL,
				       disp_p->disp_list[i].name);
		x += 2;

		button_p = add_form_button(radio_p, PFI_NULL, NOT_ACTIVE,
					   CP_NULL, y + i, x, LAB_SYS_LOW,
					   set_fs_minlab,
					   (pointer) &disp_p->disp_list[i]);
		(void) add_menu_string((pointer) button_p, NOT_ACTIVE,
				       y + i, x + 1,
				       ATTR_NORMAL, "[system_low]");
		x += 14;

		button_p = add_form_button(radio_p, PFI_NULL, NOT_ACTIVE,
					   CP_NULL, y + i, x, LAB_SYS_HIGH,
					   set_fs_minlab,
					   (pointer) &disp_p->disp_list[i]);
		(void) add_menu_string((pointer) button_p, NOT_ACTIVE,
				       y + i, x + 1,
				       ATTR_NORMAL, "[system_high]");
		x += 15;

		button_p = add_form_button(radio_p, PFI_NULL, NOT_ACTIVE,
					   CP_NULL, y + i, x, LAB_OTHER,
					   set_fs_minlab,
					   (pointer) &disp_p->disp_list[i]);
		(void) add_menu_string((pointer) button_p, NOT_ACTIVE,
				       y + i, x + 1,
				       ATTR_NORMAL, "[other]");

		/*
		 *	Partition minimum label section.  This section
		 *	starts on line 'y + i'.  The objects in this section
		 *	line up on 'x'.
		 */
		x += 10;
		(void) sprintf(name_buf, "%c_maxlab", i + 'a');
		radio_p = add_form_radio(form_p, PFI_NULL, NOT_ACTIVE,
					 strdup(name_buf),
					 &disk_p->partitions[i].fs_maxlab,
					 PFI_NULL, PTR_NULL,
					 PFI_NULL, PTR_NULL);

		button_p = add_form_button(radio_p, PFI_NULL, NOT_ACTIVE,
					   CP_NULL, y + i, x, LAB_SYS_LOW,
					   set_fs_maxlab,
					   (pointer) &disp_p->disp_list[i]);
		(void) add_menu_string((pointer) button_p, NOT_ACTIVE,
				       y + i, x + 1,
				       ATTR_NORMAL, "[system_low]");
		x += 14;

		button_p = add_form_button(radio_p, PFI_NULL, NOT_ACTIVE,
					   CP_NULL, y + i, x, LAB_SYS_HIGH,
					   set_fs_maxlab,
					   (pointer) &disp_p->disp_list[i]);
		(void) add_menu_string((pointer) button_p, NOT_ACTIVE,
				       y + i, x + 1,
				       ATTR_NORMAL, "[system_high]");
		x += 15;

		button_p = add_form_button(radio_p, PFI_NULL, NOT_ACTIVE,
					   CP_NULL, y + i, x, LAB_OTHER,
					   set_fs_maxlab,
					   (pointer) &disp_p->disp_list[i]);
		(void) add_menu_string((pointer) button_p, NOT_ACTIVE,
				       y + i, x + 1,
				       ATTR_NORMAL, "[other]");

		x = 5;				/* restore column line-up */

		/*
		 *	A dummy object for swapping between the MLS
		 *	table and the partition table.  The "a_part"
		 *	object is not needed here.
		 */
		(void) sprintf(name_buf, "%c_part", i + 'a');
		(void) add_form_field(form_p, PFI_NULL, NOT_ACTIVE,
				      strdup(name_buf), y + i, 0,
				      1, "", 0,
				      PFI_NULL, part_trap,
				      (pointer) &disp_p->disp_list[i],
				      PFI_NULL, PTR_NULL);
#endif /* SunB1 */
	}

	(void) add_confirm_obj(form_p, NOT_ACTIVE, "done", CP_NULL,
			       PFI_NULL, PTR_NULL,
			       PFI_NULL, PTR_NULL,
			       done_with_disk, (pointer) params,
			       "Ok to use this partition table [y/n] ?");

	(void) add_finish_obj((pointer) form_p,
			      PFI_NULL, PTR_NULL,
			      PFI_NULL, PTR_NULL,
			      finished_with_form, (pointer) params);

	return(form_p);
} /* end create_disk_form() */




/*
 *	Name:		default_label()
 *
 *	Description:	Setup the partition table with a default label.
 */

static int
default_label(params)
	pointer		params[];
{
	disk_info *	disk_p;			/* ptr to disk information */
	disk_disp *	disp_p;			/* ptr to disk display */
	form *		form_p;			/* ptr to form */
	int		ret_code;		/* return code */
	sys_info *	sys_p;			/* ptr to system information */


	form_p = (form *) params[0];
	disk_p = (disk_info *) params[1];
	disp_p = (disk_disp *) params[2];
	sys_p = (sys_info *) params[3];

	off_part_table(disp_p, form_p);		/* clear the partition table */

	ret_code = get_default_part(disk_p, sys_p);
	if (ret_code != 1)
		return(ret_code);

	off_form_radio(find_form_radio(form_p, "hog"));

        if (is_small_size(disk_p))
		disk_p->free_hog = 'g';
	else
		disk_p->free_hog = 'h';
	
	on_form_radio(find_form_radio(form_p, "hog"));
	on_part_table(disk_p, disp_p, form_p);

	return(1);
} /* end default_label() */




/*
 *	Name:		done_with_disk()
 *
 *	Description:	The user is done with the disk.  The following tasks
 *		are performed:
 *
 *			- saves the disk's information in a DISK_INFO file
 *			- replaces the DISK_INFO.'disk'.original file
 *			- creates a CMDFILE
 *			- makes a new MOUNT_LIST
 *			- recalculates the disk space used by software and
 *			  clients.
 */

static int
done_with_disk(params)
	pointer		params[];
{
	char		cmd[MAXPATHLEN * 2];	/* command buffer */
	disk_info *	disk_p;			/* ptr to disk information */
	char		pathname[MAXPATHLEN];	/* scratch pathname */
	int		ret_code;		/* return code */
	sys_info *	sys_p;			/* ptr to system info */
	int		i;
	int		found_mnt_pt = 0;	/* any mount point on disk */



	disk_p = (disk_info *) params[1];
	sys_p = (sys_info *) params[3];

	(void) sprintf(pathname, "%s.%s", DISK_INFO, disk_p->disk_name);

	ret_code = save_disk_info(pathname, disk_p);
	if (ret_code != 1) {
		menu_mesg("%s: cannot save disk information.", pathname);
		return(ret_code);
	}

	(void) sprintf(cmd, "cp %s %s.original 2>> %s", pathname, pathname,
		       LOGFILE);
	x_system(cmd);

	(void) sprintf(pathname, "%s.%s", CMDFILE, disk_p->disk_name);

	ret_code = save_cmdfile(pathname, disk_p);
	if (ret_code != 1) {
		menu_mesg("%s: cannot save cmdfile.", pathname);
		return(ret_code);
	}
	/*
	 * The mount list and disk space usage only needs to be
	 * updated if there are mount points that could affect them
	 */
	for (i = 0;i < NDKMAP; i++) {
		if (disk_p->partitions[i].mount_pt[0] != 0) {
			found_mnt_pt = 1;
			break;
		}
	}
	if (found_mnt_pt) {
		ret_code = make_mount_list(sys_p);
		if (ret_code != 1)
			return(ret_code);

		ret_code = calc_disk(sys_p);
		if (ret_code != 1)
			return(ret_code);
	}

	return(1);
} /* end done_with_disk() */




/*
 *	Name:		existing_label()
 *
 *	Description:	Setup the partition table with an existing label.
 */

static int
existing_label(params)
	pointer		params[];
{
	disk_info *	disk_p;			/* ptr to disk information */
	disk_disp *	disp_p;			/* ptr to disk display */
	form *		form_p;			/* ptr to form */
	int		ret_code;		/* return code */


	form_p = (form *) params[0];
	disk_p = (disk_info *) params[1];
	disp_p = (disk_disp *) params[2];

	off_part_table(disp_p, form_p);		/* clear the partition table */

	ret_code = get_existing_part(disk_p);
	if (ret_code != 1)
		return(ret_code);

	on_part_table(disk_p, disp_p, form_p);

	return(1);
} /* end default_label() */




/*
 *	Name:		finished_with_form()
 *
 *	Description:	Finish up with the form and exit.  The following
 *		checks are performed:
 *
 *			- is a root partition assigned?
 *			- is a user partition assigned?
 *			- have all the disks been configured?  If not then
 *			  ask the user if she/he wants to exit anyway.  If
 *			  the user wants to exit so then just setup datafiles
 *			  that use the existing configuration for that disk.
 */

static int
finished_with_form(params)
	pointer		params[];
{
	char		buf[BUFSIZ];		/* input buffer */
	disk_info *	disk_p;			/* pointer to disk info */
	form *		form_p;			/* pointer to DISK form */
	FILE *		fp;			/* pointer to disk_list */
	char		pathname[MAXPATHLEN];	/* path to disk_list */
	int		ret_code;		/* return code */
	sys_info *	sys_p;			/* ptr to system information */
	int		exit_anyway = ANS_NO;	/* allow exit on request */

	form_p = (form *) params[0];
	disk_p = (disk_info *) params[1];
	sys_p = (sys_info *) params[3];

	ret_code = make_mount_list(sys_p);
	if (ret_code != 1)
		return(ret_code);

	if (sys_p->root[0] == NULL) {
		menu_mesg("Must assign '/' (root) partition.");
		return(0);
	}

	if (sys_p->sys_type == SYS_DATALESS) {
		if (sys_p->user[0]) {
			menu_mesg("Dataless does not need a '/usr' partition.");
			return(0);
		}
	}
	else if (sys_p->user[0] == NULL) {
		menu_mesg("Must assign '/usr' partition.");
		return(0);
	}

        fp = fopen(DISK_LIST, "r");
	if (fp == NULL) {
		menu_log("%s: %s: cannot open file.", progname, DISK_LIST);
		return(-1);
	}

	while (fgets(buf, sizeof(buf), fp)) {
		delete_blanks(buf);

		/*
		 *      Find disks with no data files.
		 */
		(void) sprintf(pathname, "%s.%s.original", DISK_INFO, buf);
		if (access(pathname, R_OK) == 0)
			continue;

		if (exit_anyway == ANS_NO) {
			menu_mesg("One or more disks have not been configured.");
			exit_anyway = menu_ask_yesno(NOREDISPLAY,
				       "Do you want to exit this form anyway?");
		}

		if (exit_anyway == ANS_NO) {
			(void) fclose(fp);
			set_form_map(form_p, (pointer) find_form_radio(form_p,
								    "select"));
			return(menu_goto_obj());
		}
		else {	/* exit_anyway == ANS_YES */
			bzero((char *) disk_p, sizeof(*disk_p));

			(void) strcpy(disk_p->disk_name, buf);
			disk_p->label_source = DKL_EXISTING;
			disk_p->free_hog = 'h';
			disk_p->display_unit = DU_MBYTES;
#ifdef SunB1
			disk_p->disk_minlab = LAB_SYS_LOW;
			disk_p->disk_maxlab = LAB_SYS_HIGH;
#endif /* SunB1 */

			ret_code = get_disk_config(disk_p);
			if (ret_code != 1) {
				(void) fclose(fp);
				return(ret_code);
			}

			ret_code = get_existing_part(disk_p);
			if (ret_code != 1) {
				(void) fclose(fp);
				return(ret_code);
			}

			update_parts(disk_p);

			ret_code = done_with_disk(params);
			if (ret_code != 1) {
				(void) fclose(fp);
				return(ret_code);
			}
		}
	}
	(void) fclose(fp);

	return(1);
} /* end finished_with_form() */




#ifdef SunB1
/*
 *	Name:		mls_trap()
 *
 *	Description:	Trap function for swapping from the partition table
 *		to the SunOS MLS table.  Also handles menu backward commands
 *		from the SunOS MLS table to the partition table.
 */

static	int
mls_trap(part_disp_p)
	part_disp *	part_disp_p;
{
	disk_info *	disk_p;			/* ptr to disk info */
	form *		form_p;			/* ptr to DISK form */


	form_p = (form *) part_disp_p->disk_disp_p->params[0];
	disk_p = (disk_info *) part_disp_p->disk_disp_p->params[1];

	if (_menu_backward) {
		off_mls_table(part_disp_p->disk_disp_p, form_p);
		on_part_table(disk_p, part_disp_p->disk_disp_p, form_p);
	}
	else {
		/*
		 *	Do not allow the user to modify the labels if there
		 *	is no disk space or there is no mount point or if
		 *	the mount point is "/" or "/usr"
		 */
		if (map_blk(part_disp_p->name[0]) == 0 ||
		    strlen(part_disp_p->part_p->mount_pt) == 0 ||
		    strcmp(part_disp_p->part_p->mount_pt, "/") == 0 ||
		    strcmp(part_disp_p->part_p->mount_pt, "/usr") == 0)
			return(menu_skip_io());

		off_part_table(part_disp_p->disk_disp_p, form_p);
		on_mls_table(disk_p, part_disp_p->disk_disp_p, form_p);
	}

	return(menu_skip_io());
} /* end mls_trap() */
#endif /* SunB1 */




/*
 *	Name:		more_disks()
 *
 *	Description:	Trap function for configurations that have more disks
 *		than will fit in one line.
 *
 *	Note:		The menu system should be modified to handle this
 *		type of behavior.
 */

static int
more_disks(radio_p)
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
	 *	We return 0 here so that we don't leave the disk panel.
	 */
	return(0);
} /* end more_disks() */




/*
 *	Name:		mt_chk()
 *
 *	Description:	Check the mount point field for validity.  The
 *		following checks are performed:
 *
 *			- if the field is empty, then the preserve yes/no
 *			  is turned off.  The SunOS MLS table is also
 *			  turned off (SunB1).
 *			- the field must be an absolute path
 *			- if the path specified must be at system_high and
 *			  the disk does not allow system_high partitions
 *			  then the field is not allowed (SunB1).
 *
 *		If the field is valid, then the preserve yes/no object is
 *		turned on.  The SunOS MLS table is also turned on (SunB1).
 */

static int
mt_chk(part_disp_p, data_p)
	part_disp *	part_disp_p;
	char *		data_p;
{
	char		buf[TINY_STR];		/* buffer for field name */
#ifdef SunB1
	disk_info *	disk_p;			/* pointer to disk info */
#endif /* SunB1 */
	form *		form_p;			/* pointer to DISK form */


	form_p = (form *) part_disp_p->disk_disp_p->params[0];

	if (strlen(data_p) == 0) {
		(void) sprintf(buf, "%c_pre", part_disp_p->name[0]);
		off_form_yesno(find_form_yesno(form_p, buf));
#ifdef SunB1
		(void) sprintf(buf, "%c_mls", part_disp_p->name[0]);
		off_form_field(find_form_field(form_p, buf));
#endif /* SunB1 */
		return(1);
	}

	if (ckf_abspath(PTR_NULL, data_p) != 1)
		return(0);

#ifdef SunB1
	disk_p = (disk_info *) part_disp_p->disk_disp_p->params[1];

	/*
	 *	This file system must be at system_high.
	 */
	if (at_sys_high(data_p)) {
		/*
	 	 *	If this disk's maximum label is not system_high,
		 *	then this file system cannot be on this disk.
		 */
		if (disk_p->disk_maxlab != LAB_SYS_HIGH) {
			menu_mesg(
		"%s must be on a disk where the maximum label is system_high.",
			  	  data_p);
			return(0);
		}

		part_disp_p->part_p->fs_minlab = LAB_SYS_HIGH;
		part_disp_p->part_p->fs_maxlab = LAB_SYS_HIGH;
	}
#endif /* SunB1 */

	(void) sprintf(buf, "%c_pre", part_disp_p->name[0]);
	on_form_yesno(find_form_yesno(form_p, buf));
#ifdef SunB1
	(void) sprintf(buf, "%c_mls", part_disp_p->name[0]);
	on_form_field(find_form_field(form_p, buf));
#endif /* SunB1 */

	return(1);
} /* end mt_chk() */




/*
 *	Name:		mt_pre_chk()
 *
 *	Description:	Pre-check for the mount point field.  If there
 *		is no disk space associated with this partition, then
 *		there can be no mount point.
 */

static int
mt_pre_chk(part_disp_p)
	part_disp *	part_disp_p;
{
	disk_info *	disk_p;			/* ptr to disk_info */


	disk_p = (disk_info *) part_disp_p->disk_disp_p->params[1];

	/*
	 *	Skip this mount point if there are no blocks assigned to it
	 */
	if (map_blk(part_disp_p->name[0]) == 0)
		return(0);

	return(1);
} /* end mt_pre_chk() */




#ifdef SunB1
/*
 *	Name:		off_mls_table()
 *
 *	Description:	Turn off the SunOS MLS table.
 */

static void
off_mls_table(disp_p, form_p)
	disk_disp *	disp_p;
	form *		form_p;
{
	char		buf[TINY_STR];		/* buffer for names */
	int		i;			/* counter variable */


	for (i = 0; disp_p->mls_strings[i]; i++)
		off_menu_string(disp_p->mls_strings[i]);

	for (i = 0; i < NDKMAP; i++) {
		(void) sprintf(buf, "%c_mls", i + 'a');
		off_form_field(find_form_field(form_p, buf));

		(void) sprintf(buf, "%c_minlab", i + 'a');
		off_form_radio(find_form_radio(form_p, buf));
		(void) sprintf(buf, "%c_maxlab", i + 'a');
		off_form_radio(find_form_radio(form_p, buf));
	}
} /* end off_mls_table() */
#endif /* SunB1 */




/*
 *	Name:		off_part_table()
 *
 *	Description:	Turn off the partition table.
 */

static void
off_part_table(disp_p, form_p)
	disk_disp *	disp_p;
	form *		form_p;
{
	char		buf[TINY_STR];		/* buffer for names */
	int		i;			/* counter variable */


	for (i = 0; disp_p->strings[i]; i++)
		off_menu_string(disp_p->strings[i]);

	for (i = 0; i < NDKMAP; i++) {
#ifdef SunB1
		(void) sprintf(buf, "%c_part", i + 'a');
		off_form_field(find_form_field(form_p, buf));
#endif /* SunB1 */

		(void) sprintf(buf, "%c_size", i + 'a');
		off_form_field(find_form_field(form_p, buf));
		(void) sprintf(buf, "%c_mnt", i + 'a');
		off_form_field(find_form_field(form_p, buf));
		(void) sprintf(buf, "%c_pre", i + 'a');
		off_form_yesno(find_form_yesno(form_p, buf));
	}

	off_form_shared(form_p, find_form_yesno(form_p, "done"));
} /* end off_part_table() */




/*
 *	Name:		on_disk_radios()
 *
 *	Description:	Initialize default settings and turn on the disk
 *		radio panels.  Only turns on the 'data_file' button if
 *		a save file exists.
 */

static void
on_disk_radios(disk_p, disp_p, form_p)
	disk_info *	disk_p;
	disk_disp *	disp_p;
	form *		form_p;
{
	char		pathname[MAXPATHLEN];	/* path to disk_info */


	off_form_radio(find_form_radio(form_p, "label"));
	off_form_radio(find_form_radio(form_p, "hog"));
	off_form_radio(find_form_radio(form_p, "unit"));
#ifdef SunB1
	off_form_radio(find_form_radio(form_p, "disk_minlab"));
	off_form_radio(find_form_radio(form_p, "disk_maxlab"));
#endif /* SunB1 */

	disk_p->label_source = 0;
	disk_p->free_hog = 'h';
	disk_p->display_unit = DU_MBYTES;
#ifdef SunB1
	disk_p->disk_minlab = LAB_SYS_LOW;
	disk_p->disk_maxlab = LAB_SYS_HIGH;
#endif /* SunB1 */

	on_form_radio(find_form_radio(form_p, "label"));
	on_form_radio(find_form_radio(form_p, "hog"));
	on_form_radio(find_form_radio(form_p, "unit"));
#ifdef SunB1
	on_form_radio(find_form_radio(form_p, "disk_minlab"));
	on_form_radio(find_form_radio(form_p, "disk_maxlab"));
#endif /* SunB1 */

	(void) sprintf(pathname, "%s.%s", DISK_INFO, disk_p->disk_name);

	if (access(pathname, 0) != 0)
		off_form_button(find_form_button(form_p, "data_file"));
	else
		on_form_button(find_form_button(form_p, "data_file"));

	off_part_table(disp_p, form_p);
} /* end on_disk_radios() */




#ifdef SunB1
/*
 *	Name:		on_mls_table()
 *
 *	Description:	Turn on the SunOS MLS table.
 */

static void
on_mls_table(disk_p, disp_p, form_p)
	disk_info *	disk_p;
	disk_disp *	disp_p;
	form *		form_p;
{
	char		buf[TINY_STR];		/* buffer for names */
	int		i;			/* counter variable */


	for (i = 0; disp_p->mls_strings[i]; i++)
		on_menu_string(disp_p->mls_strings[i]);

	for (i = 0; i < NDKMAP; i++) {
		(void) sprintf(buf, "%c_part", i + 'a');
		on_form_field(find_form_field(form_p, buf));

		if (map_blk(i + 'a') == 0 ||
		    strlen(disk_p->partitions[i].mount_pt) == 0)
			continue;

		(void) sprintf(buf, "%c_minlab", i + 'a');
		on_form_radio(find_form_radio(form_p, buf));
		(void) sprintf(buf, "%c_maxlab", i + 'a');
		on_form_radio(find_form_radio(form_p, buf));
	}
} /* end on_mls_table() */
#endif /* SunB1 */




/*
 *	Name:		on_part_table()
 *
 *	Description:	Turn on the partition table.
 */

static void
on_part_table(disk_p, disp_p, form_p)
	disk_info *	disk_p;
	disk_disp *	disp_p;
	form *		form_p;
{
	char		buf[TINY_STR];		/* buffer for names */
	int		i;			/* counter variable */


	for (i = 0; disp_p->strings[i]; i++)
		on_menu_string(disp_p->strings[i]);

	update_parts(disk_p);

	for (i = 0; i < NDKMAP; i++) {
		disk_p->partitions[i].avail_bytes =
					     blocks_to_bytes(map_blk(i + 'a'));

		(void) sprintf(buf, "%c_size", i + 'a');
		on_form_field(find_form_field(form_p, buf));
		if (map_blk(i + 'a')) {
			(void) sprintf(buf, "%c_mnt", i + 'a');
			on_form_field(find_form_field(form_p, buf));

			if (strlen(disk_p->partitions[i].mount_pt)) {
				(void) sprintf(buf, "%c_pre", i + 'a');
				on_form_yesno(find_form_yesno(form_p, buf));
#ifdef SunB1
				(void) sprintf(buf, "%c_mls", i + 'a');
				on_form_field(find_form_field(form_p, buf));
#endif /* SunB1 */
			}
		}
	}

	on_form_shared(form_p, find_form_yesno(form_p, "done"));
} /* end on_part_table() */




/*
 *	Name:		only_c_part()
 *
 *	Description:	If a mount point and preserve yes/no answer is
 *		specified for the 'c' partition, then all other partitions
 *		are not used.
 */

static int
only_c_part(params)
	pointer		params[];
{
	disk_info *	disk_p;			/* ptr to disk information */
	disk_disp *	disp_p;			/* ptr to disk display */
	form *		form_p;			/* ptr to form */
	int		i;			/* index variables */


	form_p = (form *) params[0];
	disk_p = (disk_info *) params[1];
	disp_p = (disk_disp *) params[2];

	/*
	 *	Turn off the partition table while we change it.
	 */
	off_part_table(disp_p, form_p);

	for (i = 0; i < NDKMAP; i++) {
		if (i + 'a' == 'c')
			continue;

		bzero(disk_p->partitions[i].start_str,
		      sizeof(disk_p->partitions[i].start_str));
		bzero(disk_p->partitions[i].block_str,
		      sizeof(disk_p->partitions[i].block_str));
		bzero(disk_p->partitions[i].size_str,
		      sizeof(disk_p->partitions[i].size_str));
		bzero(disk_p->partitions[i].mount_pt,
		      sizeof(disk_p->partitions[i].mount_pt));
	        disk_p->partitions[i].preserve_flag = NULL;
	        disk_p->partitions[i].avail_bytes = 0;
		map_blk(i + 'a') = 0;
		map_cyl(i + 'a') = 0;
	}

	on_part_table(disk_p, disp_p, form_p);	/* turn it back on */

	return(1);
} /* end only_c_part() */


/*
 *	Name:		force_yesno
 *
 *	Description:	Returns MENU_GET_YN which requires that a y/n
 *			answer be returned
 */

static int
force_yesno()
{
	return (MENU_GET_YN);
}

/*
 *	Name:		preserve_valid
 *
 *	Description:	We only allow a partition to be preserved if the
 *			partition is the same as on the disk.  It doesn't
 *			matter if we're not in the miniroot since we 
 *			don't do newfs a partition
 */

static int
preserve_valid(p_info)
	preserve_info * p_info;
{
	disk_info 	disk,*disk_p;
	int		offset;
	pointer		*hold;


	hold = (pointer *) p_info->params;
	disk_p = (disk_info *) hold[1];
	if (is_miniroot()) {
		bcopy(disk_p,&disk,sizeof(disk_info));
		offset = p_info->part;
		(void) get_existing_part(&disk);
		/*
		 * Check if the partition is the same as on the disk
		 */
		if ((disk.partitions[offset].map_buf.dkl_cylno !=
		     disk_p->partitions[offset].map_buf.dkl_cylno) ||
		     (disk.partitions[offset].map_buf.dkl_nblk !=
		     disk_p->partitions[offset].map_buf.dkl_nblk)) { 
			menu_mesg("Can't preserve a partition that has been modified.");
			*p_info->preserve = 'n';
			return(MENU_REPEAT_OBJ);
		}
	}
	return(1);
}

#ifdef SunB1
/*
 *	Name:		part_trap()
 *
 *	Description:	
 *	Description:	Trap function for swapping from the SunOS MLS table
 *		to the partition table.  Also handles menu backward commands
 *		from the partition table to the SunOS MLS table.
 */

static	int
part_trap(part_disp_p)
	part_disp *	part_disp_p;
{
	disk_info *	disk_p;			/* ptr to disk info */
	form *		form_p;			/* ptr to DISK form */


	form_p = (form *) part_disp_p->disk_disp_p->params[0];
	disk_p = (disk_info *) part_disp_p->disk_disp_p->params[1];

	if (_menu_backward) {
		/*
		 *	Do not allow the user to modify the labels if there
		 *	is no disk space or there is no mount point or if
		 *	the mount point is "/" or "/usr"
		 */
		if (map_blk(part_disp_p->name[0]) == 0 ||
		    strlen(part_disp_p->part_p->mount_pt) == 0 ||
		    strcmp(part_disp_p->part_p->mount_pt, "/") == 0 ||
		    strcmp(part_disp_p->part_p->mount_pt, "/usr") == 0)
			return(menu_skip_io());

		off_part_table(part_disp_p->disk_disp_p, form_p);
		on_mls_table(disk_p, part_disp_p->disk_disp_p, form_p);
	}
	else {
		off_mls_table(part_disp_p->disk_disp_p, form_p);
		on_part_table(disk_p, part_disp_p->disk_disp_p, form_p);
	}

	return(menu_skip_io());
} /* end part_trap() */
#endif /* SunB1 */




/*
 *	Name:		saved_label()
 *
 *	Description:	Set a partition table from a saved label file.
 */

static int
saved_label(params)
	pointer		params[];
{
	disk_info *	disk_p;			/* ptr to disk information */
	disk_disp *	disp_p;			/* ptr to disk display */
	form *		form_p;			/* ptr to form */
	char		pathname[MAXPATHLEN];	/* path to disk info */
	int		ret_code;		/* return code */


	form_p = (form *) params[0];
	disk_p = (disk_info *) params[1];
	disp_p = (disk_disp *) params[2];

	clear_form_radio(find_form_radio(form_p, "label"));
	clear_form_radio(find_form_radio(form_p, "hog"));
	clear_form_radio(find_form_radio(form_p, "unit"));
#ifdef SunB1
	clear_form_radio(find_form_radio(form_p, "disk_minlab"));
	clear_form_radio(find_form_radio(form_p, "disk_maxlab"));
#endif /* SunB1 */
	off_part_table(disp_p, form_p);		/* clear the partition table */

	(void) sprintf(pathname, "%s.%s", DISK_INFO, disk_p->disk_name);

	ret_code = read_disk_info(pathname, disk_p);

	/*
	 *	Do not replace the saved value of 'label_source' with
	 *	DKL_SAVED.  The saved value is needed so that checks
	 *	for DKL_EXISTING work with a saved file.
	 */

	display_form_radio(find_form_radio(form_p, "label"));
	display_form_radio(find_form_radio(form_p, "hog"));
	display_form_radio(find_form_radio(form_p, "unit"));
#ifdef SunB1
	display_form_radio(find_form_radio(form_p, "disk_minlab"));
	display_form_radio(find_form_radio(form_p, "disk_maxlab"));
#endif /* SunB1 */

	if (ret_code != 1)
		return(ret_code);

	ret_code = check_partitions(disk_p, "cannot alter the saved disk partition.");
	if (ret_code != 1) {
		return(ret_code);
	}

	on_part_table(disk_p, disp_p, form_p);

	return(1);
} /* end saved_label() */




#ifdef SunB1
/*
 *	Name:		set_disk_maxlab()
 *
 *	Description:	The user wants to set the disk maximum label so do
 *		all the checking.
 */

static int
set_disk_maxlab(params)
	pointer		params[];
{
	disk_info *	disk_p;			/* ptr to disk_info */
	form *		form_p;			/* pointer to HOST form */
	int		i;			/* index variable */
	form_radio *	radio_p;		/* ptr to disk_???lab radio */


	form_p = (form *) params[0];
	disk_p = (disk_info *) params[1];

	/*
	 *	If this disk has root ("/") or user ("/usr") on it or
	 *	the partition must be at system_high, then its disk_maxlab
	 *	must be LAB_SYS_HIGH.
	 */
	for (i = 0; i < NDKMAP; i++)
		if (disk_p->disk_maxlab != LAB_SYS_HIGH &&
		    (strcmp(disk_p->partitions[i].mount_pt, "/") == 0 ||
		     strcmp(disk_p->partitions[i].mount_pt, "/usr") == 0 ||
		     at_sys_high(disk_p->partitions[i].mount_pt))) {
			radio_p = find_form_radio(form_p, "disk_maxlab");

			off_form_radio(radio_p);
			disk_p->disk_maxlab = LAB_SYS_HIGH;
			on_form_radio(radio_p);

			menu_mesg("Disk maximum label must be system_high.");
			return(1);
		}

	/*
	 *	Minimum label is system_high so we cannot set maxlab to
	 *	anything but system_high
	 */
	if (disk_p->disk_minlab == LAB_SYS_HIGH &&
	    disk_p->disk_maxlab != LAB_SYS_HIGH) {
		radio_p = find_form_radio(form_p, "disk_maxlab");

		off_form_radio(radio_p);
		disk_p->disk_maxlab = LAB_SYS_HIGH;
		on_form_radio(radio_p);

		menu_mesg("Disk maximum label must be system_high.");
	}

	/*
	 *	We set the maximum to system_low so the minimum must be set
	 *	to system_low.
	 */
	else if (disk_p->disk_maxlab == LAB_SYS_LOW) {
		radio_p = find_form_radio(form_p, "disk_minlab");

		off_form_radio(radio_p);
		disk_p->disk_minlab = LAB_SYS_LOW;
		on_form_radio(radio_p);
	}

	/*
	 *	We do not turn off the MLS table here since it
	 *	should not be visable.
	 */
	for (i = 0; i < NDKMAP; i++) {
		if (disk_p->partitions[i].fs_minlab > disk_p->disk_maxlab)
			disk_p->partitions[i].fs_minlab = disk_p->disk_maxlab;

		if (disk_p->partitions[i].fs_maxlab > disk_p->disk_maxlab)
			disk_p->partitions[i].fs_maxlab = disk_p->disk_maxlab;
	}

	return(1);
} /* end set_disk_maxlab() */




/*
 *	Name:		set_disk_minlab()
 *
 *	Description:	The user wants to set the disk minimum label so
 *		go through all the checking.
 */

static int
set_disk_minlab(params)
	pointer		params[];
{
	disk_info *	disk_p;			/* pointer to disk_info */
	form *		form_p;			/* pointer to HOST form */
	int		i;			/* index variable */
	form_radio *	radio_p;		/* ptr to disk_???lab radio */


	form_p = (form *) params[0];
	disk_p = (disk_info *) params[1];

	/*
	 *	If this disk has root ("/") or user ("/usr") on it, then
	 *	its disk_minlab must be LAB_SYS_LOW.
	 */
	for (i = 0; i < NDKMAP; i++)
		if (disk_p->disk_minlab != LAB_SYS_LOW &&
		   (strcmp(disk_p->partitions[i].mount_pt, "/") == 0 ||
		    strcmp(disk_p->partitions[i].mount_pt, "/usr") == 0)) {
			radio_p = find_form_radio(form_p, "disk_minlab");

			off_form_radio(radio_p);
			disk_p->disk_minlab = LAB_SYS_LOW;
			on_form_radio(radio_p);

			menu_mesg("Disk minimum label must be system_low.");
			return(1);
		}

	/*
	 *	Maximum label is system_low so we cannot set minlab to
	 *	anything but system_low.
	 */
	if (disk_p->disk_maxlab == LAB_SYS_LOW &&
	    disk_p->disk_minlab != LAB_SYS_LOW) {
		radio_p = find_form_radio(form_p, "disk_minlab");

		off_form_radio(radio_p);
		disk_p->disk_minlab = LAB_SYS_LOW;
		on_form_radio(radio_p);

		menu_mesg("Disk minimum label must be system_low.");
	}

	/*
	 *	We set the minimum to system_high so the maximum must be set
	 *	to system_high.
	 */
	else if (disk_p->disk_minlab == LAB_SYS_HIGH) {
		radio_p = find_form_radio(form_p, "disk_maxlab");

		off_form_radio(radio_p);
		disk_p->disk_maxlab = LAB_SYS_HIGH;
		on_form_radio(radio_p);
	}

	/*
	 *	We do not turn off the MLS table here since it
	 *	should not be visable.
	 */
	for (i = 0; i < NDKMAP; i++) {
		if (disk_p->partitions[i].fs_minlab < disk_p->disk_minlab)
			disk_p->partitions[i].fs_minlab = disk_p->disk_minlab;

		if (disk_p->partitions[i].fs_maxlab < disk_p->disk_minlab)
			disk_p->partitions[i].fs_maxlab = disk_p->disk_minlab;
	}

	return(1);
} /* end set_disk_minlab() */




/*
 *	Name:		set_fs_maxlab()
 *
 *	Description:	The user wants to set the file system's maximum label
 *		so go through all the checking.
 */

static int
set_fs_maxlab(part_disp_p)
	part_disp *	part_disp_p;
{
	disk_info *	disk_p;			/* pointer to disk info */
	form *		form_p;			/* pointer to HOST form */
	char		name_buf[TINY_STR];	/* name buffer */
	form_radio *	radio_p;		/* ptr to disk_???lab radio */


	form_p = (form *) part_disp_p->disk_disp_p->params[0];
	disk_p = (disk_info *) part_disp_p->disk_disp_p->params[1];

	/*
	 *	This mount point's maximum label must be at system_high
	 *	and it is not so change it back.
	 */
	if (at_sys_high(part_disp_p->part_p->mount_pt) &&
	    part_disp_p->part_p->fs_maxlab != LAB_SYS_HIGH) {
		(void) sprintf(name_buf, "%c_maxlab", part_disp_p->name[0]);
		radio_p = find_form_radio(form_p, name_buf);

		off_form_radio(radio_p);
		part_disp_p->part_p->fs_maxlab = LAB_SYS_HIGH;
		on_form_radio(radio_p);
	}

	/*
	 *	The file system maximum label does not fall the range of the
	 *	disk label range so set to the disk_maxlab.
	 */
	else if (part_disp_p->part_p->fs_maxlab < disk_p->disk_minlab ||
	         part_disp_p->part_p->fs_maxlab > disk_p->disk_maxlab) {
		(void) sprintf(name_buf, "%c_maxlab", part_disp_p->name[0]);
		radio_p = find_form_radio(form_p, name_buf);

		off_form_radio(radio_p);
		part_disp_p->part_p->fs_maxlab = disk_p->disk_maxlab;
		on_form_radio(radio_p);
	}

	/*
	 *	Minimum label is system_high so we cannot set maxlab to
	 *	anything but system_high
	 */
	else if (part_disp_p->part_p->fs_minlab == LAB_SYS_HIGH &&
	         part_disp_p->part_p->fs_maxlab != LAB_SYS_HIGH) {
		(void) sprintf(name_buf, "%c_maxlab", part_disp_p->name[0]);
		radio_p = find_form_radio(form_p, name_buf);

		off_form_radio(radio_p);
		part_disp_p->part_p->fs_maxlab = LAB_SYS_HIGH;
		on_form_radio(radio_p);
	}

	/*
	 *	We set the maximum to system_low so the minimum must be set
	 *	to system_low.
	 */
	else if (part_disp_p->part_p->fs_maxlab == LAB_SYS_LOW) {
		(void) sprintf(name_buf, "%c_minlab", part_disp_p->name[0]);
		radio_p = find_form_radio(form_p, name_buf);

		off_form_radio(radio_p);
		part_disp_p->part_p->fs_minlab = LAB_SYS_LOW;
		on_form_radio(radio_p);
	}

	return(1);
} /* end set_fs_maxlab() */




/*
 *	Name:		set_fs_minlab()
 *
 *	Description:	The user wants to set the file system's minimum label
 *		so go through all the checking.
 */

static int
set_fs_minlab(part_disp_p)
	part_disp *	part_disp_p;
{
	disk_info *	disk_p;			/* pointer to disk info */
	form *		form_p;			/* pointer to HOST form */
	char		name_buf[TINY_STR];	/* name buffer */
	form_radio *	radio_p;		/* ptr to disk_???lab radio */


	form_p = (form *) part_disp_p->disk_disp_p->params[0];
	disk_p = (disk_info *) part_disp_p->disk_disp_p->params[1];

	/*
	 *	This mount point's minimum label must be at system_high
	 *	and it is not so change it back.
	 */
	if (at_sys_high(part_disp_p->part_p->mount_pt) &&
	    part_disp_p->part_p->fs_minlab != LAB_SYS_HIGH) {
		(void) sprintf(name_buf, "%c_minlab", part_disp_p->name[0]);
		radio_p = find_form_radio(form_p, name_buf);

		off_form_radio(radio_p);
		part_disp_p->part_p->fs_minlab = LAB_SYS_HIGH;
		on_form_radio(radio_p);
	}

	/*
	 *	The file system minimum label does not fall the range of the
	 *	disk label range so set to the disk_minlab.
	 */
	else if (part_disp_p->part_p->fs_minlab < disk_p->disk_minlab ||
	         part_disp_p->part_p->fs_minlab > disk_p->disk_maxlab) {
		(void) sprintf(name_buf, "%c_minlab", part_disp_p->name[0]);
		radio_p = find_form_radio(form_p, name_buf);

		off_form_radio(radio_p);
		part_disp_p->part_p->fs_minlab = disk_p->disk_minlab;
		on_form_radio(radio_p);
	}

	/*
	 *	Maximum label is system_low so we cannot set minlab to
	 *	anything but system_low.
	 */
	else if (part_disp_p->part_p->fs_maxlab == LAB_SYS_LOW &&
	         part_disp_p->part_p->fs_minlab != LAB_SYS_LOW) {
		(void) sprintf(name_buf, "%c_minlab", part_disp_p->name[0]);
		radio_p = find_form_radio(form_p, name_buf);

		off_form_radio(radio_p);
		part_disp_p->part_p->fs_minlab = LAB_SYS_LOW;
		on_form_radio(radio_p);
	}

	/*
	 *	We set the minimum to system_high so the maximum must be set
	 *	to system_high.
	 */
	else if (part_disp_p->part_p->fs_minlab == LAB_SYS_HIGH) {
		(void) sprintf(name_buf, "%c_maxlab", part_disp_p->name[0]);
		radio_p = find_form_radio(form_p, name_buf);

		off_form_radio(radio_p);
		part_disp_p->part_p->fs_maxlab = LAB_SYS_HIGH;
		on_form_radio(radio_p);
	}

	return(1);
} /* end set_fs_minlab() */
#endif /* SunB1 */




/*
 *	Name:		size_chk()
 *
 *	Description:	Check the partition size specified by the user.
 */

static int
size_chk(part_disp_p, data_p)
	part_disp *	part_disp_p;
	char *		data_p;
{
	daddr_t		blks;			/* new size in blocks */
	long		diff_blk;		/* difference in blocks */
	disk_info *	disk_p;			/* ptr for disk macros */
	form *		form_p;			/* ptr to DISK form */
	daddr_t		size;			/* new size */
	int		i,j;			/* index */

	extern	long		atol();


	if (ckf_long(PTR_NULL, data_p) == 0)
		return(0);

	form_p = (form *) part_disp_p->disk_disp_p->params[0];
	disk_p = (disk_info *) part_disp_p->disk_disp_p->params[1];

	size = atol(data_p);

	switch (disk_p->display_unit) {
	case DU_BLOCKS:
		blks = rnd_blocks(size, disk_p->geom_buf);
		break;

	case DU_BYTES:
		blks = bytes_to_blocks(size, disk_p->geom_buf);
		break;

	case DU_CYLINDERS:
		blks = cyls_to_blocks(size, disk_p->geom_buf);
		break;

	case DU_KBYTES:
		blks = Kb_to_blocks(size, disk_p->geom_buf);
		break;

	case DU_MBYTES:
		blks = Mb_to_blocks(DI_to_Real_Meg(size), disk_p->geom_buf);
		break;
	}

	/*
	 *	Check for changes in the string that is displayed.  This
	 *	prevents megabyte quantities from getting rounded incorrectly.
	 *	e.g. 40 Mb and 40.15 Mb are both on cylinder boundaries and
	 *	are displayed as "40".  We do not want an existing 40.15 to
	 *	get rounded to 40.  This means that if the user really wants
	 *	40 Mb, then she/he has to enter, say 41, and then enter 40.
	 */
	if (strcmp(data_p, blocks_to_str(disk_p,
					 map_blk(part_disp_p->name[0]))) == 0)
		return(1);

	/*
	 *	If this is the swap partition on a hot disk, then make
	 *	sure the size does not get below the original.
	 */
	if (disk_p->is_hot && part_disp_p->name[0] == 'b' &&
	    blks < disk_p->swap_size) {
		menu_mesg("%s\n%s",
			  "Cannot decrease swap size below original size.",
			  "Use format under MUNIX to lower swap size.");

		off_part_table(part_disp_p->disk_disp_p, form_p);
		(void) strcpy(data_p,
			      blocks_to_str(disk_p,
					    map_blk(part_disp_p->name[0])));
		on_part_table(disk_p, part_disp_p->disk_disp_p, form_p);

		return(0);
	}

						/* compute difference */
	diff_blk = blks - map_blk(part_disp_p->name[0]);

	/*
	 * Can't steal space from a preserved free hog
	 */ 
	if (disk_p->partitions[disk_p->free_hog - 'a'].preserve_flag == 'y'
	    && diff_blk){
		menu_mesg("Can't steal from a preserved free hog.");
		off_part_table(part_disp_p->disk_disp_p, form_p);

		(void) strcpy(data_p,
			      blocks_to_str(disk_p,
					    map_blk(part_disp_p->name[0])));

		on_part_table(disk_p, part_disp_p->disk_disp_p, form_p);
		return(0);
	}
	/*
	 * Can't steal from free hog if there is a preserved partition
	 * between this partition and the free hog
	 */
	i = part_disp_p->name[0] - 'a';
	j = disk_p->free_hog - 'a';
	if ( i > j) {
		i = j;
		j = part_disp_p->name[0] - 'a';
	}
	for ( ; i < j ; i++) {
		if (disk_p->partitions[i].preserve_flag != 'y')
			continue;
		menu_mesg("Using the free hog would modify preserved partition %c.",i + 'a');
		off_part_table(part_disp_p->disk_disp_p, form_p);

		(void) strcpy(data_p,
			      blocks_to_str(disk_p,
					    map_blk(part_disp_p->name[0])));

		on_part_table(disk_p, part_disp_p->disk_disp_p, form_p);
		return(0);
	}
	/*
	 * Can use the free hog so check if there is enough space
	 */
	if (diff_blk > map_blk(disk_p->free_hog)) {
		menu_mesg("Not enough space in free hog partition.");
		off_part_table(part_disp_p->disk_disp_p, form_p);

		(void) strcpy(data_p,
			      blocks_to_str(disk_p,
					    map_blk(part_disp_p->name[0])));

		on_part_table(disk_p, part_disp_p->disk_disp_p, form_p);
		return(0);
	}
		
	off_part_table(part_disp_p->disk_disp_p, form_p);


	map_blk(disk_p->free_hog) -= diff_blk;
	if (map_blk(disk_p->free_hog) == 0) {
		map_cyl(disk_p->free_hog) = 0;
		disk_p->partitions[disk_p->free_hog - 'a'].mount_pt[0] = NULL;
	}

	map_blk(part_disp_p->name[0]) = blks;
	if (blks == 0)
		map_cyl(part_disp_p->name[0]) = 0;

	on_part_table(disk_p, part_disp_p->disk_disp_p, form_p);

	return(1);
} /* end size_chk() */




/*
 *	Name:		size_pre_chk()
 *
 *	Description:	Pre-check function for the size field.  This routine
 *		determines if the size field can be modified.
 */

static int
size_pre_chk(part_disp_p)
	part_disp *	part_disp_p;
{
	disk_info *	disk_p;			/* ptr to disk info */
	part_info *	part_p;			/* ptr to part info */


	disk_p = (disk_info *) part_disp_p->disk_disp_p->params[1];
	part_p =  part_disp_p->part_p;

	/*
	 *	Do not allow the size field to be changed if:
	 *
	 *		1) this is the 'c' partition
	 *		2) this is the free hog partition
	 *		3) we are using the existing label
	 *		4) this is the 'a' partition on the disk we are
	 *		   booted on.
	 *		5) The partition is preserved
	 */
	if (part_disp_p->name[0] == 'c' ||
	    part_disp_p->name[0] == disk_p->free_hog ||
	    disk_p->label_source == DKL_EXISTING ||
	    part_p->preserve_flag == 'y' ||
	    (part_disp_p->name[0] == 'a' && disk_p->is_hot))
		return(menu_ignore_obj());

	return(1);
} /* end size_pre_chk() */




/*
 *	Name:		update_units()
 *
 *	Description:	Update the display units shown on the menu.  This
 *		routine is typically called when the user has changed the
 *		display units.
 */

static int
update_units(params)
	pointer		params[];
{
	char		buf[TINY_STR];		/* buffer for field's name */
	disk_info *	disk_p;			/* ptr to disk information */
	form *		form_p;			/* ptr to form */
	int		i;			/* index variable */


	form_p = (form *) params[0];
	disk_p = (disk_info *) params[1];

	for (i = 0; i < NDKMAP; i++) {
		(void) sprintf(buf, "%c_size", i + 'a');
		clear_form_field(find_form_field(form_p, buf));

		(void) strcpy(disk_p->partitions[i].size_str,
                              blocks_to_str(disk_p, map_blk(i + 'a')));

		disk_p->partitions[i].avail_bytes =
					     blocks_to_bytes(map_blk(i + 'a'));

		display_form_field(find_form_field(form_p, buf));
	}

	return(1);
} /* end update_units() */




/*
 *	Name:		use_disk()
 *
 *	Description:	Select a particular disk for configuration.
 */

static int
use_disk(params)
	pointer		params[];
{
	disk_info *	disk_p;			/* ptr to disk information */
	disk_disp *	disp_p;			/* ptr to disk display */
	form *		form_p;			/* ptr to form */
	form_radio *	radio_p;		/* ptr to radio panel */
	int		ret_code;		/* return code */


	form_p = (form *) params[0];
	disk_p = (disk_info *) params[1];
	disp_p = (disk_disp *) params[2];

	radio_p = find_form_radio(form_p, "select");

	/*
	 *	Extract the disk name from the string on the screen
	 */
	(void) strcpy(disk_p->disk_name,
		      &radio_p->fr_pressed->fb_mstrings->ms_data[1]);
	disk_p->disk_name[strlen(disk_p->disk_name) - 1] = '\0';

	ret_code = get_disk_config(disk_p);
	if (ret_code != 1)
		return(ret_code);

	on_disk_radios(disk_p, disp_p, form_p);
	return(1);
} /* end use_disk() */
