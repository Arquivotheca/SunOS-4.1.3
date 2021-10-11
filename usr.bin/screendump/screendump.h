/*      @(#)screendump.h 1.1 92/07/30 SMI      */

/*
 * Copyright 1984, 1986 by Sun Microsystems, Inc.
 */

/*
 * Common definitions for
 *	screendump, screenload, rastrepl, rasfilter8to1
 */

#include <stdio.h>
#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/memvar.h>
#include <pixrect/pr_io.h>
#include <pixrect/pr_planegroups.h>
#include <rasterfile.h>

extern char *optarg;
extern int getopt(), optind, opterr;

extern char *basename();

extern char *Progname;
extern void error();

extern Pixrect *xmem_create();
extern char *xmalloc();
