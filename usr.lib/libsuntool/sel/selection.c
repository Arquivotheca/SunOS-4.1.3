#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)selection.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <sys/file.h>
#include <stdio.h>
#include <errno.h>
#include <suntool/selection.h>

extern	struct selection selnull;

#define	SEL_FILEMODE	0X0777

char	* selection_filename();

/*ARGSUSED*/
selection_set(sel, sel_write, sel_clear, windowfd)
	struct selection *sel;
	int (*sel_write)(), (*sel_clear)(), windowfd;
	/* Note: sel_clear not used yet */
{
	FILE	*file;
	int	firsttime = 1;

	(void)win_lockdata(windowfd);
Open:
	if ((file = fopen(selection_filename(), "w+")) == NULL) {
		if (firsttime) {
			firsttime = 0;
			if (unlink(selection_filename()) == 0)
				goto Open;
		}
		(void)win_unlockdata(windowfd);
		(void)fprintf(stderr, "%s would not open\n", selection_filename());
		(void)werror(-1, windowfd);
		return;
	}
	/* Make so everyone can access */
	(void)fchmod(fileno(file), 0666);
	(void)fprintf(file, "TYPE=%ld, ITEMS=%ld, ITEMBYTES=%ld, PUBFLAGS=%lx, PRIVDATA=%lx\n",
	    sel->sel_type, sel->sel_items, sel->sel_itembytes,
	    sel->sel_pubflags, sel->sel_privdata);
	(*sel_write)(sel, file);
	(void)fclose(file);
	(void)win_unlockdata(windowfd);
}

selection_get(sel_read, windowfd)
	int (*sel_read)(), windowfd;
{
	struct selection selection, *sel = &selection;
	FILE	*file;
	int	c;
	int	n;
	extern	errno;

	*sel = selnull;
	(void)win_lockdata(windowfd);
	if ((file = fopen(selection_filename(), "r")) == NULL) {
		(void)win_unlockdata(windowfd);
		if (errno == ENOENT)
			return;	/* No selection available */
		(void)fprintf(stderr, "%s would not open\n", selection_filename());
		(void)werror(-1, windowfd);
		return;
	}
	if ((c = getc(file)) == EOF) {
		goto Cleanup;		/* Selection has been cleared */
	} else
		(void)ungetc(c, file);
	if ((n = fscanf(file,
	    "TYPE=%ld, ITEMS=%ld, ITEMBYTES=%ld, PUBFLAGS=%lx, PRIVDATA=%lx%c",
	    &sel->sel_type, &sel->sel_items, &sel->sel_itembytes,
	    &sel->sel_pubflags, &sel->sel_privdata, &c)) != 6) {
		(void)win_unlockdata(windowfd);
		(void)fprintf(stderr, "%s not in correct format\n",
		    selection_filename());
              (void)fprintf(stderr, "TYPE=%ld, ITEMS=%ld, ITEMBYTES=%ld, PUBFLAGS=%lx, PRIVDATA=%lx c=%c, n=%ld\n",
		    sel->sel_type, sel->sel_items, sel->sel_itembytes,
		    sel->sel_pubflags, sel->sel_privdata, c, n);
		goto Cleanup;
	}
	(*sel_read)(sel, file);
Cleanup:
	(void)fclose(file);
	(void)win_unlockdata(windowfd);
}

selection_clear(windowfd)
	int	windowfd;
{
	FILE	*file;

	(void)win_lockdata(windowfd);
	if ((file = fopen(selection_filename(), "w+")) == NULL) {
		(void)win_unlockdata(windowfd);
		(void)fprintf(stderr, "%s would not open\n", selection_filename());
		(void)werror(-1, windowfd);
		return;
	}
	(void)fclose(file);
	(void)win_unlockdata(windowfd);
}

char *
selection_filename()
{
	char *getenv();
	char *name;

	if ((name = getenv("SELECTION_FILE")) == NULL)
		name = "/tmp/winselection";
	return(name);
}

