/*      @(#)getarch.c 1.1 92/07/30 SMI                              */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include "install.h"
#include "sysexits.h"

char *
getarch(progname)
char *progname;
{
	if ((system("/bin/sun4")) == 0)
		return (SUN4);
	else if ((system("/bin/sun3")) == 0)
		return (SUN3);
	else if ((system("/bin/sun3x")) == 0)
		return (SUN3X);
	else if ((system("/bin/sun2")) == 0)
		return (SUN2);
	else if ((system("/bin/sun386")) == 0)
		return (SUN386);
	log("%s:\tinvalid system architecture\n", progname);
	return(NULL);
}

int
legal_arch(arch, progname)
char *progname, *arch;
{
	if (strcmp(arch,SUN4) && 
	    strcmp(arch,SUN3) &&
	    strcmp(arch,SUN3X) && 
	    strcmp(arch,SUN2)) {
		(void) fprintf(stderr,"%s: %s illegal architecture\n", 
		    progname, arch);
		return(-1);
	}
	return(0);
}
