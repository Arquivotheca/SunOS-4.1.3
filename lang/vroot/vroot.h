/*LINTLIBRARY*/
/*	@(#)vroot.h 1.1 92/07/30 SMI	*/

/*
 *	Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <stdio.h>

#define VROOT_DEFAULT ((pathpt)-1)

typedef struct {
	char		*path;
	short		length;
} pathcellt, *pathcellpt, patht[];
typedef patht		*pathpt;

extern	void		add_dir_to_path();
extern	void		flush_path_cache();
extern	void		flush_vroot_cache();
extern	char		*get_path_name();
extern	char		*get_vroot_path();
extern	char		*get_vroot_name();
extern	pathpt		parse_path_string();
extern	void		scan_path_first();
extern	void		scan_vroot_first();
extern	void		set_path_style();
