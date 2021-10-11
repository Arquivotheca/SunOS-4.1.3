#ifndef lint
static	char sccsid[] = "@(#)mt.c 1.1 92/07/30 SMI"; /* from UCB 4.8 83/05/08 */
#endif lint
/*
 * mt -- magnetic tape manipulation program
 */
#ifdef vax
#include <vaxmba/mtreg.h>
#include <vaxmba/htreg.h>

#include <vaxuba/utreg.h>
#include <vaxuba/tmreg.h>
#undef b_repcnt         /* argh */
#include <vaxuba/tsreg.h>
#endif vax

#include <stdio.h>
#include <ctype.h>

#include <sys/errno.h>
#include <sys/types.h>
#include <sys/mtio.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/uio.h>

#ifdef sun
#include <sun/dklabel.h>
#include <sun/dkio.h>

#include <sundev/screg.h>
#include <sundev/sireg.h>
#include <sundev/scsi.h>
#include <sundev/tmreg.h>
#include <sundev/xtreg.h>
#include <sundev/arreg.h>
#endif sun

#define	equal(s1,s2)	(strcmp(s1, s2) == 0)
#define	MTASF		100	/* absolute file positioning; first file is 0 */


int mtfd;
struct mtop mt_com;
struct mtget mt_status;
static char *print_key();
char *tape;
extern int errno;

struct commands {
	char *c_name;
	int c_code;
	int c_ronly;
	int c_usecnt;
} com[] = {
	{ "weof",	MTWEOF,		0,	1 },
	{ "eof",	MTWEOF,		0,	1 },
	{ "fsf",	MTFSF,		1,	1 },
	{ "bsf",	MTBSF,		1,	1 },
	{ "asf",	MTASF,		1,	1 },
	{ "fsr",	MTFSR,		1,	1 },
	{ "bsr",	MTBSR,		1,	1 },
	{ "rewind",	MTREW,		1,	0 },
	{ "offline",	MTOFFL,		1,	0 },
	{ "rewoffl",	MTOFFL,		1,	0 },
	{ "status",	MTNOP,		1,	0 },
	{ "retension",	MTRETEN,	1,	0 },
	{ "erase",	MTERASE,	0,	0 },
	{ "eom",	MTEOM,		1,	0 },
	{ "nbsf",	MTNBSF,		1,	1 },
	{ 0 }
};

#ifdef sun
struct mt_tape_info tapes[] = MT_TAPE_INFO;
#endif sun
#ifdef vax
struct mt_tape_info tapes[] = 
{
{ MT_ISTS,		"ts11",		0,		TSXS0_BITS },
{ MT_ISHT,		"tm03",		HTDS_BITS,	HTER_BITS },
{ MT_ISTM,		"tm11",		0,		TMER_BITS },
{ MT_ISMT,		"tu78",		MTDS_BITS,	0 },
{ MT_ISUT,		"tu45",		UTDS_BITS,	UTER_BITS },
{ 0 }
}
#endif vax


main(argc, argv)
	char **argv;
{
	register char *cp;
	register struct commands *comp;
	char *getenv();

	if (argc > 2 && (equal(argv[1], "-t") || equal(argv[1], "-f"))) {
		argc -= 2;
		tape = argv[2];
		argv += 2;
	} else
		if ((tape = getenv("TAPE")) == NULL)
			tape = DEFTAPE;
	if (argc < 2) {
		fprintf(stderr, "usage: mt [ -f device ] command [ count ]\n");
		exit(1);
	}
	cp = argv[1];
	for (comp = com; comp->c_name != NULL; comp++)
		if (strncmp(cp, comp->c_name, strlen(cp)) == 0)
			break;
	if (comp->c_name == NULL) {
		fprintf(stderr, "mt: unknown command: %s\n", cp);
		exit(1);
	}
	if ((mtfd = open(tape, comp->c_ronly ? 0 : 2)) < 0) {
		/*
		 * Provide additional error message decoding since
		 * we need additional error codes to fix them problem.
		 */
		if (errno == EIO) {
			fprintf(stderr, "%s: no tape loaded or drive offline\n",
				tape);
		} else if (errno == EACCES) {
			fprintf(stderr, "%s: write protected\n", tape);
		} else {
			perror(tape);
		}
		exit(1);
	}
	/*
	 * Handle absolute file positioning.  Ask tape driver
	 * where tape is and then skip to desired file.  If
	 * driver doesn't support get location ioctl, rewind
	 * the tape and then space to the desired file.
	 */
	if (comp->c_code == MTASF) {
		int usecnt;
		int mt_fileno;
		struct mtget mt_status;

		usecnt = argc > 2  &&  comp->c_usecnt;
		mt_fileno = usecnt ? atoi(argv[2]) : 1;
		if (mt_fileno < 0) {
			fprintf(stderr, "mt: negative file number\n");
			exit(1);
		}
		(void) ioctl(mtfd, MTIOCGET, (char *)&mt_status);
		if (ioctl(mtfd, MTIOCGET, (char *)&mt_status) < 0) {
			perror("mt");
			exit(2);
		}
		/*
		 * Check if device supports reporting current file
		 * tape file position.  If not, rewind the tape, and
		 * space forward.
		 */
		if (!(mt_status.mt_flags & MTF_ASF)) {
			/*printf("mt: rewind\n");*/
			mt_status.mt_fileno = 0;
			mt_com.mt_count = 1;
			mt_com.mt_op = MTREW;
			if (ioctl(mtfd, MTIOCTOP, &mt_com) < 0) {
				fprintf(stderr, "%s %s %d ",
					tape, comp->c_name, mt_fileno);
				perror("mt");
				exit(2);
			}
		}
		if (mt_fileno < mt_status.mt_fileno) {
			mt_com.mt_op = MTNBSF;
			mt_com.mt_count =  mt_status.mt_fileno - mt_fileno;
			/*printf("mt: bsf= %d\n", mt_com.mt_count);*/
		} else {
			mt_com.mt_op = MTFSF;
			mt_com.mt_count =  mt_fileno - mt_status.mt_fileno;
			/*printf("mt: fsf= %d\n", mt_com.mt_count);*/
		}
		if (ioctl(mtfd, MTIOCTOP, &mt_com) < 0) {
			fprintf(stderr, "%s %s %d ", tape, comp->c_name,
				mt_fileno);
			perror("failed");
			exit(2);
		}

	/* Handle regular mag tape ioctls */
	} else if (comp->c_code != MTNOP) {
		int usecnt;

		mt_com.mt_op = comp->c_code;
		usecnt = argc > 2 && comp->c_usecnt;
		mt_com.mt_count = (usecnt ? atoi(argv[2]) : 1);
		if (mt_com.mt_count < 0) {
			fprintf(stderr, "mt: negative repeat count\n");
			exit(1);
		}
		if (ioctl(mtfd, MTIOCTOP, &mt_com) < 0) {
			fprintf(stderr, "%s %s %d ", tape, comp->c_name,
				mt_com.mt_count);
			perror("failed");
			exit(2);
		}

	/* Handle status ioctl */
	} else {
		if (ioctl(mtfd, MTIOCGET, (char *)&mt_status) < 0) {
			perror("mt");
			exit(2);
		}
		status(&mt_status);
	}
	exit (0);
	/* NOTREACHED */
}


/*
 * Interpret the status buffer returned
 */
status(bp)
	register struct mtget *bp;
{
	register struct mt_tape_info *mt;

	for (mt = tapes; mt->t_type; mt++)
		if (mt->t_type == bp->mt_type)
			break;

	/* Handle SCSI tape drives specially. */
	if ((bp->mt_flags & MTF_SCSI)) {

		if (mt->t_type == bp->mt_type)
			printf("%s tape drive:\n", mt->t_name);
		else
			printf("%s tape drive:\n", "SCSI");

		printf("   sense key(0x%x)= %s   residual= %d   ",
			bp->mt_erreg, print_key(bp->mt_erreg), bp->mt_resid);
		printf("retries= %d\n", bp->mt_dsreg);
		printf("   file no= %d   block no= %d\n",
			bp->mt_fileno, bp->mt_blkno);

	/* Handle other drives here. */
	} else {
		if (mt->t_type == 0) {
			printf("unknown tape drive type (0x%x)\n", bp->mt_type);
			return;
		}
		printf("%s tape drive:\n   residual= %d",
			mt->t_name, bp->mt_resid);
		printreg("   ds", (u_short)bp->mt_dsreg, mt->t_dsbits);
		printreg("   er", (u_short)bp->mt_erreg, mt->t_erbits);
		putchar('\n');
	}
}


/* Define SCSI sense key error messages. */
static char *key_error_str[] = SENSE_KEY_INFO;
#define MAX_KEY_ERROR_STR \
	(sizeof(key_error_str)/sizeof(key_error_str[0]))


/*
 * Return the text string associated with the sense key value.
 */
static char *
print_key(key_code)
	register short key_code;
{
	static char *unknown_key = "unknown key";
	if ((key_code > MAX_KEY_ERROR_STR -1)  ||
	    key_error_str[key_code] == NULL) {

		return (unknown_key);
	}
	return (key_error_str[key_code]);
}


/*
 * Print a register a la the %b format of the kernel's printf
 */
printreg(s, v, bits)
	char *s;
	register u_short v;
	register char *bits;
{
	register int i, any = 0;
	register char c;

	if (bits && *bits == 8)
		printf("%s= %o", s, v);
	else
		printf("%s= %x", s, v);
	bits++;
	if (v && bits) {
		putchar('<');
		while (i = *bits++) {
			if (v & (1 << (i-1))) {
				if (any)
					putchar(',');
				any = 1;
				for (; (c = *bits) > 32; bits++)
					putchar(c);
			} else
				for (; *bits > 32; bits++)
					;
		}
		putchar('>');
	}
}
