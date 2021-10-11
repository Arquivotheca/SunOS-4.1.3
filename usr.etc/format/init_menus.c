
#ifndef lint
static	char sccsid[] = "@(#)init_menus.c 1.1 92/07/30";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * This file contains the declarations of menus for the program.  To add
 * a new command/menu, simply add it to the appropriate table and define
 * the function that executes it.
 */
#include "global.h"
#include "menu.h"

extern	int c_disk(), c_type(), c_partition(), c_current(), c_format();
extern	int c_repair(), c_show(), c_label(), c_analyze(), c_defect();
extern	int c_backup();

/*
 * This declaration is for the command menu.  It is the menu first
 * encountered upon entering the program.
 */
struct	menu_item menu_command[] = {
	/*
	 *		command name				function
	 */
	{ "disk       - select a disk",				c_disk },
	{ "type       - select (define) a disk type",		c_type },
	{ "partition  - select (define) a partition table",	c_partition },
	{ "current    - describe the current disk",		c_current },
	{ "format     - format and analyze the disk",		c_format },
	{ "repair     - repair a defective sector",		c_repair },
	{ "show       - translate a disk address",		c_show },
	{ "label      - write label to the disk",		c_label },
	{ "analyze    - surface analysis",			c_analyze },
	{ "defect     - defect list management",		c_defect }, 
	{ "backup     - search for backup labels",		c_backup },
	{ NULL },
};

extern	int p_apart(), p_bpart(), p_cpart(), p_dpart(), p_epart(), p_fpart();
extern	int p_gpart(), p_hpart(), p_load(), p_name(), p_print();

/*
 * This declaration is for the partition menu.  It is used to create
 * and maintain partiton tables.
 */
struct	menu_item menu_partition[] = {
	/*
	 *		command name				function
	 */
	{ "a      - change `a' partition",			p_apart },
	{ "b      - change `b' partition",			p_bpart },
	{ "c      - change `c' partition",			p_cpart },
	{ "d      - change `d' partition",			p_dpart },
	{ "e      - change `e' partition",			p_epart },
	{ "f      - change `f' partition",			p_fpart },
	{ "g      - change `g' partition",			p_gpart },
	{ "h      - change `h' partition",			p_hpart },
	{ "select - select a predefined table",			p_load },
	{ "name   - name the current table",			p_name },
	{ "print  - display the current table",			p_print },
	{ "label  - write partition map and label to the disk",	c_label },
	{ NULL },
};

extern	int a_read(), a_refresh(), a_test(), a_write(), a_compare();
extern	int a_print(), a_setup(), a_config(), a_purge();

/*
 * This declaration is for the analysis menu.  It is used to set up
 * and execute surface analysis of a disk.
 */
struct menu_item menu_analyze[] = {
	/*
	 *		command name				function
	 */
	{ "read     - read only test   (doesn't harm SunOS)",	a_read },
	{ "refresh  - read then write  (doesn't harm data)",	a_refresh },
	{ "test     - pattern testing  (doesn't harm data)",	a_test },
	{ "write    - write then read      (corrupts data)",	a_write },
	{ "compare  - write, read, compare (corrupts data)",	a_compare },
	{ "purge    - write, read, write   (corrupts data)",    a_purge },
	{ "print    - display data buffer",			a_print },
	{ "setup    - set analysis parameters",			a_setup },
	{ "config   - show analysis parameters",		a_config },
	{ NULL },
};

extern	int d_restore(), d_original(), d_extract(), d_add(), d_delete();
extern	int d_print(), d_dump(), d_load(), d_commit(), d_create();

/*
 * This declaration is for the defect menu.  It is used to manipulate
 * the defect list for a disk.
 */
struct menu_item menu_defect[] = {
	/*
	 *		command name				function
	 */
	{ "restore  - set working list = current list",		d_restore },
	{ "original - extract manufacturer's list from disk",	d_original },
	{ "extract  - extract working list from disk",		d_extract },
	{ "add      - add defects to working list",		d_add },
	{ "delete   - delete a defect from working list",	d_delete },
	{ "print    - display working list",			d_print },
	{ "dump     - dump working list to file",		d_dump },
	{ "load     - load working list from file",		d_load },
	{ "commit   - set current list = working list",		d_commit },
	{ "create   - recreates maufacturers defect list on disk",   d_create},
	{ NULL },
};

