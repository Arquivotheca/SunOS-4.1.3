#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)tz_menu.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)tz_menu.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		tz_menu.c
 *
 *	Description:	This file contains all the routines for dealing
 *		with the timezone menu.
 */

#include <string.h>
#include "main_impl.h"




/*
 *	Local functions:
 */
static	int		done_with_menu();
static	int		save_tz_string();
static	int		use_sub_menu();




/*
 *	Local variables:
 */
static	tz_string	asia_strings[] = {
	3,	10,	"TIME ZONE NAME",
	3,	40,	"AREA",

	0,	0,	CP_NULL
};


static	tz_leaf		asia_leaves[] = {
	 5,	10,	"PRC",		40,	"People's Republic of China",
	 7,	10,	"ROK",		40,	"Republic of Korea",
	 9,	10,	"Japan",	40,	"Japan",
	11,	10,	"Singapore",	40,	"Singapore",
	13,	10,	"Hongkong",	40,	"Hong Kong",
	15,	10,	"ROC",		40,	"Republic of China/Taiwan",

	 0,	 0,	CP_NULL,	 0,	CP_NULL
};


static	tz_item		asia_item = {
	16, 24, "Asia", "ASIA MENU", asia_strings, asia_leaves
};


static	tz_string	ausnz_strings[] = {
	3,	10,	"TIME ZONE NAME",
	3,	40,	"AREA",

	0,	0,	CP_NULL
};


static	tz_leaf		ausnz_leaves[] = {
	 5,	10,	"Australia/Tasmania",	40,	"Tasmania, Australia",	
	 7,	10,	"Australia/Queensland",	40,    "Queensland, Australia",	
	 9,	10,	"Australia/North",	40,	"Northern Australia",	
	11,	10,	"Australia/West",	40,	"Western Australia",	
	13,	10,	"Australia/South",	40,	"Southern Australia",	
	15,	10,	"Australia/Victoria",	40,	"Victoria, Australia",	
	17,	10,	"Australia/NSW",	40,
						  "New South Wales, Australia",	
	19,	10,	"NZ",			40,	"New Zealand",	

	 0,	 0,	CP_NULL,		 0,	CP_NULL
};


static	tz_item		ausnz_item = {
	18, 24, "Australia and New Zealand", "AUSTRALIA AND NEW ZEALAND MENU",
	ausnz_strings, ausnz_leaves
};


static	tz_string	canada_strings[] = {
	3,	10,	"TIME ZONE NAME",
	3,	40,	"AREA",

	0,	0,	CP_NULL
};


static	tz_leaf		canada_leaves[] = {
	 5,	10,	"Canada/Newfoundland",	40,	"Newfoundland",
	 7,	10,	"Canada/Atlantic",	40,
						  "Atlantic time zone, Canada",
	 9,	10,	"Canada/Eastern",	40,
						   "Eastern time zone, Canada",
	11,	10,	"Canada/Central",	40,
						   "Central time zone, Canada",
	13,	10,	"Canada/East-Saskatchewan", 40,
						   "Central time zone, Canada",
	14,	 0,	CP_NULL,		40, "No Daylight Savings Time",
	16,	10,	"Canada/Mountain",	40,
						  "Mountain time zone, Canada",
	18,	10,	"Canada/Pacific",	40,
						   "Pacific time zone, Canada",
	20,	10,	"Canada/Yukon",		40,  "Yukon time zone, Canada",

	 0,	 0,	CP_NULL,		 0,	CP_NULL
};


static	tz_item		canada_item = {
	8, 24, "Canada", "CANADA MENU", canada_strings, canada_leaves
};


static	tz_string	europe_strings[] = {
	2,	10,	"TIME ZONE NAME",
	2,	40,	"AREA",

	0,	0,	CP_NULL
};


static	tz_leaf		europe_leaves[] = {
	 4,	10,	"GB-Eire",	40,	"Great Britain and Eire",
	 6,	10,	"WET",		40,	"Western Europe time",
	 8,	10,	"Iceland",	40,	"Iceland",
	10,	10,	"MET",		40,	"Middle European time",
	11,	 0,	CP_NULL,	40,
					   "also called Central European time",
	13,	10,	"Poland",	40,	"Poland",
	15,	10,	"EET",		40,	"Eastern European time",
	17,	10,	"Turkey",	40,	"Turkey",
	19,	10,	"Israel",	40,	"Israel",
	21,	10,	"W-SU",		40,	"Western Soviet Union",

	 0,	 0,	CP_NULL,	 0,	CP_NULL
};


static	tz_item		europe_item = {
	14, 24, "Europe", "EUROPE MENU", europe_strings, europe_leaves
};


static	tz_string	gmt_strings[] = {
	3,	5,	"TIME ZONE",
	4,	7,	"NAME",
	4,	24,	"AREA",
	3,	46,	"TIME ZONE",
	4,	48,	"NAME",
	4,	64,	"AREA",

	0,	 0,	CP_NULL
};


static	tz_leaf		gmt_leaves[] = {
	 6,	5,	"GMT",		15,	"Greenwich Mean time (GMT)",	
	 7,	5,	"GMT-1",	15,	" 1 hour  west of GMT",	
	 8,	5,	"GMT-2",	15,	" 2 hours west of GMT",	
	 9,	5,	"GMT-3",	15,	" 3 hours west of GMT",	
	10,	5,	"GMT-4",	15,	" 4 hours west of GMT",	
	11,	5,	"GMT-5",	15,	" 5 hours west of GMT",	
	12,	5,	"GMT-6",	15,	" 6 hours west of GMT",	
	13,	5,	"GMT-7",	15,	" 7 hours west of GMT",	
	14,	5,	"GMT-8",	15,	" 8 hours west of GMT",	
	15,	5,	"GMT-9",	15,	" 9 hours west of GMT",	
	16,	5,	"GMT-10",	15,	"10 hours west of GMT",	
	17,	5,	"GMT-11",	15,	"11 hours west of GMT",	
	18,	5,	"GMT-12",	15,	"12 hours west of GMT",	

	 6,	45,	"GMT+13",	55,	"13 hours east of GMT",	
	 7,	45,	"GMT+12",	55,	"12 hours east of GMT",	
	 8,	45,	"GMT+11",	55,	"11 hours east of GMT",	
	 9,	45,	"GMT+10",	55,	"10 hours east of GMT",	
	10,	45,	"GMT+9",	55,	" 9 hours east of GMT",	
	11,	45,	"GMT+8",	55,	" 8 hours east of GMT",	
	12,	45,	"GMT+7",	55,	" 7 hours east of GMT",	
	13,	45,	"GMT+6",	55,	" 6 hours east of GMT",	
	14,	45,	"GMT+5",	55,	" 5 hours east of GMT",	
	15,	45,	"GMT+4",	55,	" 4 hours east of GMT",	
	16,	45,	"GMT+3",	55,	" 3 hours east of GMT",	
	17,	45,	"GMT+2",	55,	" 2 hours east of GMT",	
	18,	45,	"GMT+1",	55,	" 1 hour  east of GMT",	

	 0,	 0,	CP_NULL,	 0,	CP_NULL
};


static	tz_item		gmt_item = {
	20, 24, "Greenwich Mean Time", "GREEWICH MEAN TIME MENU",
	gmt_strings, gmt_leaves
};


static	tz_string	mexico_strings[] = {
        3,	10,	"TIME ZONE NAME",
        3,	40,	"AREA",

	0,	0,	CP_NULL
};


static	tz_leaf		mexico_leaves[] = {
	 5,	10,	"Mexico/BajaNorte",	40,  "Northern Baja time zone",
	 7,	10,	"Mexico/BajaSur",	40,  "Southern Baja time zone",
	 9,	10,	"Mexico/General",	40,	"Central time zone",

	 0,	 0,	CP_NULL,		 0,	CP_NULL
};


static	tz_item		mexico_item = {
	10, 24, "Mexico", "MEXICO MENU", mexico_strings, mexico_leaves
};


static	tz_string	southam_strings[] = {
	3,	10,	"TIME ZONE NAME",
	3,	40,	"AREA",

	0,	0,	CP_NULL
};


static	tz_leaf		southam_leaves[] = {
	 5,	10,	"Brazil/Acre",		40,	"Brazil time zone",
	 7,	10,	"Brazil/DeNoronha",	40,	"Brazil time zone",
	 9,	10,	"Brazil/East",		40,	"Brazil time zone",
	11,	10,	"Brazil/West",		40,	"Brazil time zone",
	13,	10,	"Chile/Continental",	40,
						 "Continental Chile time zone",
	15,	10,	"Chile/EasterIsland",	40, "Easter Island time zone",

	 0,	 0,	CP_NULL,		 0,	CP_NULL
};


static	tz_item		southam_item = {
	12, 24, "South America", "SOUTH AMERICA MENU",
	southam_strings, southam_leaves
};


static	char		tz_buf[MEDIUM_STR];	/* buf for timezone string */


static	tz_string	us_strings[] = {
        2,	10,	"TIME ZONE NAME",
        2,	40,	"AREA",

	0,	0,	0
};


static	tz_leaf		us_leaves[] = {
	 3,	10,	"US/Eastern",	40,	"Eastern time zone, USA",
	 5,	10,	"US/Central",	40,	"Central time zone, USA",
         7,	10,	"US/Mountain",	40,	"Mountain time zone, USA",
         9,	10,	"US/Pacific",	40,	"Pacific time zone, USA",
        11,	10,	"US/Pacific-New", 40,	"Pacific time zone, USA",
	12,	 0,	CP_NULL,	40,
					   "with proposed changes to Daylight",
	13,	 0,	CP_NULL,	40, "Savings Time near election time",
        15,	10,	"US/Alaska",	40,	"Alaska time zone, USA",
        17,	10,	"US/East-Indiana", 40,	"Eastern time zone, USA",
	18,	 0,	CP_NULL,	40,	"no Daylight Savings Time",
        20,	10,	"US/Hawaii",	40,	"Hawaii",

	 0,	 0,	CP_NULL,	 0,	CP_NULL
};


static	tz_item		us_item = {
	6, 24, "United States", "UNITED STATES MENU", us_strings, us_leaves
};




static	tz_string	zone_strings[] = {
	3,	14,	"Select one of the following categories to display",
	4,	16,	"a screen of time zone names for that region.",

	0,	0,	CP_NULL
};


static	tz_item *	zone_items[] = {
	&us_item,
	&canada_item,
	&mexico_item,
	&southam_item,
	&europe_item,
	&asia_item,
	&ausnz_item,
	&gmt_item,

	(tz_item *) 0
};


static	tz_disp		zone_disp = {
	zone_strings, zone_items
};




/*
 *	Name:		create_tz_menu()
 *
 *	Description:	Create the TIMEZONE menus.  The timezone that
 *		is selected by the user is copied into 'timezone'.
 */

menu *
create_tz_menu(timezone)
	char *		timezone;
{
	tz_item **	ipp;			/* ptr to timezone item ptr */
	menu_item *	item_p;			/* pointer to a menu item */
	tz_leaf *	lp;			/* pointer to timezone leaf */
	tz_string *	sp;			/* ptr to timezone string */
	menu *		sub_p;			/* pointer to sub-menu */
	menu *		tz_p;			/* pointer to TIMEZONE menu */


	bzero(tz_buf, sizeof(tz_buf));

	init_menu();

	tz_p = create_menu(PFI_NULL, ACTIVE, "TIMEZONE MENU");


	/*
	 *	Add strings for TIMEZONE menu
	 */
	for (sp = zone_disp.tzd_strings; sp->tzs_text; sp++)
		(void) add_menu_string((pointer) tz_p, ACTIVE,
				       sp->tzs_y, sp->tzs_x, ATTR_NORMAL,
				       sp->tzs_text);

	/*
	 *	Add the sub-menus
	 */
	for (ipp = zone_disp.tzd_items; *ipp; ipp++) {
		sub_p = create_menu(PFI_NULL, ACTIVE, (*ipp)->tzi_name);

		/*
		 *	Add strings for this sub-menu
		 */
		for (sp = (*ipp)->tzi_strings; sp->tzs_text; sp++)
			(void) add_menu_string((pointer) sub_p, ACTIVE,
					       sp->tzs_y, sp->tzs_x,
					       ATTR_NORMAL, sp->tzs_text);

		/*
		 *	Add menu items for this sub-menu
		 */
		for (lp = (*ipp)->tzi_leaves; lp->tzl_text; lp++) {
			/*
			 *	If 'tzl_data' is NULL, then this entry is
			 *	a continuation of the previous entry.
			 */
			if (lp->tzl_data) {
				item_p = add_menu_item(sub_p, PFI_NULL,
						       ACTIVE, CP_NULL,
						       lp->tzl_y, lp->tzl_dx,
						       PFI_NULL, PTR_NULL,
						       save_tz_string,
						       (pointer) lp->tzl_data,
						       PFI_NULL, PTR_NULL);
				(void) add_menu_string((pointer) item_p,
						       ACTIVE, lp->tzl_y,
						       lp->tzl_dx + 2,
						       ATTR_NORMAL,
						       lp->tzl_data);
			}
			(void) add_menu_string((pointer) item_p, ACTIVE,
					       lp->tzl_y, lp->tzl_tx,
					       ATTR_NORMAL, lp->tzl_text);
		}
		(void) add_finish_obj((pointer) sub_p, PFI_NULL, PTR_NULL,
				      PFI_NULL, PTR_NULL, PFI_NULL, PTR_NULL);

		/*
		 *	Add sub-menu items to TIMEZONE menu
		 */
		item_p = add_menu_item(tz_p, PFI_NULL, ACTIVE, CP_NULL,
				       (*ipp)->tzi_y, (*ipp)->tzi_x,
				       PFI_NULL, PTR_NULL,
				       use_sub_menu, (pointer) sub_p,
				       PFI_NULL, PTR_NULL);
		(void) add_menu_string((pointer) item_p, ACTIVE,
				       (*ipp)->tzi_y, (*ipp)->tzi_x + 2,
				       ATTR_NORMAL, (*ipp)->tzi_text);
	}

	(void) add_finish_obj((pointer) tz_p, PFI_NULL, PTR_NULL, PFI_NULL,
			      PTR_NULL, done_with_menu, (pointer) timezone);

	return(tz_p);
} /* end create_tz_menu() */




/*
 *	Name:		done_with_menu()
 *
 *	Description:	Done with a menu so copy the timezone that was
 *		selected into 'timezone'.
 */

static	int
done_with_menu(timezone)
	char *		timezone;
{
	(void) strcpy(timezone, tz_buf);

	return(1);
} /* end done_with_menu() */




/*
 *	Name:		save_tz_string()
 *
 *	Description:	Save the timezone that was selected into a local
 *		buffer so the done_with_menu() routine to get it to the user.
 */

static	int
save_tz_string(str)
	char *		str;
{
	(void) strcpy(tz_buf, str);
	set_menu_item((menu *) _current_fm, (menu_item *) NULL);

	return(menu_goto_obj());
} /* end save_tz_string() */




/*
 *	Name:		use_sub_menu()
 *
 *	Description:	Invoke a sub menu and set it up so we return to
 *		the confirmer object when we return.
 */

static	int
use_sub_menu(sub_p)
	menu *		sub_p;
{
	(void) use_menu(sub_p);

	set_menu_item((menu *) _current_fm, (menu_item *) NULL);

	return(menu_goto_obj());
} /* end use_sub_menu() */
