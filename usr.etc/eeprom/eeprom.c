#ifndef lint
static  char sccsid[] = "@(#)eeprom.c 1.1 92/07/30";   /* from 4.1.1, eeprom.c 1.17 */
#endif

/*
 * Copyright 1986-1989 by Sun Microsystems, Inc.
 */


#include <stdio.h>
#include <strings.h>
#include <struct.h>
#include <sys/types.h>
#include <sun4/eeprom.h>
#include <sun4/clock.h>

char	*eeprom_dev = "/dev/eeprom";

typedef	void (*func)();

#define	EE_TRUE	0x12

#define	e(n, i, o, off)	{n, i, o, fldoff(eeprom, off)}

void i_byte(), o_byte();
void i_bool(), o_bool();
void i_date(), o_date();
void i_bootdev(), o_bootdev();
void i_console(), o_console();
void i_scrsize(), o_scrsize(); 	
void i_banner(), o_banner();
void i_diagpath(), o_diagpath();
void i_baud(), o_baud();
void i_secure(), o_secure();
void i_bad_log(), o_bad_log();
void i_passwd(), o_passwd();

void o_string();
void eefix_chksum();

struct	eevar {
	char	*name;
	func	in;
	func	out;
	int	offset;
} eevar[] = {
	e("hwupdate",		i_date, o_date,		ee_diag.eed_hwupdate),
	e("memsize",		i_byte, o_byte,		ee_diag.eed_memsize),
	e("memtest",		i_byte, o_byte,		ee_diag.eed_memtest),
	e("scrsize",		i_scrsize, o_scrsize,	ee_diag.eed_scrsize),
	e("watchdog_reboot",	i_bool, o_bool,		ee_diag.eed_dogaction),
	e("default_boot",	i_bool, o_bool,		ee_diag.eed_defboot),
	e("bootdev",		i_bootdev, o_bootdev,	ee_diag.eed_bootdev[0]),
	e("kbdtype",		i_byte, o_byte,		ee_diag.eed_kbdtype),
	e("keyclick",		i_bool, o_bool,		ee_diag.eed_keyclick),
	e("console",		i_console, o_console,	ee_diag.eed_console),
	e("custom_logo",	i_bool, o_bool,		ee_diag.eed_showlogo),
	e("banner",		i_banner, o_banner,	ee_diag.eed_banner[0]),
	e("diagdev",		i_bootdev, o_bootdev,	ee_diag.eed_diagdev[0]),
	e("diagpath",		i_diagpath, o_diagpath,	ee_diag.eed_diagpath[0]),
	e("ttya_no_rtsdtr",	i_bool, o_bool,		ee_diag.eed_ttya_def.eet_rtsdtr),
	e("ttyb_no_rtsdtr",	i_bool, o_bool,		ee_diag.eed_ttyb_def.eet_rtsdtr),
	e("ttya_use_baud",	i_bool, o_bool,		ee_diag.eed_ttya_def.eet_sel_baud),
	e("ttyb_use_baud",	i_bool, o_bool,		ee_diag.eed_ttyb_def.eet_sel_baud),
	e("ttya_baud",		i_baud, o_baud,		ee_diag.eed_ttya_def.eet_hi_baud),
	e("ttyb_baud",		i_baud, o_baud,		ee_diag.eed_ttyb_def.eet_hi_baud),
        e("columns",            i_byte, o_byte,         ee_diag.eed_colsize),
        e("rows",               i_byte, o_byte,         ee_diag.eed_rowsize),
 	e("secure",		i_secure, o_secure,	ee_diag.ee_password_inf),
 	e("bad_login",		i_bad_log, o_bad_log,	ee_diag.ee_password_inf),
 	e("password",		i_passwd, o_passwd,	ee_diag.ee_password_inf),
	{ (char *)NULL, (func)NULL, (func)NULL, 0 }
};

struct	eeprom eeprom;
char	*cmdname;
int	errors = 0;
int	any_sets = 0;		/* any variables set? */

extern	u_char chksum();

extern	*ctime();
extern  long lseek();
extern  time_t getdate();

main(argc, argv)
	int argc;
	char **argv;
{
	register struct eevar *v;
	int ignore_errors = 0;		/* ignore checksum errors? */
	int fix_chksum = 0;		/* fix the checksums? */
	int dostdin = 0;		/* read from stdin? */

	if ((cmdname = rindex(argv[0], '/')) == NULL)
		cmdname = argv[0];
	else
		cmdname++;

	argc--;
	argv++;
	while (argc && **argv == '-') {
		switch (argv[0][1]) {
		case 'c':
			fix_chksum = 1;		/* fix check sum if wrong */
			break;
		case 'i':
			ignore_errors = 1;
			break;
		case 'f':
			eeprom_dev = argv[1];
			argv++;
			argc--;
			break;
		case '\0':
			dostdin++;
			break;
		default:
			fprintf(stderr, "%s: %s: unknown flag\n", 
			    cmdname, *argv);
			exit(1);
		}
		argc--;
		argv++;
	}

	/*
	 * Read in the EEPROM and check that it is valid
	 * (proper write counts, checksums, etc.).
	 */
	eeread(&eeprom);
	if (fix_chksum) {
		if (eecheck(&eeprom, 0))
			eefix_chksum(&eeprom);
		exit(0);
	}
	if (eecheck(&eeprom, ignore_errors))
		exit(1);

	/*
	 * If "-" specified, read variables from stdin.
	 */
	if (dostdin) {
		int c;
		char *nl, line[512];

		while (fgets(line, sizeof line, stdin) != NULL) {
			/* zap newline if present */
			if (nl = index(line, '\n'))
				*nl = 0;
			/* otherwise discard rest of line */
			else
				while ((c = getchar()) != '\n' && c != EOF)
					/* nothing */ ;

			do_var(line);
		}
		if (any_sets)
			eewrite(&eeprom);
		exit(errors);
	}

	/*
	 * If no arguments, dump all fields.
	 */
	if (argc < 1) {
		for (v = eevar; v->name; v++)
			(*v->out)(v->name,
			    (((char *)&eeprom) + v->offset));
		exit(0);
	}

	/*
	 * Process each argument as a variable print or set request.
	 */
	while (argc--)
		do_var(*argv++);
	if (any_sets)
		eewrite(&eeprom);
	exit(errors);
	/* NOTREACHED */
}

/*
 * Print or set an EEPROM field.
 */
do_var(var)
	char *var;
{
	register char *p;
	register struct eevar *v;

	p = index(var, '=');
	if (p != NULL)
		*p++ = '\0';
	for (v = eevar; v->name; v++)
		if (strcmp(var, v->name) == 0)
			break;
	if (!v->name) {
		fprintf(stderr, "%s: %s: no such field\n",
		    cmdname, var);
		errors++;
	} else {
		if (p != NULL) {
			(*v->in)(p, (((char *)&eeprom) + v->offset));
			any_sets = 1;
		} else {
			(*v->out)(v->name, (((char *)&eeprom) + v->offset));
		}
	}
}

/*
 * Read the EEPROM into *ep.
 */
eeread(ep)
	struct eeprom *ep;
{
	int fd;

	if ((fd = open(eeprom_dev, 0)) < 0) {
		fprintf(stderr, "%s: ", cmdname);
		perror(eeprom_dev);
		exit(1);
	}
	if (read(fd, (char *)ep, sizeof (*ep)) != sizeof (*ep)) {
		fprintf(stderr, "%s: ", cmdname);
		perror(eeprom_dev);
		exit(1);
	}
	close(fd);
}

/*
 * Write out the new EEPROM *ep if there were any changes.
 * Update the checksums and write counts.
 */
eewrite(ep)
	struct eeprom *ep;
{
	int fd;
	struct eeprom eeorig;
	register char *op, *np;		/* old and new update dates */
	int written = 0;

	if ((fd = open(eeprom_dev, 2)) < 0) {
		fprintf(stderr, "%s: ", cmdname);
		perror(eeprom_dev);
		exit(1);
	}
	if (read(fd, (char *)&eeorig, sizeof (eeorig)) != sizeof (eeorig)) {
		fprintf(stderr, "%s: ", cmdname);
		perror(eeprom_dev);
		exit(1);
	}
	op = (char *)&eeorig.ee_diag.eed_hwupdate;
	np = (char *)&ep->ee_diag.eed_hwupdate;

	/*
	 * Write diagnostic section.
	 */
	while (op < (char *)&eeorig.ee_resv) {
		if (*np != *op) {
			lseek(fd, (long) (np - (char *)ep), 0);
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

		cp = &ep->ee_diag.eed_chksum[0];
		cp[0] = cp[1] = cp[2] =
		    chksum((u_char *)&ep->ee_diag.eed_hwupdate,
			sizeof (ep->ee_diag) -
			((char *)(&ep->ee_diag.eed_hwupdate) -
			    (char *)(&ep->ee_diag)));
		lseek(fd, (long) ((char *)cp - (char *)ep), 0);
		write(fd, (char *)cp, 3 * sizeof (*cp));
		wp = &ep->ee_diag.eed_wrcnt[0];
		wp[0] = wp[1] = wp[2] = wp[0] + 1;
		lseek(fd, (long) ((char *)wp - (char *)ep), 0);
		write(fd, (char *)wp, 3 * sizeof (*wp));
	}
	close(fd);
}

/*
 * Check the write counts and checksums in *ep.
 * If ign is true, ignore errors.
 * Return 0 on success, 1 if write count or checksum is wrong.
 */
eecheck(ep, ign)
	register struct eeprom *ep;
	int ign;
{
	register u_short *wp;
	register u_char *cp;

	/* check diagnostic area */
	wp = ep->ee_diag.eed_wrcnt;
	if (wp[0] != wp[1] || wp[0] != wp[2]) {
		fprintf(stderr, "%s: diagnostic area write count mismatch\n",
		    cmdname);
		if (!ign)
			return (1);
	}
	cp = ep->ee_diag.eed_chksum;
	if (cp[0] != cp[1] || cp[0] != cp[2]) {
		fprintf(stderr, "%s: diagnostic area checksum mismatch\n",
		    cmdname);
		if (!ign)
			return (1);
	}
	if (chksum((u_char *)&ep->ee_diag.eed_hwupdate, sizeof (ep->ee_diag) -
	    ((char *)(&ep->ee_diag.eed_hwupdate) - (char *)(&ep->ee_diag))) !=
	    cp[0]) {
		fprintf(stderr, "%s: diagnostic area checksum wrong\n",
		    cmdname);
		if (!ign)
			return (1);
	}

	/* check reserved area */
	wp = ep->ee_resv.eev_wrcnt;
	if (wp[0] != wp[1] || wp[0] != wp[2]) {
		fprintf(stderr, "%s: reserved area write count mismatch\n",
		    cmdname);
		if (!ign)
			return (1);
	}
	cp = ep->ee_resv.eev_chksum;
	if (cp[0] != cp[1] || cp[0] != cp[2]) {
		fprintf(stderr, "%s: reserved area checksum mismatch\n",
		     cmdname);
		if (!ign)
			return (1);
	}
	if (chksum((u_char *)&ep->ee_resv.eev_resv[0], sizeof (ep->ee_resv) -
	    ((char *)(&ep->ee_resv.eev_resv[0]) - (char *)(&ep->ee_resv))) !=
	    cp[0]) {
		fprintf(stderr, "%s: reserved area checksum wrong\n", cmdname);
		if (!ign)
			return (1);
	}

	/* check rom area */
	wp = ep->ee_rom.eer_wrcnt;
	if (wp[0] != wp[1] || wp[0] != wp[2]) {
		fprintf(stderr, "%s: rom area write count mismatch\n",
		    cmdname);
		if (!ign)
			return (1);
	}
	cp = ep->ee_rom.eer_chksum;
	if (cp[0] != cp[1] || cp[0] != cp[2]) {
		fprintf(stderr, "%s: rom area checksum mismatch\n", cmdname);
		if (!ign)
			return (1);
	}
	if (chksum((u_char *)&ep->ee_rom.eer_resv[0], sizeof (ep->ee_rom) -
	    ((char *)(&ep->ee_rom.eer_resv[0]) - (char *)(&ep->ee_rom))) !=
	    cp[0]) {
		fprintf(stderr, "%s: rom area checksum wrong\n", cmdname);
		if (!ign)
			return (1);
	}

	/* check software area */
	wp = ep->ee_soft.ees_wrcnt;
	if (wp[0] != wp[1] || wp[0] != wp[2]) {
		fprintf(stderr, "%s: software area write count mismatch\n",
		    cmdname);
		if (!ign)
			return (1);
	}
	cp = ep->ee_soft.ees_chksum;
	if (cp[0] != cp[1] || cp[0] != cp[2]) {
		fprintf(stderr, "%s: software area checksum mismatch\n",
		    cmdname);
		if (!ign)
			return (1);
	}
	/*
	 * Can't do checksum if the TOD clock writes into ee_soft.ees_resv[].
	 * Currently, in sun4's,
	 *
	 *	CLOCK1_ADDR == MDEVBASE+0x27F8,  while
	 *
	 * 	EEPROM_ADDR+EEPROM_SIZE == MDEVBASE+0x2800
	 *
	 * Thus, the TOD clock was overwriting the last 8 bytes of
	 * ee_soft.ees_resv[], corrupting the checksum calculation.
	 */

#if (CLOCK1_ADDR > (EEPROM_ADDR + EEPROM_SIZE))
	if (chksum((u_char *)&ep->ee_soft.ees_resv[0], sizeof (ep->ee_soft) -
	    ((char *)(&ep->ee_soft.ees_resv[0]) - (char *)(&ep->ee_soft))) !=
	    cp[0]) {
		fprintf(stderr, "%s: software area checksum wrong\n", cmdname);
		if (!ign)
			return (1);
	}
#endif
	return (0);
}

/*
 * Fix up the checksums if they are wrong.
 */
void
eefix_chksum(ep)
	register struct eeprom *ep;
{
	register u_short *wp;
	register u_char *cp;
	register u_char calc_sum;		/* calculated checksum */
	int fd;

	if ((fd = open(eeprom_dev, 2)) < 0) {
		fprintf(stderr, "%s: ", cmdname);
		perror(eeprom_dev);
		exit(1);
	}

	/* check diag area */
	cp = ep->ee_diag.eed_chksum;
	calc_sum =  chksum((u_char *)&ep->ee_diag.eed_hwupdate, 
	    sizeof (ep->ee_diag) -
	    ((char *)(&ep->ee_diag.eed_hwupdate) - (char *)(&ep->ee_diag)));
	if (calc_sum != cp[0] || calc_sum != cp[1] || calc_sum != cp[2]) {
		/* write out the correct checksum */
		cp[0] = cp[1] = cp[2] = calc_sum;
		lseek(fd, (long)((char *)cp - (char *)ep), 0);
		write(fd, (char *)cp, 3 * sizeof (*cp));
		wp = &ep->ee_diag.eed_wrcnt[0];
		wp[0] = wp[1] = wp[2] = wp[0] + 1;	/* inc write cnt */
		lseek(fd, (long)((char *)wp - (char *)ep), 0);
		write(fd, (char *)wp, 3 * sizeof (*wp));
	}

	/* check reserved area */
	cp = ep->ee_resv.eev_chksum;
	calc_sum = chksum((u_char *)&ep->ee_resv.eev_resv[0], 
	    sizeof (ep->ee_resv) -
	    ((char *)(&ep->ee_resv.eev_resv[0]) - (char *)(&ep->ee_resv)));
	if (calc_sum != cp[0] || calc_sum != cp[1] || calc_sum != cp[2]) {
		/* write out the correct checksum */
		cp[0] = cp[1] = cp[2] = calc_sum;
		lseek(fd, (long)((char *)cp - (char *)ep), 0);
		write(fd, (char *)cp, 3 * sizeof (*cp));
		wp = &ep->ee_diag.eed_wrcnt[0];
		wp[0] = wp[1] = wp[2] = wp[0] + 1;	/* inc write cnt */
		lseek(fd, (long)((char *)wp - (char *)ep), 0);
		write(fd, (char *)wp, 3 * sizeof (*wp));
	}

	/* check rom area */
	cp = ep->ee_rom.eer_chksum;
	calc_sum = chksum((u_char *)&ep->ee_rom.eer_resv[0], 
	    sizeof (ep->ee_rom) -
	    ((char *)(&ep->ee_rom.eer_resv[0]) - (char *)(&ep->ee_rom)));
	if (calc_sum != cp[0] || calc_sum != cp[1] || calc_sum != cp[2]) {
		/* write out the correct checksum */
		cp[0] = cp[1] = cp[2] = calc_sum;
		lseek(fd, (long)((char *)cp - (char *)ep), 0);
		write(fd, (char *)cp, 3 * sizeof (*cp));
		wp = &ep->ee_diag.eed_wrcnt[0];
		wp[0] = wp[1] = wp[2] = wp[0] + 1;	/* inc write cnt */
		lseek(fd, (long)((char *)wp - (char *)ep), 0);
		write(fd, (char *)wp, 3 * sizeof (*wp));
	}

	/* check software area */
	cp = ep->ee_soft.ees_chksum;
	calc_sum = chksum((u_char *)&ep->ee_soft.ees_resv[0],
	    sizeof (ep->ee_soft) -
	    ((char *)(&ep->ee_soft.ees_resv[0]) - (char *)(&ep->ee_soft)));
	if (calc_sum != cp[0] || calc_sum != cp[1] || calc_sum != cp[2]) {
		/* write out the correct checksum */
		cp[0] = cp[1] = cp[2] = calc_sum;
		lseek(fd, (long)((char *)cp - (char *)ep), 0);
		write(fd, (char *)cp, 3 * sizeof (*cp));
		wp = &ep->ee_diag.eed_wrcnt[0];
		wp[0] = wp[1] = wp[2] = wp[0] + 1;	/* inc write cnt */
		lseek(fd, (long)((char *)wp - (char *)ep), 0);
		write(fd, (char *)wp, 3 * sizeof (*wp));
	}
	close(fd);
}

u_char
chksum(p, n)
	register u_char *p;
	register int n;
{
	register u_char s = 0;

	while (n--)
		s += *p++;
	return (256 - s);
}


void
i_byte(s, p)
	char *s, *p;
{
	int val;

	(void) sscanf(s, "%d", &val);
	*(u_char *)p = val;
}

void
o_byte(name, addr)
	char *name, *addr;
{

	printf("%s=%lu\n", name, *(u_char *)addr);
}

void
i_banner(s, p)
	char *s, *p;
{

	strncpy(p, s, sizeof (eeprom.ee_diag.eed_banner));
}

void
o_banner(name, addr)
	char *name, *addr;
{

	o_string(name, addr, sizeof (eeprom.ee_diag.eed_banner));
}

void
i_diagpath(s, p)
	char *s, *p;
{

	strncpy(p, s, sizeof (eeprom.ee_diag.eed_diagpath));
}

void
o_diagpath(name, addr)
	char *name, *addr;
{

	o_string(name, addr, sizeof (eeprom.ee_diag.eed_diagpath));
}

void
o_string(name, addr, len)
	char *name, *addr;
	int len;
{

	printf("%s=%-.*s\n", name, len, (addr[0] == '\377' ? "" : addr));
}

void
i_bool(s, p)
	char *s, *p;
{
	int val;

	val = strcmp(s, "true") == 0;
	*(u_char *)p = val ? EE_TRUE : 0;
}

void
o_bool(name, addr)
	char *name, *addr;
{

	printf("%s=%s\n", name, *addr == EE_TRUE ? "true" : "false");
}

void
i_date(s, p)
	char *s, *p;
{
	int val;

	val = getdate(s, (time_t *)NULL);
	if (val < 0) {
		fprintf(stderr, "%s: '%s': bad date\n", cmdname, s);
		return;
	}

	*(u_int *)p = val;
}

void
o_date(name, addr)
	char *name, *addr;
{

	printf("%s=%s", name, ctime((time_t *)addr));
}

void
i_bootdev(s, p)
	char *s, *p;
{
	char c1, c2;
	int ctlr, unit, part;

	if (sscanf(s, "%c%c(%x,%x,%x)", &c1, &c2, &ctlr, &unit, &part) != 5 &&
	    sscanf(s, "%c%c(0x%x,%x,%x)", &c1, &c2, &ctlr, &unit, &part) != 5) {
		fprintf(stderr, "%s: %s: bad boot device\n", cmdname, s);
		return;
	}
	p[0] = c1;
	p[1] = c2;
	p[2] = ctlr;
	p[3] = unit;
	p[4] = part;
}

void
o_bootdev(name, addr)
	char *name, *addr;
{

	/*
	 * Note that we are passed the starting address here, and rely on
	 * knowledge of the adjacency of the fields.
	 */
	printf("%s=%c%c(%x,%x,%x)\n", name,
	    (addr[0] == '\377' ? '?' : addr[0]), 
	    (addr[1] == '\377' ? '?' : addr[1]), 
	    addr[2], addr[3], addr[4]);
}

void
i_console(s, p)
	char *s, *p;
{
	u_char val;

	if (strcmp(s, "b&w") == 0)
		val = EED_CONS_BW;
	else if (strcmp(s, "ttya") == 0)
		val = EED_CONS_TTYA;
	else if (strcmp(s, "ttyb") == 0)
		val = EED_CONS_TTYB;
	else if (strcmp(s, "color") == 0)
		val = EED_CONS_COLOR;
	else {
		fprintf(stderr, "%s: %s: unknown console type\n", cmdname, s);
		return;
	}
	*(u_char *)p = val;
}

void
o_console(name, addr)
	char *name, *addr;
{
	char *val;

	switch (*addr) {
	case EED_CONS_TTYA:
		val = "ttya";
		break;
	case EED_CONS_TTYB:
		val = "ttyb";
		break;
	case EED_CONS_COLOR:
		val = "color";
		break;
	case EED_CONS_BW:
	default:
		val = "b&w";
		break;
	}
	printf("%s=%s\n", name, val);
}

void
i_scrsize(s, p)
	char *s, *p;
{
	u_char val;

	if (strcmp(s, "1024x1024") == 0)
		val = EED_SCR_1024X1024;
	else if (strcmp(s, "1600x1280") == 0)
		val = EED_SCR_1600X1280;
	else if (strcmp(s, "1440x1440") == 0)
		val = EED_SCR_1440X1440;
	else if (strcmp(s, "1280x1024") == 0)
		val = EED_SCR_1280X1024;
	else
		val = EED_SCR_1152X900;
	*(u_char *)p = val;
}

void
o_scrsize(name, addr)
	char *name, *addr;
{
	char *s;

	switch (*addr) {
	case EED_SCR_1024X1024:
		s = "1024x1024";
		break;
	case EED_SCR_1600X1280:
		s = "1600x1280";
		break;
	case EED_SCR_1440X1440:
		s = "1440x1440";
		break;
	case EED_SCR_1280X1024:
		s = "1280x1024";
		break;
	default:
		s = "1152x900";
		break;
	}
	printf("%s=%s\n", name, s);
}

void
i_baud(s, p)
	char *s, *p;
{
	int val;

	(void) sscanf(s, "%d", &val);
	*((u_char *)p)++ = (val >> 8) & 0xff;
	*(u_char *)p = val & 0xff;
}

void
o_baud(name, addr)
	char *name, *addr;
{
	int val = 0;

	val = *((u_char *)addr)++ << 8;
	val += *(u_char *)addr;
	printf("%s=%lu\n", name, val);
}

void
i_secure(s, p)
	char *s;
	struct password_inf *p;
{

	if ((strcmp(s, "full") == 0) || (strcmp(s, "command") == 0)) {
		if (*p->pw_bytes == '\0') {
			/* no password yet, get one */
			if (get_password(p->pw_bytes)) {
				p->pw_mode = (strcmp(s, "full") == 0) ? 
			    			FULLY_SECURE_MODE : COMMAND_SECURE_MODE;
				return;
			} else
				exit(1);
		} else {
			p->pw_mode = (strcmp(s, "full") == 0) ? 
					FULLY_SECURE_MODE : COMMAND_SECURE_MODE;
			return;
		}
	}
	if (strcmp(s, "none") == 0) {
		(void) bzero(p->pw_bytes, PW_SIZE);
	p->pw_mode = 0;
		return;
	}
	(void) printf("Invalid security mode, mode unchanged.\n");
	exit(1);
}

void
o_secure(name, addr)
	char *name;
	struct password_inf *addr;
{

	switch (addr->pw_mode) {
	    case COMMAND_SECURE_MODE :
		printf("%s=command\n", name);
		break;
	    case FULLY_SECURE_MODE :
		printf("%s=full\n", name);
		break;
	    default:
		printf("%s=none\n", name);
		break;
	}
}

void
i_bad_log(s, p)
	char *s;
	struct password_inf *p;
{

	if (strcmp(s, "reset") == 0)
		p->bad_counter = 0;
}

void
o_bad_log(name, addr)
	char *name;
	struct password_inf *addr;
{

	printf("PROM %s=%d\n", name, addr->bad_counter);
}

void
i_passwd(s, p)
	char *s;
	struct password_inf *p;
{

	if (p->pw_mode != COMMAND_SECURE_MODE && p->pw_mode != FULLY_SECURE_MODE) {
		printf("Not in secure mode\n");
		exit(1);
	}

	if (get_password(p->pw_bytes) == 0)
		exit(1);
}

void
o_passwd(name, addr)
	char *name;
	struct password_inf *addr;
{

}

get_password(pw_dest)
	char *pw_dest;
{
	int i, insist = 0, ok, flags;
	int c, pwlen;
	char *p;
	char pwbuf[10];

tryagain:
	(void) printf("Changing PROM password:\n");
	(void) strcpy(pwbuf, getpass("New password:"));
	pwlen = strlen(pwbuf);
	if (pwlen == 0) {
		(void) printf("Password unchanged.\n");
		return(0);
	}
	/*
	 * Insure password is of reasonable length and
	 * composition.  If we really wanted to make things
	 * sticky, we could check the dictionary for common
	 * words, but then things would really be slow.
	 */
	ok = 0;
	flags = 0;
	p = pwbuf;
	while (c = *p++) {
		if (c >= 'a' && c <= 'z')
			flags |= 2;
		else if (c >= 'A' && c <= 'Z')
			flags |= 4;
		else if (c >= '0' && c <= '9')
			flags |= 1;
		else
			flags |= 8;
	}
	if (flags >= 7 && pwlen >= 4)
		ok = 1;
	if ((flags == 2 || flags == 4) && pwlen >= 6)
		ok = 1;
	if ((flags == 3 || flags == 5 || flags == 6) && pwlen >= 5)
		ok = 1;
	if (!ok && insist < 2) {
	(void) printf("Please use %s.\n", flags == 1 ?
			"at least one non-numeric character" :
			"a longer password");
		insist++;
		goto tryagain;
	}
	if (strcmp(pwbuf, getpass("Retype new password:")) != 0) {
		(void) printf("Mismatch - password unchanged.\n");
		return(0);
	}
	(void) strncpy(pw_dest, pwbuf, PW_SIZE);
	return(1);
}
