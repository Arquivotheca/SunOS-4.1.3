/*      @(#)limits.h 1.1 92/07/30 SMI	*/

#ifndef _xpg_limits.h
#define _xpg_limits.h


/* Copyright (c) 1988
 * Sun Microsystems Inc */
/*      limits.h for ANSI C and POSIX  AND XPG87 */

#ifndef _sys_limits_h
#define _sys_limits_h

#ifndef	_sys_param_h
#include <sys/param.h>
#endif

#ifndef _sys_dirent_h
#include <sys/dirent.h>
#endif

#ifndef KERNEL
#include <values.h>
#endif

#define	CHAR_BIT		 0x8
#define	SCHAR_MIN		-0x80
#define	SCHAR_MAX		 0x7F
#define	UCHAR_MAX		 0xFF
#define	CHAR_MIN		-0x80
#define	CHAR_MAX		 0x7F
#define	SHRT_MIN		-0x8000
#define	SHRT_MAX		 0x7FFF
#define	USHRT_MAX		 0xFFFF
#define	INT_MIN			-0x80000000
#define	INT_MAX			 0x7FFFFFFF
#define	UINT_MAX		 0xFFFFFFFF
#define	LONG_MIN		-0x80000000
#define	LONG_MAX		 0x7FFFFFFF
#define	ULONG_MAX		 0xFFFFFFFF

#define	MAX_CANON		CANBSIZ
#define	NGROUPS_MAX		NGROUPS
#define	PID_MAX			MAXPID
#define	UID_MAX			MAXUID
#define	ARG_MAX			NCARGS
#define	NAME_MAX		MAXNAMLEN
#define	PATH_MAX		MAXPATHLEN
#define	LINK_MAX		MAXLINK

/* 
 * All Posix systems must support the following values 
 * A system may support less restrictive values
 */
#define _POSIX_ARG_MAX		4096
#define _POSIX_CHILD_MAX	6
#define _POSIX_LINK_MAX		8
#define _POSIX_MAX_CANON	255
#define _POSIX_MAX_INPUT	255
#define _POSIX_NAME_MAX		14
#define _POSIX_NGROUPS_MAX	0
#define _POSIX_OPEN_MAX		16
#define _POSIX_PATH_MAX		255
#define _POSIX_PIPE_BUF		512

#endif /* !_sys_limits_h */

/* Warning -- these limits are for backwards compatibility with XPG2
 * The following packet of constants are different in XPG3 and POSIX. 
 * Some have become runtime variable and must be treated as such
 */

#define CLK_TCK         60
#define FCHR_MAX	1000000 /* Portabilty value */
#define PASS_MAX	8  	/* Portability value  */
#define CHILD_MAX	133 	/* With default kernel */
#define OPEN_MAX	256 	/* With default kernel */
#define PIPE_MAX	PIPE_BUF 
#define	LOCK_MAX	32 	/* XPG3 Deleted */
#define	PROC_MAX	8	/* XPG3 Deleted */

#define	TMP_MAX		10000   
#define	SYS_NMLN	9
#define DBL_DIG		DMAXEXP
#define FLT_DIG		FMAXEXP
#define LONG_BIT	32
#define MAX_CHAR	CANBSIZ
#define SYSPID_MAX	PID_MAX
#define STD_BLK		MAXBSIZE
#define USI_MAX		UINT_MAX
#define WORD_BIT	32
#define DBL_MIN		MINDOUBLE
#define FLT_MIN		MINFLOAT
#define DBL_MAX		MAXDOUBLE
#define FLT_MAX		MAXFLOAT

/*	NLS parts	*/

#define NL_ARGMAX       9       /* Max value of 'digit' in calls to
                                 * NLS printf and scanf functions.  */

#define NL_MSGMAX       32767   /* Max message number */
#define NL_SETMAX       255     /* Max set number */
#define NL_TEXTMAX      255     /* Max no. of bytes in a message string */

#endif /* _xpg_limits.h */
