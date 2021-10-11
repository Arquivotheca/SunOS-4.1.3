/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1980 Regents of the University of California.\n\
 All rights reserved.\n";
#endif not lint

#ifndef lint
static	char sccsid[] = "@(#)ranlib.c 1.1 92/07/30 SMI"; /* from UCB 5.2 9.5.85 */
#endif not lint
/*
 * ranlib - create table of contents for archive; string table version
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <ar.h>
#include <ranlib.h>
#include <a.out.h>
#include <stdio.h>

#ifdef BROWSER

#ifndef cb_library_tags_h_INCLUDED
#include "cb_library_tags.h"
#endif
     
char *cb_program_name = "ranlib";

cb_callback_write_stab()
{
}
#else
+++ -DBROWSER is to be left ON! +++
#endif

struct	ar_hdr	archdr;
#define	OARMAG 0177545
long	arsize;
struct	exec	exp;
FILE	*fi, *fo;
long	off, oldoff;
long	atol(), ftell();
#define TABSZ	6000
int	tabsz = TABSZ;
struct	ranlib *tab;
int	tnum;
#define	STRTABSZ	75000
int	strtabsz = STRTABSZ;
char	*tstrtab;
int	tssiz;
char	*strtab;
int	ssiz;
int	new;
char	tempnm[] = "__.SYMDEF";
char	firstname[17];

main(argc, argv)
char **argv;
{
	char cmdbuf[BUFSIZ];
	/* magbuf must be an int array so it is aligned on an int-ish
	   boundary, so that we may access its first word as an int! */
	int magbuf[(SARMAG+sizeof(int))/sizeof(int)];
	register int just_touch = 0;
	register int doupdate;

	/* check for the "-t" flag" */
	if (argc > 1 && strcmp(argv[1], "-t") == 0) {
		just_touch++;
		argc--;
		argv++;
	}

	--argc;
	/* allocate initial buffers */
	if ((tab = (struct ranlib *)malloc(TABSZ * sizeof (*tab))) == 0 ||
	    (tstrtab = (char *)malloc(STRTABSZ * sizeof (*tstrtab))) == 0) {
		fprintf(stderr, "ranlib: cannot allocate buffers\n");
		exit(1);
	} 
	while(argc--) {
		fi = fopen(*++argv,"r");
#ifdef BROWSER
		cb_ranlib_start_library(*argv,
					CB_CURRENT_LANGUAGE,
					CB_CURRENT_MAJOR_VERSION,
					CB_CURRENT_MINOR_VERSION,
					CB_EX_FOCUS_UNIT_LIBRARY_FORMAT);
#endif
		if (fi == NULL) {
			fprintf(stderr, "ranlib: cannot open %s\n", *argv);
			continue;
		}
		off = SARMAG;
		fread((char *)magbuf, 1, SARMAG, fi);
		if (strncmp((char *)magbuf, ARMAG, SARMAG)) {
			if (magbuf[0] == OARMAG)
				fprintf(stderr, "old format ");
			else
				fprintf(stderr, "not an ");
			fprintf(stderr, "archive: %s\n", *argv);
			continue;
		}
		if (just_touch) {
			register int	len;

			fseek(fi, (long) SARMAG, 0);
			if (fread(cmdbuf, sizeof archdr.ar_name, 1, fi) != 1) {
				fprintf(stderr, "malformed archive: %s\n",
					*argv);
				continue;
			}
			len = strlen(tempnm);
			if (bcmp(cmdbuf, tempnm, len) != 0 ||
			    cmdbuf[len] != ' ') {
				fprintf(stderr, "no symbol table: %s\n", *argv);
				continue;
			}
			fclose(fi);
			fixdate(*argv);
			continue;
		}
		fseek(fi, 0L, 0);
		new = tnum = 0;
		tssiz = 0;
		if (nextel(fi) == 0) {
			fclose(fi);
			continue;
		}
		do {
			long o;
			register n;
			struct nlist sym;

			if(strncmp(archdr.ar_name, tempnm, sizeof (archdr.ar_name)) == 0)
				continue;
			fread((char *)&exp, 1, sizeof(struct exec), fi);
			if (N_BADMAG(exp))
				continue;
			if (!strncmp(tempnm, archdr.ar_name, sizeof(archdr.ar_name)))
				continue;
			if (exp.a_syms == 0) {
				fprintf(stderr,
				  "ranlib: warning: %s(%s): no symbol table\n",
				    *argv, archdr.ar_name);
				continue;
			}
			o = N_STROFF(exp) - sizeof (struct exec);
			if (ftell(fi)+o+sizeof(ssiz) >= off) {
				fprintf(stderr,
				"ranlib: warning: %s(%s): old format .o file\n",
				    *argv, archdr.ar_name);
				continue;
			}
			fseek(fi, o, 1);
			fread((char *)&ssiz, 1, sizeof (ssiz), fi);
			if (ssiz < sizeof ssiz){
				/* sanity check */
				fprintf(stderr,
			"ranlib: warning: %s(%s): mangled string table\n",
				     *argv, archdr.ar_name);
				continue;
			}
			strtab = (char *)calloc(1, ssiz);
			if (strtab == 0) {
				fprintf(stderr, "ranlib: ran out of memory\n");
				exit(1);
			}
			fread(strtab+sizeof(ssiz), ssiz - sizeof(ssiz), 1, fi);
			fseek(fi, -(exp.a_syms+ssiz), 1);
			n = exp.a_syms / sizeof(struct nlist);
			while (--n >= 0) {
				fread((char *)&sym, 1, sizeof(sym), fi);
				if (sym.n_un.n_strx == 0)
					continue;
				sym.n_un.n_name = strtab + sym.n_un.n_strx;
#ifdef BROWSER
				if (sym.n_type == 0x48) {
					cb_ranlib_symbol(sym.n_un.n_name,
							 cb_focus_library_unit);
					continue;
				}
#endif
				if ((sym.n_type&N_EXT)==0)
					continue;
				switch (sym.n_type&N_TYPE) {

				case N_UNDF:
					if (sym.n_value!=0)
						stash(&sym);
					continue;

				default:
					stash(&sym);
					continue;
				}
			}
		} while(nextel(fi));
		new = fixsize();
		fclose(fi);
		doupdate = 0;
		if (tnum == 0) {
			/*
			 * No symbols in the archive.  Delete the table of
			 * contents from the archive, if it's there,
			 * otherwise, leave the archive alone.
			 */
			if(!new) {
				sprintf(cmdbuf, "ar dl %s %s\n", *argv,
				    tempnm);
				updatearchive(cmdbuf, *argv);
			}
		} else {
			/*
			 * There are symbols in the archive.  Build a
			 * table-of-contents file; replace the current one
			 * in the archive, if there's one there, otherwise
			 * add it.
			 */
			fo = fopen(tempnm, "w");
			if(fo == NULL) {
				fprintf(stderr, "ranlib: can't create ");
				perror(tempnm);
				exit(1);
			}
			tnum *= sizeof (struct ranlib);
			fwrite(&tnum, 1, sizeof (tnum), fo);
			tnum /= sizeof (struct ranlib);
			fwrite((char *)tab, tnum, sizeof(struct ranlib), fo);
			fwrite(&tssiz, 1, sizeof (tssiz), fo);
			fwrite(tstrtab, tssiz, 1, fo);
			fclose(fo);
			if(new)
				sprintf(cmdbuf, "ar rlb %s %s %s\n",
				    firstname, *argv, tempnm);
			else
				sprintf(cmdbuf, "ar rl %s %s\n", *argv,
				    tempnm);
			updatearchive(cmdbuf, *argv);
			unlink(tempnm);
		}
	}
#ifdef BROWSER
	cb_ranlib_exit();
#endif
	exit(0);
}

nextel(af)
FILE *af;
{
	register r;
	register char *cp;

	oldoff = off;
	fseek(af, off, 0);
	r = fread((char *)&archdr, 1, sizeof(struct ar_hdr), af);
	if (r != sizeof(struct ar_hdr))
		return(0);
	for (cp=archdr.ar_name; cp < & archdr.ar_name[sizeof(archdr.ar_name)]; cp++)
		if (*cp == ' ')
			*cp = '\0';
	arsize = atol(archdr.ar_size);
	if (arsize & 1)
		arsize++;
	off = ftell(af) + arsize;
	return(1);
}

stash(s)
	struct nlist *s;
{
	int i;
	register char *cp;

	if (tnum >= tabsz) {
		tabsz *= 2;
		tab = (struct ranlib *)realloc( tab, tabsz*sizeof *tab );
		if (tab == 0){
			fprintf(stderr, "ranlib: symbol table overflow\n");
			exit(1);
		}
	}
	tab[tnum].ran_un.ran_strx = tssiz;
	tab[tnum].ran_off = oldoff;
	for (cp = s->n_un.n_name; tstrtab[tssiz++] = *cp++;)
		if (tssiz >= strtabsz) {
			strtabsz *= 2;
			tstrtab = (char *)realloc( tstrtab, strtabsz*sizeof *tstrtab);
			if (tstrtab == 0) {
				fprintf(stderr, "ranlib: string table overflow\n");
				exit(1);
			}
		}
	tnum++;
}

fixsize()
{
	int i;
	off_t offdelta;

	if (tssiz&1)
		tssiz++;
	offdelta = sizeof(archdr) + sizeof (tnum) + tnum * sizeof(struct ranlib) +
	    sizeof (tssiz) + tssiz;
	off = SARMAG;
	nextel(fi);
	if(strncmp(archdr.ar_name, tempnm, sizeof (archdr.ar_name)) == 0) {
		new = 0;
		offdelta -= sizeof(archdr) + arsize;
	} else {
		new = 1;
		strncpy(firstname, archdr.ar_name, sizeof(archdr.ar_name));
	}
	for(i=0; i<tnum; i++)
		tab[i].ran_off += offdelta;
	return(new);
}

updatearchive(cmd, archname)
	char *cmd;
	char *archname;
{
	if(system(cmd))
		fprintf(stderr, "ranlib: ``%s'' failed\n", cmd);
	else
		fixdate(archname);
}

/* patch time */
fixdate(s)
	char *s;
{
	long time();
	char buf[24];
	int fd, tm;
	struct stat stat;

	fd = open(s, 1);
	if(fd < 0) {
		fprintf(stderr, "ranlib: can't reopen %s\n", s);
		return;
	}
	if (fstat(fd, &stat) < 0) {
		fprintf(stderr, "ranlib: can't stat %s\n", s);
		return;
	}
	tm = time((long *)NULL);
	tm = (tm > stat.st_mtime) ? tm : stat.st_mtime;
        sprintf(buf, "%-*ld", sizeof(archdr.ar_date), tm + 5);
	lseek(fd, (long)SARMAG + ((char *)archdr.ar_date-(char *)&archdr), 0);
	write(fd, buf, sizeof(archdr.ar_date));
	close(fd);
}
