/*      @(#)host_impl.h 1.1 92/07/30 SMI	*/

/*
 *	Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#ifndef _HOST_IMPL_H_
#define _HOST_IMPL_H_

/*
 *	Name:		host_impl.h
 *
 *	Description:	This file contains global definitions used in the
 *		host info implementation.
 *
 *		This file should only be included by:
 *
 *			get_host_info.c
 *			host_form.c
 */

#include "install.h"
#include "menu.h"


/*
 *	External functions:
 */
extern	form *		create_host_form();

#endif _HOST_IMPL_H_
