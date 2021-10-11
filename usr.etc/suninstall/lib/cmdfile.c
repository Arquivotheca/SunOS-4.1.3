#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)cmdfile.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)cmdfile.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		save_cmdfile()
 *
 *	Description:	Create a 'cmdfile' for the format command from
 *		the information in 'disk_p'.  The layout of a 'cmdfile'
 *		file is as follows if the label source is not DKL_EXISTING:
 *
 *			partition
 *			a <start_cyl> <n_blocks>
 *			...
 *			h <start_cyl> <n_blocks>
 *			quit
 *			label
 *			quit
 *
 *		If the label source is DKL_EXISTING, then no file is created.
 *
 *		Returns 1 if the file was saved successfully, and -1 if
 *		there was an error.
 */

#include <stdio.h>
#include "install.h"
#include "menu.h"




int
save_cmdfile(name, disk_p)
	char *		name;
	disk_info *	disk_p;
{
	FILE *		fp;			/* file pointer for name */
	int		i;			/* scratch index variable */


	/*
	 * If we are going to use the disk the way it is then there
	 * is nothing to do
	 */
	if (disk_p->label_source == DKL_EXISTING) 
		return(1);
	fp = fopen(name, "w");
	if (fp == NULL) {
		menu_log("%s: cannot open file for writing.", name);
		return (-1);
	}

	(void) fprintf(fp, "partition\n");

	for (i = 0; i < NDKMAP; i++) {
		if (map_blk(i + 'a'))
			(void) fprintf(fp, "%c %d %d\n", i + 'a',
				       map_cyl(i +'a'),
				       map_blk(i + 'a'));
		else
			(void) fprintf(fp, "%c 0 0\n", i + 'a');
	}
	(void) fprintf(fp, "quit\n");
	(void) fprintf(fp, "label\n");
	(void) fprintf(fp, "quit\n");

	(void) fclose(fp);

	return(1);
} /* end save_cmdfile() */
