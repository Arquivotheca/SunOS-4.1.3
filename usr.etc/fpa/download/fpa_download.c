#ifndef lint
static	char sccsid[] = "@(#)fpa_download.c 1.1 92/07/30 SMI";
#endif

/* Copyright (c) 1988 by Sun Microsystems, Inc. */

/* fpa_download.c: download to FPA or FPA+.
 *
 * Run manually or from /etc/rc.local when /dev/fpa exists.
 */

char *Usage =
"	arguments		comments\n\
	---------		--------\n\
	-u ufile		download microcode file\n\
	-m mfile		download map ram file (FPA only)\n\
	-c cfile		download constants file\n\
	-d			download all and enable default files names\n\
	-r			print ucode revision\n\
	-v			verbose\n\
\n\
Must be run as root because ioctls for /dev/fpa are root only.";

#include <time.h>
#include <signal.h>
#include <stdio.h>
#include <sys/file.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sundev/fpareg.h>
#include <string.h>

#define MAP_RAM_LINES 4096
#define CONST_RAM_LINES 2048
#define MAP_MASK 0xffffff
#define BYTE_MASK 0xff
#define FILLER 0x31415926
#define REG_RD_MSW 0xE0000C00
#define REG_RD_LSW 0xE0000C04
#define REG_CPY_DP 0xE0000884
	/* load pointer register RAM operations */
#define LDP_MS 0xE0000E80
#define LDP_LS 0xE0000E84
#define REG_CPY_SP 0xE0000880
#define REG_WRM_LP 0xE80
#define REG_WRL_LP 0xE84

#define MAP_CKSUM_ADDR 4*1024-1
#define REG_CKSUM_ADDR 2*1024-1
#define REG_CKSUM_MAX 0x7CF
#define CHECKSUM 0xBEADFACE

char *error_message[]={	"FPA ioctl failed",
			"FPA Bus Error",
			"Upload mismatch" };
#define IOCTL_FAILED	0
#define BUS_ERROR	1
#define UPLOAD_ERROR	2

enum fpa_type {FPA_ORIG=0, FPA_PLUS=2} fpa_type;

#define ORIG_BIN_FILE	"/usr/etc/fpa/fpa_micro_bin"
#define ORIG_MAP_FILE	"/usr/etc/fpa/fpa_micro_map"
#define ORIG_CONST_FILE	"/usr/etc/fpa/fpa_constants"
#define PLUS_BIN_FILE	"/usr/etc/fpa/fpa_micro_bin+"
#define PLUS_CONST_FILE	"/usr/etc/fpa/fpa_constants+"

extern int errno;
char	*progname;
int	errors = 0;

struct fpa_device *fpa = (struct fpa_device *) 0xE0000000;

int fpa_fd;		/* /dev/fpa */
int verbose = 0;	/* default is quiet */

	/* func types */
u_int	cksum();
void	broadcast_msg();
void	got_fpeerr();
void	got_segverr();
void	got_buserr();
void	error();
char	*calloc();

main(argc, argv)
int argc;
char *argv[];
{
char *ufile = NULL;
char *mfile = NULL;
char *cfile = NULL;
int uflg = 0;		/* download microcode */
int mflg = 0;		/* download map */
int cflg = 0;		/* download constants */
int dflg = 0;		/* download all, use default files if unspecified */
int rflg = 0;		/* print revision */

int i;

	(void)signal(SIGFPE, got_fpeerr);
	(void)signal(SIGSEGV, got_segverr);
	progname = *argv;
	for(i = 1; i < argc; i++)
	{
		if(argv[i][0] != '-')
			goto usage;
		else
			switch(argv[i][1])
			{
			case 'u':
				uflg++;
				ufile = argv[++i];
				break;
			case 'm':
				mflg++;
				mfile = argv[++i];
				break;
			case 'c':
				cflg++;
				cfile = argv[++i];
				break;
			case 'r':
				rflg++;
				break;
			case 'd':
				dflg++;		/* enable defaults files */
				mflg++;		/* download all files */
				uflg++;
				cflg++;
				break;
			case 'q':
				verbose = 0;
				break;
			case 'v':
				verbose = 1;
				break;
			default:
				(void)fprintf(stderr, "Bad flag: %c\n", argv[i][1]);
				goto usage;
			}
	}
	if((fpa_fd = open("/dev/fpa", O_RDWR, 0)) == -1)
	{
		switch(errno)
		{
		case ENOENT:
			error("can't open /dev/fpa: No 68881 present\n");
			break;
		case ENETDOWN:
			error("can't open /dev/fpa: FPA disabled due to probable hardware problems\n");
			break;
		case EIO:
			error("can't open /dev/fpa: other FPA process active\n");
			break;
		case EBUSY:
			error("can't open /dev/fpa: kernel out of FPA contexts\n");
			break;
		default:
			perror("can't open /dev/fpa");
			break;
		}
		exit(-1);
	}

	/* Determine type of FPA installed */
	fpa_type = (enum fpa_type) (FPA_VERSION & fpa->fp_imask);

	if(!(uflg + mflg + cflg + rflg))
		(void)printf("%s\n", fpa_type==FPA_PLUS ? "FPA+": "FPA");

	if(fpa_type == FPA_ORIG)
	{
		if(dflg)
		{
			if(uflg && (ufile == NULL))
				ufile = ORIG_BIN_FILE;
			if(mflg && (mfile == NULL))
				mfile = ORIG_MAP_FILE;
			if(cflg && (cfile == NULL))
				cfile = ORIG_CONST_FILE;
		}
	}
	else if(fpa_type == FPA_PLUS)
	{
		if(dflg)
		{
			if(uflg && (ufile == NULL))
				ufile = PLUS_BIN_FILE;
			if(cflg && (cfile == NULL))
				cfile = PLUS_CONST_FILE;
		}
	}
	else
	{
		error("ERROR: FPA type 0x%x unknown.\n", fpa_type);
	}

	if(verbose && (uflg + mflg + cflg))
	{
		(void)fprintf(stderr,"%8s\n\t-u %s\n\t-m %s\n\t-c %s\n",
			progname, ufile, mfile, cfile);
	}

	if(uflg)
		if(fpa_uload(ufile)) fpa_shutdown(UPLOAD_ERROR);
	if(mflg && (fpa_type == FPA_ORIG))
		if(fpa_mload(mfile)) fpa_shutdown(UPLOAD_ERROR);
	if(cflg)
		if(fpa_cload(cfile)) fpa_shutdown(UPLOAD_ERROR);
	if(rflg)
		fpa_rev();
	(void)close(fpa_fd);
	exit(0);
usage:
	(void)fprintf(stderr,"Usage: %s [-d] [-u ufile] [-m mfile] [-c cfile] [-r] [-v]\n", progname);
	error("%s\n",Usage);
}

/*		Download Micro-code To FPA[PLUS]
 *  filename must be of the following format:
 *
 * FPA_ORIG:
 *    an 16-bit short specifying no. of lines, nlines
 *    n lines of 96 bits specifying the bits in the ucode 
 * FPA_PLUS:
 *    an 16-bit short specifying no. of lines, nlines
 *    nlines of 64 bits specifying the bits in the ucode 
 *
 * A checksum will be appended at the end of good data.
 *
 */

fpa_uload(filename)
	char *filename;
{
FILE *fp;
u_short nlines;		/* no. of lines of instructions */
register u_int *ptr;
u_int ucode_add;

u_int *ucode = NULL;
u_int *v_ucode = NULL;
u_int ulines;		/* # of ucode lines */
u_int uline_size;	/* u_ints per uline */
int i;
enum fpa_type ucode_type;

	/* accomodate either ucode format */
	if(fpa_type == FPA_PLUS)
	{
		ulines = 16 * 1024;
		uline_size = 2;
	}
	else
	{
		ulines = 4 * 1024;
		uline_size = 3;
	}

	/* open ucode file and check it out */
	if((fp = fopen(filename,"r")) == NULL)
	{
		error("Can't open ucode file %s\n", filename);
	}

	/* read in the 16-bit number of microcode lines  */
	if(fread((char *)&nlines, sizeof(nlines), 1, fp) != 1)
	{
		error("Bad FPA ucode file, %s\n", filename);
	}


	/* check that it is the right type of ucode for this FPA */
	if(verbose)
		(void)fprintf(stderr,"microcode lines: %d\n", nlines);
	if(nlines <= 4096)	/* FPA capacity is 4096 lines */
		ucode_type = FPA_ORIG;
	else if(nlines <= 16 * 1024)	/* FPA+ has (x: 4k< x <= 16k) lines */
		ucode_type = FPA_PLUS;
	else
	{
		(void)fprintf(stderr, "Bad FPA ucode size %d in %s\n", nlines, filename);
		exit(-1);
	}
	if(ucode_type != fpa_type)
	{
		error("FPA board/ucode mismatch (0x%x != 0x%x)\n",
			fpa_type, ucode_type);
	}

	/* check for overflow XXX - still needed? */
	if(nlines > (ulines - 1))
	{
		error("FPA microcode overflowed available space\n");
	}

	/* get a proper size buffer to hold the ucode */
	ucode = (u_int *)calloc( ulines * uline_size , sizeof(u_int));
	if (ucode == NULL)
	{
		perror("calloc failed");
		exit(-1);
	}
	v_ucode = (u_int *)calloc( ulines, uline_size * sizeof(u_int));
	if (v_ucode == NULL)
	{
		perror("calloc failed");
		exit(-1);
	}

	/* read it in */
	if(fread((char *)ucode, sizeof(u_int), (int)(uline_size * nlines), fp) != uline_size * nlines)
	{
		error("FPA Microcode download file - bad format\n");
	}

	/* XXX FPA */
	(void)signal(SIGBUS, got_buserr);
	my_ioctl("fpa_uload", FPA_ACCESS_OFF, (char *)NULL);
	my_ioctl("fpa_uload", FPA_LOAD_ON, (char *)NULL);

	if(verbose)
		(void)fprintf(stderr,"Downloading microcode, checksum: ");
	if(fpa_type == FPA_PLUS)
	{
	    for( i = 0; i < uline_size; ++i)
	    {
		    ucode[uline_size * nlines + i] =
		     CHECKSUM ^ cksum(nlines, ucode +i, uline_size);
		    if(verbose)
			(void)fprintf(stderr,"0x%x ", ucode[uline_size * nlines + i]);
	    }	
	} else /* FPA_ORIG */
	{
	ucode[3 * nlines] = BYTE_MASK & (CHECKSUM ^ cksum(nlines, ucode, 3));
	ucode[3 * nlines + 1] = CHECKSUM ^ cksum(nlines, ucode + 1, 3);
	ucode[3 * nlines + 2] = CHECKSUM ^ cksum(nlines, ucode + 2, 3);
	if(verbose) (void)fprintf(stderr,"0x%x 0x%x 0x%x ",
	 ucode[3 * nlines + 0], ucode[3 * nlines + 1], ucode[3 * nlines + 2]);
	}

	/* the actual download */
	if(fpa_type == FPA_PLUS)
	{
		int i;
		for (ptr = ucode, ucode_add = 0, i = ulines; i > 0; i--	, ucode_add += 4)
		{
			fpa->fp_load_ptr = ucode_add | FPA_PLUS_BIT_63_32;
			fpa->fp_ld_ram = *ptr++;
			fpa->fp_load_ptr = ucode_add | FPA_PLUS_BIT_31_0;
			fpa->fp_ld_ram = *ptr++;
		}
	}
	else /* FPA_ORIG */
	{
		int i;
		for (ptr = ucode, ucode_add = 0, i = ulines; i > 0; i--, ucode_add += 4)
		{
			fpa->fp_load_ptr = ucode_add | FPA_BIT_71_64;
			fpa->fp_ld_ram = *ptr++;
			fpa->fp_load_ptr = ucode_add | FPA_BIT_63_32;
			fpa->fp_ld_ram = *ptr++;
			fpa->fp_load_ptr = ucode_add | FPA_BIT_31_0;
			fpa->fp_ld_ram = *ptr++;
		}
	}
	(void)fclose(fp);	/* close ucode file */
	if(verbose) (void)fprintf(stderr," ...");

	/* read ucode, compare with v_ucode, compute checksum */

	if(fpa_type == FPA_PLUS)
	{
		int i;
		for (ptr = v_ucode, ucode_add = 0, i = ulines; i > 0; i--, ucode_add += 4)
		{
			fpa->fp_load_ptr = ucode_add | FPA_PLUS_BIT_63_32;
			*ptr++ = fpa->fp_ld_ram;
			fpa->fp_load_ptr = ucode_add | FPA_PLUS_BIT_31_0;
			*ptr++ = fpa->fp_ld_ram;
		}
	}
	else /* FPA_ORIG */
	{
		int i;
		for (ptr = v_ucode, ucode_add = 0, i = ulines; i > 0; i--, ucode_add += 4)
		{
			fpa->fp_load_ptr = ucode_add | FPA_BIT_71_64;
			*ptr++ = fpa->fp_ld_ram & 0xff;	/* use only low byte */
			fpa->fp_load_ptr = ucode_add | FPA_BIT_63_32;
			*ptr++ = fpa->fp_ld_ram;
			fpa->fp_load_ptr = ucode_add | FPA_BIT_31_0;
			*ptr++ = fpa->fp_ld_ram;
		}
#ifdef DEBUG
		if(verbose) (void)fprintf(stderr,"Uploaded Microcode with Checksum: ");
		for( i = 0; i < uline_size; ++i)
		{
			u_int sum;
			sum = CHECKSUM ^ cksum(nlines, v_ucode +i, uline_size);
			if(verbose) (void)fprintf(stderr,"0x%8x ",sum);
		}
		if(verbose) (void)putchar('\n');
		if(verbose) (void)fprintf(stderr,"Uploaded Checksum in Microcode  : ");
		for( i = 0; i < uline_size; ++i)
		{
			if(verbose)
			(void)fprintf(stderr,"0x%8x ", v_ucode[uline_size * nlines + i]);
		}
		if(verbose) (void)putchar('\n');
#endif DEBUG
		if(verbose)
		{
			int i;
			for (i = 0; i < ulines ; ++i)
			{
				int j;
				int found = 0;

				for (j = 0;(found == 0) && (j < uline_size); ++j)
				{
				/* (void)fprintf(stderr,"%8d * %8d + %8d= %8d\n", i, uline_size, j, i*uline_size+j);*/
					if(ucode[i*uline_size + j] !=  v_ucode[i*uline_size +j]) found = 1;
				}
				if (found != 0)
				{
					(void)fprintf(stderr,"%d\t", i);
					for (j = 0; j < uline_size; ++j)
						(void)fprintf(stderr," %8x", ucode[i*uline_size + j]);
					(void)putchar('\n');
					(void)fprintf(stderr,"up:\t", i);
					for (j = 0; j < uline_size; ++j)
						(void)fprintf(stderr," %8x",v_ucode[i*uline_size + j]);
					errors++;
					(void)putchar('\n');
				}
			}
		}
	}
	if(verbose) (void)fprintf(stderr," done.\n");
	my_ioctl("fpa_uload", FPA_LOAD_OFF, (char *)NULL);

	return(errors);
}

void
got_buserr()
{
	(void)fprintf(stderr,"Got Bus error (SIGBUS) while trying to access FPA\n")
;	fpa_shutdown(IOCTL_FAILED);
}

u_int map[MAP_RAM_LINES];
u_int v_map[MAP_RAM_LINES];

/*		Download The Mapping RAM
/*  filename is of the following format:
/*    an integer specifying number of lines, followed by
/*    nlines long's containing 24 mapping ram bits in lower order
/*  a checksum will be inserted at address MAP_CKSUM_ADDR */
fpa_mload(filename)
	char *filename;
{
FILE *fp;
u_int nlines;
register u_int *ptr;
u_int map_add;

	(void)signal(SIGBUS, got_buserr);
	if((fp=fopen(filename,"r"))==NULL)
	{
		error("can't open map file %s\n", filename);
	}
	my_ioctl("fpa_mload", FPA_ACCESS_OFF, (char *)NULL);
	my_ioctl("fpa_mload", FPA_LOAD_ON, (char *)NULL);

	nlines = 4 * 1024;
	if(fread((char *)map, sizeof(long), (int) nlines, fp) != nlines)
	{
		error("FPA mapping ram download file has bad format\n");
	}
	map[MAP_CKSUM_ADDR] = MAP_MASK & (CHECKSUM ^ cksum(nlines, map, 1) ^ cksum(1, &map[MAP_CKSUM_ADDR], 1));	/* calculate the checksum of all locations BUT this one */
	if(verbose)
		(void)fprintf(stderr,"Downloading map ram, checksum: 0x%x ", map[MAP_CKSUM_ADDR]);
	for (ptr = map, map_add = 0; nlines > 0; nlines--, map_add += 4)
	{
		fpa->fp_load_ptr = map_add | FPA_BIT_23_0;
		fpa->fp_ld_ram = *ptr++;
	}
	(void)fclose(fp);
	if(verbose) (void)fprintf(stderr,"...");

	for (ptr = v_map, map_add = 0, nlines = MAP_RAM_LINES; nlines > 0; nlines--, map_add += 4)
	{
		fpa->fp_load_ptr = map_add | FPA_BIT_23_0;
		*ptr++ = fpa->fp_ld_ram & MAP_MASK;	/* take only lower 3 bytes */
	}
	/* compare */
	for (nlines = 0; nlines < MAP_RAM_LINES; ++ nlines)
	{
		if(map[nlines] != v_map[nlines])
		{
			(void)fprintf(stderr,"\nERROR: map line %d down: %x up: %x\n",nlines, map[nlines], v_map[nlines]);
			errors ++;
		}
	}
	if(verbose) (void)fprintf(stderr," done.\n");
	my_ioctl("fpa_mload", FPA_LOAD_OFF, (char *)NULL);

	return(errors);
}

u_int const[CONST_RAM_LINES][2];  /* this initializes data to 0; maybe NaN is better */
u_int v_const[CONST_RAM_LINES][2];  /* this initializes data to 0; maybe NaN is better */

/*		Download The Constants
/*  filename is of the following format:
/*    all lines start with a name which starts with s, d, c_s, or c_d.
/*    following the name is a tab
/*    if the name starts with s or c_s then the tab is followed by a
/*	hex, eight-digit number
/*    if the name starts with d or c_d then the tab is followed by a
/*	hex sixteen-digit number
/*  a checksum will be inserted at address REG_CKSUM_ADDR */
fpa_cload(filename)
	char *filename;
{
FILE *fp;
u_int const_add;
u_int constant1, constant2;
int count = 0;
int offset;
char string[100];
char name[100];

	(void)signal(SIGBUS, got_buserr);
	if((fp=fopen(filename,"r"))==NULL)
	{
		error("can't open const file %s\n", filename);
	}
	my_ioctl("fpa_cload", FPA_ACCESS_ON, (char *)NULL);
	my_ioctl("fpa_cload", FPA_LOAD_OFF, (char *)NULL);

	while(fgets(string, 100, fp) != NULL)
	{
		if((sscanf(string, "%s%x%8x%8x", name, &const_add, &constant1, &constant2)) <=2)
			continue;
		switch(*name)
		{
		case 's':
			offset = 0x400;
			ck_loc(name, const_add+offset);
			const[const_add+offset][0] = constant1;
			const[const_add+offset][1] = FILLER;
			break;
		case 'd':
			offset = 0x600;
			ck_loc(name, const_add+offset);
			const[const_add+offset][0] = constant1;
			const[const_add+offset][1] = constant2;
			break;
		case 'c':
			if(name[2] == 's')
			{
				offset = 0x500;
				ck_loc(name, const_add+offset);
				const[const_add+offset][0] = constant1;
				const[const_add+offset][1] = FILLER;
				break;
			} else if(name[2] == 'd')
			{
				offset = 0x700;
				ck_loc(name, const_add+offset);
				const[const_add+offset][0] = constant1;
				const[const_add+offset][1] = constant2;
				break;
			}
		default:
			error("FPA constant file has bad format\n");
		}
		count++;
	}
	if(verbose)
		(void)fprintf(stderr,"constants: %d\n", count);
	const[REG_CKSUM_ADDR][0] = CHECKSUM ^ cksum(REG_CKSUM_MAX - 0x400 +1, &const[0x400][0], 2);
	const[REG_CKSUM_ADDR][1] = CHECKSUM ^ cksum(REG_CKSUM_MAX - 0x400 +1, &const[0x400][1], 2);
	if(verbose)
		(void)fprintf(stderr,"Downloading constants, checksum: 0x%x  0x%x ",
			 const[REG_CKSUM_ADDR][0], const[REG_CKSUM_ADDR][1]);

	for (const_add = 1024; const_add < CONST_RAM_LINES; const_add++)
	{
		fpa->fp_load_ptr = (long)(const_add << 2);
		*(long *)((long)fpa + REG_WRM_LP) = const[const_add][0];
		*(long *)((long)fpa + REG_WRL_LP) = const[const_add][1];
	}
	if(verbose)
		(void)fprintf(stderr,"...");

	for (const_add = 1024; const_add < CONST_RAM_LINES; const_add++)
	{
		fpa->fp_load_ptr = (long)(const_add << 2);
		v_const[const_add][0] = *(long *)((long)fpa + REG_WRM_LP) ;
		v_const[const_add][1] = *(long *)((long)fpa + REG_WRL_LP) ;
	}

	for (const_add = 1024; const_add < CONST_RAM_LINES; const_add++)
	{
		if((v_const[const_add][0] != const[const_add][0]) ||(v_const[const_add][1] != const[const_add][1]))
		{
			(void)fprintf(stderr,"ERROR constant ram download/upload mismatch.\n");
			(void)fprintf(stderr,"%d	%8x %8x\n",const_add,const[const_add][0], const[const_add][1]);
			(void)fprintf(stderr,"up:	%8x %8x\n",v_const[const_add][0], v_const[const_add][1]);
			errors++;
		}
	}
	if(verbose) (void)fprintf(stderr," done.\n");
	if(errors) return(errors); /* don't finish if errors found */
	fpa->fp_initialize = 0;
	fpa->fp_restore_mode3_0 = 0x2;

	my_ioctl("fpa_cload", FPA_ACCESS_OFF, (char *)NULL);
	my_ioctl("fpa_cload", FPA_INIT_DONE, (char *)NULL);

	(void)fclose(fp);
	return(0);
}
ck_loc(name, index)
char *name;
u_int index;
{
	if(const[index][0] || const[index][1])
		(void)fprintf(stderr, "Warning! FPA Reloading constant: %12s at address %3x\n", name, index);
}

fpa_rev()
{	
	(void)printf("%s installed.\n", fpa_type==FPA_PLUS ? "FPA+":"FPA");
#ifndef COMMENT
	fpa->fp_load_ptr = 0x7f0 << 2;
	(void)printf("Microcode = level: %lx, date: %06lx\n", *(long *)LDP_MS, *(long *)LDP_LS);
	fpa->fp_load_ptr = 0x7f1 << 2;
	(void)printf("Constants = level: %lx, date: %06lx\n", *(long *)LDP_MS, *(long *)LDP_LS); 
#else COMMENT
	*(long *)REG_CPY_DP = 0xfc00;
	(void)printf("Microcode = level: %lx, date: %06lx\n", *(long *)REG_RD_MSW, *(long *)REG_RD_LSW);
	*(long *)REG_CPY_DP = 0xfc40;
	(void)printf("Constants = level: %lx, date: %06lx\n", *(long *)REG_RD_MSW, *(long *)REG_RD_LSW); */
#endif COMMENT

/* XXX*/
}
u_int cksum(nlines, ptr, inc)
u_int nlines;
u_int *ptr;
u_int inc;
{
	u_int sum = 0;

	while(nlines--)
	{
		sum ^= *ptr;
		ptr += inc;
	}
	return(sum);
}
void
got_fpeerr()
{
	(void)signal(SIGFPE, got_fpeerr);
	(void)fprintf(stderr, "Got a fpe error\n");
	(void)fprintf(stderr, "The FPA IERR is: %xl\n", fpa->fp_ierr);
	if(errno == EPIPE)
	{
		fpa_shutdown(BUS_ERROR);
	}
	exit(-1);
}
void
got_segverr()
{
	(void)signal(SIGSEGV, got_segverr);
	(void)fprintf(stderr, "Got a segmentation error\n");
	exit(-1);
}

char *time_of_day()
{
	long temptime;
	
	(void)time(&temptime);
	return(ctime(&temptime));
}

char msg[FPA_LINESIZE] = "FPA Download Failed - ";

fpa_shutdown(val)
	int val;
{

	(void)strcat(msg, error_message[val]);
	(void)strcat(msg, " - ");
	(void)strcat(msg, time_of_day());
	broadcast_msg(msg);
	log_msg(msg);

		/* disable the fpa */
	my_ioctl("fpa_shutdown", FPA_FAIL, msg);

	exit(-1);
}

log_msg(msg)
	char *msg;
{
	FILE    *tmp_fp, *fopen();

	if ((tmp_fp = fopen("/var/adm/diaglog","a")) == NULL)
	{
		perror("could not open /var/adm/diaglog");
	} else
	{
		fputs(msg,tmp_fp);
		(void)fclose(tmp_fp);		
	}
}
char	tmp_file[L_tmpnam];
char	wall_str[100] = "/bin/wall -a \0";

void
broadcast_msg(msg)
char *msg;
{
	FILE	*tmp_fp, *fopen();

		/* create temp file with error message */
	(void)tmpnam(tmp_file);
	tmp_fp = fopen(tmp_file,"w");
	if (tmp_fp == NULL) return;
	fputs(msg, tmp_fp);
	(void)fclose(tmp_fp);

	(void)strcpy(&wall_str[13], tmp_file);
	(void)strcat(wall_str, " 2> /dev/console");
	if(system(wall_str) != 0)
	{
		(void)fprintf(stderr,"%s: %s FAILED\n",progname,wall_str);
		return;
	}
	(void)unlink(tmp_file);
}

char *a[] = {
	"FPA_ACCESS_ON",
	"FPA_ACCESS_OFF",
	"FPA_LOAD_ON",
	"FPA_LOAD_OFF",
	"FPA_INIT_DONE",
	"FPA_FAIL",
	"UNKNOWN IOCTL"};
char *
io_string(io_num)
int io_num;
{
	switch(io_num)
	{
		case FPA_ACCESS_ON:	return( a[0] );
		case FPA_ACCESS_OFF:	return( a[1] );
		case FPA_LOAD_ON:	return( a[2] );
		case FPA_LOAD_OFF:	return( a[3] );
		case FPA_INIT_DONE:	return( a[4] );
		case FPA_FAIL:		return( a[5] );
		default:		return( a[6] );
	}
}

my_ioctl(func_str, io_call, data)
char* func_str;
int io_call;
char* data;
{

	char *io_str;

	io_str = io_string(io_call);
	if(ioctl(fpa_fd, io_call, data) == -1)
	{
		if(errno == EPIPE)
		{
			(void)fprintf(stderr, "%s: %s - pipe not clear.\n", func_str, io_str);
			if(io_call != FPA_FAIL)
				fpa_shutdown(IOCTL_FAILED);
		}
		else
		{
			(void)fprintf(stderr,"%s: ioctl(%s) ", progname, io_str);
			perror("");
		}
		exit(-1);
	}
#ifdef DEBUG
	if(verbose)
		(void)fprintf(stderr,"%s: %s worked.\n",func_str, io_str);
#endif DEBUG
}

/* print error string and exit */
/*VARARGS*/
void
error(ctl, a1, a2, a3)
char* ctl;
{
	(void)fprintf(stderr,"%s: ", progname);
	(void)fprintf(stderr, ctl, a1, a2, a3);
	exit(-1);
}
