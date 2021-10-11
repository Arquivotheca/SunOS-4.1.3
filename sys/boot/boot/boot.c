#ifndef lint
static	char sccsid[] = "@(#)boot.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/user.h>
#include <stand/saio.h>
#include <sys/reboot.h>
#include <mon/sunromvec.h>
#include <sys/dir.h>
#include <machine/pte.h>

#ifdef	DUMP_DEBUG
static	int	dump_debug = 10;
#endif	/* DUMP_DEBUG */
extern struct boottab blkdriver;

#undef u
struct	user	u;

#ifndef isspace
#define	isspace(c)	((c) == ' ' || (c) == '\t' || (c) == '\n' || \
			    (c) == '\r' || (c) == '\f' || (c) == '\013')
#endif
#ifndef isgraph
#define	isgraph(c)	((c) >= '!' && (c) <= '~')
#endif

/*
 * Boot program... argument from monitor determines
 * whether boot stops to ask for system name and which device
 * boot comes from.
 */

int (*readfile())();
void	exitto();
int	mountroot();

char aline[100] = "\0";

char vmunix[50] = "vmunix";	/* and room to patch to whatever */
extern	int		boothowto;	/* defined in <sys/systm.h> */
extern	int		memory_avail;

/*
 * Verbose mode --
 * Set by bootflags() in response to -v option on boot line.
 * For use in diagnosing diskless booting problems.
 * Significant success/error conditions are printing leading up
 * to the NFS vmunix Read Request stage, only error conditions
 * cause a printf while loading vmunix, and several statistics
 * are printed before we exec vmunix.
 */
extern int verbosemode;

extern char	version[];

char	boot_device[3];

main()
{
#ifndef sun4m
	register struct bootparam *bp;
	register char *arg;
	register int howto;
	register int io;
	register char *s, *p;
	int (*go2)();
	int	ctlr;
	int	unit;
	int	partition;
	char	filename[MAXNAMLEN];
	char	*dev_unit_file();
#ifdef	OPENPROMS
	extern struct bootparam *prom_bootparam();
	extern void prom_enter_mon();
#endif	OPENPROMS
#ifdef sun2
	int	*mem_p;
#endif sun2
#endif !sun4m
#ifdef	OPENPROMS
	extern void prom_init();
#endif	OPENPROMS

#ifdef DUMP_DEBUG
	printf(version);
#endif DUMP_DEBUG

#ifdef OPENPROMS
	prom_init("Boot");
	if (prom_getversion() > 0) {
		obpmain();
		return;
	}
#endif

#ifndef sun4m
	filename[0] = '\0';

#ifdef	OPENPROMS
	bp = prom_bootparam();
#else	OPENPROMS
	if (romp->v_bootparam == 0)	{
		bp = (struct bootparam *)0x2000;    /* Old Sun-1 address */
	} else {
		bp = *(romp->v_bootparam);	    /* S-2: via romvec */
	}
#endif	OPENPROMS
#ifdef sun2
	mem_p = (int *)&(bp->bp_strings[96]);
	*mem_p = 0x200000;
	bp->bp_strings[95] = '\0';
	memory_avail = 0x220000;
#endif sun2
#if defined(sun3) || defined(sun3x)
	/*
	 * Allocate memory starting below the top of physical
	 * memory, to avoid tromping on kadb.
	 */
	memory_avail = *romp->v_memoryavail - 0x100000;
#endif sun3 || sun3x
#ifdef sun4
	memory_avail = *romp->v_memoryavail - 0x200000;
#endif sun4
#ifdef OPENPROMS
	/*
	 * We'll avoid dealing with holes in the physical memory by
	 * only using the first chunk (which is guaranteed to be mapped
	 * by the monitor). This should be enough.
	 */
	memory_avail = (*(romp->v_availmemory))->address +
			(*(romp->v_availmemory))->size - 0x80000;
#endif OPENPROMS

	kmem_init();
	bhinit();
	binit();
	init_syscalls();
#ifdef sun3
	kmem_remap();		/* map around video ram */
#endif sun3
#ifdef OPENPROMS
	init_devsw();
#endif OPENPROMS

#ifdef JUSTASK
	arg = (char *)0;
	howto = RB_ASKNAME|RB_SINGLE;
#else	/* JUSTASK */
	arg = bp->bp_argv[0];
	howto = bootflags(bp->bp_argv[1]);
	/*
	 * Place in global location for gtfsype to
	 * pick up.
	 */
	boothowto = howto;
	if (verbosemode)
		printf(version);
	(void) dev_unit_file(arg, &boot_device[0],
			&ctlr, &unit, &partition, &filename[0]);
	for (s = arg, p = aline; *s; s++, p++)
		*p = *s;

	/*
	 * If no file name given, default to "vmunix"
	 */
	for (s = aline; *s; /* empty */)
		if (*s++ == ')')
			break;
	if (*s == '\0') {
		for (p = vmunix; *p; /* empty */)
			*s++ = *p++;
		*s = '\0';
	} else {
		for (p = vmunix; *p; /* empty */)
			*p++ = *s++;
	}
#endif	/* JUSTASK */
	for (;;) {
		/*
		 * First try to mount a 'root' file system.
		 */
		mountroot();
		if (u.u_error)  {
#ifdef	OPENPROMS
			extern void prom_exit_to_mon();
#endif	OPENPROMS

			/*
			 * Until we figure out why you can't try this
			 * again, we better just get out.
			 */

#ifdef	OPENPROMS
			prom_exit_to_mon();
#else	OPENPROMS
			(*romp->v_exit_to_mon)();
#endif	OPENPROMS

			howto |= RB_ASKNAME;
			boothowto = howto;
			continue;
		}
		if (howto & RB_ASKNAME) {
			printf("Boot: ");
			(void) gets(filename);
		}

		if (!strcmp(filename, "*")) {
			if (list_directory("") != 0)
				printf ("Bad directory\n");
			howto |= RB_ASKNAME;
			boothowto = howto;
			continue;
		}

		if (filename[0] == '\0')	{
			for (s = filename, p = vmunix; *p; /* empty */) {
				*s++ = *p++;
			}
			*s = '\0';
			printf("Boot: %s\n", filename);
		}
		for (s = aline; *s; /* empty */)
			if (*s++ == ')')
				break;
		for (p = filename; *p; /* empty */) {
			*s++ = *p++;
		}
		*s = '\0';
		parseparam(aline, howto, bp);
		for (p = filename; *p && *p != ' ' && *p != '\t'; ++p)
			;
		*p = '\0';
		io = open(filename, 0);
		if (io >= 0 && (int)(go2 = readfile(io, 1)) != -1) {
			if (verbosemode)
				iostats();
			if (howto & RB_HALT)  {
				printf("Boot: halted by -h flag .\n");
#ifdef	OPENPROMS
				prom_enter_mon();
#else	OPENPROMS
				(*romp->v_abortent)();
#endif	OPENPROMS
			}
			exitto(go2);
			/*
			 * If we get here, we must restart the I/O.
			 */
			reopen(0);
		} else
			printf("boot failed\n");
		howto |= RB_ASKNAME;
		boothowto = howto;
	}
#endif !sun4m
}

#ifdef OPENPROMS

int Mustask;
static char abuf[128];

obpmain()
{
	register int howto = 0;
	register int io;
	register char *s, *p;
	int (*go2)();
	char	filename[MAXNAMLEN];
	int	argc;
	char	*args[8];
	extern char *prom_bootpath();
	extern char *prom_bootargs();
	char	*bpath = prom_bootpath();
	char	*bargs = prom_bootargs();
	extern void prom_exit_to_mon(), prom_enter_mon();

	filename[0] = '\0';

	kmem_init();
	bhinit();
	binit();
	init_syscalls();
	init_devsw();

#ifdef JUSTASK
	howto = RB_ASKNAME|RB_SINGLE;
#else	/* JUSTASK */
	argc = 0;
	args[argc++] = bpath;
	(void)strcpy(abuf, bargs);
#ifdef DUMP_DEBUG
	dprint(dump_debug, 0, "bootpath : <%s>\n", bpath);
	dprint(dump_debug, 0, "bootargs : <%s>\n", bargs);
#endif DUMP_DEBUG

	argc = setparam(argc, abuf, args);

	if (args[1] && *args[1]) {
		/*
		 * There's something here. It's either flags...
		 */
		if (*args[1] == '-')
			howto = bootflags(args[1]);
		else {
		/*
		 * ...or it's a file name and the flags are in the next arg.
		 */
			howto = bootflags(args[2]);
			(void)string(args[1], vmunix);
		}
	}
	/*
	 * Place in global location for gtfsype to
	 * pick up.
	 */
	boothowto = howto;

	if (verbosemode)  {
		printf(version);
		printf("Boot: Romvec version %d.\n", prom_getversion());
	}

#endif	/* JUSTASK */
	for (;;) {
		int	err;
		/*
		 * First try to mount a 'root' file system.
		 */
		mountroot();
		if (u.u_error)  {

			/*
			 * Until we figure out why you can't try this
			 * again, we better just get out.
			 */

			prom_exit_to_mon();

			howto |= RB_ASKNAME;
			boothowto = howto;
			continue;
		}
		if (howto & RB_ASKNAME) {
			printf("Boot: ");
			(void) gets(filename);
		}

		if (!strcmp(filename, "*")) {
			if (list_directory("") != 0)
				printf ("Bad directory\n");
			howto |= RB_ASKNAME;
			boothowto = howto;
			continue;
		}

		if (filename[0] == '\0') {
			strcpy(filename, vmunix);
			printf("Boot: %s\n", filename);
		}
		for (p = filename, s = aline; *p; /* empty */)
			*s++ = *p++;
		*s = '\0';
		parseargs(aline, howto, args, bargs);
		for (p = filename; *p && *p != ' ' && *p != '\t'; ++p)
			;
		*p = '\0';
		io = open(filename, 0);
		if (io >= 0 && (int)(go2 = readfile(io, 1)) != -1) {
			if (verbosemode)
				iostats();
			if (howto & RB_HALT)  {
				printf("Boot: halted by -h flag .\n");
				prom_enter_mon();
			}
			exitto(go2);
			/*
			 * If we get here, we must restart the I/O.
			 */
			reopen(0);
		} else
			printf("boot failed\n");
		howto |= RB_ASKNAME;
		boothowto = howto;
	}
}
#endif

struct bootf {
	char	let;
	short	bit;
} bootf[] = {
	'a',	RB_ASKNAME,
	's',	RB_SINGLE,
	'i',	RB_INITNAME,
	'h',	RB_HALT,
	'b',	RB_NOBOOTRC,
	'd',	RB_DEBUG,
	'w',    RB_WRITABLE,
	0,	0,

};

#ifndef JUSTASK
/*
 * Parse the boot line to determine boot flags
 */
bootflags(cp)
	register char *cp;
{
	register int i, localhowto = 0;

	if (cp && *cp++ == '-')
		while (*cp) {
			for (i = 0; bootf[i].let; i++) {
				if (*cp == bootf[i].let) {
					localhowto |= bootf[i].bit;
#ifdef	DEBUG
					printf("bootflags: flag <%c> set\n",
					    *cp);
#endif	DEBUG
					break;
				}
			}
			if (*cp == 'v')		/* "local" option */
				verbosemode = 1;
			cp++;
		}
	return (localhowto);
}

#ifdef OPENPROMS
/*
 * Parse the boot line to set bootparam structure
 */
setparam(argc, cp, bp)
	register int  argc;
	register char *cp;
	register char *bp[];
{
	register int i;

	while (*cp && isspace(*cp))
		cp++;
	while ((argc < 8) && *cp) {
		bp[argc++] = cp;
		while (*cp && (!isspace(*cp)))
			cp++;
		if (isspace(*cp))
			*cp++ = '\0';
		while (*cp && isspace(*cp))
			cp++;
	}
	/*
	 * Null out remaining args.
	 */
	for (i = argc; i < 8; ++i)
		bp[i] = '\0';
	return (argc);
}
#endif
#endif

#ifndef sun4m
/*
 * Parse the boot line and put it in bootparam. Preserve the flags
 * unless redefined. Always preserve the remaining arguments.
 *
 * The boot line comes in in one of two forms:
 *	xx(c,u,p)file[whitespace]\0
 *	xx(c,u,p)file[whitespace]-[flags]\0
 * If the former is present, we take the bootflags and arguments from the
 * bootparam structure. If the latter is present, the user has overridden
 * the original boot flags with his/her own specification.
 */
parseparam(line, defaults, bp)
	char *line;
	int defaults;
	register struct bootparam *bp;
{
	register int i;
	register char *cp, *lp = line;

	while (*lp && isgraph(*lp))
		lp++;
	if (*lp) {
		*lp = '\0';
	}
	cp = lp++;
	while (*lp && isspace(*lp))
		lp++;
	if (*lp) {
		/*
		 * There's something here. Is it new flags?
		 */
		if (*lp == '-') {
			defaults = bootflags(lp);
		}
	}
	/*
	 * Now backtrack, write the flags, and fill in the arguments.
	 */
	lp = cp + 1;
	if (defaults) {
		*lp++ = '-';
		for (i = 0; bootf[i].let; i++) {
			if (defaults & bootf[i].bit)
				*lp++ = bootf[i].let;
		}
		*lp++ = 0;
	}
	for (i = 1; i < 8; ++i) {
		cp = bp->bp_argv[i];
	/*
	 * NOTE: the code in ifdef should actually apply to all
	 * architectures, but due to lack of test time
	 * it would only be fixed on sun3x now.
	 */
#ifdef	sun3x
		if (cp) {
			if (*cp && (*cp != '-')) {
				while (*lp++ = *cp++)
					;
			}
		} else
			/* arg. list terminates by a NULL ptr */
			break;

#else
		if (cp && *cp && (*cp != '-')) {
			while (*lp++ = *cp++)
				;
		}
#endif	sun3x
	}
	/*
	 * Copy the fully qualified boot line to the string table.
	 */
	bcopy(line, bp->bp_strings, 95);
	bp->bp_strings[95] = '\0';
	/*
	 * Scan the string table, setting the argv pointers.
	 */
	for (i = 0, cp = bp->bp_strings; i < 8; ++i) {
		if (cp && *cp) {
			bp->bp_argv[i] = cp;
			while (*cp++)
				;
		} else {
			bp->bp_argv[i] = 0;
		}
	}
}
#endif !sun4m

#ifdef OPENPROMS
/*
 * Parse the boot line and put it in bargs in OBP. Preserve the flags
 * unless redefined. Always preserve the remaining arguments.
 *
 * The bootargs comes in :
 *	file[whitespace]-[flags]\0
 * We take the bootflags and arguments from the bargs structure.
 */
parseargs(line, defaults, bp, dp)
	char *line;
	int defaults;
	register char *bp[];
	register char *dp;
{
	register int i;
	register char *cp, *lp = line;

	while (*lp && isgraph(*lp))
		lp++;
	if (*lp) {
		*lp = '\0';
	}
	cp = lp++;
	while (*lp && isspace(*lp))
		lp++;
	if (*lp) {
		/*
		 * There's something here. Is it new flags?
		 */
		if (*lp == '-') {
			defaults = bootflags(lp);
		}
	}
	/*
	 * Until we can expand an alias, this works around an obscure bug
	 * if you boot -a, then turn off -a and switch the root device in OBP.
	 */

	if (Mustask != 0)
		defaults |= RB_ASKNAME;

	/*
	 * Now backtrack, write the flags, and fill in the arguments.
	 */
	lp = cp;
	if (defaults) {
		*lp++ = ' ';
		*lp++ = '-';
		for (i = 0; bootf[i].let; i++) {
			if (defaults & bootf[i].bit)
				*lp++ = bootf[i].let;
		}
		*lp = '\0';
	}

	i = 2;
	if ((bp[1]) && (*(bp[1]) == '-'))
		i = 2;
	else if ((bp[2]) && (*(bp[2]) == '-'))
			i = 3;

	for (; i < 8; ++i) {
		cp = bp[i];
		if (cp && *cp && (*cp != '-')) {
			*lp++ = ' ';
			while (*cp)
				*lp++ = *cp++;
			*lp = '\0';
		}
	}
	/*
	 * Copy the fully qualified boot line to the bootargs
	 */
	bcopy(line, dp, 100);
}
#endif


#ifdef sun3

#define	VIDEO_START	0x100000	/*
					 * physical address of start of
					 * video ram
					 */
#define	ONE_PMEG	16*PAGESIZE
#define	VIDEO_SIZE	ONE_PMEG

kmem_remap()
{
	extern struct pte getpgmap();
	extern int setpgmap();
	extern int start;

	register int vaddr, paddr;
	struct pte p;

	p.pg_v = 1;
	p.pg_prot = KW;
	p.pg_nc = 1;
	p.pg_type = OBMEM;
	p.pg_r = 0;
	p.pg_m = 0;

	/*
	 * map 0-VIDEO_START virtual addresses 1:1 to physical addresses
	 */
	for (vaddr = 0, paddr = 0;
	    paddr < VIDEO_START;
	    vaddr += PAGESIZE, paddr += PAGESIZE) {
		p.pg_pfnum = paddr >> PAGESHIFT;
		setpgmap(vaddr, p);
	}
	paddr += VIDEO_SIZE;
	for (/* empty */;
	    vaddr < (int)&start - 2*ONE_PMEG;
	    vaddr += PAGESIZE, paddr += PAGESIZE) {
		p.pg_pfnum = paddr >> PAGESHIFT;
		setpgmap(vaddr, p);
	}
}
#endif sun3
