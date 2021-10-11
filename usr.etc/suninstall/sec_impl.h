/*      @(#)sec_impl.h 1.1 92/07/30 SMI	*/

/*
 *	Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#ifndef _SEC_IMPL_H_
#define _SEC_IMPL_H_

/*
 *	Name:		sec_impl.h
 *
 *	Description:	This file contains global definitions used in the
 *		security info implementation.
 *
 *		This file should only be included by:
 *
 *			get_sec_info.c
 *			sec_form.c
 */

#include "install.h"
#include "menu.h"



/*
 *	Global constant strings:
 */
extern	char		DEF_AUDIT_DIR[];
extern	char		DEF_AUDIT_FLAGS[];
extern	char		DEF_AUDIT_MIN[];
extern	char		DIR_FMT[];
extern	char		SALT[];




/*
 *	External functions:
 */
extern	form *		create_sec_form();

#endif _SEC_IMPL_H_
