#ifndef _find_method_impl_h
#define _find_method_impl_h

/* 
 * find_method_impl.h 
 *	This file contains definitions for find_method.c  
 *	Most of these are for error handling
 *
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 */

/*      @(#)find_method_impl.h 1.1 92/07/30 SMI */

#define	FOUND 		0
#define NOTFOUND	1	
#define	ERROR		-1	
#define OBJ		"classes"
#define SEPARATOR	'/'	/* Separates components of path name 	*/
#define	SLASH		"/"	/* Used in strcat			*/
#define BUFSIZE		250	/* Size of entry in .class_name file 	*/
#define READONLY	"r"	/* Used in fopen			*/
#define CLASSFILE	".class"
#define PATHENVV	"ADMINPATH"

#endif /* !_find_method_impl_h */
