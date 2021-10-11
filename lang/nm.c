#ifndef lint
static	char sccsid[] = "@(#)nm.c 1.1 92/07/30 SMI"; /* from UCB 4.6 1/22/85 */
#endif

/*
 * nm - print name list; string table version
 */
#include <sys/types.h>
#include <ar.h>
#include <stdio.h>
#include <ctype.h>
#include <a.out.h>
#include <stab.h>
#include <sys/stat.h>

#define	SELECT	archive ? archdr.ar_name : *xargv

int	aflg, gflg, nflg, oflg, pflg, sflg, uflg; 
int	rflg = 1;
char	**xargv;
int	archive;
struct	ar_hdr	archdr;
union {
	char	mag_armag[SARMAG+1];
	struct	exec mag_exp;
} mag_un;
#define	OARMAG	0177545
FILE	*fi;
off_t	off;
off_t	ftell();
char	*malloc();
char	*realloc();
char	*strp;
char	*stab();
off_t	strsiz;
int	ncompare();
int	scompare();
int	compare();
int	narg;
int	errs;

main(argc, argv)
char **argv;
{

	if (--argc>0 && argv[1][0]=='-' && argv[1][1]!=0) {
		argv++;
		while (*++*argv) switch (**argv) {

		case 'n':
			nflg++;
			continue;
		case 'g':
			gflg++;
			continue;
		case 'u':
			uflg++;
			continue;
		case 'r':
			rflg = -1;
			continue;
		case 'p':
			pflg++;
			continue;
		case 'o':
			oflg++;
			continue;
		case 'a':
			aflg++;
			continue;
		case 's':
			sflg++;
			continue;
		default:
			fprintf(stderr, "nm: invalid argument -%c\n",
			    *argv[0]);
			exit(2);
		}
		argc--;
	}
	if (argc == 0) {
		argc = 1;
		argv[1] = "a.out";
	}
	narg = argc;
	xargv = argv;
	while (argc--) {
		++xargv;
		namelist();
	}
	exit(errs);
	/* NOTREACHED */
}

namelist()
{
	register int j;

	archive = 0;
	fi = fopen(*xargv, "r");
	if (fi == NULL) {
		error(0, "cannot open");
		return;
	}
	off = SARMAG;
	fread((char *)&mag_un, 1, sizeof(mag_un), fi);
	if (mag_un.mag_exp.a_magic == OARMAG) {
		error(0, "old archive");
		goto out;
	}
	if (strncmp(mag_un.mag_armag, ARMAG, SARMAG)==0)
		archive++;
	else if (N_BADMAG(mag_un.mag_exp)) {
		error(0, "bad format");
		goto out;
	}
	fseek(fi, 0L, 0);
	if (archive) {
		nextel(fi);
		if (narg > 1)
			printf("\n%s:\n", *xargv);
	}
	do {
		off_t o;
		register i, n, c;
		struct nlist *symp = NULL;
		struct nlist sym;
		struct stat stb;

		fread((char *)&mag_un.mag_exp, 1, sizeof(struct exec), fi);
		if (N_BADMAG(mag_un.mag_exp))
			continue;
		if (archive == 0)
			fstat(fileno(fi), &stb);
		o = N_SYMOFF(mag_un.mag_exp) - sizeof (struct exec);
		fseek(fi, o, 1);
		n = mag_un.mag_exp.a_syms / sizeof(struct nlist);
		if (n == 0) {
			error(0, "no name list");
			continue;
		}
		if (N_STROFF(mag_un.mag_exp) + sizeof (off_t) >
		    (archive ? off : stb.st_size)) {
			error(0, "old format .o (no string table) or truncated file");
			continue;
		}
		i = 0;
		if (strp)
			free(strp), strp = 0;
		while (--n >= 0) {
			fread((char *)&sym, 1, sizeof(sym), fi);
			if (gflg && (sym.n_type&N_EXT)==0)
				continue;
			if ((sym.n_type&N_STAB) && (!aflg||gflg||uflg))
				continue;
			if (sflg
			    && (sym.n_type&N_TYPE) == N_ABS
			     || (sym.n_type&N_TYPE) == N_FN)
				continue;
			if (symp==NULL)
				symp = (struct nlist *)
				    malloc(sizeof(struct nlist));
			else
				symp = (struct nlist *)
				    realloc(symp,
					(i+1)*sizeof(struct nlist));
			if (symp == NULL)
				error(1, "out of memory");
			symp[i++] = sym;
		}
		if (archive && ftell(fi)+sizeof(off_t) >= off) {
			error(0, "no string table (old format .o?)");
			continue;
		}
		if (fread((char *)&strsiz,sizeof(strsiz),1,fi) != 1) {
			error(0, "no string table (old format .o?)");
			goto out;
		}
		strp = (char *)malloc(strsiz);
		if (strp == NULL)
			error(1, "ran out of memory");
		if (fread(strp+sizeof(strsiz),strsiz-sizeof(strsiz),1,fi) != 1)
			error(1, "error reading string table");
		for (j = 0; j < i; j++)
			if (symp[j].n_un.n_strx)
				symp[j].n_un.n_name =
				    symp[j].n_un.n_strx + strp;
			else
				symp[j].n_un.n_name = "";
		if (pflg==0) {
			if (nflg)
				qsort(symp, i, sizeof(struct nlist), ncompare);
			else if (sflg)
				qsort(symp, i, sizeof(struct nlist), scompare);
			else
				qsort(symp, i, sizeof(struct nlist), compare);
		}
		if ((archive || narg>1) && oflg==0)
			printf("\n%s:\n", SELECT);
		if (sflg) {
			dosizes(symp, i);
			/*
			 * Now that we've computed the size, sort them by size.
			 */
			qsort(symp, i, sizeof(struct nlist), ncompare);
		}
		psyms(symp, i);
		if (symp)
			free((char *)symp), symp = 0;
		if (strp)
			free((char *)strp), strp = 0;
	} while(archive && nextel(fi));
out:
	fclose(fi);
}

dosizes(symp, nsyms)
	register struct nlist *symp;
	int nsyms;
{
	register int i;
	register struct nlist *lastsymp;
	register struct nlist *nextsymp;

	lastsymp = symp + nsyms;
	nextsymp = symp + 1;
	while (nextsymp < lastsymp) {
		if (nextsymp->n_type == N_TEXT) {
			/*
			 * Sigh, 32V braindamage strikes again.
			 * Skip over those stupid file name symbols
			 * that aren't flagged as N_FN!
			 */
			i = strlen(nextsymp->n_un.n_name) - 2;
			if (i >= 0 && nextsymp->n_un.n_name[i] == '.'
			    && nextsymp->n_un.n_name[i+1] == 'o') {
				nextsymp->n_value = 0;	/* don't print */
				nextsymp++;
				continue;
			}
		}
		if ((symp->n_type&N_TYPE) == N_TEXT &&
		    (nextsymp->n_type&N_TYPE) != N_TEXT) {
			/*
			 * Stopper at end of text symbols.
			 */
			symp->n_value = 0;	/* don't print me */
		} else
			symp->n_value = nextsymp->n_value - symp->n_value;
		symp = nextsymp;
		nextsymp++;
	}
	(lastsymp-1)->n_value = 0;	/* don't print last one either */
}

psyms(symp, nsyms)
	register struct nlist *symp;
	int nsyms;
{
	register int n, c;

	for (n=0; n<nsyms; n++) {
		if (sflg && symp[n].n_value == 0)
			continue;
		c = symp[n].n_type;
		if (c & N_STAB) {
			if (oflg) {
				if (archive)
					printf("%s:", *xargv);
				printf("%s:", SELECT);
			}
			printf("%08x - %02x %04x %5.5s %s\n",
			    symp[n].n_value,
			    symp[n].n_other & 0xff, symp[n].n_desc & 0xffff,
			    stab(symp[n].n_type & 0xff),
			    symp[n].n_un.n_name);
			continue;
		}
		if (c == N_FN)
			c = 'f';
		else switch (c&N_TYPE) {

		case N_UNDF:
			c = 'u';
			if (symp[n].n_value)
				c = 'c';
			break;
		case N_ABS:
			c = 'a';
			break;
		case N_TEXT:
			c = 't';
			break;
		case N_DATA:
			c = 'd';
			break;
		case N_BSS:
			c = 'b';
			break;
		default:
			c = '?';
			break;
		}
		if (uflg && c!='u')
			continue;
		if (oflg) {
			if (archive)
				printf("%s:", *xargv);
			printf("%s:", SELECT);
		}
		if (symp[n].n_type&N_EXT)
			c = toupper(c);
		if (!uflg) {
			if (c=='u' || c=='U')
				printf("        ");
			else
				printf(N_FORMAT, symp[n].n_value);
			printf(" %c ", c);
		}
		printf("%s\n", symp[n].n_un.n_name);
l1:		;
	}
}

/*
 * Compare for numerical ordering.
 */
ncompare(p1, p2)
register struct nlist *p1, *p2;
{
	register int i;

	if (p1->n_value > p2->n_value)
		return(rflg);
	if (p1->n_value < p2->n_value)
		return(-rflg);
	i = strcmp(p1->n_un.n_name, p2->n_un.n_name);
	return(rflg < 0 ? -i : i);
}

/*
 * Compare for by-size ordering; sorts symbols by address, but with text
 * symbols preceding data symbols.  Ignore the sort order for now.
 */
scompare(p1, p2)
register struct nlist *p1, *p2;
{
	register int type1, type2;
	register int i;

	type1 = p1->n_type&N_TYPE;
	type2 = p2->n_type&N_TYPE;
	if (type1 == N_TEXT) {
		if (type2 != N_TEXT)
			return(-1);
	} else if (type2 == N_TEXT) {
		if (type1 != N_TEXT)
			return(1);
	}
	if (p1->n_value > p2->n_value)
		return(1);
	if (p1->n_value < p2->n_value)
		return(-1);
	return(strcmp(p1->n_un.n_name, p2->n_un.n_name));
}

/*
 * Compare for ordering by name.
 */
compare(p1, p2)
struct nlist *p1, *p2;
{
	register int i;

	i = strcmp(p1->n_un.n_name, p2->n_un.n_name);
	return(rflg < 0 ? -i : i);
}

nextel(af)
FILE *af;
{
	register char *cp;
	register r;
	long arsize;

	fseek(af, off, 0);
	r = fread((char *)&archdr, 1, sizeof(struct ar_hdr), af);
	if (r != sizeof(struct ar_hdr))
		return(0);
	for (cp = archdr.ar_name; cp < &archdr.ar_name[sizeof(archdr.ar_name)]; cp++)
		if (*cp == ' ')
			*cp = '\0';
	arsize = atol(archdr.ar_size);
	if (arsize & 1)
		++arsize;
	off = ftell(af) + arsize;	/* beginning of next element */
	return(1);
}

error(n, s)
char *s;
{
	fprintf(stderr, "nm: %s:", *xargv);
	if (archive) {
		fprintf(stderr, "(%s)", archdr.ar_name);
		fprintf(stderr, ": ");
	} else
		fprintf(stderr, " ");
	fprintf(stderr, "%s\n", s);
	if (n)
		exit(2);
	errs = 1;
}

#define	N_BROWS	0x48		/* Zap after 4.1 <stab.h> is installed */

struct	stabnames {
	int	st_value;
	char	*st_name;
} stabnames[] ={
	N_GSYM, "GSYM",
	N_FNAME, "FNAME",
	N_FUN, "FUN",
	N_STSYM, "STSYM",
	N_LCSYM, "LCSYM",
	N_MAIN, "MAIN",
	N_RSYM, "RSYM",
	N_SLINE, "SLINE",
	N_SSYM, "SSYM",
	N_SO, "SO",
	N_LSYM, "LSYM",
	N_BINCL, "BINCL",
	N_SOL, "SOL",
	N_PSYM, "PSYM",
	N_ENTRY, "ENTRY",
	N_LBRAC, "LBRAC",
	N_RBRAC, "RBRAC",
	N_EXCL, "EXCL",
	N_BCOMM, "BCOMM",
	N_ECOMM, "ECOMM",
	N_ECOML, "ECOML",
	N_LENG, "LENG",
	N_EINCL, "EINCL",
	N_PC, "PC",
	N_M2C, "M2C",
	N_SCOPE, "SCOPE",
	N_BROWS, "BROWS",
	0, 0
};

char *
stab(val)
{
	register struct stabnames *sp;
	static char prbuf[32];

	for (sp = stabnames; sp->st_name; sp++)
		if (sp->st_value == val)
			return (sp->st_name);
	sprintf(prbuf, "%02x", val);
	return (prbuf);
}
