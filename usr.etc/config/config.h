/*	@(#)config.h 1.1 92/07/30 SMI; from UCB 5.3 4/18/86	*/

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 */

/*
 * Config.
 */
#include <sys/types.h>

#define	NODEV	((dev_t)-1)

struct file_list {
	struct	file_list *f_next;	
	char	*f_fn;			/* the name */
	u_char	f_type;			/* see below */
	u_char	f_flags;		/* see below */
	short	f_special;		/* requires special make rule */
	char	*f_needs;
	char	*f_fstype;		/* filesystem type */
	int	f_size;			/* swap size */
	struct file_list *f_root;	/* system root info */
	struct file_list *f_swap;	/* system swap list */
	struct file_list *f_dump;	/* system dump file */
};

/*
 * Types.
 */
#define DRIVER		1
#define NORMAL		2
#define	INVISIBLE	3
#define	PROFILING	4
#define	SYSTEMSPEC	5
#define	SWAPSPEC	6
#define	ROOTSPEC	7
#define	DUMPSPEC	8

/*
 * Attributes (flags).
 */
#define	CONFIGDEP	0x1
#define	SYMBOLIC_INFO	0x2

struct	vlst {
	struct	vlst *v_next;
	char	*v_id;
	int	v_vec;
};

struct device {
	int	d_type;			/* CONTROLLER, DEVICE, UBA or MBA */
	struct	device *d_conn;		/* what it is connected to */
	char	*d_name;		/* name of device (e.g. rk11) */
	char	*d_init;		/* name	of pseudo-dev init routine */
	struct	vlst *d_vec;		/* interrupt vectors */
	int	d_pri;			/* interrupt priority */
	int	d_addr;			/* address of csr */
	int	d_unit;			/* unit number */
	int	d_drive;		/* drive number */
	int	d_slave;		/* slave number */
#define QUES	-1		/* -1 means '?' */
#define	UNKNOWN -2		/* -2 means not set yet */
	int	d_dk;			/* if init 1 set to number for iostat */
	int	d_flags;		/* nlags for device init */
	struct	device *d_next;		/* Next one in list */
	u_short	d_mach;			/* Sun - machine type (0 = all)*/
	u_short	d_bus;			/* Sun - bus type (0 = unknown) */
};
#define TO_NEXUS	(struct device *)-1

struct config {
	char	*c_dev;
	char	*s_sysname;
};

/*
 * Config has a global notion of which machine type is
 * being used.  It uses the name of the machine in locating
 * files and in constructing make rules.  Thus if the name
 * of the machine is "vax", it will use "../vax/swapvmunix.c", etc.
 */
int	machine;
char	*machinename;
#define	MACHINE_VAX	1
#define	MACHINE_SUN2	2
#define	MACHINE_SUN3	3
#define	MACHINE_SUN4	4
#define	MACHINE_SUN3X	5
#define	MACHINE_SUN4C	6
#define	MACHINE_SUN4M	7

int	devinfo;		/* support devinfo interface */
int	mainbus;		/* support mainbus interface */

/*
 * For each machine, a set of CPU's may be specified as supported.
 * These and the options (below) are put in the C flags in the makefile.
 */
struct cputype {
	char	*cpu_name;
	struct	cputype *cpu_next;
} *cputype;

/*
 * A set of options may also be specified which are like CPU types,
 * but which may also specify values for the options.
 * A separate set of options may be defined for make-style options.
 */
struct opt {
	char	*op_name;
	char	*op_value;
	struct	opt *op_next;
} *opt, *mkopt;

/*
 * A simple-minded 'include' facility is defined in mkmakefile.c.
 * It is used, for example, in processing the "files" file.
 */
#define INCNEST 4	/* max nesting depth */
struct fstack {
	FILE *fp;	/* file pointer */
	char *nm;	/* file name */
};


char	*ident;
char	*ns();
char	*tc();
char	*qu();
char	*get_word();
char	*path();
char	*raise();

int	do_trace;

char	*index();
char	*rindex();
char	*malloc();
char	*strcpy();
char	*strcat();
char	*sprintf();

#if MACHINE_VAX
int	seen_mba, seen_uba;
#endif

struct	device *connect();
struct	device *dtab;
dev_t	nametodev();
char	*devtoname();

char	errbuf[80];
int	yyline;

struct	file_list *ftab, *conf_list, **confp;
char	*PREFIX;

int	profiling;

int	maxusers;

int	yyerror_invoked;

#define eq(a,b)	(!strcmp(a,b))
