/*	@(#)util.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#ifndef _NSE_UTIL_H
#define _NSE_UTIL_H

#ifndef _NSE_TYPES_H
#include <nse/types.h>
#endif

#ifndef _NSE_LIST_H
#include <nse/list.h>
#endif

#ifndef _NSE_ERR_H
#include <nse/err.h>
#endif

/*
 * Different styles of dates:
 * 	USA:	mon/day/year
 *	EUROPE:	day/mon/year
 *	SCCS:	year/mon/day
 */
enum	Nse_datetype { NSE_USA, NSE_EUROPE, NSE_SCCS };
typedef enum	Nse_datetype Nse_datetype;

extern bool_t	nse_command_trace;

/*
 * Return values of functions defined in the library.
 */

/* abspath.c */
char		*nse_abspath(/* argname, result */);

/* netpath.c */
char		*nse_netpath(/* lclpath, host, rmtpath */);

/* parse_filenam.c */
void		_nse_parse_filename();

/* streq_func.c */
int		_nse_streq_func();

/* fully_qualify.c */
char		*_nse_fully_qualify();

/* extract_name.c */
char		*_nse_extract_name();

/* hashutil.c */
void		_nse_dispose_func();
int		_nse_hash_string();

/* hostname.c */
char		*_nse_hostname();
Nse_err		_nse_get_user();

/* cmdname.c */
char		*nse_set_cmdname();
char		*nse_get_cmdname();

/* path-util.c */
char		*nse_pathcat();

/* nse_malloc.c */
char		*nse_malloc();
char		*nse_calloc();

/* nse_fchdir.c */
int		nse_fchdir();
int		nse_fchroot();

/* copy_file.c */
Nse_err		_nse_copy_file();
Nse_err		_nse_copy_file_libscan();
Nse_err		_nse_copy_setimes();

/* fftruncate.c */
int		nse_fftruncate();

/* parse_brname.c */
void		nse_parse_branch_name();
void		nse_unparse_branch_name();
Nse_err		_nse_verify_env_name();

/* find_substr.c */
char		*nse_find_substring();

#endif
