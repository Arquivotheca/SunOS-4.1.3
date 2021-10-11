/* @(#)modloadconf.c 1.1 92/07/30 */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <nlist.h>
#include <sys/param.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/conf.h>
#include <sys/buf.h>
#include <sundev/mbvar.h>
#include <sun/autoconf.h>
#include <sun/vddrv.h>

/*
 * Process the configuration file.  You could get as fancy as you like here.
 * The code from the config program could be used to do what we have
 * to do here.  But for now, the simple table driven approach suffices.
 *
 * getconfinfo() reads each line of the configuration file and produces
 * a linked list of confinfo entries.  There is one confinfo entry per
 * non-comment line in the configuration file.  Then, the list of confinfo
 * entries is checked for consistency and if it's ok, an array of
 * vddrv entries is produced.
 *
 * The vddrv array is the actual output of this routine.  A pointer to
 * the start of the array is then used in the VDLOAD ioctl to load the module.
 *
 * Here are the types of configuration commands that we support:
 *
 * 0) # a comment line begins with a "#"
 * 1) controller <name><number> at {atmem, atio, obmem} <options>
 * 2) device	 <name><number> at {<controller>, atmem, atio, obmem} <options>
 * 3) disk	 <name><number> at {<controller>, atmem, atio, obmem} <options>
 * 4) blockmajor <number>
 * 5) charmajor  <number>
 *
 * where <options> are
 *	drive	  <drive number>
 *	csr	  <csr address>
 *	irq	  <interrupt request line>
 *	priority  <interrupt priority>
 *	dmachan	  <dma channel>
 *	flags	  <flags>
 *
 * and <controller> on the "device" line means the <name><number> of
 * a controller.  <name> must not have any numbers in it.
 *
 * For example:
 *
 * controller	fdc0 at atmem ? csr 0x001000 irq 6 priority 3
 * controller	fdc2 at atmem ? csr 0x002000 irq 5 priority 2
 * disk		fd0 at fdc0 drive 0
 * #disk	fd0 at fdc0 drive 1
 * disk		fd0 at fdc0 drive 2
 * device	fd0 at fdc2 drive 0
 * disk		fd0 at fdc2 drive 1
 * blockmajor	51
 * charmajor	52
 */

extern char *calloc();
extern long strtol();

extern char *outputfile;

#ifndef sun4c
static void conf_controller(), conf_disk(), conf_device();
#endif /* sun4c */
static void conf_blockmajor(), conf_charmajor(), conf_syscallnum();

#ifndef sun4c
static struct mb_ctlr *gen_ctlr();
static struct mb_device *gen_device();
#ifndef sun386
static struct vec *gen_vector();
#endif
#endif /* sun4c */

static void parse_line(), read_literal();
static struct vdconf *gen_vdconf();
static struct confinfo *getcip();
static char *alloc(), *getword(), *getname(), *getstring();
static int getint();
#ifndef sun4c
static int getnum(), getctlrnum();
#endif /* sun4c */

#ifndef sun4c
static void getdrive(), getcsr(), getirq(), getpriority(), getvector();
static void getdmachan(), getflags(), getbusconn();
#endif /* sun4c */

struct veclist {			/* List of VME interrupt vectors */
	struct veclist *next;
	struct vec vec;
};

/*
 * An instance of this structure is malloc'd and filled in for each non-comment
 * line in the configuration file.  After we've read the entire configuration
 * file, we go through this array checking for consistency and then generate
 * the vdconf array.
 */
static struct confinfo {
	struct	confinfo *next;		/* pointer to next */
	int	type;			/* type of configuration info */
	int	linenum;		/* linenumber this was gen'd for */
	char	*name;			/* controller or device name */
	char	*conname;		/* name of connection (ctlr or bus) */
	int	disk;			/* conf type is "disk" */
	int	space;			/* bus type */
	int	drive;			/* drive number */
	int	csr;			/* csr address */
	int	irq;			/* interrupt request number */
	int	priority;		/* interrupt priority */
	struct veclist *vectors;	/* VME interrupt vectors */
	int	dmachan;		/* dma channel */
	int	flags;			/* flags */
	int	majornum;		/* major number */
	int	syscallnum;		/* system call number */
	caddr_t	data;			/* pointer to generic data */
} *confinfo;

/*
 * Dispatch table for each type of configuration command.
 */
static struct confdispatch {
	char	*conftype;		/* configuration command */
	void	(*conffcn)();		/* function to process this type */
} confdispatch[] = {
#ifndef sun4c
	{ "controller",	conf_controller },
	{ "device",	conf_device	},
	{ "disk",	conf_disk	},
#endif /* sun4c */
	{ "blockmajor",	conf_blockmajor	},
	{ "charmajor",	conf_charmajor	},
	{ "syscallnum",	conf_syscallnum	},
	{ NULL,		0		},
};

/*
 * Various bus types.
 */
static struct bustype {
	char	*name;
	int	type;
} bustype[] = {
#ifdef sun386
	{ "obmem",	SP_OBMEM	},
	{ "atmem",	SP_ATMEM	},
	{ "atio",	SP_ATIO		},
	{ NULL, 	0		}
#else
	{ "obmem",	SP_OBMEM	},
	{ "obio",	SP_OBIO		},
	{ "mbmem",	SP_MBMEM	},
	{ "mbio",	SP_MBIO		},
	{ "vme16d16",	SP_VME16D16	},
	{ "vme24d16",	SP_VME24D16	},
	{ "vme32d16",	SP_VME32D16	},
	{ "vme16d32",	SP_VME16D32	},
	{ "vme24d32",	SP_VME24D32	},
	{ "vme32d32",	SP_VME32D32	},
	{ NULL,		0		}
#endif
};

/*
 * Configuration options.
 */
static struct options {
	char	*name;
	void	(*optfcn)();
} options[] = {
#ifndef sun4c
	{ "drive",	getdrive	},
	{ "csr",	getcsr		},
	{ "vector",	getvector	},
	{ "priority",	getpriority	},
	{ "irq",	getirq		},
#ifdef sun386
	{ "dmachan",	getdmachan	},
#endif
	{ "flags",	getflags	},
#endif /* sun4c */
	{ NULL,		NULL		}
};

#define	MAXLINE		1000
#define	COMMENT_CHAR	'#'
#define	streq(a, b)	(strcmp(a, b) == 0)
#define	new_array(T, n)	((T *)alloc(n, sizeof (T)))
#define	new_struct(T)	new_array(T, 1)
#define	new_string(n)	new_array(char, n)

static char line[MAXLINE];	/* current input line */
static int linenum = 0;		/* configuration file line number */
static int confentries = 0;	/* number of confinfo entries */

extern int dopt;		/* debug option */

/*
 * Process the configuration file.  (See comments above.)
 *
 * Returns NULL if conffile is NULL.
 * Returns vdconf if all is well.
 * If there's a fatal error, this routine prints an error message and exits.
 */
struct vdconf *
getconfinfo(conffile)
	char *conffile;		/* pointer to configuration file name */
{
	FILE *fp;

	if (conffile == NULL)
		return (NULL);	/* no configuration file */

	if ((fp = fopen(conffile, "r")) == NULL)
		error("can't open configuration file");

	while (!feof(fp))
		parse_line(fp);

	return (gen_vdconf(confinfo));	   /* generate vdconf table */
}


static void
parse_line(fp)
	FILE *fp;
{
	struct confdispatch *cdp;
	static struct confinfo *cip;
	char *word;
	int linelen;

	linenum++;
	if (fgets(line, MAXLINE, fp) == NULL)
		return;

	linelen = strlen(line);
	if (linelen == MAXLINE - 1 && line[linelen] != '\n')
		fatal("config file line %d too long\n", linenum);

	word = getword(fp); 	/* get first word of next line */
	if (word == NULL) 	/* empty line */
		return;
	/*
	 * dispatch to the appropriate configuration routine
	 */
	for (cdp = confdispatch; cdp->conftype != NULL; cdp++)
		if (streq(cdp->conftype, word))
			break;

	if (cdp->conftype != NULL) {
		if (confinfo == NULL)
			confinfo = cip = getcip();
		else {
			cip->next = getcip();
			cip = cip->next;
		}
		cip->linenum = linenum;
		(*cdp->conffcn)(fp, cip);
		confentries++;
	} else
		fatal("Unrecognized config type '%s' on line %d\n",
			word, linenum);
}

/*
 * Loop through all of the confinfo entries and produce the vdconf array.
 */
static struct vdconf *
gen_vdconf(confinfo)
	struct confinfo *confinfo;
{
	struct confinfo *cip;
	struct vdconf *vdconf, *vdc;

	vdconf = new_array(struct vdconf, confentries + 1);
	vdconf[confentries].vdc_type = VDCEND;	/* terminate table */

	for (cip = confinfo, vdc = vdconf; cip; cip = cip->next, vdc++) {
		vdc->vdc_type = cip->type;
		switch (cip->type) {
#ifndef sun4c
		case VDCCONTROLLER:
			vdc->vdc_data = (long)gen_ctlr(cip);
			break;

		case VDCDEVICE:
			vdc->vdc_data = (long)gen_device(cip);
			break;
#endif /* sun4c */

		case VDCBLOCKMAJOR:
			vdc->vdc_data = (long)cip->majornum;
			break;

		case VDCCHARMAJOR:
			vdc->vdc_data = (long)cip->majornum;
			break;

		case VDCSYSCALLNUM:
			vdc->vdc_data = (long)cip->syscallnum;
			break;

		default:
			fatal("Internal error:  unknown type %x (line %d)\n",
				cip->type, cip->linenum);
		}
	}
	return (vdconf);
}

#ifndef sun4c
/*
 * Generate an mb_ctlr entry for this controller.
 */
static struct mb_ctlr *
gen_ctlr(cip)
	struct confinfo *cip;
{
	struct mb_ctlr *mc;
	struct confinfo *c;

	if (dopt)
		printf("gen_ctlr\n");

	/*
	 * Check for duplicate controller names
	 */
	for (c = confinfo; c != NULL; c = c->next) {
		if (c == cip || c->type != VDCCONTROLLER)
			continue;

		if (streq(c->name, cip->name))
			fatal(
		"Duplicate controller name '%s' (lines %d and %d)\n",
		c->name, c->linenum, cip->linenum);
	}

	mc = new_struct(struct mb_ctlr);
	mc->mc_ctlr	= getnum(cip);
	mc->mc_addr	= (caddr_t)cip->csr;
	mc->mc_intpri	= cip->priority;
	mc->mc_space	= cip->space;
#ifdef sun386
	mc->mc_intr	= cip->irq;
	mc->mc_dmachan	= cip->dmachan;
#else
	mc->mc_intr	= gen_vector(cip->vectors);
#endif
	return (mc);
}

/*
 * Generate an mb_device entry for this device.
 */
static struct mb_device *
gen_device(cip)
	struct confinfo *cip;
{
	struct mb_device *md;
	struct confinfo *c;

	if (dopt)
		printf("gen_device\n");

	/*
	 * Check for duplicate devices on the same controller
	 */
	for (c = confinfo; c; c = c->next) {
		if (c == cip || c->type != VDCDEVICE ||
		    c->drive != cip->drive ||
		    getctlrnum(c) != getctlrnum(cip))
			continue;

		if (streq(c->name, cip->name))
			fatal(
		"Duplicate device name '%s' (lines %d and %d)\n",
		c->name, c->linenum, cip->linenum);
	}

	md = new_struct(struct mb_device);
	md->md_unit	= getnum(cip);
	md->md_ctlr	= getctlrnum(cip);
	md->md_slave	= cip->drive;
	md->md_addr	= (caddr_t)cip->csr;
	md->md_intpri	= cip->priority;
	md->md_space	= cip->space;
#ifdef sun386
	md->md_intr	= cip->irq;
	md->md_dmachan	= cip->dmachan;
#else
	md->md_intr	= gen_vector(cip->vectors);
#endif
	return (md);
}

#ifndef sun386

static struct vec *
gen_vector(veclist)
	struct veclist *veclist;
{
	int nvec;
	struct veclist *vp, *nvp;
	struct vec *v, *v0;
	int *ip;

	nvec = 0;
	for (vp = veclist; vp != NULL; vp = vp->next)
		nvec++;

	if (nvec == 0)
		return (NULL);

	v0 = v = new_array(struct vec, nvec + 1);
	ip = new_array(int, nvec + 1);

	for (vp = veclist; vp != NULL; vp = nvp) {
		*v = vp->vec;
		v->v_vptr = ip;
		*ip++ = v->v_vec;
		v++;
		nvp = vp->next;
		free((char *)vp);
	}
	v = NULL;

	return ((struct vec *)v0);
}

#endif

/*
 * Get the number from the controller or device name.
 * For example, if the name is fdc2 then this routine will return 2.
 */
static int
getnum(cip)
	struct confinfo *cip;
{
	char *np = cip->name, *p;
	long num;

	while (*np != '\0' && !isdigit(*np))
		np++;

	if (*np == '\0')
		fatal("missing unit number in '%s' on line %d\n",
			cip->name, cip->linenum);
	num = strtol(np, &p, 10);
	if (*p != '\0')
		fatal("Invalid device or controller name '%s' (line %d)\n",
			cip->name, cip->linenum);

	return (num);
}

/*
 * Get the controller number for this device.
 * returns -1 if there is no controller.
 */
static int
getctlrnum(cip)
	struct confinfo *cip;
{
	struct confinfo *c;
	char *name, *cname;
	int namelen;

	if (dopt)
		printf("getctlrnum\n");

	if (cip->space != 0)
		return (-1);		/* no controller */

	for (c = confinfo; c; c = c->next) {
		if (c->type != VDCCONTROLLER)
			continue;

		if (streq(cip->conname, c->name)) {
			/*
			 * We've found the controller that we're connected to.
			 * Now let's make sure that our name makes sense.
			 * For example, controller "fdc0" is supposed to have
			 * devices with names such as "fd0", "fd1", etc.
			 */
			name = getname(cip);	/* get dev name without num */
			cname = getname(c);	/* get ctrl name without num */
			namelen = strlen(name);

			if (strncmp(name, cname, namelen) != 0 ||
			    cname[namelen] != 'c')
				fatal(
	"invalid device name '%s' for controller %s (lines %d and %d)\n",
	cip->name, c->name, cip->linenum, c->linenum);

			return (getnum(c));
		}
	}
	fatal(
	"Device '%s' is connected to an unknown controller '%s' (line %d)\n",
	cip->name, cip->conname, cip->linenum);
	/*NOTREACHED*/
}

/*
 * Get the name (minus the number) of a controller or device.
 */
static char *
getname(cip)
	struct confinfo *cip;
{
	int len;
	char *p;

	if (dopt)
		printf("getname\n");

	p = cip->name;
	while (*p != '\0' && !isdigit(*p))
		p++;

	len = p - cip->name;
	p = new_string(len);
	strncpy(p, cip->name, len);
	p[len] = '\0';

	return (p);
}

/*
 * Generate a confinfo entry for a controller.
 */
static void
conf_controller(fp, cip)
	FILE *fp;
	struct confinfo *cip;
{
	if (dopt)
		printf("conf_controller\n");

	cip->type = VDCCONTROLLER;
	cip->name = getstring(fp);
	read_literal(fp, "at");
	getbusconn(fp, cip, VDCCONTROLLER); /* get bus that controller is on */
	read_literal(fp, "?");
	getoptions(fp, cip);
}

/*
 * Generate a confinfo entry for a disk.
 */
static void
conf_disk(fp, cip)
	FILE *fp;
	struct confinfo *cip;
{
	if (dopt)
		printf("conf_disk\n");

	conf_device(fp, cip);
	cip->disk = 1;
}

/*
 * Generate a confinfo entry for a device.
 */
static void
conf_device(fp, cip)
	FILE *fp;
	struct confinfo *cip;
{

	if (dopt)
		printf("conf_device\n");

	cip->type = VDCDEVICE;
	cip->name = getstring(fp);	/* get device name and number */
	read_literal(fp, "at");
	getbusconn(fp, cip, VDCDEVICE);	/* get bus connection */
	if (cip->space != 0)
		read_literal(fp, "?");
	getoptions(fp, cip);		/* get the rest of the options */
}
#endif /* sun4c */

/*
 * Generate a confinfo entry for a blockmajor configuration line.
 */
static void
conf_blockmajor(fp, cip)
	FILE *fp;
	struct confinfo *cip;
{
	cip->type     = VDCBLOCKMAJOR;
	cip->majornum = getint(fp, 0, 255);
}

/*
 * Generate a confinfo entry for a charmajor configuration line.
 */
static void
conf_charmajor(fp, cip)
	FILE *fp;
	struct confinfo *cip;
{
	cip->type     = VDCCHARMAJOR;
	cip->majornum = getint(fp, 0, 255);
}

/*
 * Generate a confinfo entry for a system call number configuration line.
 */
static void
conf_syscallnum(fp, cip)
	FILE *fp;
	struct confinfo *cip;
{
	cip->type       = VDCSYSCALLNUM;
	cip->syscallnum = getint(fp, 0, 255);
}

/*
 * Allocate a confinfo entry for this line in the configuration file.
 */
static struct confinfo *
getcip()
{
	struct confinfo *cip;

	cip = new_struct(struct confinfo);
	bzero((char *)cip, sizeof (struct confinfo));
	return (cip);
}

#ifndef sun4c
/*
 * Get a bus connection.  For a controller it must be on the names
 * in the table "bustype".  For a device it can either be one of
 * names in the table "bustype" or the name of a controller.
 */
static void
getbusconn(fp, cip, type)
	FILE *fp;
	struct confinfo *cip;
	int type;
{
	register struct bustype *btp;

	cip->conname = getstring(fp);	/* get bus connection */

	for (btp = bustype; btp->name != NULL; btp++) {
		if (streq(btp->name, cip->conname))
			break;
	}

	if (btp->name != NULL)
		cip->space = btp->type;
	else if (type == VDCCONTROLLER) {
		fatal("Unrecognized bus type '%s' on line %d\n",
			cip->conname, cip->linenum);
	} else
		cip->space = 0;
}
#endif /* sun4c */

/*
 * Copy a string from the configuration file.
 */
static char *
getstring(fp)
	FILE *fp;
{
	char *word;

	word = getword(fp);
	if (word == NULL)
		fatal("missing string at line %d\n", linenum);
	return (strdup(word));
}

/*
 * Get an integer from the next word in the configuration file.
 */
static int
getint(fp, min, max)
	FILE *fp;
	int min, max;
{
	char *word, *p;
	long n;

	if (dopt)
		printf("getint\n");

	word = getword(fp);
	if (word == NULL)
		fatal("missing integer at line %d\n", linenum);

	n = strtol(word, &p, 0);
	if (*p != '\0' || (min != -1 && n < min) || (max != -1 && n > max))
		fatal("Invalid number '%s' on line %d\n", word, linenum);

	if (dopt)
		printf("word: %s, value %x\n", word, n);

	return (n);
}

#ifndef sun4c
/*
 * Get device and controller options.
 */
static int
getoptions(fp, cip)
	FILE *fp;
	register struct confinfo *cip;
{
	register struct options *opt;

	char *word;

	if (dopt)
		printf("getoptions\n");

	while ((word = getword(fp)) != NULL) {
		if (dopt)
			printf("option: %s\n", word);

		for (opt = options; opt->name != NULL; opt++) {
			if (streq(opt->name, word))
				break;
		}

		if (opt->name != NULL)
			(*opt->optfcn)(fp, cip);
		else
			fatal("Unrecognized keyword '%s' on line %d\n",
				word, cip->linenum);
	}
}

/*
 * Get the drive number.
 */
static void
getdrive(fp, cip)
	FILE *fp;
	struct confinfo *cip;
{
	if (dopt)
		printf("getdrive\n");

	cip->drive = getint(fp, 0, 255);
}

/*
 * Get the csr.
 */
static void
getcsr(fp, cip)
	FILE *fp;
	struct confinfo *cip;
{
	if (dopt)
		printf("getcsr\n");

	cip->csr = getint(fp, -1, -1);
}

/*
 * Get the interrupt request number.
 */
static void
getirq(fp, cip)
	FILE *fp;
	struct confinfo *cip;
{
	if (dopt)
		printf("getirq\n");

	cip->irq = getint(fp, 0, 15);
}

/*
 * Get VME interrupt vector
 */
static void
getvector(fp, cip)
	FILE *fp;
	struct confinfo *cip;
{
	struct veclist *v;
	char *handler_name;
	struct nlist n[2];
	int vecno;

	if (dopt)
		printf("getvector\n");

	v = new_struct(struct veclist);
	handler_name = getstring(fp);
	vecno = getint(fp, 0x40, 0xff);
	bzero((char *)n, sizeof (n));
	n[0].n_name = new_string(strlen(handler_name) + 2);
	n[0].n_name[0] = '_';
	strcpy(&n[0].n_name[1], handler_name);
	if (nlist(outputfile, n) == -1)
		fatal("can't read %s symbol table\n", outputfile);
	if ((n[0].n_type & N_TYPE) != N_TEXT)
		fatal("lookup of %s failed\n", handler_name);
	v->vec.v_func = (int (*)()) n[0].n_value;
	v->vec.v_vec = vecno;
	v->vec.v_vptr = NULL;
	v->next = cip->vectors;
	cip->vectors = v;
}

/*
 * Get the interrupt priority.
 */
static void
getpriority(fp, cip)
	FILE *fp;
	struct confinfo *cip;
{
	if (dopt)
		printf("getpriority\n");

	cip->priority = getint(fp, 0, 7);
}

/*
 * Get the dma channel.
 */
static void
getdmachan(fp, cip)
	FILE *fp;
	struct confinfo *cip;
{
	if (dopt)
		printf("getdmachan\n");

	cip->dmachan = getint(fp, -1, -1);
}

/*
 * Get the flags.
 */
static void
getflags(fp, cip)
	FILE *fp;
	struct confinfo *cip;
{
	if (dopt)
		printf("getflags\n");

	cip->flags = getint(fp, -1, -1);
}
#endif /* sun4c */

/*
 * Allocate a block of memory.
 */
static char *
alloc(nelem, elsize)
	u_int nelem, elsize;
{
	register char *addr = calloc(nelem, elsize);

	if (addr == NULL)
		fatal("out of memory (size = %x)\n", nelem * elsize);

	return (addr);
}

/*
 * getword
 * return next word of current input line; returns NULL on EOF,
 * end of line, or comment.
 */
/*ARGSUSED*/
static char *
getword(fp)
	FILE *fp;
{
	static char *word =  NULL;

	word = strtok((word == NULL) ? line : NULL, " \t\n");
	if (word != NULL && *word == COMMENT_CHAR)
		word = NULL;		/* skip comments */

	return (word);
}

static void
read_literal(fp, literal)
	FILE *fp;
	char *literal;
{
	char *s = getword(fp);

	if (s == NULL || !streq(s, literal))
		fatal("missing keyword '%s' at line %d\n", literal, linenum);
}
