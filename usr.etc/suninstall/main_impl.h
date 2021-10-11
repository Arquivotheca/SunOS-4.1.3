/*      @(#)main_impl.h 1.1 92/07/30 SMI	*/

/*
 *	Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#ifndef _MAIN_IMPL_H_
#define _MAIN_IMPL_H_

/*
 *	Name:		main_impl.h
 *
 *	Description:	This file contains global definitions used in the
 *		main driver implementation.
 *
 *		This file should only be included by:
 *
 *			main.c
 *			main_form.c
 */

#include "install.h"
#include "menu.h"


/*
 *	Global data structures:
 */

/*
 *	Definition of a timezone leaf:
 *
 *		tzl_y		- y-coordinate of this leaf
 *		tzl_dx		- x-coordinate for tzi_data
 *		tzl_data	- data associated with this leaf
 *		tzl_tx		- x-coordinate for tzi_text
 *		tzl_text	- text associated with this leaf
 */
typedef struct tz_leaf_t {
	unsigned char	tzl_y;
	unsigned char	tzl_dx;
	char *		tzl_data;
	unsigned char	tzl_tx;
	char *		tzl_text;
} tz_leaf;


/*
 *	Definition of a timezone string:
 *
 *		tzs_y		- y-coordinate
 *		tzs_x		- x-coordinate
 *		tzs_text	- text
 */
typedef struct tz_string_t {
	unsigned char	tzs_y;
	unsigned char	tzs_x;
	char *		tzs_text;
} tz_string;


/*
 *	Definition of a timezone item:
 *
 *		tzi_y		- y-coordinate for this item
 *		tzi_x		- x-coordinate for this item
 *		tzi_text	- text for this item
 *		tzi_name	- name of the sub-menu
 *		tzi_strings	- strings associated with the sub-menu
 *		tzi_leaves	- leaves associated with the sub-menu
 *		tzi_sub		- sub-menu pointer
 */
typedef struct tz_item_t {
	unsigned char	tzi_y;
	unsigned char	tzi_x;
	char *		tzi_text;
	char *		tzi_name;
	tz_string *	tzi_strings;
	tz_leaf *	tzi_leaves;
	menu *		tzi_sub;
} tz_item;


/*
 *	Definition of a timezone display:
 *
 *		tzd_strings	- timezone display strings
 *		tzd_items	- timezone display items
 */
typedef struct tz_disp_t {
	tz_string *	tzd_strings;
	tz_item **	tzd_items;
} tz_disp;




/*
 *	External functions:
 */
extern	menu *		create_main_menu();
extern	menu *		create_tz_menu();
extern	int		is_client_ok();
extern	int		is_disk_ok();
extern	int		is_sec_ok();
extern	int		is_soft_ok();
extern	int		is_sys_ok();
extern	void		set_items();

#endif _MAIN_IMPL_H_
