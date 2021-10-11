/*      @(#)posix_fcntl.s 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#include "SYS.h" 
ENTRY(posix_fcntl);
mov     SYS_fcntl, %g1;
t       0;
CERROR(o5) 
RET 
