/*      @(#)soft_impl.h 1.1 92/07/30 SMI	*/

/*
 *	Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#ifndef _SOFT_IMPL_H_
#define _SOFT_IMPL_H_

/*
 *	Name:		soft_impl.h
 *
 *	Description:	This file contains global definitions used in the
 *		software info implementation.
 *
 *		This file should only be included by:
 *
 *			get_software_info.c
 *			soft_form.c
 */

#include "install.h"
#include "menu.h"


/*
 *	Global type definitions:
 */


/*
 *	Definition of a media file display structure:
 *
 *		dest_str	- destination file system display string
 *		dest_fs		- destination file system
 *		dest_avail	- available space in dest_fs
 *		hog_fs		- free hog file system
 *		hog_avail	- available space in hog_fs
 *		media_name	- name of the media file
 *		media_size	- size of the media file
 *		media_select	- select this media?
 *		media_status	- current select status of this media
 */
typedef struct	mf_disp_t {
	char		dest_str[MAXPATHLEN];
	char		dest_fs[MAXPATHLEN];
	char		dest_avail[TINY_STR];
	char		hog_fs[MAXPATHLEN];
	char		hog_avail[TINY_STR];
	char		media_name[MEDIUM_STR];
	char		media_size[TINY_STR];
	char		media_select;
	char		media_status[TINY_STR];
} mf_disp;




/*
 *	External functions:
 */
extern	int		ask_mf_choice();
extern	form *		create_soft_form();
extern	int		get_media_path();
extern	int		get_mf_disp_info();
extern	int		read_file();
extern	int		toc_xlat();
extern	int		update_arch_buttons();

#endif _SOFT_IMPL_H_
