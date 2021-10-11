#ifndef lint
static  char sccsid[] = "@(#)tpboot.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */
/*
 * this version of tpboot does not ask for anything.
 * it merely boots the MUNIX that is on the tape, forceing "-sw".
 */
#define RB_DEFAULT	(RB_SINGLE | RB_WRITABLE)

#include <stand/saio.h>
#include <sys/reboot.h>
#include <mon/sunromvec.h>
#include <sys/param.h>
#include <machine/pte.h>

#define	skipblank(p)	{ while (*(p) == ' ') (p)++; }

/*
 * No ctype, alas.
 */
#ifndef isspace
#define isspace(c)	((c) == ' ' || (c) == '\t' || (c) == '\n' || \
			 (c) == '\r' || (c) == '\f' || (c) == '\013')
#endif
#ifndef isgraph
#define isgraph(c)	((c) >= '!' && (c) <= '~')
#endif
#ifndef	isdigit
#define	isdigit(c)	((c) >= '0' && (c) <= '9')
#endif
#ifndef	isalpha
#define	isalpha(c)	(((c) >= 'a' && (c) <= 'z') || \
			 ((c) >= 'A' && (c) <= 'Z'))
#endif

extern char	*gethex();
extern struct saioreq *open_sip;

/*
 * Boot program... argument from monitor determines
 * whether boot stops to ask for system name and which device
 * boot comes from.
 */
int (*readfile())();

char aline[100] = "\0";

main()
{
	register struct bootparam *bp;
	register int io, aflag=0;
	register char *cp;
	int (*go2)();

	if (romp->v_bootparam == 0)
		bp = (struct bootparam *)0x2000;	/* Old Sun-1 address */
	else
		bp = *(romp->v_bootparam);		/* S-2: via romvec */
#ifdef OPENPROMS
	kmem_init();
	init_devsw();
#endif

#ifdef	sun3
	kmem_remap();	/* map around video ram */
#endif	sun3

	cp = bp->bp_argv[1];
	skipblank(cp);
	if (strcmp(cp, "-a") == 0) 
		aflag = 1;
	else if (*cp == '-') {
		printf("Huh?\n");
		aflag = 1;
	}
	for (;;) {
		if (aflag) {
			printf("Boot: ");
			gets(aline);
                        if (parseparam(aline, RB_DEFAULT, bp))
                                continue;
		}
		else {
			/*
	 	 	 * stuff the first part of the boot args into a buffer,
	 	 	 * forcing file 2 and "-sw"
	 	 	 */
			sprintf(aline, "%c%c(%x,%x,%x) -sw", bp->bp_dev[0], 
				bp->bp_dev[1], bp->bp_ctlr, bp->bp_unit, 2);

			if (parseparam(aline, RB_DEFAULT, bp)) {
				printf("tpboot: unparsable command %s", 
				bp->bp_argv[0]);
				_stop((char *) 0);
			}
		}

		io = open(aline, 0);
		if (io >= 0) {
			/* FIXME: really need to rebuild line strings too */
			bp->bp_ctlr = open_sip->si_ctlr;
			bp->bp_unit = open_sip->si_unit;

			go2 = readfile(io, 1);
#ifdef	OPENPROMS
			close(io);
#endif
			if ((int)go2 != -1)
				_exitto(go2, bp->bp_argv[0]);
		} else {
			printf("boot failed\n");
			_stop((char *) 0);
		}
	}
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
	'w',	RB_WRITABLE,
	0,	0,
};

/*
 * Parse the boot line to determine boot flags 
 */
bootflags(cp)
	register char *cp;
{
	register int i, boothowto = 0;

	if (*cp++ == '-')
	do {
		for (i = 0; bootf[i].let; i++) {
			if (*cp == bootf[i].let) {
				boothowto |= bootf[i].bit;
				break;
			}
		}
		cp++;
	} while (bootf[i].let && *cp);
#ifdef FORCE_ASKNAME
	boothowto |= RB_ASKNAME;
#endif
	return (boothowto);
}

/*
 * Parse the boot line and put it in bootparam.  Stuff in -a -s or -as if
 * s/he only typed one argument and if they were in effect before.
 */
parseparam(line, defaults, bp)
	char *line;
	int defaults;
	register struct bootparam *bp;
{
	register int i;
	register char *cp, *lp = line;

	/* Check for blank line and skip over spaces  */
	if (*lp == '\0') {
		return (1);
	}
	skipblank(lp);

	/* Get device name  */
	if (*lp == '\0') {
		return (1);
	}
	bp->bp_dev[0] = *lp++;
	bp->bp_dev[1] = *lp++;

	/* Get ctlr, unit, part */
	if (*lp++ == '(' ) {
		lp = gethex(lp, &bp->bp_ctlr);
		lp = gethex(lp, &bp->bp_unit);
		lp = gethex(lp, &bp->bp_part);
		if (*lp++ != ')') {
			return (1);
		}
	} else {
		return (1);
	}

	lp = line;
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
		*lp++ = '\0';
	}
	for (i = 1; i < 8; ++i) {
		cp = bp->bp_argv[i];
	/*
	 * NOTE: the code in ifdef should actually apply to all
   	 *	 architectyures, but due to lack of test time
	 *	 it would only be fixed on sun3x now.
	 */
#ifdef sun3x
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
#endif sun3x
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
	return(0);
}


#ifdef sun3
#define VIDEO_START	0x100000	/* physical address of start of video ram */
#define ONE_PMEG	16*PAGESIZE
#define VIDEO_SIZE	ONE_PMEG

kmem_remap()
{
    extern struct pte getpgmap();
    extern int setpgmap();
    extern int start;

    register int vaddr, paddr;
    struct pte p;

    p.pg_v = 1; p.pg_prot = KW; p.pg_nc = 1; p.pg_type = OBMEM;
    p.pg_r = 0; p.pg_m = 0; 

    /* map 0-VIDEO_START virtual addresses 1:1 to physical addresses */
    for (vaddr = 0, paddr = 0; paddr < VIDEO_START;
	 vaddr += PAGESIZE, paddr += PAGESIZE) {

	p.pg_pfnum = paddr >> PAGESHIFT;
	setpgmap(vaddr,p);
    }

    paddr += VIDEO_SIZE;
    for (; vaddr < (int)&start - 2*ONE_PMEG;
	 vaddr += PAGESIZE, paddr += PAGESIZE) {

	p.pg_pfnum = paddr >> PAGESHIFT;
	setpgmap(vaddr,p);
    }
}
#endif sun3

