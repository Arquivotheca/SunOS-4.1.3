#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)sec_form.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)sec_form.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		sec_form.c
 *
 *	Description:	This file contains all the routines for dealing
 *		with a security form.
 */

#include <sys/types.h>
#include <sys/label.h>
#include <sys/audit.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include "sec_impl.h"




/*
 *	External functions:
 */
extern	char *		_crypt();
extern	char *		sprintf();




/*
 *	Local functions:
 */
static	int		chk_audit_word();
static	int		chk_flags();
static	int		chk_root_word();
static	int		dir_trap();
static	void		init_dir_ptrs();
static	int		on_audit_ck();
static	int		on_root_ck();




/*
 *	Name:		create_sec_form()
 *
 *	Description:	Create the SECURITY form.  Uses information in
 *		'dir_ptrs' and 'sec_p' to create the form.
 */

form *
create_sec_form(dir_ptrs, sec_p)
	pointer		dir_ptrs[MAX_AUDIT_DIRS][2];
	sec_info *	sec_p;
{
	int		count;			/* audit directory count */
	int		dummy;			/* dummy active state */
	form_field *	field_p;		/* ptr to form field */
	form *		form_p;			/* ptr to SECURITY form */
	form_noecho *	noecho_p;		/* ptr to form noecho */
	menu_coord	x, y;			/* screen coordinates */

	static	pointer		params[2];	/* pointer to parameters */


        init_menu();

	form_p = create_form(PFI_NULL, ACTIVE, "SECURITY FORM");

	params[0] = (pointer) form_p;
	params[1] = (pointer) sec_p;

	init_dir_ptrs(params, dir_ptrs);


	/*
	 *	Hostname field:
	 */
	y = 3;
	x = 12;
	field_p = add_form_field(form_p, PFI_NULL, ACTIVE, CP_NULL, y, x, 0,
				 sec_p->hostname, sizeof(sec_p->hostname),
				 PFI_NULL, menu_skip_io, PTR_NULL,
				 PFI_NULL, PTR_NULL);
	(void) add_menu_string((pointer) field_p, ACTIVE, y, 1,
			       ATTR_NORMAL, "Hostname :");

	/*
	 *	Root password object and confirmer
	 */
	y += 2;
	x = 19;
	noecho_p = add_form_noecho(form_p, PFI_NULL, ACTIVE, "root_word", y, x,
				   sec_p->root_word, sizeof(sec_p->root_word),
				   PFI_NULL, PTR_NULL,
				   on_root_ck, (pointer) form_p);
	(void) add_menu_string((pointer) noecho_p, ACTIVE, y, 1,
			       ATTR_NORMAL, "Root password  :");

	x = 40;
	noecho_p = add_form_noecho(form_p, PFI_NULL, NOT_ACTIVE, "root_ck",
				   y, x + 8,
				   sec_p->root_ck, sizeof(sec_p->root_ck),
				   PFI_NULL, PTR_NULL,
				   chk_root_word, (pointer) params);
	(void) add_menu_string((pointer) noecho_p, NOT_ACTIVE, y, x,
			       ATTR_NORMAL, "Again :");

	/*
	 *	Audit password object and confirmer
	 */
	y++;
	x = 19;
	noecho_p = add_form_noecho(form_p, PFI_NULL, ACTIVE, "audit_word",
				   y, x, sec_p->audit_word, 
				   sizeof(sec_p->audit_word),
				   PFI_NULL, PTR_NULL,
				   on_audit_ck, (pointer) form_p);
	(void) add_menu_string((pointer) noecho_p, ACTIVE, y, 1,
			       ATTR_NORMAL, "Audit password :");

	x = 40;
	noecho_p = add_form_noecho(form_p, PFI_NULL, NOT_ACTIVE, "audit_ck",
				   y, x + 8,
				   sec_p->audit_ck, sizeof(sec_p->audit_ck),
				   PFI_NULL, PTR_NULL,
				   chk_audit_word, (pointer) params);
	(void) add_menu_string((pointer) noecho_p, NOT_ACTIVE, y, x,
			       ATTR_NORMAL, "Again :");


	/*
	 *	Audit flags:
	 */
	y += 2;
	x = 21;
	field_p = add_form_field(form_p, PFI_NULL, ACTIVE, CP_NULL, y, x, 0,
				 sec_p->audit_flags,
				 sizeof(sec_p->audit_flags), lex_no_ws,
				 PFI_NULL, PTR_NULL, chk_flags, PTR_NULL);
	(void) add_menu_string((pointer) field_p, ACTIVE, y, 1,
			       ATTR_NORMAL, "Audit flags      :");

	/*
	 *	Audit min free:
	 */
	y++;
	field_p = add_form_field(form_p, PFI_NULL,
				 ACTIVE, CP_NULL, y, x, 0,
				 sec_p->audit_min, sizeof(sec_p->audit_min),
				 lex_no_ws, PFI_NULL, PTR_NULL,
				 ckf_uint, PTR_NULL);
	(void) add_menu_string((pointer) field_p, ACTIVE, y, 1,
			       ATTR_NORMAL, "Audit min-free % :");

	/*
	 *	Audit directory(ies):
	 */
	y++;
	count = 0;
	field_p = add_form_field(form_p, PFI_NULL,
				 ACTIVE, (char *) dir_ptrs[0][0], y, x, 0,
				 sec_p->audit_dirs[count],
				 sizeof(sec_p->audit_dirs[count]),
				 lex_no_ws, PFI_NULL, PTR_NULL,
				 dir_trap, (pointer) dir_ptrs[0]);
	(void) add_menu_string((pointer) field_p, ACTIVE, y, 1,
			       ATTR_NORMAL, "Audit directory  :");

	for (count = 1; count < MAX_AUDIT_DIRS; count++) {
		y++;
		dummy = (sec_p->audit_dirs[count][0]) ? ACTIVE : NOT_ACTIVE;

		field_p = add_form_field(form_p, PFI_NULL, dummy,
					 (char *) dir_ptrs[count][0], y, x, 0,
				         sec_p->audit_dirs[count],
				         sizeof(sec_p->audit_dirs[count]),
				         lex_no_ws, PFI_NULL, PTR_NULL,
					 dir_trap, (pointer) dir_ptrs[count]);
		(void) add_menu_string((pointer) field_p, dummy, y, 1,
				       ATTR_NORMAL, "Audit directory  :");
	}

	/*
	 *	Finish object
	 */
	(void) add_finish_obj((pointer) form_p,
			      PFI_NULL, PTR_NULL,
			      PFI_NULL, PTR_NULL,
			      PFI_NULL, PTR_NULL);

	return(form_p);
} /* end create_sec_form() */




/*
 *	Name:		chk_audit_word()
 *
 *	Description:	Check the audit password.  Enables a second noecho
 *		object to ask for the password again.  Returns 1 if the
 *		passwords match and 0 otherwise.
 */

static	int
chk_audit_word(params, data_p)
	pointer		params[];
	char *		data_p;
{
	form *		form_p;			/* ptr to SECURITY form */
	sec_info *	sec_p;			/* ptr to security info */
						 /* password check buffer */


	form_p = (form *) params[0];
	sec_p = (sec_info *) params[1];

	if (ckf_empty((pointer) params, data_p) != 1)
		return(0);

	off_form_noecho(find_form_noecho(form_p, "audit_ck"));

	(void) strcpy(data_p, _crypt(data_p, SALT));

	if (strcmp(sec_p->audit_word, sec_p->audit_ck) != 0) {
		bzero(sec_p->audit_word, sizeof(sec_p->audit_word));
		menu_mesg("Passwords do not match.  Try again.");

		set_form_map(form_p, (pointer) find_form_noecho(form_p, 
								"audit_word"));
		return(menu_goto_obj());
	}

	return(1);
} /* end chk_audit_word() */




/*
 *	Name:		chk_flags()
 *
 *	Description:	Check the validity of the audit flags.  Returns 1
 *		if the audit flags are valid and 0 otherwise.
 */

static	int
chk_flags(params, data_p)
	pointer		params;
	char *		data_p;
{
	audit_state_t	dummy;			/* dummy audit state */


#ifdef lint
	params = params;
#endif

	bzero((char *) &dummy, sizeof(dummy));
	if (getauditflagsbin(data_p, &dummy) != 0) {
		menu_mesg("Field is not a valid audit flags string.");
		return(0);
	}

	return(1);
} /* end chk_flags() */




/*
 *	Name:		chk_root_word()
 *
 *	Description:	Check the root password.  Enables a second noecho
 *		object to ask for the password again.  Returns 1 if the
 *		passwords match and 0 otherwise.
 */

static	int
chk_root_word(params, data_p)
	pointer		params[];
	char *		data_p;
{
	form *		form_p;			/* ptr to SECURITY form */
	sec_info *	sec_p;			/* ptr to security info */


	form_p = (form *) params[0];
	sec_p = (sec_info *) params[1];

	if (ckf_empty((pointer) params, data_p) != 1)
		return(0);

	off_form_noecho(find_form_noecho(form_p, "root_ck"));

	(void) strcpy(data_p, _crypt(data_p, SALT));

	if (strcmp(sec_p->root_word, sec_p->root_ck) != 0) {
		bzero(sec_p->root_word, sizeof(sec_p->root_word));
		menu_mesg("Passwords do not match.  Try again.");

		set_form_map(form_p, (pointer) find_form_noecho(form_p,
								"root_word"));
		return(menu_goto_obj());
	}

	return(1);
} /* end chk_root_word() */




/*
 *	Name:		dir_trap()
 *
 *	Description:	Trap routine for audit directories.  This routine
 *		determines if the audit directory is valid and enables the
 *		next empty slot if one is available.  An audit directory
 *		must be an absolute network path or an absolute path.
 *		Returns 1 if the audit directory is valid and 0 otherwise.
 */

static	int
dir_trap(params, data_p)
	pointer		params[];
	char *		data_p;
{
	char		buf[TINY_STR];		/* buffer for obj name */
	char *		cp;			/* scratch char pointer */
	form *		form_p;			/* ptr to SECURITY form */
	int		len;			/* scratch length */
	int		num;			/* number of this dir object */


	cp = (char *) params[0];
	form_p = (form *) ((pointer *) params[1])[0];

	/*
	 *	Use the name to determine which dir object we are.
	 */
	(void) sscanf(cp, DIR_FMT, &num);

	/*
	 *	If this is not the first directory object and the field
	 *	is empty, then turn off the field and say we are done.
	 */
	if (num > 0 && strlen(data_p) == 0) {
		off_form_field(find_form_field(form_p, cp));
		return(1);
	}

	if (ckf_netabs((pointer) params[1], data_p) != 1)
		return(0);

	/*
	 *	Skip past any network part of the path.
	 */
	cp = strchr(data_p, ':');
	if (cp == NULL)
		cp = data_p;
	else
		cp++;

	/*
	 *	Make sure the path has AUDIT_DIR as its prefix.
	 */
	len = strlen(AUDIT_DIR);
	if (strncmp(cp, AUDIT_DIR, len) != 0 || cp[len] != '/' ||
	    cp[len + 1] == NULL) {
		menu_mesg("Audit directories must be in %s/...", AUDIT_DIR);
		return(0);
	}

	/*
	 *	If we are here because of a menu forward or menu backward
	 *	command, do not do anything but say everything is ok.
	 */
	if (_menu_forward || _menu_backward)
		return(1);

	/*
	 *	Turn on the next field.
	 */
	if (num < MAX_AUDIT_DIRS - 1) {
		(void) sprintf(buf, DIR_FMT, num + 1);
		on_form_field(find_form_field(form_p, buf));
	}

	return(1);
} /* end dir_trap() */




/*
 *	Name:		init_dir_ptrs()
 *
 *	Description:	Initialize the pointers for the audit directory objs.
 */

static	void
init_dir_ptrs(params, dir_ptrs)
	pointer		params[];
	pointer		dir_ptrs[MAX_AUDIT_DIRS][2];
{
	char		buf[TINY_STR];		/* buffer for the name */
	int		i;			/* index variable */


	for (i = 0; i < MAX_AUDIT_DIRS; i++) {
		(void) sprintf(buf, DIR_FMT, i);
		dir_ptrs[i][0] = (pointer) strdup(buf);
		dir_ptrs[i][1] = (pointer) params;

		if (dir_ptrs[i][0] == NULL) {
			menu_log("%s: cannot allocate memory for pointers.",
				 progname);
			menu_abort(1);
		}
	}
} /* end init_dir_ptrs() */




/*
 *	Name:		on_audit_ck()
 *
 *	Description:	If a valid audit password was entered, then turn
 *		on the audit password confirmer object.
 */

static	int
on_audit_ck(form_p, data_p)
	form *		form_p;
	char *		data_p;
{
	if (ckf_empty(PTR_NULL, data_p) != 1)
		return(0);

	(void) strcpy(data_p, _crypt(data_p, SALT));
	on_form_noecho(find_form_noecho(form_p, "audit_ck"));

	return(1);
} /* end on_audit_ck() */




/*
 *	Name:		on_root_ck()
 *
 *	Description:	If a valid root password was entered, then turn
 *		on the root password confirmer object.
 */

static	int
on_root_ck(form_p, data_p)
	form *		form_p;
	char *		data_p;
{
	if (ckf_empty(PTR_NULL, data_p) != 1)
		return(0);

	(void) strcpy(data_p, _crypt(data_p, SALT));
	on_form_noecho(find_form_noecho(form_p, "root_ck"));

	return(1);
} /* end on_root_ck() */
