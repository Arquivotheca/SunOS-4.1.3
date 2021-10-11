/*      @(#)disk_impl.h 1.1 92/07/30 SMI	*/

/*
 *	Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#ifndef _DISK_IMPL_H_
#define _DISK_IMPL_H_

/*
 *	Name:		disk_impl.h
 *
 *	Description:	This file contains global definitions used in the
 *		disk info implementation.
 *
 *		This file should only be included by:
 *
 *			get_disk_info.c
 *			disk_form.c
 */

#include "install.h"
#include "menu.h"

/*
 *	Do the typedefs up front so the data structures can be in
 *	alphabetical order.
 */
typedef struct disk_disp_t	disk_disp;
typedef struct part_disp_t	part_disp;


/*
 *	Definition of a partition display structure:
 *
 *		name		- name of the partition
 *		disk_disp_p	- pointer to the disk display
 *		part_p		- pointer to part_info struct
 */
struct part_disp_t {
	char		name[2];
	disk_disp *	disk_disp_p;
	part_info *	part_p;
};


/*
 *	Definition of a disk display structure:
 *
 *		disp_list	- partition display list
 *		params		- parameters to pass to the menu functions
 *		strings		- strings for disk display
 *		mls_strings	- MLS strings (SunB1)
 */
struct disk_disp_t {
	part_disp	disp_list[NDKMAP];
	pointer	* 	params;
	menu_string *	strings[3];
#ifdef SunB1
	menu_string *	mls_strings[3];
#endif /* SunB1 */
};




/*
 *	External functions:
 */
extern	int		check_partitions();
extern	form *		create_disk_form();
extern	int		get_default_part();
extern	int		get_existing_part();

#endif _DISK_IMPL_H_
