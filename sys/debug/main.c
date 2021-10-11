#ifndef lint
static  char sccsid[] = "@(#)main.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/reboot.h>
#include <machine/reg.h>
#include <debug/debugger.h>
#include <a.out.h>
#include <ctype.h>
#include <errno.h>
#ifdef OPENPROMS
#include <sun/openprom.h>
#include <sun/autoconf.h>
#include <machine/pte.h>

extern char	*prom_bootpath(), *prom_bootargs();
#endif

char vmunix[50] = "vmunix";	/* and room to patch to whatever */
char myname[50];		/* name of the debugger */	
char aline[LINEBUFSZ];		/* generic buffer used for console input */
extern int debugcmd();
extern char *malloc();
extern char estack[];
char	*malreturn;

#ifdef	MULTIPROCESSOR
unsigned	nodeid;
unsigned	who;
extern	int cpus_enabled[];
#ifdef	PROM_IDLECPUS_WORKS
extern	unsigned cpus_nodeid[];
#endif	/* PROM_IDLECPUS_WORKS */
#endif	MULTIPROCESSOR

#define	MALLOC_PAD	0x1000		/* malloc pad */

static interactive = 0;			/* true if -d flag passed in */

#ifndef sun2
u_int	memory_avail;
#endif

main()
{
	func_t go2, load_it();
	char *arg;
#ifdef mc68000
	struct regs fakeregs;
	struct stkfmt fakestkfmt;
	int fakedfc, fakesfc;
#endif
#ifdef OPENPROMS
	register struct memlist *pmem;
	extern struct memlist *getmemlist();
#endif

	ttysync();
#ifdef OPENPROMS
	if (romp->op_romvec_version > 0) {
		pmem = getmemlist("memory", "available");
		if (pmem == (struct memlist *)0)
			return;
	} else
		pmem = *romp->v_availmemory;
	for (memory_avail = 0; pmem->next != (struct memlist *)NULL;
	    pmem = pmem->next) {
			memory_avail += pmem->size;
	}
	memory_avail += pmem->size;
#elif !defined(sun2)
	/*
	 * Initialize so that the standalone code doesn't
	 * use the same pages the debugger is in.
	 */
	memory_avail = *romp->v_memoryavail;
#endif OPENPROMS || !sun2
	spawn((int *)estack, debugcmd);
	while ((go2 = load_it(&arg)) == (func_t)-1)
		continue;
	if (go2 == (func_t)-2)
		return;
	free(malloc(MALLOC_PAD));

	printf("%s loaded - 0x%x bytes used\n",
	    arg, mmu_ptob(pagesused));

#ifdef	MULTIPROCESSOR
        nodeid = prom_nextnode(0); 		/* root node */
        for (nodeid = prom_childnode(nodeid);
             (nodeid != 0) && (nodeid != -1);
             nodeid = prom_nextnode(nodeid)) {
                if ((prom_getproplen(nodeid, "mid") == sizeof who) &&
                    (prom_getprop(nodeid, "mid", &who) != -1)) {
                        if (who == 15)
                                who = 0;	/* level-1 mbus module */
                        else
                                who &= 3;
#ifdef	PROM_IDLECPUS_WORKS
			cpus_nodeid[who] = nodeid;
#endif	/* PROM_IDLECPUS_WORKS */
			cpus_enabled[who] = 1;
		}
	}
#endif	MULTIPROCESSOR
	if (interactive) {
#ifdef mc68000
		bzero((caddr_t)&fakeregs, sizeof (fakeregs));
		bzero((caddr_t)&fakestkfmt, sizeof (fakestkfmt));
		(void) cmd(fakedfc, fakesfc, fakeregs, fakestkfmt);
#else
		(void) cmd();
#endif
		if (dotrace) {
			scbstop = 1;
			dotrace = 0;
		}
	}
	nobrk = 1;		/* no more sbrk's allowed */
	exitto(go2);
}

#define	LOAD	0x4000

/*
 * Read in a Unix executable file and return its entry point.
 * Handle the various a.out formats correctly.
 * "Io" is the standalone file descriptor to read from.
 * Print informative little messages if "print" is on.
 * Returns -1 for errors.
 */
func_t
readfile(io, print, name)
	register int io;
	int print;
	char *name;
{
	struct exec x;
	register int i;
	register char *addr;
	register int shared = 0;
	register int loadaddr;
	register int segsiz;
	register int datasize;

	i = read(io, (char *)&x, sizeof x);
	if (i != sizeof (x)) {
		printf("Bad read: expected 0x%x got 0x%x\n", sizeof(x), i);
		return ((func_t)-1);
	}
	if (N_BADMAG(x)) {
		printf("Not executable\n");
		return ((func_t)-1);
	}
	shared = (x.a_magic == OMAGIC? 0: 1);
	if (print)
		printf("Size: %d", x.a_text);
	datasize = x.a_data;
	if (!shared) {
	        x.a_text = x.a_text + x.a_data;
	        x.a_data = 0;
	}
	if (lseek(io, N_TXTOFF(x), 0) == -1)
		goto shread;
	if (read(io, (char *)LOAD, (int)x.a_text) < x.a_text)
		goto shread;
	addr = (char *)(x.a_text + LOAD);
	if (x.a_machtype == M_OLDSUN2)
	        segsiz = OLD_SEGSIZ;
	else
	        segsiz = SEGSIZ;
	if (shared)
		while ((int)addr & (segsiz-1))
			*addr++ = 0;
	if (print)
		printf("+%d", datasize);
	if (read(io, addr, (int)x.a_data) < x.a_data)
		goto shread;
	if (print)
		printf("+%d", x.a_bss);
	addr += x.a_data;
	for (i = 0; i < x.a_bss; i++)
		*addr++ = 0;
	if (print)
		printf(" bytes\n");
	if (x.a_machtype != M_OLDSUN2 && x.a_magic == ZMAGIC)
	        loadaddr = LOAD + sizeof (struct exec);
	else
	        loadaddr = LOAD;
	debuginit(io, &x, name);
	return ((func_t)loadaddr);

shread:
	printf("Truncated file\n");
	return ((func_t)-1);
}

/*
 * Prompt for name of file to be read into memory for debugging.
 */
#ifdef	OPENPROMS
func_t
obp_load_it(arg)
	register char **arg;
{
        register int howto;
        register int io;
        func_t go2;
        extern char myname_default[];
        static char filename[LINEBUFSZ];
        register char *bargs;
        register char *bpath;
        char *args[8];
        register char abuf[128];

        bpath = prom_bootpath();
        bargs = prom_bootargs();
        (void)strcpy(abuf, bargs);
        setparam(abuf, args);
 
        if (myname[0] == '\0') {
                register char *s, *p;
 
                for (s = bargs; isspace(*s); s++)
                        ;
                if (*s == '-')
                        s = myname_default;
                for (p = myname; *s && !isspace(*s); ) 
                        *p++ = *s++;
        } else if (interactive == 0) {
                /*
                 * 2nd time thru and we are not interactive,
                 * return fatal error code back to caller.
                 */
                return ((func_t)-2);
        }        
 
        if (*args[0] == '-')
                howto = bootflags(args[0]);
        else
                howto = bootflags(args[1]);
        if (howto & RB_DEBUG)
                interactive = 1;
 
        /*
         * Now we have to ask for the name of the program to load
         * if we are interactive.
         */
        *filename = '\0';
        if (interactive) {
                printf("%s: ", myname);
                gets(filename);
        }
 
        /*
         * If no file name given (or we are non-interactive),
         * default to patchable string in vmunix[]
         */
        if (*filename == '\0') {
                (void)strcpy(filename, vmunix);
                printf("%s: %s\n", myname, filename);
        }
        parseargs(filename, howto, args, bargs);
 
        (void) reopen(0);       /* Restart the I/O. */
        io = open(filename, 0);
        if (io >= 0) {
                go2 = readfile(io, 1, filename);
        } else {
                printf("boot failed\n");
		interactive = 1;
                go2 = (func_t)-1;
        }
        close(io);              /* Done with it. */
        *arg = filename;
        return (go2);
}

/*
 * Parse the boot line to set bootparam structure
 */
setparam(cp, bp)
        register char *cp;
        register char *bp[];
{
        register int i = 0;
 
        while (*cp && isspace(*cp))
                cp++;
        while ((i < 8) && *cp) {
                bp[i++] = cp;
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
        for (; i < 8; ++i)
                bp[i] = '\0';
}
#endif OPENPROMS

func_t
load_it(arg)
	register char **arg;
{
	register struct bootparam *bp;
	register int howto;
	register int io;
	func_t go2;
	extern char myname_default[];
	register char *ap;
	static char filename[LINEBUFSZ];

#ifdef OPENPROMS
	if (romp->op_romvec_version > 0)
		return (obp_load_it(arg));
#endif

#if defined(sun2) || defined(sun3)
	if (romp->v_bootparam == 0)
		bp = (struct bootparam *)0x2000;	/* Old Sun-1 address */
	else
#endif sun2 || sun3
		bp = *romp->v_bootparam;		/* S-2: via romvec */

	*arg = bp->bp_argv[0];
	if (*arg == NULL)
		*arg = "";
	if (myname[0] == '\0') {
		register char *s, *p;

		for (s = *arg, ap = aline; *s && *s != ')';)
			*ap++ = *s++;
		*ap++ = *s++;
		if (*s == '\0')
			s = myname_default;
		for (p = myname; *s;)
			*p++ = *s++;
	} else if (interactive == 0) {
		/*
		 * 2nd time thru and we are not interactive,
		 * return fatal error code back to caller.
		 */
		return ((func_t)-2);
	}

	howto = bootflags(bp->bp_argv[1]);
	if (howto & RB_DEBUG)
		interactive = 1;

	/*
	 * Now we have to ask for the name of the program to load
	 * if we are interactive.
	 */
	*filename = '\0';
	if (interactive) {
		printf("%s: ", myname);
		gets(filename);
	}

	/*
	 * If no file name given (or we are non-interactive),
	 * default to patchable string in vmunix[]
	 */
	if (*filename == '\0') {
		register char *s, *p;

		for (s = filename, p = vmunix; *p;)
			*s++ = *p++;
		*s = '\0';
		printf("%s: %s\n", myname, filename);
		for (s = filename; *s; )
			*ap++ = *s++;
		*ap = '\0';
	}
	parseparam(filename, howto, bp);

	(void) reopen(aline);	/* Restart the I/O. */
	io = open(filename, 0);
	if (io >= 0) {
		go2 = readfile(io, 1, filename);
	} else {
		printf("boot failed\n");
		go2 = (func_t)-1;
	}
	close(io);		/* Done with it. */
	*arg = filename;
	return (go2);
}

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

/*
 * Parse the boot line to determine boot flags 
 */
bootflags(cp)
	register char *cp;
{
	register int i, boothowto = 0;

	if ((cp) && (*cp++ == '-'))
		while (*cp) {
			for (i = 0; bootf[i].let; i++) {
				if (*cp == bootf[i].let) {
					boothowto |= bootf[i].bit;
					break;
				}
			}
			cp++;
		}
	return (boothowto);
}

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
	defaults |= RB_DEBUG;		/* or in debug flag */
	*lp++ = '-';
	for (i = 0; bootf[i].let; i++) {
		if (defaults & bootf[i].bit)
			*lp++ = bootf[i].let;
	}
	*lp++ = 0;
	for (i = 1; i < 8; ++i) {
		cp = bp->bp_argv[i];
		if (cp && *cp && (*cp != '-')) {
			while (*lp++ = *cp++)
				;
		}
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

#ifdef OPENPROMS
/*
 * Parse the line argument and put it in the bootargs in OBP.
 * Preserve the flags unless redefined. Always preserve the remaining
 * arguments. Leave just a filename in line.
 *
 * The line looks like:
 *      file[whitespace]-[flags]\0
 *
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
        char buf[128];
 
        /*
         * Find the end of the file argument.
         */
        while (*lp && isgraph(*lp))
                lp++;
        cp = lp++;
 
        /*
         * Scan ahead, looking for flags.
         */
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
        lp = cp;
        defaults |= RB_DEBUG;
        if (defaults) {
                *lp++ = ' ';
                *lp++ = '-';
                for (i = 0; bootf[i].let; i++) {
                        if (defaults & bootf[i].bit)
                                *lp++ = bootf[i].let;
                }
                *lp = '\0';
        }
        if (bp[1] && *(bp[1]) == '-')
                i = 2;
        else
                i = 1;
        for (; i < 8; ++i) {
                cp = bp[i];
                if (cp && *cp && (*cp != '-')) {
                        *lp++ = ' ';
                        while (*lp++ = *cp++)
                                ;
                        *lp = '\0';
                }
        }
 
        /*
         * Copy the fully qualified boot line to the bootargs.
         * Reset the bootparam arguments.
         */
        bcopy(line, dp, 100);
        setparam(line, bp);
}
#endif OPENPROMS
