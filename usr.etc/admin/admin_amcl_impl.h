/*  @(#)admin_amcl_impl.h 1.1 92/07/30 SMI  */

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#ifndef _admin_amcl_impl_h
#define _admin_amcl_impl_h

/*
 *	This file contains private definitions for internal use within
 *	the Administrative Methods Client Library.  The file contains
 *	definitions for:
 *
 *	      o Standard file descriptors.
 *	      o Message and buffer sizes.
 *	      o Error messages.
 */

#define STDIN_DES		0	/* STDIN file descriptor */
#define STDOUT_DES		1	/* STDOUT file descriptor */
#define STDERR_DES		2	/* STDERR file descriptor */

#define ADMIN_RESBLKSIZE        1024    /* Block size by which to extend a */
                                        /* result buffer if more space is */
                                        /* needed. */

#define ADMIN_MSGSIZE		4096	/* Maximum admin message size */

#endif /* !_admin_amcl_impl_h */
