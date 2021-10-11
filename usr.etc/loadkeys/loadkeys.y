%{

#ifndef lint
static  char sccsid[] = "@(#)loadkeys.y 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <sys/param.h>

#include <ctype.h>

#include <stdio.h>

#include <search.h>

extern char	*bsearch();

#include <string.h>

extern long	strtol();

#include <fcntl.h>

extern int	open();

extern char	*malloc();

extern int	ioctl();

#include <errno.h>
#include <sundev/kbd.h> 
#include <sundev/kbio.h>
#include <machine/eeprom.h>

/* The follwing are loaded when (and only when) the EEPROM is being
 * written for what we think is the very first time. Note that this is 
 * a different structure to that found in the sunos driver kbd.c,
 * therofore the old EEPROM constants still prevail.
 */

#define	O_SHIFTKEYS	0x80	
#define	O_BUCKYBITS	0x90	
#define	O_FUNNY		0xA0	
#define	O_NOP		0xA0	
#define	O_OOPS		0xA1	
#define	O_HOLE		0xA2	
#define	O_NOSCROLL	0xA3	
#define	O_CTRLS		0xA4	
#define	O_CTRLQ		0xA5	
#define	O_RESET		0xA6	
#define	O_ERROR		0xA7	
#define	O_IDLE		0xA8	
#define	O_STRING	0xB0	
#define	O_HOMEARROW	0x00
#define	O_UPARROW	0x01
#define	O_DOWNARROW	0x02
#define	O_LEFTARROW	0x03
#define	O_RIGHTARROW	0x04
#define	O_PF1		0x05
#define	O_PF2		0x06
#define	O_PF3		0x07
#define	O_PF4		0x08
#define O_SYSTEMBIT	1
#define O_CAPSLOCK       0               
#define O_CAPSMASK       0x0001
#define O_SHIFTLOCK      1               
#define O_LEFTSHIFT      2               
#define O_RIGHTSHIFT     3               
#define O_SHIFTMASK      0x000E
#define O_LEFTCTRL       4               
#define O_RIGHTCTRL      5               
#define O_ALT            6               
#define O_CTRLMASK       0x0030
#define O_METABIT	 0

struct ee_keymap keytab_s2_lc[1] = {
/*  0 */ O_HOLE, O_BUCKYBITS+O_SYSTEMBIT,
    O_OOPS, O_OOPS, O_HOLE, O_OOPS, O_OOPS, O_OOPS,
/*  8 */ O_OOPS,  O_OOPS,  O_OOPS, O_OOPS, O_OOPS, O_OOPS, O_OOPS, O_OOPS,
/* 16 */ O_OOPS,  O_OOPS,  O_OOPS, O_ALT, O_HOLE, O_OOPS, O_OOPS, O_OOPS,
/* 24 */ O_HOLE,  O_OOPS,  O_OOPS, O_OOPS, O_HOLE, '\033', '1', '2',
/* 32 */ '3', '4', '5', '6', '7', '8', '9', '0',
/* 40 */ '-', '=', '`', '\b', O_HOLE, O_OOPS, O_OOPS, O_OOPS,
/* 48 */ O_HOLE, O_OOPS, O_OOPS, O_OOPS, O_HOLE, '\t', 'q', 'w',
/* 56 */ 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
/* 64 */ '[', ']', 0x7F, O_HOLE, O_OOPS, O_STRING+O_UPARROW, O_OOPS, O_HOLE,
/* 72 */ O_OOPS, O_OOPS, O_OOPS, O_HOLE, O_SHIFTKEYS+O_LEFTCTRL, 'a',  's', 'd',
/* 80 */ 'f', 'g', 'h', 'j', 'k', 'l', '!', '\'',
/* 88 */ '\\', '\r', O_HOLE, O_STRING+O_LEFTARROW, O_OOPS, O_STRING+O_RIGHTARROW,
        O_HOLE, O_OOPS,
/* 96 */ O_OOPS, O_OOPS, O_HOLE, O_SHIFTKEYS+O_LEFTSHIFT, 'z', 'x', 'c', 'v',
/*104 */ 'b', 'n', 'm', ',', '.', '/', O_SHIFTKEYS+O_RIGHTSHIFT, '\n',
/*112 */ O_OOPS, O_STRING+O_DOWNARROW, O_OOPS, O_HOLE, O_HOLE, O_HOLE, O_HOLE, O_CAPSLOCK,
/*120 */ O_BUCKYBITS+O_METABIT, ' ', O_BUCKYBITS+O_METABIT,
     O_HOLE, O_HOLE, O_HOLE, O_ERROR, O_IDLE,
};

struct ee_keymap keytab_s2_uc[1] = {
/*  0 */ O_HOLE, O_BUCKYBITS+O_SYSTEMBIT,
    O_OOPS, O_OOPS, O_HOLE, O_OOPS, O_OOPS, O_OOPS,
/*  8 */ O_OOPS,  O_OOPS,  O_OOPS, O_OOPS, O_OOPS, O_OOPS, O_OOPS, O_OOPS,
/* 16 */ O_OOPS,  O_OOPS,  O_OOPS, O_ALT, O_HOLE, O_OOPS, O_OOPS, O_OOPS,
/* 24 */ O_HOLE,  O_OOPS,  O_OOPS, O_OOPS, O_HOLE, '\033', '!', '@',
/* 32 */ '#', '$', '%', '^', '&', '*', '(', ')',
/* 40 */ '_', '+', '~', '\b', O_HOLE, O_OOPS, O_OOPS, O_OOPS,
/* 48 */ O_HOLE, O_OOPS, O_OOPS, O_OOPS, O_HOLE, '\t', 'Q', 'W',
/* 56 */ 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',
/* 64 */ '{', '}', 0x7F, O_HOLE, O_OOPS, O_STRING+O_UPARROW, O_OOPS, O_HOLE,
/* 72 */ O_OOPS, O_OOPS, O_OOPS, O_HOLE, O_SHIFTKEYS+O_LEFTCTRL, 'A',  'S', 'D',
/* 80 */ 'F', 'G', 'H', 'J', 'K', 'L', ':', '"',
/* 88 */ '|', '\r', O_HOLE, O_STRING+O_LEFTARROW, O_OOPS, O_STRING+O_RIGHTARROW,
        O_HOLE, O_OOPS,
/* 96 */ O_OOPS, O_OOPS, O_HOLE, O_SHIFTKEYS+O_LEFTSHIFT,
      'Z', 'X', 'C', 'V',
/*104 */ 'B', 'N', 'M', '<', '>', '?', O_SHIFTKEYS+O_RIGHTSHIFT, '\n',
/*112 */ O_OOPS, O_STRING+O_DOWNARROW, O_OOPS, O_HOLE, O_HOLE, O_HOLE, O_HOLE, O_CAPSLOCK,
/*120 */ O_BUCKYBITS+O_METABIT, ' ', O_BUCKYBITS+O_METABIT,
     O_HOLE, O_HOLE, O_HOLE, O_ERROR, O_IDLE,
};


#define	ALL	-1	/* special symbol for all tables */

static char	keytable_dir[] = "/usr/share/lib/keytables/";


char    *eeprom_dev = "/dev/eeprom";
char	openprom_dev[] = "/dev/openprom";

int has_openprom;

struct keyentry {
	struct keyentry	*ke_next;
	struct kiockeymap ke_entry;
};

typedef struct keyentry keyentry;

static keyentry *firstentry;
static keyentry *lastentry;

struct dupentry {
	struct dupentry *de_next;
	int	de_station;
	int	de_otherstation;
};

typedef struct dupentry dupentry;

static dupentry *firstduplicate;
static dupentry *lastduplicate;

static dupentry *firstswap;
static dupentry *lastswap;

static char	*infilename;
static FILE	*infile;
static int	lineno;
static int	begline;

static char *strings[16] = {
	"\033[H",		/* HOMEARROW */
	"\033[A",		/* UPARROW */
	"\033[B",		/* DOWNARROW */
	"\033[D",		/* LEFTARROW */
	"\033[C",		/* RIGHTARROW */
};

static int nstrings = 5;	/* start out with 5 strings */
static short changeeeprom = 0;	/* This is set if eeprom is out of date */
static short doeeprom = 0;	/* Set when -e option specified */
struct eeprom eeprom;
struct eeprom eeorig;

typedef enum {
	SM_INVALID,	/* this shift mask is invalid for this keyboard */
	SM_NORMAL,	/* "normal", valid shift mask */
	SM_NUMLOCK,	/* "Num Lock" shift mask */
	SM_UP		/* "Up" shift mask */
} smtype_t;

typedef struct {
	int	sm_mask;
	smtype_t sm_type;
} smentry_t;

smentry_t shiftmasks[] = {
	{ 0,		SM_NORMAL },
	{ SHIFTMASK,	SM_NORMAL },
	{ CAPSMASK,	SM_NORMAL },
	{ CTRLMASK,	SM_NORMAL },
	{ ALTGRAPHMASK,	SM_NORMAL },
	{ NUMLOCKMASK,	SM_NUMLOCK },
	{ UPMASK,	SM_UP },
};

/* Unshifted keyboard table for all Sun workstations post sun-2
 * Although this work only really applies to type4 with international
 * flavours set.
 */


#define	NSHIFTS	(sizeof shiftmasks / sizeof shiftmasks[0])

static void	enter_mapentry(/*int station, keyentry *entrylistp*/);
static keyentry *makeentry(/*int tablemask, int entry*/);
static int	loadkey(/*int kbdfd, keyentry *kep*/);
static int	dupkey(/*int kbdfd, dupentry *dep, int shiftmask*/);
static int	swapkey(/*int kbdfd, dupentry *dep, int shiftmask*/);
static int	yylex();
static int	readesc(/*FILE *stream, int delim*/);
static int	wordcmp(/*word_t *w1, word_t *w2*/);
static int	yyerror(/*char *msg*/);
static u_char 	chksum(/*u_char p, int n*/);

int
main(argc, argv)
	int argc;
	char **argv;
{
	register int kbdfd;
	int type;
	int layout;
	char pathbuf[MAXPATHLEN];
	register int shift;
	struct kiockeymap mapentry;
	register keyentry *kep;
	register dupentry *dep;
	short Error = 0;

	if (argc > 3) 
			Error = 1;
	if (argc > 1)
		if (argv[1][0] == '-' ) {
			if (argv[1][1] != 'e')
				Error = 1;
			else
				doeeprom = 1;
		}
	if (Error) {
		(void) fprintf(stderr, "usage: loadkeys [-e] [ file ]\n");
		return (1);
	}

	if ((kbdfd = open("/dev/kbd", O_WRONLY)) < 0) {
		/* perror("loadkeys: /dev/kbd"); */
		return (1);
	}

	if (ioctl(kbdfd, KIOCTYPE, &type) < 0) {
		perror("loadkeys: KIOCTYPE");
		return (1);
	}

	if (argc == 1 || (argc == 2 && doeeprom) ) {
		/*
		 * If this isn't a Type 4 keyboard, don't do anything.
		 * The "layout_N" tables are Type 4 and vt220 only
		 */
		if (type != KB_SUN4 && type != KB_VT220 && type != KB_VT220I) {
			/* We must still check to see if we have 
			 * switched back to a non-type4 keyboard
			 * Note vt220 eeprom tables can be == type4
			 */
			if (doeeprom && checkeeprom(type, 0))
				return 1;
			if (changeeeprom)
				write_eetable();
			return 0;
		}
		if (type == KB_VT220 || type == KB_VT220I)
			layout = type;
		else {
			if (ioctl(kbdfd, KIOCLAYOUT, &layout) < 0) {
				perror("loadkeys: KIOCLAYOUT");
				return (1);
			}
		}
		(void) sprintf(pathbuf, "/usr/share/lib/keytables/layout_%.2x",
		    layout);
		infilename = pathbuf;
		
		/* Now we must see if the keyboard has changed 
		 * since the last time we ran loadkeys. In particular
		 * this will set up a variant type4 keyboard when this is 
		 * run for the very first time on a new system. 
		 * Ignore US keyboard case.
		 */
		if (doeeprom)
			(void) checkeeprom(type, layout);

	} else 
		infilename = argv[argc-1];

	if ((infile = fopen(infilename, "r")) == NULL) {
		if (errno == ENOENT && *infilename != '/') {
			if (strlen(keytable_dir) + strlen(infilename) + 1
			    > MAXPATHLEN) {
				(void) fprintf(stderr,
				    "loadkeys: Name %s is too long\n",
				    infilename);
				return (1);
			}
			(void) strcpy(pathbuf, keytable_dir);
			(void) strcat(pathbuf, infilename);
			if ((infile = fopen(pathbuf, "r")) == NULL
			    && errno == ENOENT) {
				(void) fprintf(stderr,
				    "loadkeys: Can't find keytable %s\n",
				    infilename);
				return (1);
			} else
				infilename = pathbuf;
		}
	}

	if (infile == NULL) {
		(void) fprintf(stderr, "loadkeys: ");
		perror(infilename);
		return (1);
	}

	lineno = 0;
	begline = 1;
	(void) yyparse();
	(void) fclose(infile);

	/*
	 * See which shift masks are valid for this keyboard.
	 * We do that by trying to get the entry for keystation 0 and that
	 * shift mask; if the "ioctl" fails, we assume it's because the shift
	 * mask is invalid.
	 */
	for (shift = 0; shift < NSHIFTS; shift++) {
		mapentry.kio_tablemask =
		    shiftmasks[shift].sm_mask;
		mapentry.kio_station = 0;
		if (ioctl(kbdfd, KIOCGKEY, &mapentry) < 0)
			shiftmasks[shift].sm_type = SM_INVALID;
	}

	for (kep = firstentry; kep != NULL; kep = kep->ke_next) {
		if (kep->ke_entry.kio_tablemask == ALL) {
			for (shift = 0; shift < NSHIFTS; shift++) {
				switch (shiftmasks[shift].sm_type) {

				case SM_INVALID:
					continue;

				case SM_NUMLOCK:
					/*
					 * Defaults to NONL, not to a copy of
					 * the base entry.
					 */
					if (kep->ke_entry.kio_entry != HOLE)
						kep->ke_entry.kio_entry = NONL;
					break;

				case SM_UP:
					/*
					 * Defaults to NOP, not to a copy of
					 * the base entry.
					 */
					if (kep->ke_entry.kio_entry != HOLE)
						kep->ke_entry.kio_entry = NOP;
					break;
				}
				kep->ke_entry.kio_tablemask =
				    shiftmasks[shift].sm_mask;
				if (!loadkey(kbdfd, kep))
					return (1);
			}
		} else {
			if (!loadkey(kbdfd, kep))
				return (1);
			/* Change only unshifted and shifted keytables */
			if (changeeeprom && ((kep->ke_entry.kio_tablemask == 0) ||
				(kep->ke_entry.kio_tablemask == SHIFTMASK)) )
				change_eetable(kep);
		}
	}

	for (dep = firstswap; dep != NULL; dep = dep->de_next) {
		for (shift = 0; shift < NSHIFTS; shift++) {
			if (shiftmasks[shift].sm_type != SM_INVALID) {
				if (!swapkey(kbdfd, dep,
				    shiftmasks[shift].sm_mask))
					return (0);
			}
		}
	}

	for (dep = firstduplicate; dep != NULL; dep = dep->de_next) {
		for (shift = 0; shift < NSHIFTS; shift++) {
			if (shiftmasks[shift].sm_type != SM_INVALID) {
				if (!dupkey(kbdfd, dep,
				    shiftmasks[shift].sm_mask))
					return (0);
			}
		}
	}

	/* load up new eeprom here if requested */

	if (changeeeprom) 
		write_eetable();

	(void) close(kbdfd);
	return (0);
}

/*
 * We have a list of entries for a given keystation, and the keystation number
 * for that keystation; put that keystation number into all the entries in that
 * list, and chain that list to the end of the main list of entries.
 */
static void
enter_mapentry(station, entrylistp)
	int station;
	keyentry *entrylistp;
{
	register keyentry *kep;

	if (lastentry == NULL)
		firstentry = entrylistp;
	else
		lastentry->ke_next = entrylistp;
	kep = entrylistp;
	for (;;) {
		kep->ke_entry.kio_station = station;
		if (kep->ke_next == NULL) {
			lastentry = kep;
			break;
		}
		kep = kep->ke_next;
	}
}

/*
 * Allocate and fill in a new entry.
 */
static keyentry *
makeentry(tablemask, entry)
	int tablemask;
	int entry;
{
	register keyentry *kep;
	register int index;

	if ((kep = (keyentry *) malloc((unsigned)sizeof (keyentry))) == NULL)
		yyerror("out of memory for entries");
	kep->ke_next = NULL;
	kep->ke_entry.kio_tablemask = tablemask;
	kep->ke_entry.kio_station = 0;
	kep->ke_entry.kio_entry = entry;
	index = entry - STRING;
	if (index >= 0 && index <= 15)
		(void) strncpy(kep->ke_entry.kio_string, strings[index],
		    KTAB_STRLEN);
	return (kep);
}

/*
 * Make a set of entries for a keystation that indicate that that keystation's
 * settings should be copied from another keystation's settings.
 */
static void
duplicate_mapentry(station, otherstation)
	int station;
	int otherstation;
{
	register dupentry *dep;

	if ((dep = (dupentry *) malloc((unsigned)sizeof (dupentry))) == NULL)
		yyerror("out of memory for entries");

	if (lastduplicate == NULL)
		firstduplicate = dep;
	else
		lastduplicate->de_next = dep;
	lastduplicate = dep;
	dep->de_next = NULL;
	dep->de_station = station;
	dep->de_otherstation = otherstation;
}

/*
 * Make a set of entries for a keystation that indicate that that keystation's
 * settings should be swapped with another keystation's settings.
 */
static void
swap_mapentry(station, otherstation)
	int station;
	int otherstation;
{
	register dupentry *dep;

	if ((dep = (dupentry *) malloc((unsigned)sizeof (dupentry))) == NULL)
		yyerror("out of memory for entries");

	if (lastswap == NULL)
		firstswap = dep;
	else
		lastswap->de_next = dep;
	lastswap = dep;
	dep->de_next = NULL;
	dep->de_station = station;
	dep->de_otherstation = otherstation;
}

static int
loadkey(kbdfd, kep)
	int kbdfd;
	register keyentry *kep;
{
	if (ioctl(kbdfd, KIOCSKEY, &kep->ke_entry) < 0) {
		perror("loadkeys: ioctl(KIOCSKEY)");
		return (0);
	}
	return (1);
}

static int
dupkey(kbdfd, dep, shiftmask)
	int kbdfd;
	register dupentry *dep;
	int shiftmask;
{
	struct kiockeymap entry;

	entry.kio_tablemask = shiftmask;
	entry.kio_station = dep->de_otherstation;
	if (ioctl(kbdfd, KIOCGKEY, &entry) < 0) {
		perror("loadkeys: ioctl(KIOCGKEY)");
		return (0);
	}
	entry.kio_station = dep->de_station;
	if (ioctl(kbdfd, KIOCSKEY, &entry) < 0) {
		perror("loadkeys: ioctl(KIOCSKEY)");
		return (0);
	}
	return (1);
}

static int
checkeeprom(type, layout)
register int type, layout;
{

	/* 
	 * Check what the keyboard tells us against what we 
	 * think it is in the eeprom
	 */

	int fd, size;
	extern int errno;

	has_openprom = 0;
	fd = open(openprom_dev, 0);
	if (fd >= 0) {
		has_openprom = 1;
		(void) close(fd);
	} else if (errno != ENOENT && errno != ENODEV) {
		perror(openprom_dev);
		return (1);
	}

	/*
	 * XXX -e not supported on openprom machines
	 */
	if (has_openprom) {
		changeeeprom = 0;
		return (1);
	}

        if ((fd = open(eeprom_dev, 0)) < 0) {
                perror(eeprom_dev);
                return(1);
        }
        size = sizeof (struct eeprom);
	if (read(fd, (char *) &eeprom, size) != size) {
		perror(eeprom_dev);
                fprintf(stderr, "Error in reading %s\n", eeprom_dev);
                return(1);
        }

        close(fd);
	
	/* The following means that we've changed away from a type 4 */
	if (type != 4 && eeprom.ee_diag.ee_kb_id == 4) {
		eeprom.ee_diag.which_kbt = 0;
		eeprom.ee_diag.ee_kb_id = 0;
		eeprom.ee_diag.ee_kb_type = 0;
		changeeeprom = 1;
		return 0;
	}

	/* The following means that we've changed up to a type 4 */
	
	if (type == 4 && eeprom.ee_diag.ee_kb_id != 4 && layout != 0) {
		load_uslayout();
		eeprom.ee_diag.ee_kb_id = type;
		eeprom.ee_diag.which_kbt = 0x58;
		changeeeprom = 1;
	}
	if ((layout != eeprom.ee_diag.ee_kb_type) && (type == 4)) {
		if (layout == 0) {
			/* This is the standard layout might as well
			 * default back to the PROM version */
			eeprom.ee_diag.which_kbt = 0;
			eeprom.ee_diag.ee_kb_id = 0;
			eeprom.ee_diag.ee_kb_type = 0;
		}
		eeprom.ee_diag.ee_kb_type = layout;
		changeeeprom = 1;
	}


	return(0);
}

load_uslayout() {

	/* This should be writing over empty stuff, but its still ok
	 * even if information previously existed there, because by this
 	 * stage we're supposed to be a type4 keyboard
	 */
	eeprom.ee_diag.ee_keytab_lc[0] = keytab_s2_lc[0]; /*Lower case */
	eeprom.ee_diag.ee_keytab_uc[0] = keytab_s2_uc[0]; /*Upshifted  */
}

static int 
write_eetable()
{
	int fd;
	char *op, *np;		/* old and new update dates */
	int written;

	written = 0;

	/* Don't even bother if we are not uid root - because 
	 * this will get picked up on the next reboot and
	 * if we are up and running we should not be worried about 
	 * monitor-time. (we don't want ugly messages)
	 */

	if ((getuid() != 0) && (geteuid() != 0))
		return (1);
	if ((fd = open(eeprom_dev, 2)) < 0) {
		perror(eeprom_dev);
		return (1);
	}
	if (read(fd, (char *)&eeorig, sizeof (struct eeprom)) != sizeof (struct eeprom)) {
		fprintf(stderr,"Error in reading from %s\n", eeprom_dev);
		return (1);
	}
	op = (char *)&eeorig.ee_diag.eed_hwupdate;
	np = (char *)&eeprom.ee_diag.eed_hwupdate;

	/*
	 * Write diagnostic section.
	 */
	while (op < (char *)&eeorig.ee_resv) {
		if (*np != *op) {
			lseek(fd, (long) (np - (char *)&eeprom), 0);
			write(fd, np, 1);
			written = 1;
		}
		op++;
		np++;
	}
	if (written) {
		/*
		 * Recalculate checksum.
		 */
		u_char *cp;
		u_short *wp;

		cp = &eeprom.ee_diag.eed_chksum[0];
		cp[0] = cp[1] = cp[2] =
		    chksum((u_char *)&eeprom.ee_diag.eed_hwupdate,
			sizeof (eeprom.ee_diag) -
			((char *)(&eeprom.ee_diag.eed_hwupdate) -
			    (char *)(&eeprom.ee_diag)));
		lseek(fd, (long) ((char *)cp - (char *)&eeprom), 0);
		write(fd, (char *)cp, 3 * sizeof (*cp));
		wp = &eeprom.ee_diag.eed_wrcnt[0];
		wp[0] = wp[1] = wp[2] = wp[0] + 1;
		lseek(fd, (long) ((char *)wp - (char *)&eeprom), 0);
		if (write(fd, (char *)wp, 3 * sizeof (*wp)) < 0) {
			fprintf(stderr,"Error in writing new EEPROM keytables\n");
			(void) close(fd);
			return 1;
		}
	}
	close(fd);
	return 0;
}



static int 
change_eetable(kep)
register keyentry *kep;
{
	unsigned short keystation, entry;
	char *cp;

	keystation = kep->ke_entry.kio_station;
	entry = kep->ke_entry.kio_entry;
	if (kep->ke_entry.kio_tablemask == 0) {
		cp = (char *)&eeprom.ee_diag.ee_keytab_lc[0];
		*(cp + keystation) = entry;
	}
	if (kep->ke_entry.kio_tablemask == SHIFTMASK) {
		cp = (char *)&eeprom.ee_diag.ee_keytab_uc[0];
		*(cp + keystation) = entry;
	}
		
}

static unsigned char
chksum(p, n)
        register u_char *p;
        register int n;
{
        register u_char s = 0;

        while (n--)
                s += *p++;
        return (256 - s);
}



static int
swapkey(kbdfd, dep, shiftmask)
	int kbdfd;
	register dupentry *dep;
	int shiftmask;
{
	struct kiockeymap entry1, entry2;

	entry1.kio_tablemask = shiftmask;
	entry1.kio_station = dep->de_station;
	if (ioctl(kbdfd, KIOCGKEY, &entry1) < 0) {
		perror("loadkeys: ioctl(KIOCGKEY)");
		return (0);
	}
	entry2.kio_tablemask = shiftmask;
	entry2.kio_station = dep->de_otherstation;
	if (ioctl(kbdfd, KIOCGKEY, &entry2) < 0) {
		perror("loadkeys: ioctl(KIOCGKEY)");
		return (0);
	}
	entry1.kio_station = dep->de_otherstation;
	if (ioctl(kbdfd, KIOCSKEY, &entry1) < 0) {
		perror("loadkeys: ioctl(KIOCSKEY)");
		return (0);
	}
	entry2.kio_station = dep->de_station;
	if (ioctl(kbdfd, KIOCSKEY, &entry2) < 0) {
		perror("loadkeys: ioctl(KIOCSKEY)");
		return (0);
	}
	return (1);
}
%}

%term TABLENAME INT CHAR CHARSTRING CONSTANT FKEY KEY SAME AS SWAP WITH

%union {
	keyentry *keyentry;
	int	number;
};

%type <keyentry>	entrylist entry
%type <number>		CHARSTRING CHAR INT CONSTANT FKEY TABLENAME
%type <number>		code expr term number

%%

table:
	table line
|	/* null */
;

line:
	KEY number entrylist '\n'
		{
		enter_mapentry($2, $3);
		}
|	KEY number SAME AS number '\n'
		{
		duplicate_mapentry($2, $5);
		}
|	SWAP number WITH number '\n'
		{
		swap_mapentry($2, $4);
		}
|	'\n'
;

entrylist:
	entrylist entry
		{
		/*
		 * Append this entry to the end of the entry list.
		 */
		register keyentry *kep;
		kep = $1;
		for (;;) {
			if (kep->ke_next == NULL) {
				kep->ke_next = $2;
				break;
			}
			kep = kep->ke_next;
		}
		$$ = $1;
		}
|	entry
		{
		$$ = $1;
		}
;

entry:
	TABLENAME code
		{
		$$ = makeentry($1, $2);
		}
;

code:
	CHARSTRING
		{
		$$ = $1;
		}
|	CHAR
		{
		$$ = $1;
		}
|	'('
		{
		$$ = '(';
		}
|	')'
		{
		$$ = ')';
		}
|	'+'
		{
		$$ = '+';
		}
|	expr
		{
		$$ = $1;
		}
;

expr:
	term
		{
		$$ = $1;
		}
|	expr '+' term
		{
		$$ = $1 + $3;
		}
;

term:
	CONSTANT
		{
		$$ = $1;
		}
|	FKEY '(' number ')'
		{
		if ($3 < 1 || $3 > 16)
			yyerror("invalid function key number");
		$$ = $1 + $3 - 1;
		}
;

number:
	INT
		{
		$$ = $1;
		}
|	CHAR
		{
		if (isdigit($1))
			$$ = $1 - '0';
		else
			yyerror("syntax error");
		}
;

%%

typedef struct {
	char	*w_string;
	int	w_type;		/* token type */
	int	w_lval;		/* yylval for this token */
} word_t;

/*
 * Table must be in alphabetical order.
 */
word_t	wordtab[] = {
	{ "all",	TABLENAME,	ALL },
	{ "alt",	CONSTANT,	ALT },
	{ "altg",	TABLENAME,	ALTGRAPHMASK },
	{ "altgraph",	CONSTANT,	ALTGRAPH },
	{ "as",		AS,		0 },
	{ "base",	TABLENAME,	0 },
	{ "bf",		FKEY,		BOTTOMFUNC },
	{ "buckybits",	CONSTANT,	BUCKYBITS },
	{ "caps",	TABLENAME,	CAPSMASK },
	{ "capslock",	CONSTANT,	CAPSLOCK },
	{ "compose",	CONSTANT,	COMPOSE },
	{ "ctrl",	TABLENAME,	CTRLMASK },
	{ "ctrlq",	CONSTANT,	CTRLQ },
	{ "ctrls",	CONSTANT,	CTRLS },
	{ "downarrow",	CONSTANT,	DOWNARROW },
	{ "error",	CONSTANT,	ERROR },
	{ "fa_acute",	CONSTANT,	FA_ACUTE },
	{ "fa_cedilla",	CONSTANT,	FA_CEDILLA },
	{ "fa_cflex",	CONSTANT,	FA_CFLEX },
	{ "fa_grave",	CONSTANT,	FA_GRAVE },
	{ "fa_tilde",	CONSTANT,	FA_TILDE },
	{ "fa_umlaut",	CONSTANT,	FA_UMLAUT },
	{ "hole",	CONSTANT,	HOLE },
	{ "homearrow",	CONSTANT,	HOMEARROW },
	{ "idle",	CONSTANT,	IDLE },
	{ "key",	KEY,		0 },
	{ "leftarrow",	CONSTANT,	LEFTARROW },
	{ "leftctrl",	CONSTANT,	LEFTCTRL },
	{ "leftshift",	CONSTANT,	LEFTSHIFT },
	{ "lf",		FKEY,		LEFTFUNC },
	{ "metabit",	CONSTANT,	METABIT },
	{ "nonl",	CONSTANT,	NONL },
	{ "nop",	CONSTANT,	NOP },
	{ "noscroll",	CONSTANT,	NOSCROLL },
	{ "numl",	TABLENAME,	NUMLOCKMASK },
	{ "numlock",	CONSTANT,	NUMLOCK },
	{ "oops",	CONSTANT,	OOPS },
	{ "pad0",	CONSTANT,	PAD0 },
	{ "pad1",	CONSTANT,	PAD1 },
	{ "pad2",	CONSTANT,	PAD2 },
	{ "pad3",	CONSTANT,	PAD3 },
	{ "pad4",	CONSTANT,	PAD4 },
	{ "pad5",	CONSTANT,	PAD5 },
	{ "pad6",	CONSTANT,	PAD6 },
	{ "pad7",	CONSTANT,	PAD7 },
	{ "pad8",	CONSTANT,	PAD8 },
	{ "pad9",	CONSTANT,	PAD9 },
	{ "paddot",	CONSTANT,	PADDOT },
	{ "padenter",	CONSTANT,	PADENTER },
	{ "padequal",	CONSTANT,	PADEQUAL },
	{ "padminus",	CONSTANT,	PADMINUS },
	{ "padplus",	CONSTANT,	PADPLUS },
	{ "padsep",	CONSTANT,	PADSEP },
	{ "padslash",	CONSTANT,	PADSLASH },
	{ "padstar",	CONSTANT,	PADSTAR },
	{ "reset",	CONSTANT,	RESET },
	{ "rf",		FKEY,		RIGHTFUNC },
	{ "rightarrow",	CONSTANT,	RIGHTARROW },
	{ "rightctrl",	CONSTANT,	RIGHTCTRL },
	{ "rightshift",	CONSTANT,	RIGHTSHIFT },
	{ "same",	SAME,		0 },
	{ "shift",	TABLENAME,	SHIFTMASK },
	{ "shiftkeys",	CONSTANT,	SHIFTKEYS },
	{ "shiftlock",	CONSTANT,	SHIFTLOCK },
	{ "string",	CONSTANT,	STRING },
	{ "swap",	SWAP,		0 },
	{ "systembit",	CONSTANT,	SYSTEMBIT },
	{ "tf",		FKEY,		TOPFUNC },
	{ "up",		TABLENAME,	UPMASK },
	{ "uparrow",	CONSTANT,	UPARROW },
	{ "with",	WITH,		0 },
};

#define	NWORDS		(sizeof wordtab / sizeof wordtab[0])

static int
yylex()
{
	register int c;
	char tokbuf[256+1];
	register char *cp;
	register int tokentype;

	while ((c = getc(infile)) == ' ' || c == '\t')
		;
	if (begline) {
		lineno++;
		begline = 0;
		if (c == '#') {
			while ((c = getc(infile)) != EOF && c != '\n')
				;
		}
	}
	if (c == EOF)
		return (0);	/* end marker */
	if (c == '\n') {
		begline = 1;
		return (c);
	}

	switch (c) {

	case '\'':
		tokentype = CHAR;
		if ((c = getc(infile)) == EOF)
			yyerror("unterminated character constant");
		if (c == '\n') {
			(void) ungetc(c, infile);
			yylval.number = '\'';
		} else {
			switch (c) {

			case '\'':
				yyerror("null character constant");
				break;

			case '\\':
				yylval.number = readesc(infile, '\'');
				break;

			default:
				yylval.number = c;
				break;
			}
			if ((c = getc(infile)) == EOF || c == '\n')
				yyerror("unterminated character constant");
			else if (c != '\'')
				yyerror("only one character allowed in character constant");
		}
		break;

	case '"':
		if ((c = getc(infile)) == EOF)
			yyerror("unterminated string constant");
		if (c == '\n') {
			(void) ungetc(c, infile);
			tokentype = CHAR;
			yylval.number = '"';
		} else {
			tokentype = CHARSTRING;
			cp = &tokbuf[0];
			do {
				if (cp > &tokbuf[256])
					yyerror("line too long");
				if (c == '\\')
					c = readesc(infile, '"');
				*cp++ = c;
			} while ((c = getc(infile)) != EOF && c != '\n'
			    && c != '"');
			if (c != '"')
				yyerror("unterminated string constant");
			*cp = '\0';
			if (nstrings == 16)
				yyerror("too many strings");
			if (strlen(tokbuf) > KTAB_STRLEN)
				yyerror("string too long");
			strings[nstrings] = strdup(tokbuf);
			yylval.number = STRING+nstrings;
			nstrings++;
		}
		break;

	case '(':
	case ')':
	case '+':
		tokentype = c;
		break;

	case '^':
		if ((c = getc(infile)) == EOF)
			yyerror("missing newline at end of line");
		tokentype = CHAR;
		if (c == ' ' || c == '\t' || c == '\n') {
			/*
			 * '^' by itself.
			 */
			yylval.number = '^';
		} else {
			yylval.number = c & 037;
			if ((c = getc(infile)) == EOF)
				yyerror("missing newline at end of line");
			if (c != ' ' && c != '\t' && c != '\n')
				yyerror("invalid control character");
		}
		(void) ungetc(c, infile);
		break;

	default:
		cp = &tokbuf[0];
		do {
			if (cp > &tokbuf[256])
				yyerror("line too long");
			*cp++ = c;
		} while ((c = getc(infile)) != EOF
		    && (isalnum(c) || c == '_'));
		if (c == EOF)
			yyerror("newline missing");
		(void) ungetc(c, infile);
		*cp = '\0';
		if (strlen(tokbuf) == 1) {
			tokentype = CHAR;
			yylval.number = (unsigned char)tokbuf[0];
		} else if (strlen(tokbuf) == 2 && tokbuf[0] == '^') {
			tokentype = CHAR;
			yylval.number = (unsigned char)(tokbuf[1] & 037);
		} else {
			word_t word;
			register word_t *wptr;
			char *ptr;

			for (cp = &tokbuf[0]; (c = *cp) != '\0'; cp++) {
				if (isupper(c))
					*cp = tolower(c);
			}
			word.w_string = tokbuf;
			wptr = (word_t *)bsearch((char *)&word,
			    (char *)wordtab, NWORDS, sizeof (word_t),
			    wordcmp);
			if (wptr != NULL) {
				yylval.number = wptr->w_lval;
				tokentype = wptr->w_type;
			} else {
				yylval.number = strtol(tokbuf, &ptr, 10);
				if (ptr == tokbuf)
					yyerror("syntax error");
				else
					tokentype = INT;
			}
			break;
		}
	}

	return (tokentype);
}

static int
readesc(stream, delim)
	FILE *stream;
	int delim;
{
	register int c;
	register int val;
	register int i;

	if ((c = getc(stream)) == EOF || c == '\n')
		yyerror("unterminated character constant");

	if (c >= '0' && c <= '7') {
		val = 0;
		i = 1;
		for (;;) {
			val = val*8 + c - '0';
			if ((c = getc(stream)) == EOF
			    || c == '\n')
				yyerror("unterminated character constant");
			if (c == delim)
				break;
			i++;
			if (i > 3)
				yyerror("escape sequence too long");
			if (c < '0' || c > '7')
				yyerror("illegal character in escape sequence");
		}
		(void) ungetc(c, stream);
	} else {
		switch (c) {

		case 'n':
			val = '\n';
			break;

		case 't':
			val = '\t';
			break;

		case 'b':
			val = '\b';
			break;

		case 'r':
			val = '\r';
			break;

		case 'v':
			val = '\v';
			break;

		case '\\':
			val = '\\';
			break;

		default:
			if (c == delim)
				val = delim;
			else
				yyerror("illegal character in escape sequence");
		}
	}
	return (val);
}

static int
wordcmp(w1, w2)
	word_t *w1, *w2;
{
	return (strcmp(w1->w_string, w2->w_string));
}

static int
yyerror(msg)
	char *msg;
{
	(void) fprintf(stderr, "%s, line %d: %s\n", infilename, lineno, msg);
	exit(1);
}
