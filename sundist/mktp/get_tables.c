#ifndef lint
static  char sccsid[] = "@(#)get_tables.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif
/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */
#include "mktp.h"

/*
 * retrieve entries table from an intermediate file..
 */


bool_t
get_tables(file)
char *file;
{
	register FILE *ptr;
	register int amt;

	/*
	 * 1) Zero out tables...
	 *
	 */

	bzero((char *)&dinfo,sizeof dinfo);
	bzero((char *)&vinfo,sizeof vinfo);
	bzero((char *)entries,NENTRIES * sizeof (entries[0]));


	/*
	 * 2) Open Input file...
	 *
	 */


	if((ptr = fopen(file,"r")) == NULL)
	{
		(void) fprintf(stderr,"Could not open TOC file '%s'\n",
			file);
		return(FALSE);
	}

	/*
	 *
	 */

	if((amt = read_xdr_toc(ptr,&dinfo,&vinfo,entries,NENTRIES)) <= 0
			|| fclose(ptr) == EOF)
		return(FALSE);
	
	ep = entries + amt;
	return(TRUE);
}	
