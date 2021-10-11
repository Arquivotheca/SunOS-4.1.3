#ifndef lint
static  char sccsid[] = "@(#)dump_tables.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif
/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */
#include "mktp.h"

/*
 * dump entries table to an intermediate file..
 */

bool_t
dump_tables(file)
char *file;
{
	register FILE *ptr;

	if((ptr = fopen(file,"w")) == NULL)
	{
		(void) fprintf(stderr,"Could not create TOC file '%s'\n",file);
		return(FALSE);
	}
	else if(!write_xdr_toc(ptr,&dinfo,&vinfo,entries,ep-entries))
	{
		fclose(ptr);
		unlink(file);
		return(FALSE);
	}
	else if(fclose(ptr) == EOF)
	{
		unlink(file);
		return(FALSE);
	}
	return(TRUE);
}
