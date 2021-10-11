#ifndef lint
static	char sccsid[] = "@(#)symtab.c 1.1 92/07/30 SMI";
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash-3b2:symtab.c	1.11"
/*
 * This file contains code for the crash functions: nm, ds, and ts, as well
 * as the initialization routine rdsymtab.
 */

#include "a.out.h"
#include "stdio.h"
#include "string.h"
#include "crash.h"

#ifdef sparc
#include <sys/proc.h>
#include <sys/file.h>
#include "struct.h"
#endif
extern	char	nmlst_version[];	/* namelist version */
extern char *namelist;
struct nlist *stbl;			/* symbol table */
int symcnt;				/* symbol count */
char *strtbl;				/* pointer to string table */
extern char *malloc();
#define bad(x) ((x & N_STAB) || ((x & N_TYPE) == N_COMM) || ((x & N_TYPE) == N_FN))
#ifdef sparc
int     use_shlib = 0; /* non-zero __DYNAMIC ==> extended maps for text */
addr_t  dynam_addr = 0;
struct  asym *symhash[HSIZE];
#endif

/* symbol table initialization function */

int
rdsymtab()
{
	FILE *np;
	struct exec filehdr;
	struct nlist	*sp,
			*ts_symb;
	int	i;
	char *str;
	long *str2;
	long strtblsize;

	/*
	 * Open the namelist and associate a stream with it. Read the file into a buffer.
	 * Determine if the file is in the correct format via a magic number check.
	 * An invalid format results in a return to main(). Otherwise, dynamically 
	 * allocate enough space for the namelist. 
	 */

	if (!(np = fopen(namelist, "r")))
		fatal("cannot open namelist file\n");
	if (fread((char *)&filehdr, sizeof(struct exec), 1, np) != 1)
		fatal("read error in namelist file\n");
	if (N_BADMAG(filehdr))
		fatal("namelist not in a.out format\n");

	if (!(stbl=(struct nlist *)malloc((unsigned)(filehdr.a_syms))))
		fatal("cannot allocate space for namelist\n");

	/*
	 * Find the beginning of the namelist and read in the contents of the list.
	 *
	 * Additionally, locate all auxiliary entries in the namelist and ignore.
	 */

	fseek(np, N_SYMOFF(filehdr), 0);
	symcnt = 0;
	for (i = 0, sp = stbl; i < (filehdr.a_syms / sizeof(struct nlist)); i++, sp++) {
		if (fread(sp, sizeof(struct nlist), 1, np) != 1)
			fatal("read error in namelist file\n");
		symcnt++;
	}
	
	/*
	 * Now find the string table (if one exists) and
	 * read it in.
	 */
	if (fseek(np, N_STROFF(filehdr), 0) != 0)
		fatal("error in seeking to string table\n");
	
	if (fread((char *)&strtblsize, sizeof(int), 1, np) != 1)
		fatal("read error for string table size\n");
	
	if (strtblsize)
	{
		if (!(strtbl = (char *)malloc((unsigned)strtblsize)))
			fatal("cannot allocate space for string table\n");

		str2 = (long *)strtbl;
		*str2 = strtblsize;

		for (i = 0,str = (char *)((int)strtbl + (int)sizeof(long)); i < strtblsize - sizeof(long); i++, str++)
			if (fread(str, sizeof(char), 1, np) != 1)
				fatal("read error in string table\n");
	}
	else
		str = 0;

	fclose(np);
}

/* find symbol */
struct nlist *
findsym(addr)
unsigned long addr;
{
	struct nlist *sp;
	struct nlist *save;
	unsigned long value;

	value = 0;
	save = NULL;

	for (sp = stbl; sp < &stbl[symcnt]; sp++) {
		if (bad(sp->n_type)) continue;
		if (index(strtbl + sp->n_un.n_strx, '.')) continue;
		if (((unsigned long)sp->n_value <= addr)
		  && ((unsigned long)sp->n_value > value)) {
			value = (unsigned long)sp->n_value;
			save = sp;
		}
	}
	return(save);
}

/* get arguments for ds and ts functions */
int
getsymbol()
{
	int c;

	optind = 1;
	while ((c = getopt(argcnt,args,"w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	if (args[optind]) {
		do {prsymbol(args[optind++]);
		} while(args[optind]);
	}
	else longjmp(syn,0);
}

/* print result of ds and ts functions */
int
prsymbol(string)
char *string;
{
	struct nlist *spd,*spt,*sp;
	long addr;

	spd = spt = sp = NULL;
	if ((addr = strcon(string,'h')) == -1)
		error("\n");
	if (!(spd = findsym((unsigned long)addr)) &&
		!(spt = findsym((unsigned long)addr))) {
			prerrmes("%s does not match\n",string);
			return;
	}
	if (!spd) 
		sp = spt;
	else if (!spt) 
		sp = spd;
	else {
		if ((addr - spt->n_value) < (addr - spd->n_value)) 
			sp = spt;
		else sp = spd;
	}
	fprintf(fp,"%s",strtbl + sp->n_un.n_strx);		
	fprintf(fp," + %x\n",addr - (long)sp->n_value);
}

/* search symbol table */
struct nlist *
symsrch(s)
char *s;
{
	struct nlist *sp;
	struct nlist *found;
	char *name;

	found = 0;

	for (sp = stbl; sp < &stbl[symcnt]; sp++) {
		name = strtbl + sp->n_un.n_strx;
		if (bad(sp->n_type)) continue;
		if (index(strtbl + sp->n_un.n_strx, '.')) continue;
		if ((!strcmp(name,s)) || ((*name++ == '_') && (!strcmp(name,s)))) {
			found = sp;
			break;
		}
	}
	return(found);
}

/* get arguments for nm function */
int
getnm()
{
	int c;

	optind = 1;
	while((c = getopt(argcnt,args,"w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	if (args[optind]) 
		do { prnm(args[optind++]);
		} while(args[optind]);
	else longjmp(syn,0);
}

/* print result of nm function */
int
prnm(string)
char *string;
{
	char *cp;
	struct nlist *sp;
	register unsigned char type;

	if (!(sp = symsrch(string))) {
		prerrmes("%s does not match in symbol table\n",string);
		return;
	}
	fprintf(fp,"%s   %8lx  ", string, sp->n_value);
	type = sp->n_type & N_TYPE;

	if      (type == N_TEXT)
		cp = " text";
	else if (type == N_DATA)
		cp = " data";
	else if (type == N_BSS)
		cp = " bss";
	else if (type == N_UNDF)
		cp = " undefined";
	else if (type == N_ABS)
		cp = " absolute";
	else
		cp = " type unknown";

	fprintf(fp,"%s\n", cp);
}
#ifdef sparc
struct asym *
enter(np, flag)
	struct nlist *np;
{
	register struct asym *s;
	register int h;

	if (NGLOBALS == 0) {
		NGLOBALS = GSEGSIZE;
		globals = (struct asym **)
		    malloc(GSEGSIZE * sizeof (struct asym *));
		if (globals == 0)
			outo_fmem();
		global_segment = nextglob = (struct asym *)
		    malloc(GSEGSIZE * sizeof(struct asym));
		if (nextglob == 0)
			outo_fmem();
	}
	if (nglobals == NGLOBALS) {
		NGLOBALS += GSEGSIZE;
		globals = (struct asym **)
		    realloc((char *)globals, NGLOBALS*sizeof (struct asym *));
		if (globals == 0)
			outo_fmem();
		global_segment = nextglob = (struct asym *)
		    malloc( GSEGSIZE*sizeof (struct asym));
		if (nextglob == 0)
			outo_fmem();
	}
	globals[nglobals++] = s = nextglob++;
	s->s_name = np->n_un.n_name;
	s->s_type = np->n_type;
	s->s_f = 0; s->s_fcnt = 0; s->s_falloc = 0;
	s->s_flag = flag;
	if (flag == AOUT)
		s->s_value = np->n_value;
	else if (s->s_type != (N_EXT | N_UNDF))
		/* flag == SHLIB and not COMMON */
		s->s_value = np->n_value + SHLIB_OFFSET;
	s->s_fileloc = 0;
	if(!use_shlib && strcmp(s->s_name, "__DYNAMIC")==0){
		dynam_addr = (addr_t)s->s_value;
		if (dynam_addr){
			use_shlib++;
		}
	}
	h = hashval(np->n_un.n_name);
	s->s_link = symhash[h];
	symhash[h] = s;
	return (s);
}
static struct asym *
new_lookup(cp)
	char *cp;
{
	register struct asym *s;
	register int h;

	h = hashval(cp);
	for (s = symhash[h]; s; s = s->s_link){
		if( eqsym(s->s_name, cp, '_')) {
			cursym = s;
			return (s);
		}
		switch(s->s_type){
		case N_FUN:
		case N_GSYM:
			/* only known cases */
			if (eqsym(cp, s->s_name,  '_')) {
				cursym = s;
				return (s);
			}
		}
	}
	cursym = 0;
	return (0);
}
struct asym *
lookup(cp)
	char *cp;
{
	register struct asym *s;
	register int h;

	h = hashval(cp);
	for (s = symhash[h]; s; s = s->s_link)
		if (!strcmp(s->s_name, cp)) {
			cursym = s;
			return (s);
		}
	for (s = symhash[h]; s; s = s->s_link)
		if (eqsym(s->s_name, cp, '_')) {
			cursym = s;
			return (s);
		}
	cursym = 0;
	return (0);
}
static
hashval(cp)
	register char *cp;
{
	register int h = 0;
	
	while (*cp == '_')
		cp++;
	while (*cp && (*cp != '_' || cp[1])) {
		h *= 2;
		h += *cp++;
	}
	h %= HSIZE;
	if (h < 0)
		h += HSIZE;
	return (h);
}
outo_fmem ()
{
	printf("ran out of memory for symbol table.\n");
	exit(1);
}
eqsym(s1, s2, c)
	register char *s1, *s2;
	char c;
{
	if (!strcmp(s1, s2))
		return (1);
	if (*s1 == c && !strcmp(s1+1, s2))
		return (1);
	return (0);
}
stinit(fsym, hp, flag)
	int fsym;
	struct exec *hp;
{
	struct nlist nl[BUFSIZ / sizeof (struct nlist)];
	register struct nlist *np;
	register struct asym *s;
	int ntogo = hp->a_syms / sizeof (struct nlist), ninbuf = 0;
	off_t loc; int ssiz; char *strtab;
	register int i;

	if (hp->a_syms == 0)
		return;
	loc = N_SYMOFF(*hp);
	/* allocate and read string table */
	(void) lseek(fsym, (long)(loc + hp->a_syms), 0);
	if (read(fsym, (char *)&ssiz, sizeof (ssiz)) != sizeof (ssiz)) {
		printf("old format a.out - no string table\n");
		return;
	}
	if (ssiz == 0) {        /* check whether there is a string table */
		fprintf(stderr, "Warning -- empty string table. No symbols.\n");
		fflush(stderr);
		return;
	}
	strtab = (char *)malloc(ssiz + 8);
	if (strtab == 0)
		outo_fmem();
	*(int *)strtab = ssiz;
	ssiz -= sizeof (ssiz);
	if (read(fsym, strtab + sizeof (ssiz), ssiz) != ssiz)
		goto readerr;
	/* read and process symbols */
	(void) lseek(fsym, (long)loc, 0);
	while (ntogo) {
		if (ninbuf == 0) {
			int nread = ntogo, cc;
			if (nread > BUFSIZ / sizeof (struct nlist))
				nread = BUFSIZ / sizeof (struct nlist);
			cc = read(fsym,(char *)nl,nread*sizeof (struct nlist));
			if (cc != nread * sizeof (struct nlist))
				goto readerr;
			ninbuf = nread;
			np = nl;
		}
		if (np->n_un.n_strx)
			np->n_un.n_name = strtab + np->n_un.n_strx;
		if (dosym(np++, strtab, strtab + ssiz + sizeof(ssiz), flag))
			break;
		ninbuf--; ntogo--;
	}
	if (flag == AOUT){
		sort_globals(flag);
	}
	return;
readerr:
	printf("error reading symbol or string table\n");
	exit(1);
}
struct afield *
field(np, fpp, fnp, fap)
	struct nlist *np;
	struct afield **fpp;
	int *fnp, *fap;
{
	register struct afield *f;

	if (*fap == 0) {
		*fpp = (struct afield *)
		    calloc(10, sizeof (struct afield));
		if (*fpp == 0)
			outo_fmem();
		*fap = 10;
	}
	if (*fnp == *fap) {
		*fap *= 2;
		*fpp = (struct afield *)
		    realloc((char *)*fpp, *fap * sizeof (struct afield));
		if (*fpp == 0)
			outo_fmem();
	}
	f = *fpp + *fnp; (*fnp)++;
	f->f_name = np->n_un.n_name;
	f->f_type = np->n_type;
	f->f_offset = np->n_value;
	return (f);
}
static
dosym(np, start, end, flag)
	char *start, *end;
	register struct nlist *np;
{
	register struct asym *s;
	register struct afield *f;
	register char *cp;
	int h;

	if (cp=np->n_un.n_name) {
		if ((cp < start) || (cp > end)) {
			printf("Warning: string table corrupted\n");
			return 1;
		}
		if( strcmp( "-lg", cp ) == 0  )
			return 0;
		for (; *cp; cp++)
			if (*cp == ':')
				return 0;
	} else {
		return 0;
	}
	switch (np->n_type) {
	case N_PC:
		return 0;
	case N_TEXT:
		for (cp=np->n_un.n_name; *cp; cp++)
			if (*cp == '.') return 0;
		/* fall thru */
	case N_TEXT|N_EXT:
	case N_DATA:
	case N_DATA|N_EXT:
	case N_BSS:
	case N_BSS|N_EXT:
	case N_ABS:
	case N_ABS|N_EXT:
	case N_UNDF:
	case N_UNDF|N_EXT:
	case N_GSYM:
	case N_FNAME:
	case N_FUN:
	case N_STSYM:
	case N_LCSYM:
	case N_ENTRY:
		s = new_lookup(np->n_un.n_name);
		if (s)
			if(flag == AOUT)
				mergedef(s, np);
			else
				re_enter(s, np, flag);
		else
			s = enter(np, flag);
 		if (np->n_type == N_FUN) {
			if (curfunc && curfunc->s_f) {
				curfunc->s_f = (struct afield *)
				    realloc((char *)curfunc->s_f,
					curfunc->s_fcnt *
					    sizeof (struct afield));
				if (curfunc->s_f == 0)
					outo_fmem();
				curfunc->s_falloc = curfunc->s_fcnt;
			}
			curfunc = s;
		}
		switch (np->n_type) {
		case N_FUN:
		case N_ENTRY:
		case N_GSYM:
		case N_STSYM:
		case N_LCSYM:
			s->s_fileloc = FILEX(curfile, np->n_desc);
			goto sline;
		}
		break;
	case N_RSYM:
	case N_LSYM:
	case N_PSYM:
		s = curfunc;
		if(s) (void) field(np, &s->s_f, &s->s_fcnt, &s->s_falloc);
		break;
	case N_SO:
	case N_SOL:
		curfile = fenter(np->n_un.n_name);
		break;
	case N_SSYM:
		if (s = curcommon) {
			(void) field(np, &s->s_f, &s->s_fcnt, &s->s_falloc);
			break;
		}
		f = globalfield(np->n_un.n_name);
		if (f == 0) {
			h = hashval(np->n_un.n_name);
			if (nfields >= NFIELDS) {
				NFIELDS += 50; /* up the ante by 50 each time */
				if ((fields = (struct afield *)
				    calloc(NFIELDS, sizeof(struct afield)))
					== NULL)
					outo_fmem();
				nfields = 0;
			}
			f = field(np, &fields, &nfields, &NFIELDS);
			f->f_link = fieldhash[h];
			fieldhash[h] = f;
		}
		break;
	case N_LBRAC:
	case N_RBRAC:
	case N_LENG:
		break;
	case N_BCOMM:
		curcommon = enter(np, flag);            /* name ? */
		break;
	case N_ECOMM:
	case N_ECOML:
		curcommon = 0;
		break;
sline:
	case N_SLINE:
		lenter(FILEX(curfile, (unsigned short)np->n_desc),
		    (int)np->n_value);
		break;
	}
	return 0;
}
static
mergedef(s, np)
	struct asym *s;
	struct nlist *np;
{
	switch (s->s_type) {
	case N_TEXT:
	case N_TEXT|N_EXT:
	case N_DATA:
	case N_DATA|N_EXT:
	case N_BSS:
	case N_BSS|N_EXT:
		s->s_type = np->n_type;
		break;
	case N_GSYM:
	case N_FUN:
		s->s_name = np->n_un.n_name;
		s->s_value = np->n_value;
		break;
	}
}
static off_t
textaddr(fhdr)
	struct exec *fhdr;
{
	if (fhdr->a_magic == ZMAGIC) {
		return (N_TXTADDR(*fhdr));
	}
	return (fhdr->a_entry);
}
setsym()
{
	off_t loc;
	unsigned long txtbas;
	off_t   datbas;
	struct map_range *fsym_rng1, *fsym_rng2;

	fsym_rng1 = (struct map_range*) calloc(1, sizeof (struct map_range));
	fsym_rng2 = (struct map_range*) calloc(1, sizeof (struct map_range));
	txtmap.map_head = fsym_rng1;
	txtmap.map_tail = fsym_rng2;
	fsym_rng1->mpr_next = fsym_rng2;
	fsym_rng1->mpr_fn = fsym_rng2->mpr_fn  = namelist;
	fsym = getfile1(namelist, 1);
	fsym_rng1->mpr_fd  = fsym_rng2->mpr_fd = fsym;
	if (read(fsym, (char *)&filhdr, sizeof filhdr) != sizeof filhdr ||
			N_BADMAG(filhdr))
	{
		fsym_rng1->mpr_e = MAXFILE;
		return;
	}
	loc = filhdr.a_text+filhdr.a_data;
	fsym_rng1->mpr_f = fsym_rng2->mpr_f =  N_TXTOFF(filhdr);
	fsym_rng1->mpr_b = 0;
	switch (filhdr.a_machtype) {
	case M_SPARC:
		break;
	case M_OLDSUN2:
	case M_68010:
	case M_68020:
		break;
	default:
		printf("warning: unknown machine type %d\n", filhdr.a_machtype);
		break;
	}
	switch (filhdr.a_magic) {
	case OMAGIC:
		txtbas  = textaddr(&filhdr);
		fsym_rng1->mpr_b = txtbas;
		fsym_rng1->mpr_e =  txtbas + filhdr.a_text;
		fsym_rng2->mpr_b = datbas = txtbas;
		fsym_rng2->mpr_e =  txtbas + loc;
		break;
	case ZMAGIC:
	case NMAGIC:
		fsym_rng1->mpr_b = textaddr(&filhdr);
		fsym_rng1->mpr_e = filhdr.a_text+N_TXTADDR(filhdr);
		fsym_rng2->mpr_b = datbas = roundup((int)fsym_rng1->mpr_e,
						N_SEGSIZ(filhdr));
		fsym_rng2->mpr_e = datbas + filhdr.a_data;
		fsym_rng2->mpr_f += filhdr.a_text;
	}
	stinit(fsym, &filhdr, AOUT);
	(void) lookup( "__sigtramp");
	if (trampsym = cursym) {
		(void) lookup( "__sigfunc" );
		funcsym = cursym;
	} else
		funcsym = 0;
}
struct file_new *
fget(name)
	char *name;
{
	register struct file_new *fi;

	for (fi = file_new; fi < filenfile; fi++)
		if (!strcmp(fi->f_name, name))
			return (fi);
	return (0);
}
fenter(name)
	char *name;
{
	register struct file_new *fi;

	if (fi = fget(name))
		return (fi - file_new + 1);
	if (NFILE == 0) {
		NFILE = 10;
		file_new = (struct file_new *)malloc(NFILE * sizeof (struct file_new));
		filenfile = file_new+nfile;
	}
	if (nfile == NFILE) {
		NFILE *= 2;
		file_new = (struct file_new *)
		    realloc((char *)file_new, NFILE*sizeof (struct file_new));
		filenfile = file_new+nfile;
	}
	fi = &file_new[nfile++];
	filenfile++;
	fi->f_name = savestr(name);
	fi->f_flags = 0;
	fi->f_nlines = 0;
	fi->f_lines = 0;
	return (nfile);
}
struct afield *
globalfield(cp)
	char *cp;
{
	register int h;
	register struct afield *f;

	h = hashval(cp);
	for (f = fieldhash[h]; f; f = f->f_link)
		if (!strcmp(cp, f->f_name))
			return (f);
	return (0);
}
static
lenter(afilex, apc)
	unsigned afilex, apc;
{

	if (NLINEPC == 0) {
		NLINEPC = 1000;
		linepc = (struct linepc *)
		   malloc(NLINEPC * sizeof (struct linepc));
		linepclast = linepc;
	}
	if (nlinepc == NLINEPC) {
		NLINEPC += 1000;
		linepc = (struct linepc *)
		   realloc((char *)linepc, NLINEPC * sizeof (struct linepc));
		linepclast = linepc+nlinepc;
	}
	if( XLINE(afilex) == 0xffff ) afilex = 0; /* magic */
	linepclast->l_fileloc = afilex;
	linepclast->l_pc = apc;
	linepclast++; nlinepc++;
}
sort_globals(flag)
{
	int i;

	if (nlinepc){
		qsort((char *)linepc, nlinepc, sizeof (struct linepc),lpccmp );
		pcline = (struct linepc*) calloc((nlinepc == 0 ? 1 : nlinepc),
			sizeof(struct linepc));
		if (pcline == 0)
			outo_fmem();
		for (i = 0; i < nlinepc; i++)
			pcline[i] = linepc[i];
		qsort((char *)pcline, nlinepc, sizeof (struct linepc),pclcmp );
	}
	if (nglobals == 0)
		return;
	globals = (struct asym **)
		    realloc((char *)globals, nglobals * sizeof (struct asym *));
	if (globals == 0)
		outo_fmem();
	/* arrange the globals in ascending value order, for findsym()*/
	qsort((char *)globals, nglobals, sizeof(struct asym *),symvcmp);
	/* prepare the free space for shared libraries */
	if(flag == AOUT)
		globals = (struct asym **)
		    realloc((char*)globals, NGLOBALS * sizeof(struct asym*));
}
static
pclcmp(l1, l2)
	struct linepc *l1, *l2;
{
	return (l1->l_pc - l2->l_pc);
}
static
lpccmp(l1, l2)
	struct linepc *l1, *l2;
{
	return (l1->l_fileloc - l2->l_fileloc);
}
static
symvcmp(s1, s2)
	struct asym **s1, **s2;
{
	return ((unsigned int)(*s1)->s_value - (unsigned int)(*s2)->s_value);
}
re_enter(s, np, flag)
	struct asym *s;
	struct nlist *np;
{
	if (flag != AOUT && s->s_value == 0 && s->s_type != (N_EXT | N_UNDF))
		s->s_value = np->n_value + SHLIB_OFFSET;
}
static char *
savestr(cp)
	char *cp;
{
	char *dp = (char *)malloc(strlen(cp) + 1);

	(void) strcpy(dp, cp);
	return (dp);
}
create(f)
	char *f;
{
	int fd;

	if ((fd = creat(f, 0644)) >= 0) {
		close(fd);
		return (open(f, wtflag));
	} else
		return (-1);
}
#endif
