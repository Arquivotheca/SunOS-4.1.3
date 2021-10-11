#ifndef lint
#ifdef SunB1
#ident			"@(#)host_form.c 1.1 92/07/30 SMI; SunOS MLS";
#else
#ident			"@(#)host_form.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1990 Sun Microsystems, Inc.
 */

/*
 *	Name:		host_form.c
 *
 *	Description:	This file contains all the routines for dealing
 *		with a host form.
 *
 *		This implementation assumes no more than two ethernet
 *		interfaces.  This is demonstrably wrong and should be fixed.
 */

#include "host_impl.h"


/*
 *	Local functions:
 */
static	int		ether();
static	int		inet_addr();

#ifdef SunB1
static	int		set_ip0_maxlab();
static	int		set_ip0_minlab();
static	int		set_ip1_maxlab();
static	int		set_ip1_minlab();
#endif /* SunB1 */

static	int		sys_dataless();
static	int		sys_not_dataless();
static int yp_sel();

static int last_ether;


/*
 *	External functions:
 */
extern	char *		sprintf();


/*
 *	Name:		create_host_form()
 *
 *	Description:	Create a HOST form.  Uses information in 'sys_p'
 *		to create the form.
 */

form *
create_host_form(sys_p)
	sys_info *	sys_p;
{
	form_button *	button_p;		/* ptr to button */
	int		dummy;			/* dummy parameter */
	form_field *	field_p;		/* ptr to field */
	form *		form_p;			/* pointer to HOST form */
	form_radio *	radio_p;		/* ptr to radio panel */
	menu_coord	x, y;			/* scratch menu coordinates */
	int		i;			/* scratch var */
	static	pointer		params[2];	/* params for menu functions */


        init_menu();

	form_p = create_form(PFI_NULL, ACTIVE, "HOST FORM");

	params[0] = (pointer) form_p;
	params[1] = (pointer) sys_p;

	/*
	 *	Workstation information section.  This section starts on
	 *	line 'y'.  The objects in this section line up on 'x'.
	 */
	y = 2;
	x = 6;
	(void) add_menu_string((pointer) form_p, ACTIVE, y, 1,
			       ATTR_NORMAL, "Workstation Information :");

	y++;
	field_p = add_form_field(form_p, PFI_NULL,
				 ACTIVE, CP_NULL, y, x + 7, 0,
				 sys_p->hostname0, sizeof(sys_p->hostname0),
				 lex_no_ws, PFI_NULL, PTR_NULL,
				 ckf_empty, PTR_NULL);
	(void) add_menu_string((pointer) field_p, ACTIVE, y, x,
			       ATTR_NORMAL, "Name :");

	y++;
	radio_p = add_form_radio(form_p, PFI_NULL,
				 ACTIVE, CP_NULL, &sys_p->sys_type,
				 PFI_NULL, PTR_NULL,
				 PFI_NULL, PTR_NULL);
	(void) add_menu_string((pointer) radio_p, ACTIVE, y, x,
			       ATTR_NORMAL, "Type :");
	x += 7;
	button_p = add_form_button(radio_p, PFI_NULL,
				   ACTIVE, CP_NULL,
				   y, x, SYS_STANDALONE,
				   sys_not_dataless, (pointer) form_p);
	(void) add_menu_string((pointer) button_p, ACTIVE, y, x + 1,
			       ATTR_NORMAL, "[standalone]");
	x += 14;
	button_p = add_form_button(radio_p, PFI_NULL,
				   ACTIVE, CP_NULL,
				   y, x, SYS_SERVER,
				   sys_not_dataless, (pointer) form_p);
	(void) add_menu_string((pointer) button_p, ACTIVE, y, x + 1,
			       ATTR_NORMAL, "[server]");
	x += 10;
	button_p = add_form_button(radio_p, PFI_NULL,
				   ACTIVE, CP_NULL,
				   y, x, SYS_DATALESS,
				   sys_dataless, (pointer) params);
	(void) add_menu_string((pointer) button_p, ACTIVE, y, x + 1,
			       ATTR_NORMAL, "[dataless]");
	x = 6;					/* restore column line up */

	y++;
	if (sys_p->sys_type == SYS_DATALESS)
		dummy = ACTIVE;
	else
		dummy = NOT_ACTIVE;
	field_p = add_form_field(form_p, PFI_NULL,
				 dummy, "serv", y, x + 14, 0,
				 sys_p->server, sizeof(sys_p->server),
				 lex_no_ws, PFI_NULL, PTR_NULL,
				 ckf_empty, PTR_NULL);
	(void) add_menu_string((pointer) field_p, dummy, y, x,
			       ATTR_NORMAL, "Server name :");

	y++;
	field_p = add_form_field(form_p, PFI_NULL,
				 dummy, "svip", y, x + 26, 0,
				 sys_p->server_ip, sizeof(sys_p->server_ip),
				 lex_no_ws, PFI_NULL, PTR_NULL,
				 ckf_inet_addr, PTR_NULL);
	(void) add_menu_string((pointer) field_p, dummy, y, x,
			       ATTR_NORMAL, "Server Internet Address :");

	y++;
	field_p = add_form_field(form_p, PFI_NULL,
				 dummy, "exec", y, x + 36, 0,
				 sys_p->exec_path,
				 sizeof(sys_p->exec_path),
				 lex_no_ws, PFI_NULL, PTR_NULL,
				 ckf_empty, PTR_NULL);
	(void) add_menu_string((pointer) field_p, dummy, y, x,
			       ATTR_NORMAL,
			       "Path of the executables on server :");

	y++;
	field_p = add_form_field(form_p, PFI_NULL,
				 dummy, "kvm", y, x + 43, 0,
				 sys_p->kvm_path,
				 sizeof(sys_p->kvm_path),
				 lex_no_ws, PFI_NULL, PTR_NULL,
				 ckf_empty, PTR_NULL);
	(void) add_menu_string((pointer) field_p, dummy, y, x,
			       ATTR_NORMAL,
			       "Path of the kernel executables on server :");

	/*
	 *	Network information section.  This section starts on
	 *	line 'y'.  The objects in this section line up on 'x'.
	 */
#ifdef SunB1
	y++;
#else
	y += 2;
#endif /* SunB1 */
	x = 6;
	(void) add_menu_string((pointer) form_p, ACTIVE, y, 1,
			       ATTR_NORMAL, "Network Information :");

	y++;
	radio_p = add_form_radio(form_p, PFI_NULL,
				 ACTIVE, "ether_select", &sys_p->ether_type,
				 PFI_NULL, PTR_NULL,
				 ether, (pointer) params);
	(void) add_menu_string((pointer) radio_p, ACTIVE, y, x,
			       ATTR_NORMAL, "Ethernet Interface :");
	x += 21;
	button_p = add_form_button(radio_p, PFI_NULL,
				   ACTIVE, CP_NULL,
				   y, x, ETHER_NONE,
				   PFI_NULL, PTR_NULL);
	(void) add_menu_string((pointer) button_p, ACTIVE, y, x + 1,
			       ATTR_NORMAL, "[none]");
	x += 8;
	last_ether = 0;
	for (i = 0; i < MAX_ETHER_INTERFACES; i++) {
		if (*sys_p->ether_namex(i)) {
			char *buf = (char *) menu_alloc(10);

			last_ether = i;
			dummy = sys_p->ether_type == ETHER_0+i ? ACTIVE : NOT_ACTIVE;

			button_p = add_form_button(radio_p, PFI_NULL,
				   ACTIVE, CP_NULL,
				   y, x, ETHER_0+i,
				   PFI_NULL, PTR_NULL);
			(void) sprintf(buf, "[%s]", sys_p->ether_namex(i));
			(void) add_menu_string((pointer) button_p, ACTIVE, y, x + 1,
			       ATTR_NORMAL, buf);
			x += strlen(buf) + 2;
		}
	}
	y++;
	field_p = add_form_field(form_p, PFI_NULL, NOT_ACTIVE,
		"iname", y, 27, 0,
		 sys_p->hostnamex(i),
		 sizeof(sys_p->hostnamex(i)),
		 lex_no_ws, PFI_NULL, PTR_NULL,
		 ckf_empty, PTR_NULL);
	(void) add_menu_string((pointer) field_p, NOT_ACTIVE, y, 6,
	       ATTR_NORMAL, "Internet Name      :");

	y++;
	field_p = add_form_field(form_p, PFI_NULL, NOT_ACTIVE,
		"iaddr", y, 27, 0,
		sys_p->ipx(i),
		sizeof(sys_p->ipx(i)),
		lex_no_ws, PFI_NULL, PTR_NULL,
		inet_addr, (pointer) params);

	(void) add_menu_string((pointer) field_p, NOT_ACTIVE, y, 6,
	       ATTR_NORMAL, "Internet Address   :");

#ifdef SunB1
	y++;
	radio_p = add_form_radio(form_p, PFI_NULL,
				 dummy, "ip0_minlab", &sys_p->ip0_minlab,
				 PFI_NULL, PTR_NULL,
				 PFI_NULL, PTR_NULL);
	(void) add_menu_string((pointer) radio_p, dummy, y, x,
			       ATTR_NORMAL, "IP 0 Minimum Label :");
	x += 21;
	button_p = add_form_button(radio_p, PFI_NULL,
				   dummy, CP_NULL,
				   y, x, LAB_SYS_LOW,
				   set_ip0_minlab, (pointer) params);
	(void) add_menu_string((pointer) button_p, dummy, y, x + 1,
			       ATTR_NORMAL, "[system_low]");
	x += 14;
	button_p = add_form_button(radio_p, PFI_NULL,
				   dummy, CP_NULL,
				   y, x, LAB_SYS_HIGH,
				   set_ip0_minlab, (pointer) params);
	(void) add_menu_string((pointer) button_p, dummy, y, x + 1,
			       ATTR_NORMAL, "[system_high]");
	x += 15;
	button_p = add_form_button(radio_p, PFI_NULL,
				   dummy, CP_NULL,
				   y, x, LAB_OTHER,
				   set_ip0_minlab, (pointer) params);
	(void) add_menu_string((pointer) button_p, dummy, y, x + 1,
			       ATTR_NORMAL, "[other]");
	x = 6;					/* restore column line up */

	y++;
	radio_p = add_form_radio(form_p, PFI_NULL,
				 dummy, "ip0_maxlab", &sys_p->ip0_maxlab,
				 PFI_NULL, PTR_NULL,
				 PFI_NULL, PTR_NULL);
	(void) add_menu_string((pointer) radio_p, dummy, y, x,
			       ATTR_NORMAL, "IP 0 Maximum Label :");
	x += 21;
	button_p = add_form_button(radio_p, PFI_NULL,
				   dummy, CP_NULL,
				   y, x, LAB_SYS_LOW,
				   set_ip0_maxlab, (pointer) params);
	(void) add_menu_string((pointer) button_p, dummy, y, x + 1,
			       ATTR_NORMAL, "[system_low]");
	x += 14;
	button_p = add_form_button(radio_p, PFI_NULL,
				   dummy, CP_NULL,
				   y, x, LAB_SYS_HIGH,
				   set_ip0_maxlab, (pointer) params);
	(void) add_menu_string((pointer) button_p, dummy, y, x + 1,
			       ATTR_NORMAL, "[system_high]");
	x += 15;
	button_p = add_form_button(radio_p, PFI_NULL,
				   dummy, CP_NULL,
				   y, x, LAB_OTHER,
				   set_ip0_maxlab, (pointer) params);
	(void) add_menu_string((pointer) button_p, dummy, y, x + 1,
			       ATTR_NORMAL, "[other]");
	x = 6;					/* restore column line up */
#endif /* SunB1 */

#ifdef SunB1
	y++;
	radio_p = add_form_radio(form_p, PFI_NULL,
				 dummy, "ip1_minlab", &sys_p->ip1_minlab,
				 PFI_NULL, PTR_NULL,
				 PFI_NULL, PTR_NULL);
	(void) add_menu_string((pointer) radio_p, dummy, y, x,
			       ATTR_NORMAL, "IP 1 Minimum Label :");
	x += 21;
	button_p = add_form_button(radio_p, PFI_NULL,
				   dummy, CP_NULL,
				   y, x, LAB_SYS_LOW,
				   set_ip1_minlab, (pointer) params);
	(void) add_menu_string((pointer) button_p, dummy, y, x + 1,
			       ATTR_NORMAL, "[system_low]");
	x += 14;
	button_p = add_form_button(radio_p, PFI_NULL,
				   dummy, CP_NULL,
				   y, x, LAB_SYS_HIGH,
				   set_ip1_minlab, (pointer) params);
	(void) add_menu_string((pointer) button_p, dummy, y, x + 1,
			       ATTR_NORMAL, "[system_high]");
	x += 15;
	button_p = add_form_button(radio_p, PFI_NULL,
				   dummy, CP_NULL,
				   y, x, LAB_OTHER,
				   set_ip1_minlab, (pointer) params);
	(void) add_menu_string((pointer) button_p, dummy, y, x + 1,
			       ATTR_NORMAL, "[other]");
	x = 6;					/* restore column line up */

	y++;
	radio_p = add_form_radio(form_p, PFI_NULL,
				 dummy, "ip1_maxlab", &sys_p->ip1_maxlab,
				 PFI_NULL, PTR_NULL,
				 PFI_NULL, PTR_NULL);
	(void) add_menu_string((pointer) radio_p, dummy, y, x,
			       ATTR_NORMAL, "IP 1 Maximum Label :");
	x += 21;
	button_p = add_form_button(radio_p, PFI_NULL,
				   dummy, CP_NULL,
				   y, x, LAB_SYS_LOW,
				   set_ip1_maxlab, (pointer) params);
	(void) add_menu_string((pointer) button_p, dummy, y, x + 1,
			       ATTR_NORMAL, "[system_low]");
	x += 14;
	button_p = add_form_button(radio_p, PFI_NULL,
				   dummy, CP_NULL,
				   y, x, LAB_SYS_HIGH,
				   set_ip1_maxlab, (pointer) params);
	(void) add_menu_string((pointer) button_p, dummy, y, x + 1,
			       ATTR_NORMAL, "[system_high]");
	x += 15;
	button_p = add_form_button(radio_p, PFI_NULL,
				   dummy, CP_NULL,
				   y, x, LAB_OTHER,
				   set_ip1_maxlab, (pointer) params);
	(void) add_menu_string((pointer) button_p, dummy, y, x + 1,
			       ATTR_NORMAL, "[other]");
	x = 6;					/* restore column line up */
#endif /* SunB1 */

	y++;
	x = 6;
	dummy = sys_p->ether_type == ETHER_NONE ? NOT_ACTIVE : ACTIVE;

	radio_p = add_form_radio(form_p, PFI_NULL,
				 dummy, "yp", &sys_p->yp_type,
				 PFI_NULL, PTR_NULL,
				 yp_sel, (pointer) params);
	(void) add_menu_string((pointer) radio_p, dummy, y, x,
			       ATTR_NORMAL, "NIS Type           :");
	x += 21;
	button_p = add_form_button(radio_p, PFI_NULL,
				   dummy, CP_NULL,
				   y, x, YP_NONE,
				   PFI_NULL, PTR_NULL);
	(void) add_menu_string((pointer) button_p, dummy, y, x + 1,
			       ATTR_NORMAL, "[none]");
	x += 8;
	button_p = add_form_button(radio_p, PFI_NULL,
				   dummy, CP_NULL,
				   y, x, YP_MASTER,
				   PFI_NULL, PTR_NULL);
	(void) add_menu_string((pointer) button_p, dummy, y, x + 1,
			       ATTR_NORMAL, "[master]");
	x += 10;
	button_p = add_form_button(radio_p, PFI_NULL,
				   dummy, CP_NULL,
				   y, x, YP_SLAVE,
				   PFI_NULL, PTR_NULL);
	(void) add_menu_string((pointer) button_p, dummy, y, x + 1,
			       ATTR_NORMAL, "[slave]");
	x += 9;
	button_p = add_form_button(radio_p, PFI_NULL,
				   dummy, CP_NULL,
				   y, x, YP_CLIENT,
				   PFI_NULL, PTR_NULL);
	(void) add_menu_string((pointer) button_p, dummy, y, x + 1,
			       ATTR_NORMAL, "[client]");
	x = 6;					/* restore column line up */

	y++;

	dummy = dummy && sys_p->yp_type != YP_NONE ? ACTIVE : NOT_ACTIVE;

	field_p = add_form_field(form_p, PFI_NULL,
				 dummy, "domain", y, x + 21, 0,
				 sys_p->domainname, sizeof(sys_p->domainname),
				 lex_no_ws, PFI_NULL, PTR_NULL,
				 ckf_empty, PTR_NULL);
	(void) add_menu_string((pointer) field_p, dummy, y, x,
			        ATTR_NORMAL, "Domain name        :");

	ether(params);

	/*
	 *	Miscellaneous information section.  This section starts on
	 *	line 'y'.  The objects in this section line up on 'x'.
	 */
	y += 2;
	x = 6;
	(void) add_menu_string((pointer) form_p, ACTIVE, y, 1,
			       ATTR_NORMAL, "Misc Information :");

#ifdef UPGRADE_FLAG
	y++;
	radio_p = add_form_radio(form_p, PFI_NULL,
				 ACTIVE, CP_NULL, &sys_p->op_type,
				 PFI_NULL, PTR_NULL,
				 PFI_NULL, PTR_NULL);
	(void) add_menu_string((pointer) radio_p, ACTIVE, y, x,
			       ATTR_NORMAL, "Operation type                :");
	x += 32;
	button_p = add_form_button(radio_p, PFI_NULL,
				   ACTIVE, CP_NULL,
				   y, x, OP_INSTALL,
				   PFI_NULL, PTR_NULL);
	(void) add_menu_string((pointer) button_p, ACTIVE, y, x + 1,
			       ATTR_NORMAL, "[install]");
	x += 11;
	button_p = add_form_button(radio_p, PFI_NULL,
				   ACTIVE, CP_NULL,
				   y, x, OP_UPGRADE,
				   PFI_NULL, PTR_NULL);
	(void) add_menu_string((pointer) button_p, ACTIVE, y, x + 1,
			       ATTR_NORMAL, "[upgrade]");

	x = 6;					/* restore column line up */

#endif UPGRADE_FLAG

	y++;
	radio_p = add_form_radio(form_p, PFI_NULL,
				 ACTIVE, CP_NULL, &sys_p->reboot,
				 PFI_NULL, PTR_NULL,
				 PFI_NULL, PTR_NULL);
	(void) add_menu_string((pointer) radio_p, ACTIVE, y, x,
			       ATTR_NORMAL, "Reboot after completed        :");
	x += 32;
	button_p = add_form_button(radio_p, PFI_NULL,
				   ACTIVE, CP_NULL,
				   y, x, ANS_YES,
				   PFI_NULL, PTR_NULL);
	(void) add_menu_string((pointer) button_p, ACTIVE, y, x + 1,
			       ATTR_NORMAL, "[y]");
	x += 5;
	button_p = add_form_button(radio_p, PFI_NULL,
				   ACTIVE, CP_NULL,
				   y, x, ANS_NO,
				   PFI_NULL, PTR_NULL);
	(void) add_menu_string((pointer) button_p, ACTIVE, y, x + 1,
			       ATTR_NORMAL, "[n]");

	(void) add_finish_obj((pointer) form_p,
			      PFI_NULL, PTR_NULL,
		       	      PFI_NULL, PTR_NULL,
		       	      PFI_NULL, PTR_NULL);

	return(form_p);
} /* end create_host_form() */

static int
inet_addr(params, field_p)
	pointer params[2];
	char* field_p;
{
	form *form_p;
	sys_info *sys_p;

	form_p = (form*) params[0];
	sys_p = (sys_info*) params[1];

	if (ckf_inet_addr((pointer) 0, field_p) == 0) {
		return 0;
	}
	if (! _menu_forward && ! _menu_backward &&
	    (sys_p->ether_type - ETHER_0) != last_ether) {
		set_form_map(form_p,
			     (pointer) find_form_radio(form_p,"ether_select"));
		return(menu_goto_obj());
	}
	return 1;
}


/*
 *	Name:		ether()
 *
 *	Description:	Select an ethernet for configuration.
 */

static int
ether(params)
	pointer		params[];
{
	form *		form_p;
	sys_info *	sys_p;
	int             i;


	form_p = (form *) params[0];
	sys_p = (sys_info *) params[1];

	if (sys_p->ether_type == ETHER_0 || sys_p->ether_type == ETHER_NONE) {
		/* turn off intername name request */
		off_form_field(find_form_field(form_p, "iname"));
	}
	if (sys_p->ether_type == ETHER_NONE) {
		off_form_field(find_form_field(form_p, "iaddr"));
		off_form_radio(find_form_radio(form_p, "yp"));
		off_form_field(find_form_field(form_p, "domain"));
		/*
		 * Clean up the data structure since there aren't
		 * any ethernet interfaces.  
		 */
		sys_p->yp_type = YP_NONE;
		for (i = 0; i < MAX_ETHER_INTERFACES; i++) {
			bzero(sys_p->ipx(i),sizeof(sys_p->ipx(i)));
			bzero(sys_p->ether_namex(i),
				sizeof(sys_p->ether_namex(i)));
			if (i != 0)
				bzero(sys_p->hostnamex(i),sizeof(sys_p->hostnamex(i)));
#ifdef SunB1
			sys_p->ethers[i].ip_minlab = 0;
			sys_p->ethers[i].ip_maxlab = 0;
#endif SunB1
		}

	} else {
		form_field* ff;
		int i = sys_p->ether_type - ETHER_0;

		if (sys_p->ether_type != ETHER_0) {
			ff = find_form_field(form_p, "iname");
			ff->ff_data = sys_p->hostnamex(i);
			on_form_field(ff);
		}
		ff = find_form_field(form_p, "iaddr");
		ff->ff_data = sys_p->ipx(i);
		on_form_field(ff);
		on_form_radio(find_form_radio(form_p, "yp"));
		if (sys_p->yp_type != YP_NONE) {
			on_form_field(find_form_field(form_p, "domain"));
		}
	}
	/*
	** 	If CTRL-F is used here (_menu_forward == 1), then skip the
	**	internet specifications
	*/
	if (_menu_forward) {
		set_form_map(form_p, (pointer) find_form_radio(form_p,"yp"));
		return(menu_goto_obj());
	}

	return(1);
}


#ifdef SunB1
/*
 *	Name:		set_ip0_maxlab()
 *
 *	Description:	The user wants to set the maximum label for IP 0 so
 *		do all the necessary checks.
 */

static int
set_ip0_maxlab(params)
	pointer		params[];
{
	form *		form_p;			/* pointer to HOST form */
	form_radio *	radio_p;		/* ptr to ip0_???lab radio */
	sys_info *	sys_p;			/* pointer to system info */


	form_p = (form *) params[0];
	sys_p = (sys_info *) params[1];

	/*
	 *	Minimum label is system_high so we cannot set maxlab to
	 *	anything but system_high
	 */
	if (sys_p->ip0_minlab == LAB_SYS_HIGH &&
	    sys_p->ip0_maxlab != LAB_SYS_HIGH) {
		radio_p = find_form_radio(form_p, "ip0_maxlab");

		off_form_radio(radio_p);
		sys_p->ip0_maxlab = LAB_SYS_HIGH;
		on_form_radio(radio_p);
	}

	/*
	 *	We set the maximum to system_low so the minimum must be set
	 *	to system_low.
	 */
	else if (sys_p->ip0_maxlab == LAB_SYS_LOW) {
		radio_p = find_form_radio(form_p, "ip0_minlab");

		off_form_radio(radio_p);
		sys_p->ip0_minlab = LAB_SYS_LOW;
		on_form_radio(radio_p);
	}

	return(1);
} /* end set_ip0_maxlab() */




/*
 *	Name:		set_ip0_minlab()
 *
 *	Description:	The user wants to set the minimum label for IP 0
 *		so do all the necessary checks.
 */

static int
set_ip0_minlab(params)
	pointer		params[];
{
	form *		form_p;			/* pointer to HOST form */
	form_radio *	radio_p;		/* ptr to ip0_???lab radio */
	sys_info *	sys_p;			/* pointer to system info */


	form_p = (form *) params[0];
	sys_p = (sys_info *) params[1];

	/*
	 *	Maximum label is system_low so we cannot set minlab to
	 *	anything but system_low.
	 */
	if (sys_p->ip0_maxlab == LAB_SYS_LOW &&
	    sys_p->ip0_minlab != LAB_SYS_LOW) {
		radio_p = find_form_radio(form_p, "ip0_minlab");

		off_form_radio(radio_p);
		sys_p->ip0_minlab = LAB_SYS_LOW;
		on_form_radio(radio_p);
	}

	/*
	 *	We set the minimum to system_high so the maximum must be set
	 *	to system_high.
	 */
	else if (sys_p->ip0_minlab == LAB_SYS_HIGH) {
		radio_p = find_form_radio(form_p, "ip0_maxlab");

		off_form_radio(radio_p);
		sys_p->ip0_maxlab = LAB_SYS_HIGH;
		on_form_radio(radio_p);
	}

	return(1);
} /* end set_ip0_minlab() */




/*
 *	Name:		set_ip1_maxlab()
 *
 *	Description:	The user wants to set the maximum label for IP 1
 *		so do all the necessary checks.
 */

static int
set_ip1_maxlab(params)
	pointer		params[];
{
	form *		form_p;			/* pointer to HOST form */
	form_radio *	radio_p;		/* ptr to ip1_???lab radio */
	sys_info *	sys_p;			/* pointer to system info */


	form_p = (form *) params[0];
	sys_p = (sys_info *) params[1];

	/*
	 *	Minimum label is system_high so we cannot set maxlab to
	 *	anything but system_high
	 */
	if (sys_p->ip1_minlab == LAB_SYS_HIGH &&
	    sys_p->ip1_maxlab != LAB_SYS_HIGH) {
		radio_p = find_form_radio(form_p, "ip1_maxlab");

		off_form_radio(radio_p);
		sys_p->ip1_maxlab = LAB_SYS_HIGH;
		on_form_radio(radio_p);
	}

	/*
	 *	We set the maximum to system_low so the minimum must be set
	 *	to system_low.
	 */
	else if (sys_p->ip1_maxlab == LAB_SYS_LOW) {
		radio_p = find_form_radio(form_p, "ip1_minlab");

		off_form_radio(radio_p);
		sys_p->ip1_minlab = LAB_SYS_LOW;
		on_form_radio(radio_p);
	}

	return(1);
} /* end set_ip1_maxlab() */




/*
 *	Name:		set_ip1_minlab()
 *
 *	Description:	The user wants to set the minimum label for IP 1
 *		so do all the necessary checks.
 */

static int
set_ip1_minlab(params)
	pointer		params[];
{
	form *		form_p;			/* pointer to HOST form */
	form_radio *	radio_p;		/* ptr to ip1_???lab radio */
	sys_info *	sys_p;			/* pointer to system info */


	form_p = (form *) params[0];
	sys_p = (sys_info *) params[1];

	/*
	 *	Maximum label is system_low so we cannot set minlab to
	 *	anything but system_low.
	 */
	if (sys_p->ip1_maxlab == LAB_SYS_LOW &&
	    sys_p->ip1_minlab != LAB_SYS_LOW) {
		radio_p = find_form_radio(form_p, "ip1_minlab");

		off_form_radio(radio_p);
		sys_p->ip1_minlab = LAB_SYS_LOW;
		on_form_radio(radio_p);
	}

	/*
	 *	We set the minimum to system_high so the maximum must be set
	 *	to system_high.
	 */
	else if (sys_p->ip1_minlab == LAB_SYS_HIGH) {
		radio_p = find_form_radio(form_p, "ip1_maxlab");

		off_form_radio(radio_p);
		sys_p->ip1_maxlab = LAB_SYS_HIGH;
		on_form_radio(radio_p);
	}

	return(1);
} /* end set_ip1_minlab() */
#endif /* SunB1 */




/*
 *	Name:		sys_dataless()
 *
 *	Description:	The user wants to configure a dataless system
 *		so turn on the fields for the server's information.
 */

static int
sys_dataless(params)
	pointer		params[];
{
	form *		form_p;
	sys_info *	sys_p;
	char		buf[MEDIUM_STR];


	form_p = (form *) params[0];
	sys_p = (sys_info *) params[1];

	if (strlen(sys_p->exec_path) == 0)
		(void) sprintf(sys_p->exec_path, "/export/exec/%s",
			       aprid_to_aid(sys_p->arch_str, buf));

	if (strlen(sys_p->kvm_path) == 0)
		(void) sprintf(sys_p->kvm_path, "/export/exec/kvm/%s",
			       aprid_to_iid(sys_p->arch_str, buf));

	on_form_field(find_form_field(form_p, "serv"));
	on_form_field(find_form_field(form_p, "svip"));
	on_form_field(find_form_field(form_p, "exec"));
	on_form_field(find_form_field(form_p, "kvm"));

	return(1);
} /* end sys_dataless() */




/*
 *	Name:		sys_not_dataless()
 *
 *	Description:	The user configured a system other than dataless
 *		so turn off the fields for the server's information.
 */

static int
sys_not_dataless(form_p)
	form *		form_p;
{
	off_form_field(find_form_field(form_p, "serv"));
	off_form_field(find_form_field(form_p, "svip"));
	off_form_field(find_form_field(form_p, "exec"));
	off_form_field(find_form_field(form_p, "kvm"));

	return(1);
} /* end sys_not_dataless() */


/*
 *	Name:		yp_sel()
 *
 *		A button on the NIS field has been pushed
 */

static int
yp_sel(params)
	pointer params[];
{
	form* form_p = (form*) params[0];
	sys_info* sys_p = (sys_info*) params[1];

	if (sys_p->yp_type == YP_NONE) {
		off_form_field(find_form_field(form_p, "domain"));
	} else {
		on_form_field(find_form_field(form_p, "domain"));
	}
	if (_menu_backward) {
		set_form_map(form_p, find_form_radio(form_p,"ether_select"));
		return menu_goto_obj();
	}
	return 1;
}





