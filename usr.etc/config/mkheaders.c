#ifndef lint
static	char sccsid[] = "@(#)mkheaders.c 1.1 92/07/30 SMI"; /* from UCB 1.13 82/12/09 */
#endif

/*
 * Make all the .h files for the optional entries
 */

#include <stdio.h>
#include <ctype.h>
#include "config.h"
#include "y.tab.h"

headers()
{
	register struct file_list *fl;

	for (fl = ftab; fl != 0; fl = fl->f_next) {
		/*
		 * No .h file for symbols in upper case.
		 */
		if (!fl->f_needs || isupper(fl->f_needs[0]))
			continue;
		do_count(fl->f_needs, fl->f_needs, 1);
	}
}

/*
 * count all the devices of a certain type and recurse to count
 * whatever the device is connected to
 */
do_count(dev, hname, search)
	register char *dev, *hname;
	int search;
{
	register struct device *dp, *mp;
	register int count;

	for (count = 0, dp = dtab; dp != 0; dp = dp->d_next) {
		if (dp->d_unit != -1 && eq(dp->d_name, dev)) {
			/*
			 * Avoid making .h files for bus types on sun machines
			 */
			if ((machine == MACHINE_SUN2 ||
			     machine == MACHINE_SUN3 ||
			     machine == MACHINE_SUN3X ||
			     machine == MACHINE_SUN4 ||
			     machine == MACHINE_SUN4C ||
			     machine == MACHINE_SUN4M) &&
			    dp->d_conn == TO_NEXUS)
				return;

			if (dp->d_type == PSEUDO_DEVICE) {
				count =
				    dp->d_slave != UNKNOWN ? dp->d_slave : 1;
				break;
			}

			if (machine != MACHINE_SUN2 &&
			    machine != MACHINE_SUN3 &&
			    machine != MACHINE_SUN3X &&
			    machine != MACHINE_SUN4 &&
			    machine != MACHINE_SUN4C &&
			    machine != MACHINE_SUN4M)
				/* avoid ie0,ie0,ie1 setting NIE to 3 */
				count++;

			/*
			 * Allow holes in unit numbering,
			 * assumption is unit numbering starts
			 * at zero.
			 */
			if (dp->d_unit + 1 > count)
				count = dp->d_unit + 1;

			if (search) {
				mp = dp->d_conn;
				if (mp != 0 && mp != TO_NEXUS &&
				    mp->d_conn != TO_NEXUS &&
				    mp->d_type != SCSIBUS) {
					/*
					 * Check for the case of the
					 * controller that the device
					 * is attached to is in a separate
					 * file (e.g. "sd" and "sc").
					 * In this case, do NOT define
					 * the number of controllers
					 * in the hname .h file.
					 */
					if (!file_needed(mp->d_name))
						do_count(mp->d_name, hname, 0);
					search = 0;
				}
			}
		}
	}
	do_header(dev, hname, count);
}

/*
 * Scan the file list to see if name is needed to bring in a file.
 */
file_needed(name)
	char *name;
{
	register struct file_list *fl;

	for (fl = ftab; fl != 0; fl = fl->f_next) {
		if (fl->f_needs && strcmp(fl->f_needs, name) == 0)
			return (1);
	}
	return (0);
}

do_header(dev, hname, count)
	char *dev, *hname;
	int count;
{
	char *file, *name, *inw, *toheader(), *tomacro();
	struct file_list *fl, *fl_head;
	FILE *inf, *outf;
	int inc, oldcount;

	file = toheader(hname);
	name = tomacro(dev);
	inf = fopen(file, "r");
	oldcount = -1;
	if (inf == 0) {
		outf = fopen(file, "w");
		if (outf == 0) {
			perror(file);
			exit(1);
		}
		fprintf(outf, "#define %s %d\n", name, count);
		(void) fclose(outf);
		return;
	}
	fl_head = 0;
	for (;;) {
		char *cp;

		if ((inw = get_word(inf)) == 0 || inw == (char *)EOF)
			break;
		if ((inw = get_word(inf)) == 0 || inw == (char *)EOF)
			break;
		inw = ns(inw);
		cp = get_word(inf);
		if (cp == 0 || cp == (char *)EOF)
			break;
		inc = atoi(cp);
		if (eq(inw, name)) {
			oldcount = inc;
			inc = count;
		}
		cp = get_word(inf);
		if (cp == (char *)EOF)
			break;
		fl = (struct file_list *) malloc(sizeof *fl);
		fl->f_fn = inw;
		fl->f_type = inc;
		fl->f_next = fl_head;
		fl_head = fl;
	}
	(void) fclose(inf);
	if (count == oldcount) {
		for (fl = fl_head; fl != 0; fl = fl->f_next)
			free((char *)fl);
		return;
	}
	if (oldcount == -1) {
		fl = (struct file_list *) malloc(sizeof *fl);
		fl->f_fn = name;
		fl->f_type = count;
		fl->f_next = fl_head;
		fl_head = fl;
	}
	outf = fopen(file, "w");
	if (outf == 0) {
		perror(file);
		exit(1);
	}
	for (fl = fl_head; fl != 0; fl = fl->f_next) {
		fprintf(outf, "#define %s %d\n",
		    fl->f_fn, fl->f_type);
		free((char *)fl);
	} 
	(void) fclose(outf);
}

/*
 * convert a dev name to a .h file name
 */
char *
toheader(dev)
	char *dev;
{
	static char hbuf[80];

	(void) strcpy(hbuf, path(dev));
	(void) strcat(hbuf, ".h");
	return (hbuf);
}

/*
 * convert a dev name to a macro name
 */
char *tomacro(dev)
	register char *dev;
{
	static char mbuf[20];
	register char *cp;

	cp = mbuf;
	*cp++ = 'N';
	while (*dev)
		*cp++ = toupper(*dev++);
	*cp++ = 0;
	return (mbuf);
}
