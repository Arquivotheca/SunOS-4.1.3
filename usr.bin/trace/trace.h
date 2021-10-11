/*	@(#)trace.h 1.1 92/07/30  SMI	*/

/*
 *  Copyright (c) 1986 by Sun Microsystems, Inc.
 */

enum argtype {
   V,  /* void  */
   S,  /* string */
   D,  /* decimal */
   R,  /* read buffer */
   W,  /* write buffer */
   X,  /* word (hex) */
   O,  /* octal */
   U,  /* unsigned */
   L,  /* long     */
   } ;

struct sysinfo {
   enum argtype result ;
   enum argtype args [8] ;
   char *name ;
   } ;
