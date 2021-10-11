/*      @(#)client_impl.h 1.1 92/07/30 SMI	*/

/*
 *	Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#ifndef _CLIENT_IMPL_H_
#define _CLIENT_IMPL_H_

/*
 *	Name:		client_impl.h
 *
 *	Description:	This file contains global definitions used in the
 *		client info implementation.
 *
 *		This file should only be included by:
 *
 *			get_client_info.c
 *			client_form.c
 */

#include "install.h"
#include "menu.h"


/*
 *	Global type definitions:
 */


/*
 *	Definition of a client display structure:
 *
 *		root_fs		- root file system
 *		root_avail	- available space in root_fs
 *		rhog_fs		- root's free hog file system
 *		rhog_avail	- available space in rhog_fs
 *		swap_fs		- swap file system
 *		swap_avail	- available space in swap_fs
 *		shog_fs		- swap's free hog file system
 *		shog_avail	- available space in shog_fs
 *		list_header	- header for the client list
 */
typedef struct	clnt_disp_t {
	char		root_fs[MAXPATHLEN];
	char		root_avail[TINY_STR];
	char		rhog_fs[MAXPATHLEN];
	char		rhog_avail[TINY_STR];
	char		swap_fs[MAXPATHLEN];
	char		swap_avail[TINY_STR];
	char		shog_fs[MAXPATHLEN];
	char		shog_avail[TINY_STR];
	char		list_header[SMALL_STR];
} clnt_disp;




/*
 *	External functions:
 */
extern	int		add_client_to_list();
extern	void		clear_client_list();
extern	int		client_okay();
extern	int		create_client();
extern	form *		create_client_form();
extern	int		delete_client();
extern	int		delete_client_from_list();
extern	int		display_client();
extern	int		edit_client();
extern	int		get_clnt_display();
extern	int		init_client_info();
extern	void		off_client_disp();
extern	void		off_client_info();
extern	void		on_client_disp();
extern	void		on_client_info();
extern	void		show_client_list();
extern	int		verify_arch();

#endif _CLIENT_IMPL_H_
