/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)lp.h 1.1 92/07/30 SMI; from UCB 5.1 6/6/85
 */

/*
 * Global definitions for the line printer system.
 */

#include <stdio.h>
#include <sys/param.h>
#include <sys/file.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pwd.h>
#include <syslog.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "lp.local.h"

extern int	DU;		/* daeomon user-id */
extern int	MX;		/* maximum number of blocks to copy */
extern int	MC;		/* maximum number of copies allowed */
extern char	*LP;		/* line printer device name */
extern char	*RM;		/* remote machine name */
extern char	*RG;		/* restricted group */
extern char	*RP;		/* remote printer name */
extern char	*LO;		/* lock file name */
extern char	*ST;		/* status file name */
extern char	*SD;		/* spool directory */
extern char	*AF;		/* accounting file */
extern char	*LF;		/* log file for error messages */
extern char	*OF;		/* name of output filter (created once) */
extern char	*IF;		/* name of input filter (created per job) */
extern char	*RF;		/* name of fortran text filter (per job) */
extern char	*TF;		/* name of troff(1) filter (per job) */
extern char	*NF;		/* name of ditroff(1) filter (per job) */
extern char	*DF;		/* name of tex filter (per job) */
extern char	*GF;		/* name of graph(1G) filter (per job) */
extern char	*VF;		/* name of raster filter (per job) */
extern char	*CF;		/* name of cifplot filter (per job) */
extern char	*FF;		/* form feed string */
extern char	*TR;		/* trailer string to be output when Q empties */
extern short	SC;		/* suppress multiple copies */
extern short	SF;		/* suppress FF on each print job */
extern short	SH;		/* suppress header page */
extern short	SB;		/* short banner instead of normal header */
extern short	HL;		/* print header last */
extern short	RW;		/* open LP for reading and writing */
extern int	PW;		/* page width */
extern int	PX;		/* page width in pixels */
extern int	PY;		/* page length in pixels */
extern int	PL;		/* page length */
extern int	BR;		/* baud rate if lp is a tty */
extern int	FC;		/* flags to clear if lp is a tty */
extern int	FS;		/* flags to set if lp is a tty */
extern int	XC;		/* flags to clear for local mode */
extern int	XS;		/* flags to set for local mode */
extern short	RS;		/* restricted to those with local accounts */

extern char	line[BUFSIZ];
extern char	pbuf[];		/* buffer for printcap entry */
extern char	*bp;		/* pointer into ebuf for pgetent() */
extern char	*name;		/* program name */
extern char	*printer;	/* printer name */
extern char	host[MAXHOSTNAMELEN + 1]; /* host machine name */
extern char	*from;		/* client's machine name */
extern int 	from_remote;	/* true if job sent from remote machine */
extern int	errno;

/*
 * Structure used for building a sorted list of control files.
 */
struct queue {
	time_t	q_time;			/* modification time */
	char	q_name[MAXNAMLEN+1];	/* control file name */
};

char	*pgetstr();
char	*malloc();
char	*getenv();
char	*index();
char	*rindex();
