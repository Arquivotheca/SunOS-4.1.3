#ifndef lint
static	char sccsid[] = "@(#)file.c 1.1 92/07/30 SMI"; /* from S5R3 1.17 */
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#include	<ctype.h>
#include 	<locale.h>
#include	<fcntl.h>
#include	<stdio.h>
#include	<a.out.h>
#include	<vfont.h>
#include	<rasterfile.h>
#include	<sys/core.h>
#include	<sys/param.h>
#include	<sys/types.h>
#include	<sys/sysmacros.h>
#include	<sys/stat.h>

/*
**	Types
*/

#define	BYTE	0
#define	SHORT	1
#define	LONG	2
#define	STR	3

#define	NTYPES	4

char	*typenames[NTYPES] = {
	"byte",
	"short",
	"long",
	"string"
};

/*
**	Opcodes
*/

#define	EQ	0
#define	GT	1
#define	LT	2
#define	STRC	3	/* string compare */
#define	ANY	4
#define AND	5
#define NSET	6	/* True if bit is not set */
#define	SUB	64	/* or'ed in, SUBstitution string, for example %ld, %s, %lo */
			/* mask: with bit 6 on, used to locate print formats */

char	opnames[] = {
	'=',
	'>',
	'<',
	'=',
	'x',
	'&',
	'^'
};

#define	NOPS	(sizeof opnames / sizeof opnames[0])

/*
**	Misc
*/

#define	NENT	200
#define	BSZ	128
#define	FBSZ	512
#define	reg	register

/* Ass lang comment char */
#ifdef pdp11
#define ASCOMCHAR '/'
#endif
#ifdef mc68000
#define ASCOMCHAR '|'
#endif
#ifdef sparc
#define ASCOMCHAR '!'
#endif
#ifdef vax
#define ASCOMCHAR '#'
#endif
/*
**	Structure of magic file entry
*/

struct	entry	{
	char	e_level;	/* 0 or 1 */
	long	e_off;		/* in bytes */
	char	e_type;		/* BYTE, SHORT, LONG, STR */
	char	e_opcode;	/* EQ, GT, LT, ANY, AND, NSET */
	long	e_mask;		/* if non-zero, mask value with this */
	union	{
		long	num;
		char	*str;
	}	e_value;
	char	*e_str;
};

typedef	struct entry	Entry;

Entry	*mtab, *mend;
char	fbuf[FBSZ];
char	*mfile = "/etc/magic";
char *troff[] = {	/* new troff intermediate lang */
	"x","T","res","init","font","202","V0","p1",0};
								/* Fortran */
char	*fort[] = {
	"function","subroutine","common","dimension","block","integer",
	"real","data","double",0};
char	*asc[] = {
#ifdef pdp11
	"sys","mov","movb","tst","tstb","clr","clrb","jmp",0};
#endif
#ifdef mc68000
	"trap","movel","movew","moveb","movl","movw","movb","moveq",
	"tstl","tstw","tstb","clrl","clrw","clrb","jmp",0};
#endif
#ifdef sparc
	"t","mov",0};
#endif
#ifdef vax
	"chmk","movl","movw","movb","tstl","tstw","tstb",
	"clrl","clrw","clrb","jmp",0};
#endif
								/* C Language */
char	*c[] = {
	"int","char","float","double","struct","extern","static",0};
								/* Assembly Language */
char	*as[] = {
	"globl","byte","even","text","data","bss","comm",0};
								/* Bourne Shell */
char *sh[] = {
	"fi", "elif", "esac", "done", "export",
	"readonly", "trap", "PATH", "HOME", 0 };
								/* C Shell */
char *csh[] = {
	"alias", "breaksw", "endsw", "foreach", "limit",  "onintr",
	"repeat", "setenv", "source", "path", "home", 0 };
char	*strchr();
char	*malloc();
long	atolo();
char	*getstr();
void	showstr();
char	*errmsg();
int	i = 0;
int	fbsz;
int	ifd = -1;
int	Lflg = 0;	/* True if -L seen: resolve symbolic links */
void	exit();
extern int errno;

#define	prf(x)	printf("%s:%s", x, strlen(x)>6 ? "\t" : "\t\t");

main(argc, argv)
char **argv;
{
	reg	char	*p;
	reg	int	ch;
	reg	FILE	*fl;
	reg	int	cflg = 0, eflg = 0, fflg = 0;
	auto	char	ap[MAXPATHLEN + 1];
	extern	int	optind;
	extern	char	*optarg;

	setlocale(LC_ALL, "");
	while((ch = getopt(argc, argv, "cLf:m:")) != EOF)
	switch(ch) {
	case 'c':
		cflg++;
		break;

	case 'L':
		Lflg++;
		break;

	case 'f':
		fflg++;
		if ((fl = fopen(optarg, "r")) == NULL) {
			fprintf(stderr, "file: %s: %s\n", optarg,
			    errmsg(errno));
			goto use;
		}
		break;

	case 'm':
		mfile = optarg;
		break;

	case '?':
		eflg++;
		break;
	}
	if(!cflg && !fflg && (eflg || optind == argc)) {
use:
		fprintf(stderr,
			"usage: file [-cL] [-f ffile] [-m mfile] file...\n");
		exit(2);
	}
	if(cflg) {
		reg	Entry	*ep;

		mkmtab(1);
		printf("level\toff\ttype\topcode\tvalue\tstring\n");
		for(ep = mtab; ep < mend; ep++) {
			printf("%d\t%d\t%s", ep->e_level, ep->e_off,
				typenames[ep->e_type]);
			if(ep->e_mask != 0L)
				printf("&%#lo", ep->e_mask);
			printf("\t%c\t", opnames[ep->e_opcode & ~SUB]);
			if((ep->e_opcode & ~SUB) != ANY) {
				if(ep->e_type == STR)
					showstr(ep->e_value.str);
				else
					printf("%lo\t", ep->e_value.num);
			} else
				printf("\t");
			printf("%s", ep->e_str);
			if(ep->e_opcode & SUB)
				printf("\tsubst");
			printf("\n");
		}
		exit(0);
	}
	for(; fflg || optind < argc; optind += !fflg) {
		reg	int	l;

		if(fflg) {
			if((p = fgets(ap, 128, fl)) == NULL) {
				fflg = 0;
				optind--;
				continue;
			}
			l = strlen(p);
			if(l > 0)
				p[l - 1] = '\0';
		} else
			p = argv[optind];
		prf(p);					/* print "file_name:<tab>" */
		type(p);
		if(ifd >= 0)
			close(ifd);
	}
	exit(0);	/*NOTREACHED*/
}

type(file)
char	*file;
{
	int	j,nl;
	char	ch;
	struct	stat	mbuf;
	char slink[MAXPATHLEN + 1];
	struct	exec	ex;
	struct	utimbuf {
		time_t	actime;
		time_t	modtime;
	}	utb;

	ifd = -1;
	if (!Lflg || stat(file, &mbuf) < 0) {
		/*
		 * If -L and stat fails on symlink, print "Symbolic link to..."
		 */
		if (lstat(file, &mbuf) < 0) {
			printf("%s\n", errmsg(errno));
			return;
		}
	}
	switch (mbuf.st_mode & S_IFMT) {

	case S_IFLNK:
		printf("symbolic link");
		j = readlink(file, slink, sizeof slink - 1);
		if (j >= 0) {
			slink[j] = '\0';
			printf(" to %s", slink);
		}
		printf("\n");
		return;

	case S_IFREG:
		break;	/* Regular file */

	case S_IFCHR:
		printf("character");
		goto spcl;

	case S_IFDIR:
		if (mbuf.st_mode & S_ISVTX)
			printf("append-only ");
		printf("directory\n");
		return;

	case S_IFIFO:
		printf("fifo\n");
		return;

	case S_IFBLK:
		printf("block");
					/* major and minor, see sys/sysmacros.h */

spcl:
		printf(" special (%d/%d)\n", major(mbuf.st_rdev),
							minor(mbuf.st_rdev));
		return;

	case S_IFSOCK:
		printf("socket\n");
		/* FIXME, should open and try to getsockname. */
		return;

	default:
		printf("strange file with mode=%o\n", mbuf.st_mode);
		return;
	}
	ifd = open(file, O_RDONLY);
	if(ifd < 0) {
		if (mbuf.st_mode&((S_IEXEC)|(S_IEXEC>>3)|(S_IEXEC>>6)))
			printf("executable, ");
		if (mbuf.st_mode & S_ISUID)
			printf("suid, ");
		if (mbuf.st_mode & S_ISGID)
			printf("sgid, ");
		if (mbuf.st_mode & S_ISVTX)
			printf("sticky, ");
		printf("%s\n", errmsg(errno));
		return;
	}
	fbsz = read(ifd, fbuf, FBSZ);
	if(fbsz == 0) {
		printf("empty\n");
		goto out;
	}
	if(sccs()) {			/* look for "1hddddd" where d is a digit */
		printf("sccs\n");
		goto out;
	}

	ex = *((struct exec*)fbuf);
	if(!N_BADMAG(ex)) {
#if defined(mc68000) || defined(sparc)
		switch(ex.a_machtype) {
		case M_OLDSUN2:
			printf("old sun-2 ");
			break;
		case M_68010:
			printf("mc68010 ");
			break;
		case M_68020:
			printf("mc68020 ");
			break;
		case M_SPARC:
			printf("sparc ");
			break;
		default:
			goto not_program;
		}
#endif
		switch(ex.a_magic) {
		case ZMAGIC:
			printf("demand paged ");
 	  		goto exec;

		case NMAGIC:
			printf("pure ");
			goto exec;

		case OMAGIC:
	exec:
			if ((ex.a_magic == ZMAGIC) && (ex.a_entry < PAGSIZ) && ex.a_dynamic)
				printf("shared library ");
			if ((ex.a_entry >= PAGSIZ) && ex.a_dynamic)
				printf("dynamically linked ");
			if (mbuf.st_mode & S_ISUID)
				printf("set-uid ");
			if (mbuf.st_mode & S_ISGID)
				printf("set-gid ");
			if (mbuf.st_mode & S_ISVTX)
				printf("sticky ");
			printf("executable");
			if(ex.a_syms != 0) {
				printf(" not stripped");
				if(oldo(fbuf))
					printf(" (old format symbol table)");
			}
			printf("\n");
			goto out;

		default:
			break;
		} /*switch*/
	} /* !N_BADMAG(ex) */

not_program:
	switch(*(int *)fbuf) {

	case RAS_MAGIC:
#		define ras ((struct rasterfile *)fbuf)
		printf("rasterfile, %dx%dx%d", ras->ras_width,
			ras->ras_height, ras->ras_depth);
		switch (ras->ras_type) {
		case RT_OLD:
			printf(" old format");
			break;
		case RT_STANDARD:
			printf(" standard format");
			break;
		case RT_BYTE_ENCODED:
			printf(" run-length byte encoded");
			break;
		case RT_EXPERIMENTAL:
			printf(" experimental format");
			break;
		default:
			printf(" client defined format type %d", ras->ras_type);
			break;
		}
		printf(" image\n");
		goto out;
#		undef ras
	case CORE_MAGIC:
		printf("core file");
		if('\0' != *((struct core *)fbuf)->c_cmdname)
			printf(" from '%s'", ((struct core *)fbuf)->c_cmdname);
		printf("\n");
		goto out;

	}

	if (VFONT_MAGIC == ((struct header *)fbuf)->magic) {
		printf("vfont definition\n");
		goto out;
	}

	if (fbuf[0] == '#' && fbuf[1] == '!' && shellscript(fbuf+2, &mbuf))
		return;
	if(ckmtab())			/* ChecK against Magic Table entries */
		goto out;
	if (mbuf.st_size % 512 == 0) {	/* it may be a PRESS file */
		lseek(ifd, -512L, 2);	/* last block */
		if (read(ifd, fbuf, FBSZ) > 0
		 && *(short int *)fbuf == 12138) {
			printf("PRESS file\n");
			goto out;
		}
	}
	i = 0;
	if(ccom() == 0)
		goto notc;
	while(fbuf[i] == '#') {
		j = i;
		while(fbuf[i++] != '\n') {
			if(i - j > 255) {
#ifdef DBG
			printf("printing data (1)-");
#endif
				printf("data\n"); 
				goto out;
			}
			if(i >= fbsz)
				goto notc;
		}
		if(ccom() == 0)
			goto notc;
	}
check:
	if(lookup(c) == 1) {
		while((ch = fbuf[i++]) != ';' && ch != '{')
			if(i >= fbsz)
				goto notc;
		printf("c program text");
		goto outa;
	}
	nl = 0;
	while(fbuf[i] != '(') {
		if(fbuf[i] <= 0)
			goto notas;
		if(fbuf[i] == ';'){
			i++; 
			goto check; 
		}
		if(fbuf[i++] == '\n')
			if(nl++ > 6)goto notc;
		if(i >= fbsz)goto notc;
	}
	while(fbuf[i] != ')') {
		if(fbuf[i++] == '\n')
			if(nl++ > 6)
				goto notc;
		if(i >= fbsz)
			goto notc;
	}
	while(fbuf[i] != '{') {
		if(fbuf[i++] == '\n')
			if(nl++ > 6)
				goto notc;
		if(i >= fbsz)
			goto notc;
	}
	printf("c program text");
	goto outa;
notc:
	i = 0;
	while(fbuf[i] == 'c' || fbuf[i] == '#') {
		while(fbuf[i++] != '\n')
			if(i >= fbsz)
				goto notfort;
	}
	if(lookup(fort) == 1){
		printf("fortran program text");
		goto outa;
	}
notfort:
	i = 0;
	if(ascom() == 0)
		goto notas;
	j = i-1;
	if(fbuf[i] == '.') {
		i++;
		if(lookup(as) == 1){
			printf("assembler program text"); 
			goto outa;
		}
		else if(j != -1 && fbuf[j] == '\n' && isalpha(fbuf[j+2])){
			printf("[nt]roff, tbl, or eqn input text");
			goto outa;
		}
	}
	while(lookup(asc) == 0) {
		if(ascom() == 0)
			goto notas;
		while(fbuf[i] != '\n' && fbuf[i++] != ':')
			if(i >= fbsz)
				goto notas;
		while(fbuf[i] == '\n' || fbuf[i] == ' ' || fbuf[i] == '\t')
			if(i++ >= fbsz)
				goto notas;
		j = i - 1;
		if(fbuf[i] == '.'){
			i++;
			if(lookup(as) == 1) {
				printf("assembler program text"); 
				goto outa; 
			}
			else if(fbuf[j] == '\n' && isalpha(fbuf[j+2])) {
				printf("[nt]roff, tbl, or eqn input text");
				goto outa;
			}
		}
	}
	printf("assembler program text");
	goto outa;
notas:
	for(i=0; i < fbsz; i++)
		if((fbuf[i]&0200) && (isprint((unsigned char)fbuf[i]) == 0)) {
			if (fbuf[0]=='\100' && fbuf[1]=='\357') {
				printf("otroff output\n");
				goto out;
			}
#ifdef DBG
			printf("c = %c, %d\n", fbuf[i], isprint(fbuf[i]));
			printf("printing data (2)-");
#endif
			printf("data\n"); 
			goto out; 
		}
	if (mbuf.st_mode&((S_IEXEC)|(S_IEXEC>>3)|(S_IEXEC>>6))) {
		if (mbuf.st_mode & S_ISUID)
			printf("set-uid ");
		if (mbuf.st_mode & S_ISGID)
			printf("set-gid ");
		if (mbuf.st_mode & S_ISVTX)
			printf("sticky ");
		if (shell(fbuf, fbsz, sh))
			printf("shell script");
		else if (shell(fbuf, fbsz, csh))
			printf("c-shell script");
		else
			printf("commands text");
	} else if (troffint(fbuf, fbsz))
		printf("troff intermediate output text");
	else if (shell(fbuf, fbsz, sh))
		printf("shell commands");
	else if (shell(fbuf, fbsz, csh))
		printf("c-shell commands");
	else if(english(fbuf, fbsz))
		printf("English text");
	else
		printf("ascii text");
outa:
	while(i < fbsz)
		if((fbuf[i++]&0377) > 127) {
			printf(" with garbage\n");
			goto out;
		}
	/* if next few lines in then read whole file looking for nulls ...
		while((fbsz = read(ifd,fbuf,FBSZ)) > 0)
			for(i = 0; i < fbsz; i++)
				if((fbuf[i]&0377) > 127){
					printf(" with garbage\n");
					goto out;
				}
		/*.... */
	printf("\n");
					/* reset access & modification times */
out:
	utb.actime = mbuf.st_atime;
	utb.modtime = mbuf.st_mtime;
	(void)utime(file, &utb);
}

mkmtab(cflg)
reg	int	cflg;
{
	reg	Entry	*ep;
	reg	FILE	*fp;
	reg	int	lcnt = 0;
	reg	int	i;
	auto	char	buf[BSZ];
	auto	int	curentry;
	auto	int	nentries;

	mtab = (Entry *) malloc(sizeof(Entry)*NENT);
	if(mtab == NULL) {
		fprintf(stderr, "file: no memory for magic table\n");
		exit(2);
	}
	curentry = 0;
	nentries = NENT;
	fp = fopen(mfile, "r");
	if(fp == NULL) {
		fprintf(stderr, "file: cannot open magic file <%s>: %s\n",
		    mfile, errmsg(errno));
		exit(2);
	}
	while(fgets(buf, BSZ, fp) != NULL) {
		reg	char	*p = buf;
		reg	char	*p2;
		reg	char	*p3;
		reg	char	opc;

		ep = &mtab[curentry];
		lcnt++;
		if(*p == '\n' || *p == '#')
			continue;
			

			/* LEVEL */
		if(*p == '>') {
			ep->e_level = 1;
			p++;
		} else
			ep->e_level = 0;
			/* OFFSET */
		p2 = strchr(p, '\t');
		if(p2 == NULL) {
			if(cflg)
				fprintf(stderr, "fmt error, no tab after %son line %d\n", p, lcnt);
			continue;
		}
		*p2++ = NULL;
		ep->e_off = atolo(p);
		while(*p2 == '\t')
			p2++;
			/* TYPE */
		p = p2;
		p2 = strchr(p, '\t');
		if(p2 == NULL) {
			if(cflg)
				fprintf(stderr, "fmt error, no tab after %son line %d\n", p, lcnt);
			continue;
		}
		*p2++ = NULL;
		p3 = strchr(p, '&');
		if(p3 != NULL) {
			*p3++ = '\0';
			ep->e_mask = atolo(p3);
		} else
			ep->e_mask = 0L;
		for (i = 0; i < NTYPES; i++) {
			if (strcmp(p, typenames[i]) == 0)
				goto foundtype;
		}
		if(cflg)
			fprintf(stderr, "file: illegal type %s on line %d\n",
			    p, lcnt);
		continue;
foundtype:
		ep->e_type = i;
		while(*p2 == '\t')
			p2++;
			/* OP-VALUE */
		p = p2;
		p2 = strchr(p, '\t');
		if(p2 == NULL) {
			if(cflg)
				fprintf(stderr, "file: fmt error, no tab after %son line %d\n", p, lcnt);
			continue;
		}
		*p2++ = '\0';
		if(ep->e_type != STR) {
			opc = *p++;
			for (i = 0; i < NOPS; i++) {
				if (opc == opnames[i]) {
					ep->e_opcode = i;
					goto foundop;
				}
			}
			/*
			 * EQ is default.
			 */
			ep->e_opcode = EQ;
			p--;
		} else {
			/*
			 * All string tests are equality comparisons.
			 */
			ep->e_opcode = EQ;
		}
foundop:
		if(ep->e_opcode != ANY) {
			if(ep->e_type != STR)
				ep->e_value.num = atolo(p);
			else
				ep->e_value.str = getstr(p);
		}
		while(*p2 == '\t')
			p2++;
			/* STRING */
		ep->e_str = malloc(strlen(p2) + 1);
		if(ep->e_str == NULL) {
			fprintf(stderr, "file: no memory for magic table\n");
			exit(2);
		}
		p = ep->e_str;
		while(*p2 != '\n') {
			if(*p2 == '%')
				ep->e_opcode |= SUB;
			*p++ = *p2++;
		}
		*p = NULL;
		curentry++;
		if(curentry >= nentries) {
			mtab = (Entry *) realloc((char *) mtab, sizeof(Entry)*NENT);
			if(mtab == NULL) {
				fprintf(stderr, "file: no memory for magic table\n");
				exit(2);
			}
			nentries += NENT;
		}
	}
	mend = &mtab[curentry];
}

long
atolo(s)				/* determine read format and get e_value.num */
reg	char	*s;
{
	reg	char	*fmt = "%ld";
	auto	long	j = 0L;

	if(*s == '0') {
		s++;
		if(*s == 'x') {
			s++;
			fmt = "%lx";
		} else
			fmt = "%lo";
	}
	sscanf(s, fmt, &j);
	return(j);
}

char *
getstr(s)
reg	char	*s;
{
	auto	char	*store;
	reg	char	*p;
	reg	char	c;
	reg	int	val;

	if((store = malloc(strlen(s) + 1)) == NULL) {
		fprintf(stderr, "file: no memory for magic table\n");
		exit(2);
	}
	p = store;
	while((c = *s++) != '\0') {
		if(c == '\\') {
			switch(c = *s++) {

			case '\0':
				goto out;

			default:
				*p++ = c;
				break;

			case 'n':
				*p++ = '\n';
				break;

			case 'r':
				*p++ = '\r';
				break;

			case 'b':
				*p++ = '\b';
				break;

			case 't':
				*p++ = '\t';
				break;

			case 'f':
				*p++ = '\f';
				break;

			case 'v':
				*p++ = '\v';
				break;

			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
				val = c - '0';
				c = *s++;  /* try for 2 */
				if(c >= '0' && c <= '7') {
					val = (val<<3) | (c - '0');
					c = *s++;  /* try for 3 */
					if(c >= '0' && c <= '7')
						val = (val<<3) | (c-'0');
					else
						--s;
				}
				else
					--s;
				*p++ = val;
				break;
			}
		} else
			*p++ = c;
	}
out:
	*p = '\0';
	return(store);
}

void
showstr(s)
reg	char	*s;
{
	reg	char	c;

	while((c = *s++) != '\0') {
		if(c >= 040 && c <= 0176)
			putchar(c);
		else {
			putchar('\\');
			switch (c) {
			
			case '\n':
				putchar('n');
				break;

			case '\r':
				putchar('r');
				break;

			case '\b':
				putchar('b');
				break;

			case '\t':
				putchar('t');
				break;

			case '\f':
				putchar('f');
				break;

			case '\v':
				putchar('v');
				break;

			default:
				printf("%.3o", c & 0377);
				break;
			}
		}
	}
	putchar('\t');
}

ckmtab()				/* ChecK for Magic Table entries in the file */
{

	reg	Entry	*ep;
	reg	char	*p;
	reg	int	lev1 = 0;
	auto	union	{
		long	l;
		char	ch[4];
		}	val, revval;
	static	char	init = 0, tmpbyte;

	if(!init) {
		mkmtab(0);
		init = 1;
	}					/* -1 offset marks end of   */
	for(ep = mtab; ep < mend; ep++) {	/* valid magic file entries */
		if(lev1) {
			if(ep->e_level != 1)
				break;
		} else if(ep->e_level == 1)
			continue;
		p = &fbuf[ep->e_off];
		switch(ep->e_type) {
		case STR:
		{
			if(strncmp(p,ep->e_value.str,strlen(ep->e_value.str)))
				continue;
			if(lev1)
				putchar(' ');
			if(ep->e_opcode & SUB)
				printf(ep->e_str, ep->e_value.str);
			else
				printf(ep->e_str);
			lev1 = 1;
			continue;
		}

		case BYTE:
			val.l = (long)(*(unsigned char *) p);
			break;

		case SHORT:
			val.l = (long)(*(unsigned short *) p);
			break;

		case LONG:
			val.l = (*(long *) p);
			break;
		}
		if(ep->e_mask)
			val.l &= ep->e_mask;
		switch(ep->e_opcode & ~SUB) {
		case EQ:
#ifdef u3b
			if(val.l != ep->e_value.num)
				if(ep->e_type == SHORT) {
					/* reverse bytes */
					revval.l = 0L;
					tmpbyte = val.ch[3];
					revval.ch[3] = val.ch[2];
					revval.ch[2] = tmpbyte;
					if(revval.l != ep->e_value.num)
						continue;
					else
						break;
				}
				else	continue;
			else
				break;
#else
			if(val.l != ep->e_value.num)
				continue;
			break;
#endif
		case GT:
			if(val.l <= ep->e_value.num)
				continue;
			break;

		case LT:
			if(val.l >= ep->e_value.num)
				continue;
			break;

		case AND:
			if((val.l & ep->e_value.num) == ep->e_value.num)
				break;
			continue;
		case NSET:
			if((val.l & ep->e_value.num) != ep->e_value.num)
				break;
			continue;
		}
		if(lev1)
			putchar(' ');
		if(ep->e_opcode & SUB)
			printf(ep->e_str, val.l);
		else
			printf(ep->e_str);
		lev1 = 1;
	}
	if(lev1) {
		putchar('\n');
		return(1);
	}
	return(0);
}

oldo(cp)
char *cp;
{
	struct exec ex;
	struct stat stb;

	ex = *(struct exec *)cp;
	if (fstat(ifd, &stb) < 0)
		return(0);
	if (N_STROFF(ex)+sizeof(off_t) > stb.st_size)
		return (1);
	return (0);
}

troffint(bp, n)
char *bp;
int n;
{
	int k;

	i = 0;
	for (k = 0; k < 6; k++) {
		if (lookup(troff) == 0)
			return(0);
		if (lookup(troff) == 0)
			return(0);
		while (i < n && fbuf[i] != '\n')
			i++;
		if (i++ >= n)
			return(0);
	}
	return(1);
}

lookup(tab)
reg	char **tab;
{
	reg	char	r;
	reg	int	k,j,l;

	while(fbuf[i] == ' ' || fbuf[i] == '\t' || fbuf[i] == '\n')
		i++;
	for(j=0; tab[j] != 0; j++) {
		l = 0;
		for(k=i; ((r=tab[j][l++]) == fbuf[k] && r != '\0');k++);
		if(r == '\0')
			if(fbuf[k] == ' ' || fbuf[k] == '\n' || fbuf[k] == '\t'
			    || fbuf[k] == '{' || fbuf[k] == '/') {
				i=k;
				return(1);
			}
	}
	return(0);
}

ccom()
{
	reg	char	cc;

	while((cc = fbuf[i]) == ' ' || cc == '\t' || cc == '\n')
		if(i++ >= fbsz)
			return(0);
	if(fbuf[i] == '/' && fbuf[i+1] == '*') {
		i += 2;
		while(fbuf[i] != '*' || fbuf[i+1] != '/') {
			if(fbuf[i] == '\\')
				i += 2;
			else
				i++;
			if(i >= fbsz)
				return(0);
		}
		if((i += 2) >= fbsz)
			return(0);
	}
	if(fbuf[i] == '\n')
		if(ccom() == 0)
			return(0);
	return(1);
}

ascom()
{
	while(fbuf[i] == ASCOMCHAR) {
		i++;
		while(fbuf[i++] != '\n')
			if(i >= fbsz)
				return(0);
		while(fbuf[i] == '\n')
			if(i++ >= fbsz)
				return(0);
	}
	return(1);
}

sccs() {				/* look for "1hddddd" where d is a digit */
	reg int j;

	if(fbuf[0] == 1 && fbuf[1] == 'h')
		for(j=2; j<=6; j++)
			if(isdigit(fbuf[j])) continue;
			else return(0);
	else
		return(0);
	return(1);
}

english (bp, n)
char *bp;
{
#	define NASC 128
	reg	int	j, vow, freq, rare;
	reg	int	badpun = 0, punct = 0;
	auto	int	ct[NASC];

	if (n<50)
		return(0); /* no point in statistics on squibs */
	for(j=0; j<NASC; j++)
		ct[j]=0;
	for(j=0; j<n; j++)
	{
		if (isascii(bp[j])) {
			if (isupper(bp[j]))
				ct[tolower(bp[j])]++;
			else
				ct[bp[j]]++;
		}
		switch (bp[j])
		{
		case '.': 
		case ',': 
		case ')': 
		case '%':
		case ';': 
		case ':': 
		case '?':
			punct++;
			if(j < n-1 && bp[j+1] != ' ' && bp[j+1] != '\n')
				badpun++;
		}
	}
	if (badpun*5 > punct)
		return(0);
	vow = ct['a'] + ct['e'] + ct['i'] + ct['o'] + ct['u'];
	freq = ct['e'] + ct['t'] + ct['a'] + ct['i'] + ct['o'] + ct['n'];
	rare = ct['v'] + ct['j'] + ct['k'] + ct['q'] + ct['x'] + ct['z'];
	if(2*ct[';'] > ct['e'])
		return(0);
	if((ct['>']+ct['<']+ct['/'])>ct['e'])
		return(0);	/* shell file test */
	return (vow*5 >= n-ct[' '] && freq >= 10*rare);
}

shellscript(buf, sb)
	char buf[];
	struct stat *sb;
{
	register char *tp;
	char *cp, *xp;

	cp = strchr(buf, '\n');
	if (cp == 0 || cp - fbuf > fbsz)
		return (0);
	for (tp = buf; tp != cp && isspace(*tp); tp++)
		if (!isascii(*tp))
			return (0);
	for (xp = tp; tp != cp && !isspace(*tp); tp++)
		if (!isascii(*tp))
			return (0);
	if (tp == xp)
		return (0);
	if (sb->st_mode & S_ISUID)
		printf("set-uid ");
	if (sb->st_mode & S_ISGID)
		printf("set-gid ");
	if (strncmp(xp, "/bin/sh", tp-xp) == 0)
		xp = "shell";
	else if (strncmp(xp, "/bin/csh", tp-xp) == 0)
		xp = "c-shell";
	else
		*tp = '\0';
	printf("executable %s script\n", xp);
	return (1);
}

shell(bp, n, tab)
	char *bp;
	int n;
	char *tab[];
{

	i = 0;
	do {
		if (fbuf[i] == '#' || fbuf[i] == ':')
			while (i < n && fbuf[i] != '\n')
				i++;
		if (++i >= n)
			break;
		if (lookup(tab) == 1)
			return (1);
	} while (i < n);
	return (0);
}

char *
errmsg(errnum)
int errnum;
{
	static char errmsgbuf[6+10+1];
	extern int sys_nerr;
	extern char *sys_errlist[];

	if (errnum >= 0 && errnum < sys_nerr)
		return (sys_errlist[errnum]);
	else {
		(void) sprintf(errmsgbuf, "Error %d", errnum);
		return (errmsgbuf);
	}
}
