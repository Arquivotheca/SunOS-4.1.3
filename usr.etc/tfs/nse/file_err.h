/*	@(#)file_err.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1989 Sun Microsystems, Inc.
 */

#ifndef _NSE_FILE_ERR_H
#define _NSE_FILE_ERR_H

/*
 * General-purpose format strings for NSE data file errors.
 */

#define NSE_FILE_BAD_FORMAT	-2501
#define NSE_FILE_BAD_VERS	-2502

#ifdef NSE_ERROR_COLLECT
static char *(nse_file_errors[]) = {
	"nse_file",
	"-2501 -2599",
	NSE_BEGIN_ERROR_TAB,
	"Internal error: format error in file \"%s\" at line %d",
	"Internal error: \"%s\" has bad version number %d",
	NSE_END_ERROR_TAB
	};
#undef NSE_BEGIN_ERROR_TAB
#define NSE_BEGIN_ERROR_TAB (char *) nse_file_errors
#endif

#endif _NSE_FILE_ERR_H
