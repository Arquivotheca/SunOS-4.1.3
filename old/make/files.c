#ifndef lint
static	char sccsid[] = "@(#)files.c 1.1 92/08/05 SMI"; /* from S5R2 1.3 03/28/83 */
#endif

#include "defs"
#include <sys/stat.h>
#include <pwd.h>
#include <ar.h>
#ifdef BSD
#include <ranlib.h>

long	sgetl();
#endif
/* UNIX DEPENDENT PROCEDURES */

#define equaln		!strncmp

/*
* For 6.0, create a make which can understand all three archive
* formats.  This is kind of tricky, and <ar.h> isn't any help.
* Note that there is no sgetl() and sputl() on the pdp11, so
* make cannot handle anything but the one format there.
*/
char	archmem[64];		/* archive file member name to search for */
char	archname[64];		/* archive file to be opened */

int	ar_type;	/* to distiguish which archive format we have */
#define ARpdp	1
#define AR5	2
#define ARport	3

long	first_ar_mem;	/* where first archive member header is at */
long	sym_begin;	/* where the symbol lookup starts */
long	num_symbols;	/* the number of symbols available */
long	sym_size;	/* length of symbol directory file */

/*
* Defines for all the different archive formats.  See next comment
* block for justification for not using <ar.h>'s versions.
*/
#define ARpdpMAG	0177545		/* old format (short) magic number */

#define AR5MAG		"<ar>"		/* 5.0 format magic string */
#define SAR5MAG		4		/* 5.0 format magic string length */

#define ARportMAG	"!<arch>\n"	/* Port. (6.0) magic string */
#define SARportMAG	8		/* Port. (6.0) magic string length */
#define ARFportMAG	"`\n"		/* Port. (6.0) end of header string */

/*
* These are the archive file headers for the three formats.  Note
* that it really doesn't matter if these structures are defined
* here.  They are correct as of the respective archive format
* releases.  If the archive format is changed, then since backwards
* compatability is the desired behavior, a new structure is added
* to the list.
*/
struct		/* pdp11 -- old archive format */
{
	char	ar_name[14];	/* '\0' terminated */
	long	ar_date;	/* native machine bit representation */
	char	ar_uid;		/* 	"	*/
	char	ar_gid;		/* 	"	*/
	int	ar_mode;	/* 	"	*/
	long	ar_size;	/* 	"	*/
} ar_pdp;

struct		/* pdp11 a.out header */
{
	short		a_magic;
	unsigned	a_text;
	unsigned	a_data;
	unsigned	a_bss;
	unsigned	a_syms;		/* length of symbol table */
	unsigned	a_entry;
	char		a_unused;
	char		a_hitext;
	char		a_flag;
	char		a_stamp;
} arobj_pdp;

struct		/* pdp11 a.out symbol table entry */
{
	char		n_name[8];	/* null-terminated name */
	int		n_type;
	unsigned	n_value;
} ars_pdp;

struct		/* UNIX 5.0 archive header format: vax family; 3b family */
{
	char	ar_magic[SAR5MAG];	/* AR5MAG */
	char	ar_name[16];		/* ' ' terminated */
	char	ar_date[4];		/* sgetl() accessed */
	char	ar_syms[4];		/* sgetl() accessed */
} arh_5;

struct		/* UNIX 5.0 archive symbol format: vax family; 3b family */
{
	char	sym_name[8];	/* ' ' terminated */
	char	sym_ptr[4];	/* sgetl() accessed */
} ars_5;

struct		/* UNIX 5.0 archive member format: vax family; 3b family */
{
	char	arf_name[16];	/* ' ' terminated */
	char	arf_date[4];	/* sgetl() accessed */
	char	arf_uid[4];	/*	"	*/
	char	arf_gid[4];	/*	"	*/
	char	arf_mode[4];	/*	"	*/
	char	arf_size[4];	/*	"	*/
} arf_5;

struct		/* Portable (6.0) archive format: vax family; 3b family */
{
#ifdef BSD
	char	ar_name[16];	/* ' ' terminated */
#else
	char	ar_name[16];	/* '/' terminated */
#endif
	char	ar_date[12];	/* left-adjusted; decimal ascii; blank filled */
	char	ar_uid[6];	/*	"	*/
	char	ar_gid[6];	/*	"	*/
	char	ar_mode[8];	/* left-adjusted; octal ascii; blank filled */
	char	ar_size[10];	/* left-adjusted; decimal ascii; blank filled */
	char	ar_fmag[2];	/* special end-of-header string (ARFportMAG) */
} ar_port;


#if 0		/* no longer used! */
	/*
	 *	New common object version of files.c
	 */
	char		archmem[64];
	char		archname[64];		/* name of archive library */
	struct ar_hdr	head;		/* archive file header */
	struct ar_sym	symbol;		/* archive file symbol table entry */
	struct arf_hdr	fhead;		/* archive file object file header */
#endif

TIMETYPE	afilescan();
TIMETYPE	entryscan();
TIMETYPE	pdpentrys();
FILE		*arfd;
char		BADAR[] = "BAD ARCHIVE";


#include <errno.h>

extern int errno;

TIMETYPE
exists(pname)
NAMEBLOCK pname;
{
	register CHARSTAR s;
	struct stat buf;
	TIMETYPE lookarch();
	CHARSTAR filename;
	static char fname[MAXPATHLEN+1];

	filename = pname->namep;

	if(any(filename, LPAREN))
		return(lookarch(filename));

	if(stat(filename,&buf) < 0) 
	{
		if(errno != ENOENT) 
			return(0);	/* file exists but is inaccessible */
		s = findfl(filename, &buf);
		if(s != NULL)
		{
			pname->alias = copys(s);
			return(buf.st_mtime);
		}
		return(0);
	}
	else
		return(buf.st_mtime);
}


TIMETYPE
prestime()
{
	extern long time();
	TIMETYPE t;
	(void)time(&t);
	return(t);
}



DEPBLOCK
srchdir(pat, mkchain)
register CHARSTAR pat;		/* pattern to be matched in directory */
int mkchain;			/* nonzero if results to be remembered */
{
	DIR * dirf;
	CHARSTAR dirname, dirpref, endir, filepat, p;
	char temp[85+MAXNAMLEN];
	char fullname[85+MAXNAMLEN];
	NAMEBLOCK q;
	DEPBLOCK thisdbl = NULL;
	DEPBLOCK nextdbl = NULL;
	OPENDIR od;
	int dirofl = 0;
	static opendirs = 0;
	PATTERN patp;
	VARBLOCK cp;
	CHARSTAR path, strcpy(), strcat();
	char pth[BUFSIZ];

	struct direct *entry;


	if(mkchain == NO && hashpat(pat))
		return (0);

	endir = 0;

	for(p=pat; *p!=CNULL; ++p)
		if(*p==SLASH)
			endir = p;

	if(endir==0)
	{
		dirpref = "";
		filepat = pat;
		cp = varptr("VPATH");
		if(*cp->varval == 0)
			path = ".";
		else
		{
			path = pth;
			*path = CNULL;
			if(strncmp(cp->varval, ".:", 2) != 0)
				(void) strcpy(pth, ".:");
			(void) strcat(pth, cp->varval);
		}
	}
	else
	{
		*endir = CNULL;
		path = strcpy(pth, pat);
		dirpref = concat(pat, "/", temp);
		filepat = endir+1;
	}

	while(*path)
	{
		dirname = path;
		for(; *path; path++)
			if(*path == KOLON)
			{
				*path++ = CNULL;
				break;
			}
		if(mkchain == NO && hashdir(dirname))
			goto out;

		dirf = NULL;

		for(od=firstod ; od!=0; od = od->nextopendir)
			if(equal(dirname, od->dirn))
			{
				dirf = od->dirfc;
				rewinddir(dirf); /* start over at the beginning  */
				break;
			}

		if(dirf == NULL)
		{
			dirf = opendir(dirname);
			if(dirf == NULL)
			{
#ifdef SCCSDIR
				register int dnamlen = strlen(dirname);

				if ((dnamlen == 4 && strcmp(dirname, "SCCS") == 0)
				    || (dnamlen > 4 && strcmp(dirname + dnamlen - 5, "/SCCS") == 0))
					goto out;
#endif
				(void)fprintf(stderr, "Directory %s: ", dirname);
				fatal("Cannot open");
			}
			if(++opendirs < MAXODIR)
			{
				od = ALLOC(opendirectory);
				od->nextopendir = firstod;
				firstod = od;
				od->dirfc = dirf;
				od->dirn = copys(dirname);
			}
			else
				dirofl = 1;
		}

		while ((entry = readdir(dirf)) != NULL)
		{
			(void)concat(dirpref,entry->d_name,fullname);
			if( (q=srchname(fullname)) ==0)
				q = makename(copys(fullname));
			if(mkchain && amatch(entry->d_name,filepat) )
			{
				thisdbl = ALLOC(depblock);
				thisdbl->nextdep = nextdbl;
				thisdbl->depname = q;
				nextdbl = thisdbl;
			}
		}

	out:
		if(endir != 0)
			*endir = SLASH;
		if(dirofl)
			closedir(dirf);
	}

	return(thisdbl);
}

#define	DIR_HASHSIZE	16
FSTATIC struct dirtabent {
	struct dirtabent *nextdir;
	CHARSTAR	dirval;
	time_t		mtime;
	short		flush;
}  *dirtab[DIR_HASHSIZE]; 

hashdir(dir)
	CHARSTAR dir;
{
	register CHARSTAR p;
	register struct dirtabent *dirp;
	unsigned short val;
	struct stat s;
	int ok;

	for (val=0, p=dir; *p;)
		val += *p++;
	val %= DIR_HASHSIZE;
	dirp = dirtab[val];
	while (dirp) {
		if (equal(dirp->dirval, dir)) {
			if (dirp->flush) {
				dirp->flush = 0;
				s.st_mtime = 0;
				stat(dir, &s);
				ok = (dirp->mtime == s.st_mtime);
				dirp->mtime = s.st_mtime;
				return (ok);
			}
			return (1);
		}
		dirp = dirp->nextdir;
	}
	dirp = ALLOC(dirtabent);
	dirp->nextdir = dirtab[val];
	dirtab[val] = dirp;
	dirp->dirval = copys(dir);
	dirp->flush = 0;
	s.st_mtime = 0;
	stat(dir, &s);
	dirp->mtime = s.st_mtime;
	return (0);
}

dirflush()
{
	register struct dirtabent *dirp;
	int i;

	for (i = 0; i < DIR_HASHSIZE; i++) {
		dirp = dirtab[i];
		while (dirp) {
			dirp->flush = 1;
			dirp = dirp->nextdir;
		}
	}
}

#define	PAT_HASHSIZE	64
FSTATIC PATTERN pattab[PAT_HASHSIZE]; 

hashpat(pat)
	CHARSTAR pat;
{
	register CHARSTAR p;
	register PATTERN patp;
	unsigned short val;

	for (val=0, p=pat; *p;)
		val += *p++;
	val %= PAT_HASHSIZE;
	patp = pattab[val];
	while (patp) {
		if (equal(patp->patval, pat))
			return (1);
		patp = patp->nextpattern;
	}
	patp = ALLOC(pattern);
	patp->nextpattern = pattab[val];
	pattab[val] = patp;
	patp->patval = copys(pat);
	return (0);
}

/* stolen from glob through find */

amatch(s, p)
register CHARSTAR s, p;
{
	register int cc, scc, k;
	int c, lc;

top:
	scc = *s;
	lc = 077777;
	switch (c = *p)
	{

	case LSQUAR:
		k = 0;
		while (cc = *++p)
		{
			switch (cc)
			{

			case RSQUAR:
				if (k)
					return(amatch(++s, ++p));
				else
					return(0);

			case MINUS:
				k |= lc <= scc && scc <= (cc=p[1]);
			}
			if(scc==(lc=cc))
				k++;
		}
		return(0);

	case QUESTN:
	caseq:
		if(scc == 0)
			return(0);
		p++, s++;
		goto top;

	case STAR:
		return(umatch(s, ++p));
	case 0:
		return(!scc);
	}
	if(c!=scc)
		return(0);
	p++, s++;
	goto top;
}

umatch(s, p)
register CHARSTAR s, p;
{
	register c;

	switch (*p) {
	case 0:
		return (1);

	case LSQUAR:
	case QUESTN:
	case STAR:
		while(*s)
			if(amatch(s++,p))
				return(1);
		break;

	default:
		c = *p++;
		while(*s)
			if (*s++ == c && amatch(s, p))
				return (1);
		break;
	}
	return(0);
}


/* look inside archives for notations a(b) and a((b))
	a(b)	is file member   b   in archive a
	a((b))	is entry point   b  in object archive a
*/


#ifdef BSD
#define	ARNLEN	15		/* max number of characters in name */
#else
#define	ARNLEN	14		/* max number of characters in name */
#endif

TIMETYPE
lookarch(filename)
register CHARSTAR filename;
{
	register int i;
	CHARSTAR p, q;
	extern CHARSTAR strncpy();
	char s[ARNLEN+1];
	int objarch;
	TIMETYPE la();

	for(p = filename; *p!= LPAREN ; ++p);
	i = p - filename;
	(void)strncpy(archname, filename, i);
	archname[i] = CNULL;
	if(archname[0] == CNULL)
		fatal1("Null archive name `%s'", filename);
	p++;
	if(*p == LPAREN)
	{
		objarch = YES;
		++p;
		if((q = strchr(p, RPAREN)) == NULL)
			q = p + strlen(p);
		i = q - p;
		if(i > ARNLEN)
			i = ARNLEN;
	}
	else
	{
		objarch = NO;
		if((q = strchr(p, RPAREN)) == NULL)
			q = p + strlen(p);
		i = q - p;
		(void)strncpy(archmem, p, i);
		archmem[i] = CNULL;
		if(archmem[0] == CNULL)
			fatal1("Null archive member name `%s'", filename);
		if(p = strrchr(archmem, SLASH))
			++p;
		else
			p = archmem;
		i = ARNLEN;
	}
	(void)strncpy(s, p, i);
	s[i] = CNULL;
	return(la(s, objarch));
}
TIMETYPE
la(am,flag)
register char *am;
register int flag;
{
	TIMETYPE date = 0L;

	if(openarch(archname) == -1)
		return(0L);
	if(flag)
		date = entryscan(am);	/* fatals if cannot find entry */
	else
		date = afilescan(am);
	clarch();
	return(date);
}

TIMETYPE
afilescan(name)		/* return date for named archive member file */
char *name;
{
	int	len = strlen(name);
	long	ptr;

	if (fseek(arfd, first_ar_mem, 0) != 0)
	{
	seek_error:;
		fatal1("Seek error on archive file %s", archname);
	}
	/*
	* Hunt for the named file in each different type of
	* archive format.
	*/
	switch (ar_type)
	{
	case ARpdp:
		for (;;)
		{
			if (fread((char *)&ar_pdp,
				sizeof(ar_pdp), 1, arfd) != 1)
			{
				if (feof(arfd))
					return (0L);
				break;
			}
			if ((len == sizeof (ar_pdp.ar_name) ||
				ar_pdp.ar_name[len] == '\0') &&
				equaln(ar_pdp.ar_name, name, len))
				return (ar_pdp.ar_date);
			ptr = ar_pdp.ar_size;
			ptr += (ptr & 01);
			if (fseek(arfd, ptr, 1) != 0)
				goto seek_error;
		}
		break;
#ifndef pdp11
	case AR5:
		for (;;)
		{
			if (fread((char *)&arf_5, sizeof(arf_5), 1, arfd) != 1)
			{
				if (feof(arfd))
					return (0L);
				break;
			}
			if ((len == sizeof (arf_5.arf_name) ||
				arf_5.arf_name[len] == ' ' ||
				arf_5.arf_name[len] == '\0') &&
				equaln(arf_5.arf_name, name, len))
				return (sgetl(arf_5.arf_date));
			ptr = sgetl(arf_5.arf_size);
			ptr += (ptr & 01);
			if (fseek(arfd, ptr, 1) != 0)
				goto seek_error;
		}
		break;
	case ARport:
		for (;;)
		{
			if (fread((char *)&ar_port, sizeof(ar_port), 1,
				arfd) != 1 || !equaln(ar_port.ar_fmag,
				ARFportMAG, sizeof(ar_port.ar_fmag)))
			{
				if (feof(arfd))
					return (0L);
				break;
			}
			if ((len == sizeof (ar_port.ar_name) ||
				ar_port.ar_name[len] == ' ' ||
				ar_port.ar_name[len] == '/' ||
				ar_port.ar_name[len] == '\0') &&
				equaln(ar_port.ar_name, name, len))
			{
				long date;

				if (sscanf(ar_port.ar_date, "%ld", &date) != 1)
				{
					fatal1("Bad date field for %.*s in %s",
						ARNLEN, name, archname);
				}
				return (date);
			}
			if (sscanf(ar_port.ar_size, "%ld", &ptr) != 1)
			{
				fatal1("Bad size field for %.*s in archive %s",
					ARNLEN, name, archname);
			}
			ptr += (ptr & 01);
			if (fseek(arfd, ptr, 1) != 0)
				goto seek_error;
		}
		break;
#endif
	}
	/*
	* Only here if fread() [or equaln()] failed and not at EOF
	*/
	fatal1("Read error on archive %s", archname);
	/*NOTREACHED*/
#if 0
	long date;
	long nsyms;
	long ptr;

	nsyms = sgetl(head.ar_syms);
	if(fseek(arfd, (long)( nsyms*sizeof(symbol)+sizeof(head) ), 0) == -1)
		fatal("Seek error on archive file %s", archname);
	for(;;)
	{
		if(fread(&fhead, sizeof(fhead), 1, arfd) != 1)
			if(feof(arfd))
				break;
			else
				fatal("Read error on archive %s", archname);
		if(equaln(fhead.arf_name, name, 14))
		{
			date = sgetl(fhead.arf_date);
			return(date);
		}
		ptr = sgetl(fhead.arf_size);
		ptr = (ptr+1)&(~1);
		fseek(arfd, ptr, 1);
	}
	return( 0L );
#endif
}

TIMETYPE
entryscan(name)		/* return date of member containing global var named */
char *name;
{
	/*
	* Hunt through each different archive format for the named
	* symbol.  Note that the old archive format does not support
	* this convention since there is no symbol directory to
	* scan through for all defined global variables.
	*/
	if (ar_type == ARpdp)
		return (pdpentrys(name));
	if (sym_begin == 0L || num_symbols == 0L)
	{
	no_such_sym:;
		fatal1("Cannot find symbol %s in archive %s", name, archname);
	}
	if (fseek(arfd, sym_begin, 0) != 0)
	{
	seek_error:;
		fatal1("Seek error on archive file %s", archname);
	}
#ifndef pdp11
	if (ar_type == AR5)
	{
		register int i;
		int len = strlen(name) + 1;

		if (len > 8)
			len = 8;
		for (i = 0; i < num_symbols; i++)
		{
			if (fread((char *)&ars_5, sizeof(ars_5), 1, arfd) != 1)
			{
			read_error:;
				fatal1("Read error on archive %s", archname);
			}
			if (equaln(ars_5.sym_name, name, len))
			{
				if (fseek(arfd, sgetl(ars_5.sym_ptr), 0) != 0)
					goto seek_error;
				if (fread((char *)&arf_5,
					sizeof(arf_5), 1, arfd) != 1)
				{
					goto read_error;
				}
				/* replace symbol name with member name */
				strncpy(archmem, arf_5.arf_name,
					sizeof (arf_5.arf_name));
				return (sgetl(arf_5.arf_date));
			}
		}
	}
	else	/* ar_type == ARport */
	{
		extern char *malloc();
		int strtablen;
#ifdef BSD
		register struct ranlib *offs;
#else
		register char *offs;	/* offsets table */
#endif
		register char *syms;	/* string table */
#ifdef BSD
		register struct ranlib *offend;	/* end of offsets table */
#else
		register char *strend;	/* end of string table */
#endif
		char *strbeg;

		/*
		* Format of the symbol directory for this format is
#ifdef BSD
		* as follows:	[sputl()d number_of_symbols * sizeof(struct ranlib)]
#else
		* as follows:	[sputl()d number_of_symbols]
#endif
		*		[sputl()d first_symbol_offset]
#ifdef BSD
		*		[sputl()d first_string_table_offset]
#endif
		*			...
		*		[sputl()d number_of_symbols'_offset]
#ifdef BSD
		*		[sputl()d last_string_table_offset]
#endif
		*		[null_terminated_string_table_of_symbols]
		*/
#ifdef BSD
		/* have already read the num_symbols info */
		if ((offs = (struct ranlib *)malloc(num_symbols * sizeof(struct ranlib))) == NULL)
#else
		if ((offs = malloc(num_symbols * sizeof(long))) == NULL)
#endif
		{
			fatal1("Cannot alloc offset table for archive %s",
				archname);
		}
#ifdef BSD
		if (fread(offs, sizeof(struct ranlib), num_symbols, arfd)
								!= num_symbols)
#else
		if (fread(offs, sizeof(long), num_symbols, arfd) != num_symbols)
#endif
			goto read_error;
#ifdef BSD
		if (fread(&strtablen, sizeof(int), 1, arfd) != 1)
			goto read_error;
#else
		strtablen = sym_size - ((num_symbols + 1) * sizeof(long));
#endif
		if ((syms = malloc(strtablen)) == NULL)
		{
			fatal1("Cannot alloc string table for archive %s",
				archname);
		}
		if (fread(syms, sizeof(char), strtablen, arfd) != strtablen)
			goto read_error;
#ifdef BSD
		offend = &offs[num_symbols];
#else
		strend = &syms[strtablen];
#endif
		strbeg = syms;
#ifdef BSD
		while (offs < offend )
#else
		while (syms < strend)
#endif
		{
#ifdef BSD
			if (strcmp(&syms[offs->ran_un.ran_strx], name) == 0)
#else
			if (equal(syms, name))
#endif
			{
				long ptr, date;
				register CHARSTAR ap, hp;

#ifdef BSD
				ptr = offs->ran_off;
#else
				ptr = sgetl(offs);
#endif
				if (fseek(arfd, ptr, 0) != 0)
					goto seek_error;
				if (fread((char *)&ar_port, sizeof(ar_port), 1,
					arfd) != 1 || !equaln(ar_port.ar_fmag,
					ARFportMAG, sizeof(ar_port.ar_fmag)))
				{
					goto read_error;
				}
				if (sscanf(ar_port.ar_date, "%ld", &date) != 1)
				{
					fatal1("Bad date for %.*s, archive %s",
						ARNLEN, ar_port.ar_name, archname);
				}
				/* replace symbol name with member name */	
				ap = archmem;
				hp = ar_port.ar_name;
				while (*hp && *hp != '/' &&
					ap < &archmem[sizeof (archmem)])
				{
					*ap++ = *hp++;
				}
				free(strbeg);
				return (date);
			}
#ifdef BSD
			offs += sizeof(struct ranlib);
#else
			syms += strlen(syms) + 1;
			offs += sizeof(long);
#endif
		}
		free(strbeg);
	}
#endif
	goto no_such_sym;
#if 0
	register int i;
	long date;
	long ptr;
	long nsyms;

	nsyms = sgetl(head.ar_syms);
	for(i = 0; i < nsyms; i++)
	{
		if(fread(&symbol, sizeof(symbol), 1, arfd) != 1)
badread:
			fatal("Read error on archive %s", archname);
		if(equaln(symbol.sym_name, name, 8))
		{
			ptr = sgetl(symbol.sym_ptr);
			if(fseek(arfd, ptr, 0) == -1)
				fatal("Seek error on archive file %s",archname);
			if(fread(fhead, sizeof(fhead), 1, arfd) != 1)
				goto badread;
			date = sgetl(fhead.arf_date);
			return(date);
		}
	}
	fatal("Cannot find symbol %s in archive %s", name, archname);
#endif
}

TIMETYPE
pdpentrys(name)
	char *name;
{
	long	skip;
	long	last;
	int	len;
	register int i;

#ifndef pdp11
	fatal("Cannot do global variable search in pdp11 or old object file.");
	/*NOTREACHED*/
#else
	len = strlen(name);
	if (len > 8)
		len = 8;
	/*
	* Look through archive, an object file entry at a time.  For each
	* object file, jump to its symbol table and check each external
	* symbol for a match.  If found, return the date of the module
	* containing the symbol.
	*/
	if (fseek(arfd, sizeof(short), 0) != 0)
	{
	seek_error:;
		fatal1("Cannot seek on archive file %s", archname);
	}
	while (fread((char *)&ar_pdp, sizeof(ar_pdp), 1, arfd) == 1)
	{
		last = ftell(arfd);
		if (ar_pdp.ar_size < sizeof(arobj_pdp) ||
			fread((char *)&arobj_pdp, sizeof(arobj_pdp), 1, arfd)
			!= 1 ||
			(arobj_pdp.a_magic != 0401 &&	/* A_MAGIC0 */
			arobj_pdp.a_magic != 0407 &&	/* A_MAGIC1 */
			arobj_pdp.a_magic != 0410 &&	/* A_MAGIC2 */
			arobj_pdp.a_magic != 0411 &&	/* A_MAGIC3 */
			arobj_pdp.a_magic != 0405 &&	/* A_MAGIC4 */
			arobj_pdp.a_magic != 0437)) 	/* A_MAGIC5 */
		{
			fatal1("%s is not an object module", ar_pdp.ar_name);
		}
		skip = arobj_pdp.a_text + arobj_pdp.a_data;
		if (!arobj_pdp.a_flag)
			skip *= 2;
		if (skip >= ar_pdp.ar_size || fseek(arfd, skip, 1) != 0)
			goto seek_error;
		skip = ar_pdp.ar_size;
		skip += (skip & 01) + last;
		i = (arobj_pdp.a_syms / sizeof(ars_pdp)) + 1;
		while (--i > 0)		/* look through symbol table */
		{
			if (fread((char *)&ars_pdp, sizeof(ars_pdp), 1, arfd)
				!= 1)
			{
				fatal1("Read error on archive %s", archname);
			}
			if ((ars_pdp.n_type & 040)	/* N_EXT for pdp11 */
				&& equaln(ars_pdp.n_name, name, len))
			{
				(void)strncpy(archmem, ar_pdp.ar_name, ARNLEN);
				archmem[ARNLEN] = '\0';
				return (ar_pdp.ar_date);
			}
		}
		if (fseek(arfd, skip, 0) != 0)
			goto seek_error;
	}
	return (0L);
#endif
}


openarch(f)
register CHARSTAR f;
{
	unsigned short	mag_pdp;		/* old archive format */
	char		mag_5[SAR5MAG];		/* 5.0 archive format */
	char		mag_port[SARportMAG];	/* port (6.0) archive format */

	arfd = fopen(f, "r");
	if(arfd == NULL)
		return(-1);
	/*
	* More code for three archive formats.  Read in just enough to
	* distinguish the three types and set ar_type.  Then if it is
	* one of the newer archive formats, gather more info.
	*/
	if (fread((char *)&mag_pdp, sizeof(mag_pdp), 1, arfd) != 1)
		return (-1);
	if (mag_pdp == (unsigned short)ARpdpMAG)
	{
		ar_type = ARpdp;
		first_ar_mem = ftell(arfd);
		sym_begin = num_symbols = sym_size = 0L;
		return (0);
	}
	if (fseek(arfd, 0L, 0) != 0 || fread(mag_5, SAR5MAG, 1, arfd) != 1)
		return (-1);
	if (equaln(mag_5, AR5MAG, SAR5MAG))
	{
		ar_type = AR5;
		/*
		* Must read in header to set necessary info
		*/
		if (fseek(arfd, 0L, 0) != 0 ||
			fread((char *)&arh_5, sizeof(arh_5), 1, arfd) != 1)
		{
			return (-1);
		}
#ifdef pdp11
		fatal1("Cannot handle 5.0 archive format for %s", archname);
		/*NOTREACHED*/
#else
		sym_begin = ftell(arfd);
		num_symbols = sgetl(arh_5.ar_syms);
		first_ar_mem = sym_begin + sizeof(ars_5) * num_symbols;
		sym_size = 0L;
		return (0);
#endif
	}
	if (fseek(arfd, 0L, 0) != 0 ||
		fread(mag_port, SARportMAG, 1, arfd) != 1)
	{
		return (-1);
	}
	if (equaln(mag_port, ARportMAG, SARportMAG))
	{
		ar_type = ARport;
		/*
		* Must read in first member header to find out
		* if there is a symbol directory
		*/
		if (fread((char *)&ar_port, sizeof(ar_port), 1, arfd) != 1 ||
			!equaln(ARFportMAG, ar_port.ar_fmag,
			sizeof(ar_port.ar_fmag)))
		{
			return (-1);
		}
#ifdef pdp11
		fatal1("Cannot handle portable archive format for %s",
			archname);
		/*NOTREACHED*/
#else
#ifdef BSD
		if (equaln(ar_port.ar_name,"__.SYMDEF       ",16))
#else
		if (ar_port.ar_name[0] == '/')
#endif
		{
			char s[4];

			if (sscanf(ar_port.ar_size, "%ld", &sym_size) != 1)
				return (-1);
			sym_size += (sym_size & 01);	/* round up */
			if (fread(s, sizeof(s), 1, arfd) != 1)
				return (-1);
#ifdef BSD
			num_symbols = sgetl(s) / sizeof(struct ranlib);
#else
			num_symbols = sgetl(s);
#endif
			sym_begin = ftell(arfd);
			first_ar_mem = sym_begin + sym_size - sizeof(s);
		}
		else	/* there is no symbol directory */
		{
			sym_size = num_symbols = sym_begin = 0L;
			first_ar_mem = ftell(arfd) - sizeof(ar_port);
		}
		return (0);
#endif
	}
	fatal1("%s is not an archive", f);
	/*NOTREACHED*/
#if 0
	if(fread(&head, sizeof(head), 1, arfd) != 1)
		return(-1);
	if(!equaln(head.ar_magic, ARMAG, 4))
		fatal1("%s is not an archive", f);
	return(0);
#endif
}

clarch()
{
	if(arfd != NULL)
		fclose(arfd);
}

/*
 *	Used when unlinking files. If file cannot be stat'ed or it is
 *	a directory, then do not remove it.
 */
isdir(p)
char *p;
{
	struct stat statbuf;

	if(stat(p, &statbuf) == -1)
		return(1);		/* no stat, no remove */
	if((statbuf.st_mode&S_IFMT) == S_IFDIR)
		return(1);
	return(0);
}

#ifdef BSD
/*
 * The intent here is to provide a means to make the value of
 * bytes in an io-buffer correspond to the value of a long
 * in the memory while doing the io a `long' at a time.
 * Files written and read in this way are machine-independent.
 *
 */
#include <values.h>

long
sgetl(buffer)
register char *buffer;
{
	register long w = 0;
	register int i = BITSPERBYTE * sizeof(long);

	while ((i -= BITSPERBYTE) >= 0)
		w |= (long) ((unsigned char) *buffer++) << i;
	return (w);
}
#endif
