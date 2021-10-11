/*
 * Copyright (C) 1989-1990 Sun Microsystems, Inc.
 */

#ifndef	lint
static char sccsid[] = "@(#) devinfo.c 1.1 92/07/30";
#endif	lint

/*
 * usage:	devinfo [-fpv] [ promdev ]
 * compilation: cc -o -o devinfo devinfo.c -lkvm
 *
 * For machines that support the new openprom, fetch and print the list
 * of devices that the kernel has fetched from the prom or conjured up.
 * Or, with -p, print the prom's original devinfo list.
 *
 */

#include <stdio.h>
#include <string.h>
#include <kvm.h>
#include <nlist.h>
#include <fcntl.h>
#include <ctype.h>
#include <varargs.h>
#include <sun/openprom.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sundev/openpromio.h>


/*
 * function declarations
 */

extern char *malloc();
static void build_devs(), walk_devs(), dump_devs(), wrongarch(), setprogname();
static char *getkname();
static int _error();

/*
 * local data
 */
static kvm_t *kd;
static char *mfail = "malloc";
static char *progname;
static char *promdev = "/dev/openprom";
static int prominfo = 0;
static char *usage = "%s [ -fpv ] [ promdev ]\n";
static int verbose = 0;

#define	DSIZE	(sizeof (struct dev_info))
#define	P_DSIZE	(sizeof (struct dev_info *))
#define	NO_PERROR	((char *) 0)


main(a, v)
char **v;
{
	auto struct dev_info root_node, *rnodep;
	int c;
	static struct nlist list[] = { { "_top_devinfo" }, 0 };
	extern char *optarg;
	extern int optind;

	setprogname(v[0]);
	while ((c = getopt(a, v, "f:pv")) != -1)
		switch (c) {
		case 'v':
			verbose++;
			break;
		case 'p':
			prominfo++;
			break;
		case 'f':
			promdev = optarg;
			break;
		default:
			(void)fprintf(stderr, usage, progname);
			exit(1);
		}
	if (prominfo)
		exit(do_prominfo());

	if ((kd = kvm_open((char *)0, (char *)0, (char *)0, O_RDONLY, progname))
	    == NULL) {
		exit(_error("kvm_open failed"));
	} else if ((kvm_nlist(kd, &list[0])) != 0) {
		wrongarch();
		exit(1);
	}

	/*
	 * first, build the root node...
	 */

	if (kvm_read(kd, list[0].n_value, (char *)&rnodep, P_DSIZE)
	    != P_DSIZE) {
		exit(_error("kvm_read of root node pointer fails"));
	}

	if (kvm_read(kd, (u_long)rnodep, (char *)&root_node, DSIZE) != DSIZE) {
		exit(_error("kvm_read of root node fails"));
	}

	/*
	 * call build_devs to fetch and construct a user space copy
	 * of the dev_info tree.....
	 */

	build_devs(&root_node);

	/*
	 * ...and call walk_devs to report it out...
	 */
	walk_devs (&root_node);
}

/*
 * build_devs copies all appropriate information out of the kernel
 * and gets it into a user addrssable place. the argument is a
 * pointer to a just-copied out to user space dev_info structure.
 * all pointer quantities that we might de-reference now have to
 * be replaced with pointers to user addressable data.
 */

static void
build_devs(dp)
register struct dev_info *dp;
{
	char *tptr;
	unsigned amt;

	if (dp->devi_name)
		dp->devi_name = getkname(dp->devi_name);

	if (dp->devi_nreg) {
		amt = dp->devi_nreg * sizeof (struct dev_reg);
		tptr = malloc(amt);
		if (!tptr) {
			exit(_error(mfail));
		}
		if (kvm_read(kd, (u_long)dp->devi_reg, tptr, amt) != amt) {
			exit(_error("kvm_read of devi_reg"));
		}
		dp->devi_reg = (struct dev_reg *) tptr;
	}

	if (dp->devi_nintr) {
		amt = dp->devi_nintr * sizeof (struct dev_intr);
		tptr = malloc(amt);
		if (!tptr) {
			exit(_error(mfail));
		}
		if (kvm_read(kd, (u_long)dp->devi_intr, tptr, amt) != amt) {
			exit(_error("kvm_read of devi_intr"));
		}
		dp->devi_intr = (struct dev_intr *) tptr;
	}

	if (dp->devi_nrng) {
		amt = dp->devi_nrng * sizeof (struct dev_rng);
		tptr = malloc(amt);
		if (!tptr) {
			exit(_error(mfail));
		}
		if (kvm_read(kd, (u_long)dp->devi_rng, tptr, amt) != amt) {
			exit(_error("kvm_read of devi_rng"));
		}
		dp->devi_rng = (struct dev_rng *) tptr;
	}

	if (dp->devi_slaves) {
		if (!(tptr = malloc(DSIZE))) {
			exit(_error(mfail));
		}
		if (kvm_read(kd, (u_long)dp->devi_slaves, tptr, DSIZE)
		    != DSIZE) {
			exit(_error("kvm_read of devi_slaves"));
		}
		dp->devi_slaves = (struct dev_info *) tptr;
		build_devs(dp->devi_slaves);
	}

	if (dp->devi_next) {
		if (!(tptr = malloc(DSIZE))) {
			exit(_error(mfail));
		}
		if (kvm_read(kd, (u_long)dp->devi_next, tptr, DSIZE) != DSIZE) {
			exit(_error("kvm_read of devi_next"));
		}
		dp->devi_next = (struct dev_info *) tptr;
		build_devs(dp->devi_next);
	}
}

/*
 * print out information about this node, descend to children, then
 * go to siblings
 */

static void
walk_devs(dp)
register struct dev_info *dp;
{
	static int indent_level = 0;
	register i;

	for (i = 0; i < indent_level; i++)
		(void)putchar('\t');

	(void)printf("Node '%s', unit #%d", dp->devi_name, dp->devi_unit);
	if (dp->devi_driver == 0)
		(void)printf(" (no driver)");
	(void)putchar('\n');
	if (verbose) {
		dump_devs(dp, indent_level+1);
	}
	if (dp->devi_slaves) {
		indent_level++;
		walk_devs(dp->devi_slaves);
		indent_level--;
	}
	if (dp->devi_next) {
		walk_devs(dp->devi_next);
	}
}

/*
 * utility routines
 */

static void
setprogname(name)
char *name;
{
	register char *p;

	if (p = strrchr(name, '/'))
		progname = p + 1;
	else
		progname = name;
}

static void
dump_devs(dp, ilev)
register struct dev_info *dp;
{
	register i, j;

	if (dp->devi_nreg) {
		struct dev_reg *rp = dp->devi_reg;
		for (i = 0; i < ilev; i++)
			(void)putchar('\t');
		(void)printf("  Register specifications:\n");
		for (j = 0; j < dp->devi_nreg; j++, rp++) {
			for (i = 0; i < ilev; i++)
				(void)putchar('\t');
			(void)printf(
			    "    Bus Type=0x%x, Address=0x%x, Size=%x\n",
			    rp->reg_bustype, rp->reg_addr, rp->reg_size);
		}
	}
	if (dp->devi_nrng) {
		struct dev_rng *rp = dp->devi_rng;
		for (i = 0; i < ilev; i++)
			(void)putchar('\t');
		(void)printf("  Range specifications:\n");
		for (j = 0; j < dp->devi_nrng; j++, rp++) {
			for (i = 0; i < ilev; i++)
				(void)putchar('\t');
			(void)printf("    Ch %.2x.%.8x:",
				rp->rng_cbustype, rp->rng_coffset);
			(void)printf( " Pa %.2x.%.8x",
			    rp->rng_bustype, rp->rng_offset);
			(void)printf( " Sz 0x%x\n",
			    rp->rng_size);
		}
	}
	if (dp->devi_nintr) {
		struct dev_intr *ip = dp->devi_intr;
		for (i = 0; i < ilev; i++)
			(void)putchar('\t');
		(void)printf("  Interrupt specifications:\n");
		for (j = 0; j < dp->devi_nintr; j++, ip++) {
			for (i = 0; i < ilev; i++)
				(void)putchar('\t');
			(void)printf("    Interrupt Priority=%d",
				ip->int_pri, ip->int_pri);
			if (ip->int_vec) {
				(void)printf(", vector=0x%x (%d)",
					ip->int_vec, ip->int_vec);
			}
			(void)putchar('\n');
		}
	}
}

static void
wrongarch()
{
	static char buf[32] = "????\n";
	FILE *ip = popen("/bin/arch -k", "r");
	if (ip) {
		(void)fgets(buf, 32, ip);
		(void)pclose(ip);
	}
	(void)_error(NO_PERROR,
	    "%s not available on kernel architecture %s (yet).", progname, buf);
}

static char *
getkname(kaddr)
char *kaddr;
{
	auto char buf[32], *rv;
	register i = 0;
	char c;

	if (kaddr == (char *) 0) {
		(void)strcpy(buf, "<null>");
		i = 7;
	} else {
		while (i < 31) {
			if (kvm_read(kd, (u_long)kaddr++, (char *)&c, 1) != 1) {
				exit(_error("kvm_read of name string"));
			}
			if ((buf[i++] = c) == (char) 0)
				break;
		}
		buf[i] = 0;
	}
	if ((rv = malloc((unsigned)i)) == 0) {
		exit(_error(mfail));
	}
	bcopy(buf, rv, i);
	return (rv);
}

/*
 * The rest of the routines handle printing the raw prom devinfo (-p option).
 *
 * 64 is the size of the largest (currently) property name
 * 512 is the size of the largest (currently) property value, viz. oem-message
 * the sizeof (u_int) is from struct openpromio
 */

#define	MAXPROPSIZE	64
#define	MAXVALSIZE	512
#define	BUFSIZE		(MAXPROPSIZE + MAXVALSIZE + sizeof (u_int))
typedef union {
	char buf[BUFSIZE];
	struct openpromio opp;
} Oppbuf;

static void dump_node(), print_one(), promclose(), promopen(), walk();
static int child(), getpropval(), next(), unprintable();

static int prom_fd;

int
do_prominfo()
{
	promopen(O_RDONLY);
	if (next(0) == 0)
		return (1);
	walk(next(0), 0);
	promclose();
	return (0);
}

static void
walk(id, level)
int id, level;
{
	register int curnode;

	dump_node(id, level);
	if (curnode = child(id))
		walk(curnode, level+1);
	if (curnode = next(id))
		walk(curnode, level);
}

/*
 * Print all properties and values
 */
static void
dump_node(id, level)
{
	Oppbuf	oppbuf;
	register struct openpromio *opp = &(oppbuf.opp);
	register int i = level;

	while (i--)
		(void)putchar('\t');
	(void)printf("Node");
	if (verbose)
		(void)printf(" %#08lx\n", id);

	/* get first prop by asking for null string */
	bzero(oppbuf.buf, BUFSIZE);
	while (1) {
		/*
		 * get property
		 */
		opp->oprom_size = MAXPROPSIZE;

		if (ioctl(prom_fd, OPROMNXTPROP, opp) < 0)
			exit(_error("OPROMNXTPROP"));

		if (opp->oprom_size == 0) {
			break;
		}
		print_one(opp->oprom_array, level+1);
	}
	(void)putchar('\n');
}

/*
 * Print one property and its value.
 */
static void
print_one(var, level)
char	*var;
int	level;
{
	Oppbuf	oppbuf;
	register struct openpromio *opp = &(oppbuf.opp);
	register int i;

	while (verbose && level--)
		(void)putchar('\t');
	if (verbose)
		(void)printf("%s: ", var);
	(void)strcpy(opp->oprom_array, var);
	if (getpropval(opp) || opp->oprom_size == -1) {
		(void)printf("data not available.\n");
		return;
	}
	if (verbose && unprintable(opp)) {
		(void)printf(" ");
		for (i = 0; i < opp->oprom_size; ++i) {
			if (i && (i % 4 == 0))
				(void)putchar('.');
			(void)printf("%02x",
			    opp->oprom_array[i] & 0xff);
		}
		(void)putchar('\n');
	} else if (verbose) {
		(void)printf(" '%s'\n", opp->oprom_array);
	} else if (strcmp(var, "name") == 0)
		(void)printf(" '%s'", opp->oprom_array);
}

static int
unprintable(opp)
struct openpromio *opp;
{
	register int i;

	/*
	 * Is this just a zero?
	 */
	if (opp->oprom_size == 0 || opp->oprom_array[0] == '\0')
		return (1);
	/*
	 * If any character is unprintable, or if a null appears
	 * anywhere except at the end of a string, the whole
	 * property is "unprintable".
	 */
	for (i = 0; i < opp->oprom_size; ++i) {
		if (opp->oprom_array[i] == '\0')
			return (i != (opp->oprom_size - 1));
		if (!isascii(opp->oprom_array[i]) ||
		    iscntrl(opp->oprom_array[i]))
			return (1);
	}
	return (0);
}

static void
promopen(oflag)
register int oflag;
{
	if ((prom_fd = open(promdev, oflag)) < 0)
		exit(_error("cannot open %s", promdev));
}

static void
promclose()
{
	if (close(prom_fd) < 0)
		exit(_error("close error on %s", promdev));
}

static
getpropval(opp)
register struct openpromio *opp;
{
	opp->oprom_size = MAXVALSIZE;

	if (ioctl(prom_fd, OPROMGETPROP, opp) < 0)
		return (_error("OPROMGETPROP"));
	return (0);
}

static int
next(id)
int id;
{
	Oppbuf	oppbuf;
	register struct openpromio *opp = &(oppbuf.opp);
	int *ip = (int *)(opp->oprom_array);

	bzero(oppbuf.buf, BUFSIZE);
	opp->oprom_size = MAXVALSIZE;
	*ip = id;
	if (ioctl(prom_fd, OPROMNEXT, opp) < 0)
		return (_error("OPROMNEXT"));
	return (*(int *)opp->oprom_array);
}

static int
child(id)
int id;
{
	Oppbuf	oppbuf;
	register struct openpromio *opp = &(oppbuf.opp);
	int *ip = (int *)(opp->oprom_array);

	bzero(oppbuf.buf, BUFSIZE);
	opp->oprom_size = MAXVALSIZE;
	*ip = id;
	if (ioctl(prom_fd, OPROMCHILD, opp) < 0)
		return (_error("OPROMCHILD"));
	return (*(int *)opp->oprom_array);
}

/* _error([no_perror, ] fmt [, arg ...]) */
/*VARARGS*/
static int
_error(va_alist)
va_dcl
{
	int saved_errno;
	va_list ap;
	int no_perror = 0;
	char *fmt;
	extern int errno, _doprnt();

	saved_errno = errno;

	if (progname)
		(void) fprintf(stderr, "%s: ", progname);

	va_start(ap);
	if ((fmt = va_arg(ap, char *)) == 0) {
		no_perror = 1;
		fmt = va_arg(ap, char *);
	}
	(void) _doprnt(fmt, ap, stderr);
	va_end(ap);

	if (no_perror)
		(void) fprintf(stderr, "\n");
	else {
		(void) fprintf(stderr, ": ");
		errno = saved_errno;
		perror("");
	}

	return (1);
}
