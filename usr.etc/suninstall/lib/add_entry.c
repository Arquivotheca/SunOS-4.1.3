/*      @(#)add_entry.c 1.1 92/07/30 SMI                              */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include "stdio.h"

int
add_entry(new_entry, file, progname)
char *new_entry, *file, *progname;
{
	int 	add_flag=0;
	char	file_entry[BUFSIZ];
	FILE 	*fp;

	/* check local file */
	if (getfileent(new_entry, file_entry, file) == 0) {
		/* there is a local entry - see if it matches */
		/* if not replace it - if matches return */
	} else {
		/* no entry so write one in */
		if (fp = fopen(file, "a")) {
			(void) fprintf(fp, new_entry);
			(void) fprintf(fp, "\n");
			(void) fclose(fp);
			add_flag++;
		} else {
			(void) fprintf(stderr,"%s:\tcannot open %s\n",
			    progname, file);
			(void) fprintf(stderr,"\t%s entry not made\n", new_entry);
		}
	}
	return(add_flag);
}
