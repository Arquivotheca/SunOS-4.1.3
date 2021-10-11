%union {
	char	*str;
	int	val;
	struct	file_list *file;
	struct	vlst *vlst;
}

%token	AND
%token	ANY
%token	AT
%token	CHANNEL
%token	COMMA
%token	CONFIG
%token	CONTROLLER
%token	CPU
%token	CSR
%token	DEVICE
%token	DEVICE_DRIVER
%token	DISK
%token	DRIVE
%token	DST
%token	DUMPS
%token	EQUALS
%token	FLAGS
%token	IDENT
%token	INIT
%token	IPI
%token	IPI_ADDR
%token	LUN
%token	MACHINE
%token	MAJOR
%token	MASTER
%token	MAXUSERS
%token	MBA
%token	MINOR
%token	MINUS
%token	NEXUS
%token	ON
%token	OPTIONS
%token	MAKEOPTIONS
%token	PRIORITY
%token	PSEUDO_DEVICE
%token	ROOT
%token	SCSIBUS
%token	SEMICOLON
%token	SIZE
%token	SLAVE
%token	SWAP
%token	TARGET
%token	TRACE
%token	TYPE
%token	UBA
%token	VECTOR
%token	VME16D16
%token	VME24D16
%token	VME32D16
%token	VME16D32
%token	VME24D32
%token	VME32D32

%token	<str>	ID
%token	<val>	NUMBER
%token	<val>	FPNUMBER

%type	<str>	Save_id
%type	<str>	Opt_value
%type	<str>	Dev
%type	<vlst>	Vec_list
%type	<val>	Optional_size
%type	<str>	Device_name
%type	<str>	Optional_type
%type	<str>	Optional_name_spec
%type	<file>	File_name_spec

%{

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static	char sccsid[] = "@(#)config.y 1.1 92/07/30 SMI"; /* from UCB 5.2 4/18/86 */
#endif

#include "../../sys/sun/autoconf.h"	/* here to config from source */
#include <ctype.h>
#include <stdio.h>
#include "config.h"

struct	device cur;
struct	device *curp = 0;
char	*temp_id;
char	*val_id;
char	*malloc();
struct file_list *newfile();

%}
%%
Configuration:
	Many_specs
		= { verifysystemspecs(); }
		;

Many_specs:
	Many_specs Spec
		|
	/* lambda */
		;

Spec:
	Device_spec SEMICOLON
	      = { newdev(&cur); } |
	Config_spec SEMICOLON
		|
	TRACE SEMICOLON
	      = { do_trace = !do_trace; } |
	SEMICOLON
		|
	error SEMICOLON
		;

Config_spec:
	MACHINE Save_id
	    = {
/* do we still support "vax"??? */
		if (eq($2, "vax")) {
			machine = MACHINE_VAX;
			machinename = "vax";
		} else if (eq($2, "sun")) {
			yywarn("defaulting machine type to sun2");
			machine = MACHINE_SUN2;
			machinename = "sun2";
			mainbus = 1;
		} else if (eq($2, "sun2")) {
			machine = MACHINE_SUN2;
			machinename = "sun2";
			mainbus = 1;
		} else if (eq($2, "sun3")) {
			machine = MACHINE_SUN3;
			machinename = "sun3";
			mainbus = 1;
		} else if (eq($2, "sun4")) {
			machine = MACHINE_SUN4;
			machinename = "sun4";
			mainbus = 1;
		} else if (eq($2, "sun3x")) {
			machine = MACHINE_SUN3X;
			machinename = "sun3x";
			mainbus = 1;
		} else if (eq($2, "sun4c")) {
			machine = MACHINE_SUN4C;
			machinename = "sun4c";
			devinfo = 1;
		} else if (eq($2, "sun4m")) {
			machine = MACHINE_SUN4M;
			machinename = "sun4m";
			devinfo = 1;
			mainbus = 1; /* %%% this might not work */
		} else
			yyerror("Unknown machine type");
	      } |
	CPU Save_id
	      = {
		struct cputype *cp =
		    (struct cputype *)malloc(sizeof (struct cputype));
		cp->cpu_name = ns($2);
		cp->cpu_next = cputype;
		cputype = cp;
		free(temp_id);
	      } |
	OPTIONS Opt_list
		|
	MAKEOPTIONS Mkopt_list
		|
	IDENT ID
	      = { ident = ns($2); } |
	System_spec
		|
	MAXUSERS NUMBER
	      = { maxusers = $2; };

System_spec:
	  System_id System_parameter_list
		= { checksystemspec(*confp); }
	;

System_id:
	  CONFIG Save_id
		= { mkconf($2); }
	;

System_parameter_list:
	  System_parameter_list System_parameter
	| System_parameter
	;

System_parameter:
	  Swap_spec
	| Root_spec
	| Dump_spec
	;

File_name_spec:
	Optional_type Optional_name_spec
		= { $$ = newfile($1, $2, 0); }
	;

Optional_type:
	TYPE Save_id
		= { $$ = $2; }
	| /* empty */
		= { $$ = NULL; }
	;

Optional_name_spec:
	Device_name
		= { $$ = $1; }
	| /* empty */
		= {
			$$ = NULL;
		}
	;

Swap_spec:
	SWAP Optional_on File_name_spec Optional_size
		= {
			if ((*confp)->f_swap && (*confp)->f_swap->f_flags != 0)
				yywarn("extraneous swap device specification");
			else {
				$3->f_size = $4;
				(*confp)->f_swap = $3;
			}
		}
	;

Root_spec:
	ROOT Optional_on File_name_spec
		= {
			if ((*confp)->f_root && (*confp)->f_root->f_flags != 0)
				yywarn("extraneous root device specification");
			else
				(*confp)->f_root = $3;
		}
	;

Dump_spec:
	DUMPS Optional_on File_name_spec
		= {
			if ((*confp)->f_dump && (*confp)->f_dump->f_flags != 0)
				yywarn("extraneous dump device specification");
			else
				(*confp)->f_dump = $3;
		}

	;

Optional_on:
	  ON
	| /* empty */
	;

Optional_size:
	  SIZE NUMBER
	      = { $$ = $2; }
	| /* empty */
	      = { $$ = 0; }
	;

Device_name:
	Save_id
		= {
			char buf[80];
			if (strcmp($1, "generic") == 0)
				buf[0] = '\0';
			else
				strcpy(buf, $1);
			$$ = ns(buf); free($1);
		}
	;

Opt_list:
	Opt_list COMMA Option
		|
	Option
		;

Option:
	Save_id
	      = {
		struct opt *op = (struct opt *)malloc(sizeof (struct opt));
		op->op_name = ns($1);
		op->op_next = opt;
		op->op_value = 0;
		opt = op;
		free(temp_id);
	      } |
	Save_id EQUALS Opt_value
	      = {
		struct opt *op = (struct opt *)malloc(sizeof (struct opt));
		op->op_name = ns($1);
		op->op_next = opt;
		op->op_value = ns($3);
		opt = op;
		free(temp_id);
		free(val_id);
	      };

Opt_value:
	ID
	      = { $$ = val_id = ns($1); } |
	NUMBER
	      = { char nb[16]; (void)sprintf(nb, "%d", $1); $$ = val_id = ns(nb); };


Save_id:
	ID
	      = { $$ = temp_id = ns($1); }
	;

Device_spec:
	DEVICE Dev_name Dev_info Int_spec
		= { cur.d_type = DEVICE; } |
	MASTER Dev_name Dev_info Int_spec
		= { cur.d_type = MASTER; } |
	DISK Dev_name Dev_info Int_spec
		= { cur.d_dk = 1; cur.d_type = DEVICE; } |
	CHANNEL Dev_name Dev_info
		= { cur.d_drive = 0; cur.d_type = DEVICE; } |
	CONTROLLER Dev_name Dev_info Int_spec
		= { cur.d_type = CONTROLLER; } |
	PSEUDO_DEVICE Init_dev Dev Pseudo_init
		= {
		cur.d_name = $3;
		cur.d_type = PSEUDO_DEVICE;
		} |
	DEVICE_DRIVER Init_dev Dev
		= {
		cur.d_name = $3;
		cur.d_type = DEVICE_DRIVER; /* WAS PSEUDO_DEVICE */
		cur.d_unit = 0;	/* DID NOT EXIST */
		} |
	PSEUDO_DEVICE Init_dev Dev NUMBER Pseudo_init
	      = {
		cur.d_name = $3;
		cur.d_type = PSEUDO_DEVICE;
		cur.d_slave = $4;
		} |
	SCSIBUS Init_dev NUMBER AT Dev NUMBER
		= {
			cur.d_name = "scsibus";
			cur.d_type = SCSIBUS;
			cur.d_unit = $3;
			cur.d_conn = connect($5, $6);
		} |
	SCSIBUS Init_dev NUMBER AT Dev
		= {
			cur.d_name = "scsibus";
			cur.d_unit = $3;
			cur.d_type = SCSIBUS;
			cur.d_conn = connect($5, 0);
		};

Mkopt_list:
	Mkopt_list COMMA Mkoption
		|
	Mkoption
		;

Mkoption:
	Save_id EQUALS Opt_value
	      = {
		struct opt *op = (struct opt *)malloc(sizeof (struct opt));
		op->op_name = ns($1);
		op->op_next = mkopt;
		op->op_value = ns($3);
		mkopt = op;
		free(temp_id);
		free(val_id);
	      } ;

Dev:
	UBA
	      = {
		if (machine != MACHINE_VAX)
			yyerror("wrong machine type for uba");
		$$ = ns("uba");
		} |
	MBA
	      = {
		if (machine != MACHINE_VAX)
			yyerror("wrong machine type for mba");
		$$ = ns("mba");
		} |
	VME16D16
	      = {
		if (machine != MACHINE_SUN2 &&
		    machine != MACHINE_SUN3 &&
		    machine != MACHINE_SUN3X &&
		    machine != MACHINE_SUN4 &&
		    machine != MACHINE_SUN4M)
			yyerror("wrong machine type for vme16d16");
		$$ = ns("vme16d16");
		} |
	VME24D16
	      = {
		if (machine != MACHINE_SUN2 &&
		    machine != MACHINE_SUN3 &&
		    machine != MACHINE_SUN3X &&
		    machine != MACHINE_SUN4 &&
		    machine != MACHINE_SUN4M)
			yyerror("wrong machine type for vme24d16");
		$$ = ns("vme24d16");
		} |
	VME32D16
	      = {
		if (machine != MACHINE_SUN3 &&
		    machine != MACHINE_SUN4 &&
		    machine != MACHINE_SUN4M)
			yyerror("wrong machine type for vme32d16");
		$$ = ns("vme32d16");
		} |
	VME16D32
	      = {
		if (machine != MACHINE_SUN3 &&
		    machine != MACHINE_SUN3X &&
		    machine != MACHINE_SUN4 &&
		    machine != MACHINE_SUN4M)
			yyerror("wrong machine type for vme16d32");
		$$ = ns("vme16d32");
		} |
	VME24D32
	      = {
		if (machine != MACHINE_SUN3 &&
		    machine != MACHINE_SUN3X &&
		    machine != MACHINE_SUN4 &&
		    machine != MACHINE_SUN4M)
			yyerror("wrong machine type for vme24d32");
		$$ = ns("vme24d32");
		} |
	VME32D32
	      = {
		if (machine != MACHINE_SUN3 &&
		    machine != MACHINE_SUN3X &&
		    machine != MACHINE_SUN4 &&
		    machine != MACHINE_SUN4M)
			yyerror("wrong machine type for vme32d32");
		$$ = ns("vme32d32");
		} |
	IPI
		= {
		if (machine != MACHINE_SUN3 &&
		    machine != MACHINE_SUN3X &&
		    machine != MACHINE_SUN4 &&
		    machine != MACHINE_SUN4M)
			yyerror("wrong machine type for ipi");
		$$ = ns("ipi");
		} |
	ID
	      = { $$ = ns($1); };

Pseudo_init:
	INIT Save_id = {
		cur.d_init = $2;
		}
	| /* empty */
	;

Dev_name:
	Init_dev Dev NUMBER
	      = {
		cur.d_name = $2;
		if (eq($2, "mba"))
			seen_mba = 1;
		else if (eq($2, "uba"))
			seen_uba = 1;
		cur.d_unit = $3;
		};

Init_dev:
	/* lambda */
	      = { init_dev(&cur); };

Dev_info:
	Con_info Info_list
		|
	/* lambda */
		;

Con_info:
	AT Dev NUMBER
	      = {
		if (eq(cur.d_name, "mba") || eq(cur.d_name, "uba")) {
			(void)sprintf(errbuf, "%s must be connected to a nexus", cur.d_name);
			yyerror(errbuf);
		}
		cur.d_conn = connect($2, $3);
		/* Sbus devices don't have csr's, encode bus now instead */
		if (eq(cur.d_conn->d_name, "somewhere"))
			bus_encode(0, &cur);
		} |
	AT NEXUS NUMBER
	      = { check_nexus(&cur, $3); cur.d_conn = TO_NEXUS; } |
	AT SCSIBUS NUMBER
	      = {
		cur.d_conn = find_scsibus($3);
	      };

Info_list:
	Info_list Info
		|
	/* lambda */
		;

Info:
	CSR NUMBER
		{
		cur.d_addr = $2;
		if (machine == MACHINE_SUN2 ||
		    machine == MACHINE_SUN3 ||
		    machine == MACHINE_SUN3X ||
		    machine == MACHINE_SUN4 ||
		    machine == MACHINE_SUN4M)
			bus_encode($2, &cur);
		} |
	DRIVE NUMBER
	      = { cur.d_drive = $2; } |
	SLAVE NUMBER
	      = {
		if (cur.d_conn != 0 &&
		    cur.d_conn != TO_NEXUS &&
		    cur.d_conn->d_type == MASTER)
			cur.d_slave = $2;
		else
			yyerror("can't specify slave--not to master");
		} |
	IPI_ADDR NUMBER
		= { cur.d_flags = $2; } |
	FLAGS NUMBER
	      = { cur.d_flags = $2; } |
	TARGET NUMBER LUN NUMBER
	      = {
		cur.d_type = DEVICE;
		cur.d_slave = $2<<3 | $4;
	      };
Int_spec:
	Vec_spec
	      = { cur.d_pri = 0; } |
	PRIORITY NUMBER
	      = { cur.d_pri = $2; } |
	Vec_spec PRIORITY NUMBER
	      = { cur.d_pri = $3; } |
	PRIORITY NUMBER Vec_spec
	      = { cur.d_pri = $2; } |
	/* lambda */
		;

Vec_spec:
	VECTOR Vec_list
	      = { cur.d_vec = $2; };

Vec_list:
	Save_id
	      = {
		struct vlst *v = (struct vlst *)malloc(sizeof(struct vlst));
		v->v_next = 0; v->v_id = $1; v->v_vec = 0; $$ = v;
		} |
	Save_id NUMBER
	      = {
		struct vlst *v = (struct vlst *)malloc(sizeof(struct vlst));
		v->v_next = 0; v->v_id = $1; v->v_vec = $2; $$ = v;
		} |
	Save_id Vec_list
	      = {
		struct vlst *v = (struct vlst *)malloc(sizeof(struct vlst));
		v->v_next = $2; v->v_id = $1; v->v_vec = 0; $$ = v;
		} |
	Save_id NUMBER Vec_list
	      = {
		struct vlst *v = (struct vlst *)malloc(sizeof(struct vlst));
		v->v_next = $3; v->v_id = $1; v->v_vec = $2; $$ = v;
		};

%%

yywarn(s)
	char *s;
{

	fprintf(stderr, "config: line %d: %s\n", yyline, s);
}

yyerror(s)
	char *s;
{

	fprintf(stderr, "config: line %d: %s\n", yyline, s);
	yyerror_invoked++;
}

/*
 * return the passed string in a new space
 */
char *
ns(str)
	register char *str;
{
	register char *cp;

	cp = malloc((unsigned)(strlen(str)+1));
	(void) strcpy(cp, str);
	return (cp);
}

/*
 * add a device to the list of devices
 */
newdev(dp)
	register struct device *dp;
{
	register struct device *np;

	np = (struct device *) malloc(sizeof *np);
	*np = *dp;
	if (curp == 0)
		dtab = np;
	else
		curp->d_next = np;
	curp = np;
}

/*
 * note that a configuration should be made
 */
mkconf(sysname)
	char *sysname;
{
	register struct file_list *fl, **flp;

	fl = newfile(0, 0, 0);
	fl->f_type = SYSTEMSPEC;
	fl->f_needs = sysname;
	fl->f_fn = sysname;
	for (flp = confp; *flp; flp = &(*flp)->f_next)
		;
	*flp = fl;
	confp = flp;
}

struct file_list *
newfile(type, name, size)
	char *type;
	char *name;
	int size;
{
	register struct file_list *new;

	new = (struct file_list *)malloc(sizeof(*new));
	if (new == 0) {
		yyerror("out of memory");
		return (0);
	}
	bzero(new, sizeof(*new));
	new->f_fstype = type;
	new->f_size = size;
	if (name && !eq(name, "generic")) {
		new->f_fn = name;
	}
	return (new);
}

/*
 * find the pointer to connect to the given device and number.
 * returns 0 if no such device and prints an error message
 */
struct device *
connect(dev, num)
	register char *dev;
	register int num;
{
	register struct device *dp;
	struct device *huhcon();

	if (num == QUES)
		return (huhcon(dev));
	for (dp = dtab; dp != 0; dp = dp->d_next) {
		if ((num != dp->d_unit) || !eq(dev, dp->d_name))
			continue;
		if ((dp->d_type != CONTROLLER) &&
		    (dp->d_type != MASTER) &&
		    (dp->d_type != DEVICE_DRIVER)) {
			(void)sprintf(errbuf, "%s connected to non-controller", dev);
			yyerror(errbuf);
			return (0);
		}
		return (dp);
	}
	(void)sprintf(errbuf, "%s %d not defined", dev, num);
	yyerror(errbuf);
	return (0);
}

/*
 * connect to an unspecific thing
 */
struct device *
huhcon(dev)
	register char *dev;
{
	register struct device *dp, *dcp;
	struct device rdev;
	int oldtype;

	/*
	 * First make certain that there are some of these to wildcard on
	 */
	for (dp = dtab; dp != 0; dp = dp->d_next)
		if (eq(dp->d_name, dev))
			break;
	if (dp == 0) {
		(void)sprintf(errbuf, "no %s's to wildcard", dev);
		yyerror(errbuf);
		return (0);
	}
	oldtype = dp->d_type;
	dcp = dp->d_conn;
	/*
	 * Now see if there is already a wildcard entry for this device
	 * (e.g. Search for a "uba ?")
	 */
	for (; dp != 0; dp = dp->d_next)
		if (eq(dev, dp->d_name) && dp->d_unit == QUES)
			break;
	/*
	 * If there isn't, make one because everything needs to be connected
	 * to something.
	 */
	if (dp == 0) {
		dp = &rdev;
		init_dev(dp);
		dp->d_unit = QUES;
		dp->d_name = ns(dev);
		dp->d_type = oldtype;
		newdev(dp);
		dp = curp;
		/*
		 * Connect it to the same thing that other similar things are
		 * connected to, but make sure it is a wildcard unit
		 * (e.g. up connected to sc ?, here we make connect sc? to a
		 * uba?).  If other things like this are on the NEXUS or
		 * if they aren't connected to anything, then make the same
		 * connection, else call ourself to connect to another
		 * unspecific device.
		 */
		if (dcp == TO_NEXUS || dcp == 0)
			dp->d_conn = dcp;
		else
			dp->d_conn = connect(dcp->d_name, QUES);
	}
	return (dp);
}

init_dev(dp)
	register struct device *dp;
{

	dp->d_name = "OHNO!!!";
	dp->d_type = DEVICE;
	dp->d_addr = dp->d_slave = dp->d_drive = dp->d_unit = UNKNOWN;
	dp->d_mach = dp->d_bus = 0;
	dp->d_pri = dp->d_flags = dp->d_dk = 0;
	dp->d_conn = (struct device *)0;
	dp->d_vec = (struct vlst *)0;
	dp->d_init = 0;
}

/*
 * make certain that this is a reasonable type of thing to connect to a nexus
 */
check_nexus(dev, num)
	register struct device *dev;
	int num;
{

	switch (machine) {

	case MACHINE_VAX:
		if (!eq(dev->d_name, "uba") && !eq(dev->d_name, "mba"))
			yyerror("only uba's and mba's should be connected to the nexus");
		if (num != QUES)
			yyerror("can't give specific nexus numbers");
		break;

	case MACHINE_SUN2:
		if (!eq(dev->d_name, "virtual") &&
		    !eq(dev->d_name, "obmem") &&
		    !eq(dev->d_name, "obio") &&
		    !eq(dev->d_name, "ipi") &&
		    !eq(dev->d_name, "mbmem") &&
		    !eq(dev->d_name, "mbio") &&
		    !eq(dev->d_name, "vme16d16") &&
		    !eq(dev->d_name, "vme24d16")) {
			(void)sprintf(errbuf,
			    "unknown bus type `%s' for nexus connection on %s",
			    dev->d_name, machinename);
			yyerror(errbuf);
		}
		break;

	case MACHINE_SUN3:
	case MACHINE_SUN4:
		if (!eq(dev->d_name, "virtual") &&
		    !eq(dev->d_name, "obmem") &&
		    !eq(dev->d_name, "obio") &&
		    !eq(dev->d_name, "ipi") &&
		    !eq(dev->d_name, "vme16d16") &&
		    !eq(dev->d_name, "vme24d16") &&
		    !eq(dev->d_name, "vme32d16") &&
		    !eq(dev->d_name, "vme16d32") &&
		    !eq(dev->d_name, "vme24d32") &&
		    !eq(dev->d_name, "vme32d32")) {
			(void)sprintf(errbuf,
			    "unknown bus type `%s' for nexus connection on %s",
			    dev->d_name, machinename);
			yyerror(errbuf);
		}
		break;

	case MACHINE_SUN3X:
		if (!eq(dev->d_name, "virtual") &&
		    !eq(dev->d_name, "obmem") &&
		    !eq(dev->d_name, "obio") &&
		    !eq(dev->d_name, "ipi") &&
		    !eq(dev->d_name, "vme16d16") &&
		    !eq(dev->d_name, "vme16d32") &&
		    !eq(dev->d_name, "vme24d16") &&
		    !eq(dev->d_name, "vme24d32") &&
		    !eq(dev->d_name, "vme32d32")) {
			(void)sprintf(errbuf,
			    "unknown bus type `%s' for nexus connection on %s",
			    dev->d_name, machinename);
			yyerror(errbuf);
		}
		break;

	case MACHINE_SUN4C:
		if (!eq(dev->d_name, "somewhere")) {
			(void)sprintf(errbuf,
			    "unknown bus type `%s' for nexus connection on %s",
			    dev->d_name, machinename);
			yyerror(errbuf);
		}
		break;

	case MACHINE_SUN4M:
		if (!eq(dev->d_name, "somewhere") &&
		    !eq(dev->d_name, "virtual") &&
		    !eq(dev->d_name, "obmem") &&
		    !eq(dev->d_name, "obio") &&
		    !eq(dev->d_name, "ipi") &&
		    !eq(dev->d_name, "vme16d16") &&
		    !eq(dev->d_name, "vme24d16") &&
		    !eq(dev->d_name, "vme32d16") &&
		    !eq(dev->d_name, "vme16d32") &&
		    !eq(dev->d_name, "vme24d32") &&
		    !eq(dev->d_name, "vme32d32")) {
			(void)sprintf(errbuf,
			    "unknown bus type `%s' for nexus connection on %s",
			    dev->d_name, machinename);
			yyerror(errbuf);
		}
		break;
	}
}

/*
 * Check system specification and apply defaulting
 * rules on root, dump, and swap devices.
 */
checksystemspec(fl)
	register struct file_list *fl;
{
	char buf[BUFSIZ];
	register struct file_list *swap;

	if (fl == 0 || fl->f_type != SYSTEMSPEC) {
		yyerror("internal error, bad system specification");
		exit(1);
	}
}

/*
 * Verify all devices specified in the system specification
 * are present in the device specifications.
 */
verifysystemspecs()
{
}

/*
 * bi_info gives the magic number used to construct the token for
 * the autoconf code.  bi_max is the maximum value (across all
 * machine types for a given architecture) that a given "bus
 * type" can legally have.
 */
struct bus_info {
	char	*bi_name;
	u_short	bi_info;
	u_int	bi_max;
};

#define MAX_ADDR	(~0)		/* (1<<32)-1 */

struct bus_info sun2_info[] = {
	{ "virtual",	SP_VIRTUAL,	(1<<24)-1 },
	{ "obmem",	SP_OBMEM,	(1<<23)-1 },
	{ "obio",	SP_OBIO,	(1<<23)-1 },
	{ "mbmem",	SP_MBMEM,	(1<<20)-1 },
	{ "mbio",	SP_MBIO,	(1<<16)-1 },
	{ "vme16d16",	SP_VME16D16,	(1<<16)-1 },
	{ "vme24d16",	SP_VME24D16,	(1<<24)-(1<<16)-1 },
	{ (char *)0,	0,	0 }
};

struct bus_info sun3_info[] = {
	{ "virtual",	SP_VIRTUAL,	MAX_ADDR },
	{ "obmem",	SP_OBMEM,	MAX_ADDR },
	{ "obio",	SP_OBIO,	(1<<21)-1 },
	{ "ipi",	SP_IPI,		MAX_ADDR },
	{ "vme16d16",	SP_VME16D16,	(1<<16)-1 },
	{ "vme24d16",	SP_VME24D16,	(1<<24)-(1<<16)-1 },
	{ "vme32d16",	SP_VME32D16,	MAX_ADDR-(1<<24) },
	{ "vme16d32",	SP_VME16D32,	(1<<16) },
	{ "vme24d32",	SP_VME24D32,	(1<<24)-(1<<16)-1 },
	{ "vme32d32",	SP_VME32D32,	MAX_ADDR-(1<<24) },
	{ (char *)0,	0,	0 }
};

struct bus_info sun4_info[] = {
	{ "virtual",	SP_VIRTUAL,	MAX_ADDR },
	{ "obmem",	SP_OBMEM,	MAX_ADDR },
	{ "obio",	SP_OBIO,	MAX_ADDR },
	{ "ipi",	SP_IPI,		MAX_ADDR },
	{ "vme16d16",	SP_VME16D16,	(1<<16)-1 },
	{ "vme24d16",	SP_VME24D16,	(1<<24)-(1<<16)-1 },
	{ "vme32d16",	SP_VME32D16,	MAX_ADDR-(1<<24) },
	{ "vme16d32",	SP_VME16D32,	(1<<16) },
	{ "vme24d32",	SP_VME24D32,	(1<<24)-(1<<16)-1 },
	{ "vme32d32",	SP_VME32D32,	MAX_ADDR-(1<<24) },
	{ (char *)0,	0,	0 }
};

struct bus_info sun3x_info[] = {
	{ "virtual",	SP_VIRTUAL,	MAX_ADDR },
	{ "obmem",	SP_OBMEM,	0x58000000-1 },
	{ "obio",	SP_OBIO,	0x70000000-1 },
	{ "ipi",	SP_IPI,		MAX_ADDR },
	{ "vme16d16",	SP_VME16D16,	(1<<16)-1 },
	{ "vme16d32",   SP_VME16D32,    (1<<16)-1 },
	{ "vme24d16",	SP_VME24D16,	(1<<24)-1 },
	{ "vme24d32",	SP_VME24D32,	(1<<24)-1 },
	{ "vme32d32",	SP_VME32D32,	(1<<31)-1 },
	{ (char *)0,	0,	0 }
};

struct bus_info sun4c_info[] = {
	{ (char *)0,	0,	0 }
};
struct bus_info sun4m_info[] = {
	{ "virtual",	SP_VIRTUAL,	MAX_ADDR },
	{ "obmem",	SP_OBMEM,	MAX_ADDR },
	{ "obio",	SP_OBIO,	MAX_ADDR },
	{ "ipi",	SP_IPI,		MAX_ADDR },
	{ "vme16d16",	SP_VME16D16,	(1<<16)-1 },
	{ "vme24d16",	SP_VME24D16,	(1<<24)-(1<<16)-1 },
	{ "vme32d16",	SP_VME32D16,	MAX_ADDR-(1<<24) },
	{ "vme16d32",	SP_VME16D32,	(1<<16) },
	{ "vme24d32",	SP_VME24D32,	(1<<24)-(1<<16)-1 },
	{ "vme32d32",	SP_VME32D32,	MAX_ADDR-(1<<24) },
	{ (char *)0,	0,	0 }
};

bus_encode(addr, dp)
	u_int addr;
	register struct device *dp;
{
	register char *busname;
	register struct bus_info *bip;
	register int num;

	switch (machine) {
	case MACHINE_SUN2:
		bip =sun2_info;
		break;
	case MACHINE_SUN3:
		bip =sun3_info;
		break;
	case MACHINE_SUN4:
		bip =sun4_info;
		break;
	case MACHINE_SUN3X:
		bip =sun3x_info;
		break;
	case MACHINE_SUN4C:
		bip =sun4c_info;
		break;
	case MACHINE_SUN4M:
		bip =sun4m_info;
		break;
	default:
		yyerror("bad machine type for bus_encode");
		exit(1);
	}

	if (dp->d_conn == TO_NEXUS || dp->d_conn == 0) {
		yyerror("bad connection");
		exit(1);
	}

	busname = dp->d_conn->d_name;
	num = dp->d_conn->d_unit;

	for (; bip->bi_name != 0; bip++)
		if (eq(busname, bip->bi_name))
			break;

	if (bip->bi_name == 0) {
		(void)sprintf(errbuf, "bad bus type '%s' for machine %s",
			busname, machinename);
		yyerror(errbuf);
	} else if (addr > bip->bi_max) {
		(void)sprintf(errbuf,
			"0x%x exceeds maximum address 0x%x allowed for %s",
			addr, bip->bi_max, busname);
		yyerror(errbuf);
	} else {
		dp->d_bus = bip->bi_info;	/* set up bus type info */
		if (num != QUES)
			/*
			 * Set up cpu type since the connecting
			 * bus type is not wildcarded.
			 */
			dp->d_mach = num;
	}
}

/*
 * find the pointer to a scsibus to connect this scsi device to.
 * returns 0 if no such device and prints an error message
 */
struct device *
find_scsibus(num)
	register int num;
{
	register struct device *dp;

	for (dp = dtab; dp != 0; dp = dp->d_next) {
		if (dp->d_type != SCSIBUS || num != dp->d_unit)
			continue;
		return (dp);
	}
	(void)sprintf(errbuf, "scsibus %d not defined", num);
	yyerror(errbuf);
	return (0);
}
