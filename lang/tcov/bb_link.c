#ifndef lint
static	char sccsid[] = "@(#)bb_link.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/file.h>
#include "strings.h"


/*
 * CAVEAT: If you change the format of the following structure,
 * you must also change the code generator.  (cf., cgrdr/pcc.c)
 *
 * Iropt made me do this.
 */
struct bblink {
	int	initflag;		/* 1 if initialized */
	char	*filename;		/* source file name */
	unsigned int	*counters;	/* array of basic block counters */
	int	ncntrs;			/* num of elements in counters[] */
	struct	bblink	*next;		/* chain to next structure */
};

static	struct	bblink	*linkhdr=NULL;

static	int __tcov_init = 0;
static	void __tcov_init_func();

/* import */
extern	int getpid();

/* export */

void	__bb_init_func (/*bb*/);

/*
 * __tcov_exit - this routine is called from the standard exit routine
 * via the on_exit() routine.
 * It dumps the values of the counters to the appropriate
 * .d files.
 */
static void
__tcov_exit(retcode)
int	retcode;
{
	FILE	*dotd_fp;
	FILE	*tmp_fp;
	unsigned int	*bb;
	int	line;
	int	count;
	int	c;
	int	n;
	int	fd;
	int	tries;
	struct	bblink 	*linkp;
	static	int	beenhere;
	char	tmp[50];
	char	*lock = "/tmp/tcov.lock";
	extern	int	errno;

	if (beenhere) {
		unlink(lock);
		_exit(retcode);
	}
	for (tries = 0; tries < 5; tries++) {
		fd = open(lock, O_CREAT | O_EXCL, 666);
		if (fd == -1) {
			sleep(1);
		} else {
			break;
		}
	}
	if (fd != -1) {
		close(fd);
	} else {
		fprintf(stderr, "Tcov lock file is busy - could not write data\n");
		_exit(retcode);
	}
	beenhere = 1;
	sprintf(tmp, "/tmp/count.%d", getpid());
	for (linkp = linkhdr; linkp != NULL; linkp = linkp->next) {
		dotd_fp = fopen(linkp->filename, "r");
		if (dotd_fp == NULL) {
			fprintf(stderr, "count: Could not open %s, errno %d\n", 
				linkp->filename, errno);
			continue;
		}
		tmp_fp =  fopen(tmp, "w");
		if (tmp_fp == NULL) {
			fclose(dotd_fp);
			fprintf(stderr, "count: Could not create tmp file\n");
			continue;
		}
		bb = linkp->counters;
		n = linkp->ncntrs;
		while (n > 0 && fscanf(dotd_fp, "%d %d", &line, &count) == 2) {
			count += *bb;
			bb++;
			fprintf(tmp_fp, "%d %d\n", line, count);
			n--;
		}
		if (n != 0) {
			fprintf(stderr, "%s: Corrupt file: %d blocks left\n", 
				linkp->filename, n);
		}
		fclose(dotd_fp);
		fclose(tmp_fp);
		dotd_fp = fopen(linkp->filename, "w");
		if (dotd_fp == NULL) {
			unlink(tmp);
			continue;
		}
		tmp_fp = fopen(tmp, "r");
		if (tmp_fp == NULL) {
			unlink(tmp);
			fclose(dotd_fp);
			continue;
		}
		while ((c = getc(tmp_fp)) != EOF) {
			putc(c, dotd_fp);
		}
		fclose(dotd_fp);
		fclose(tmp_fp);
		unlink(tmp);
	}
	unlink(lock);
}

/*
 * Routines compiled with the -a option call __bb_init_func
 * the first time they are called.  They pass an initialized
 * bblink structure, which associates a source file name with
 * a list of counters for each basic block in the source file.
 *
 * History: this is copied from the Fortran version of bb_link.c.
 * The old C version didn't have a __bb_init_func, since the
 * bb_count preprocessor happily generated N similar but slightly
 * different versions of the same function (1 per source file).
 *
 * A more recent change is to require the compiler(s) to allocate
 * static storage for the bblink structure and the array of counters.
 * This used to call malloc(), which caused a great deal of trouble
 * with programs that use sbrk().
 */
void
__bb_init_func ( bb )
	struct bblink *bb;
{
	/*
	 * first time a tcov-compiled routine is called,
	 * arm the tcov exit handler.  Earlier versions
	 * used to catch signals here.  Bad idea for a test
	 * coverage analyzer.
	 */
	if (!__tcov_init) {
		on_exit(__tcov_exit, 0);
		__tcov_init = 1;
	}
	if (bb != NULL) {
		bb->next = linkhdr;
		linkhdr = bb;
	}
	bb->initflag = 1;
}
