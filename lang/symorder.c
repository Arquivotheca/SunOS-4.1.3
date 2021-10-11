#ifndef lint
static	char sccsid[] = "@(#)symorder.c 1.1 92/07/30 SMI"; /* from UCB X.X XX/XX/XX */
#endif

#include <stdio.h>
#include <a.out.h>

struct	symlist	{
	char	*name;			/* Name of the symbol */
	struct	nlist	*sym;		/* Ptr to its symbol table entry */
};
#define	SYMLIST_SIZE	100		/* Start with this much */
#define streq(a, b)	(strcmp(a, b) == 0)
#define strdup(str)	strcpy(malloc(strlen(str) + 1), str)

struct	symlist	*symlist;		/* Head of the list */
int	symlist_size;			/* Current size of symlist[] */
int	insert_pt;			/* Current insertion point */
int	silent;				/* Keep quiet about missing symbols */
int	compare();			/* Qsort() comparison routine */
char	*cmdname;
char	*copy_string();
char	*strcpy();
char	*malloc();

main(argc, argv)
int	argc;
char	*argv[];
{
	cmdname = argv[0];
	argc--;
	argv++;
	while (argc > 0 && argv[0][0] == '-') {
		switch (argv[0][1]) {
		case 's':
			silent = 1;
			break;
		}
		argc--;
		argv++;
	}
			
	if (argc != 2) {
		error("Usage: %s [-s] symsfile objectfile", cmdname);
	}
	read_syms2order(argv[0]);
	order_syms(argv[1]);
	exit(0);
	/* NOTREACHED */
}

/*
 * Read the list of symbols to move to the head of the class.
 */
read_syms2order(file)
char	*file;
{
	FILE	*fp;
	char	name[100];

	fp = fopen(file, "r");
	if (fp == NULL) {
		error("Could not open %s", file);
	}
	while (fscanf(fp, "%s", name) == 1) {
		insert(name);
	}
	fclose(fp);
}

/*
 * Insert another name into the list of symbols to look for.
 */
insert(name)
char	*name;
{
	int	size;

	if (symlist_size == 0) {
		symlist_size = SYMLIST_SIZE;
		size = symlist_size * sizeof(struct symlist);
		symlist = (struct symlist *) calloc(size, 1);
	}
	if (insert_pt >= symlist_size) {
		symlist_size += SYMLIST_SIZE;
		size = symlist_size * sizeof(struct symlist);
		symlist = (struct symlist *) realloc(symlist, symlist_size);
	}
	symlist[insert_pt].name = strdup(name);
	symlist[insert_pt].sym = NULL;
	insert_pt++;
}

/*
 * Re-order the symbols.
 * Move the requested symbols to the beginning of the symbol table.
 * Do not otherwise disturb the order of the remaining symbols.
 */
order_syms(objfile)
char	*objfile;
{
	FILE	*fp;
	struct	exec	hdr;
	struct	nlist	*syms;
	struct 	nlist	*symp;
	char	*strings;
	int	stringtabsize;
	int	nsyms;
	int	i;

	if ((fp = fopen(objfile, "r+")) == NULL) {
		error("Could not open %s", objfile);
	}
	if ((fread(&hdr, sizeof(hdr), 1, fp)) != 1 || N_BADMAG(hdr)) {
		error("%s is not an object file", objfile);
	}
	if (hdr.a_syms == 0) {
		error("%s is stripped", objfile);
	}
	syms = (struct nlist *) malloc(hdr.a_syms);
	fseek(fp, N_SYMOFF(hdr), 0);
	nsyms = hdr.a_syms/sizeof(struct nlist);
	fread(syms, sizeof(struct nlist), nsyms, fp); 
	fread(&stringtabsize, sizeof(stringtabsize), 1, fp);
	strings = malloc(stringtabsize);
	fseek(fp, N_STROFF(hdr), 0);
	fread(strings, stringtabsize, 1, fp);

	for (symp = syms; symp < &syms[nsyms]; symp++) {
		if (symp->n_un.n_strx > stringtabsize) {
			fprintf(stderr, "symbol %d - string index too large %d\n", i, symp->n_un.n_strx);
		} else {
			check_inlist(symp, strings);
		}
	}
	qsort(symlist, insert_pt, sizeof(struct symlist), compare);
	fseek(fp, N_SYMOFF(hdr), 0);
	write_symtab(fp, syms, nsyms, strings, stringtabsize);

	fclose(fp);
	free(syms);
	free(strings);
}

/*
 * Check if a symbol is in the list to be re-ordered.
 */
check_inlist(symp, strings)
struct	nlist	*symp;
char	*strings;
{
	struct	symlist	*sl;

	for (sl = symlist; sl < &symlist[insert_pt]; sl++) {
		if (streq(sl->name, &strings[symp->n_un.n_strx])) {
			sl->sym = symp;
		}
	}
}

/*
 * Qsort comparison routine.
 * Null symbol pointers are greater than any valid symbol pointer,
 * so symbols which aren't found float to the end of the table.
 */
compare(a, b)
struct	symlist	*a;
struct	symlist	*b;
{
	if (a->sym == NULL) {
		return(1);
	} else if (b->sym == NULL) {
		return(-1);
	} else if (a->sym < b->sym) {
		return(-1);
	} else if (a->sym == b->sym) {
		return(0);
	} else {
		return(1);
	}
}

/*
 * Write the symbol table in the new order.
 * First, walk the symlist writing each symbol.
 * Then walk the list again writing the intervening symbols.
 */
write_symtab(fp, syms, nsyms, strings, stringsize)
FILE	*fp;
struct	nlist	*syms;
int	nsyms;
char	*strings;
int	stringsize;
{
	register struct	symlist	*sl, *nextsl;
	char	*newstrings;
	char	*to;
	int	tot;
	int	n;

	newstrings = malloc(stringsize);
	to = newstrings;
	tot = 0;
	for (sl = symlist; sl < &symlist[insert_pt]; sl++) {
		if (sl->sym != NULL) {
			to = copy_string(newstrings, to, strings, sl->sym, 1);
			fwrite(sl->sym, sizeof(struct nlist), 1, fp);
			tot++;
		} else if (!silent) {
			fprintf(stderr, "Symbol `%s' not found\n",
			    sl->name);
		}
	}
	if (tot == 0) {
		error("No symbols found!");
	}

	/*
	 * "tot" counts the number of symbols which were found.
	 * Set "insert_pt" to "tot", so that symbols that weren't
	 * found are eliminated from the list.
	 */
	insert_pt = tot;

	n = symlist[0].sym - syms;
	to = copy_string(newstrings, to, strings, syms, n);
	fwrite(syms, sizeof(struct nlist), n, fp);
	tot += n;

	for (sl = symlist; sl < &symlist[insert_pt - 1]; sl++) {
		n = sl[1].sym - sl->sym - 1;
		to = copy_string(newstrings, to, strings, &sl->sym[1], n);
		fwrite(&sl->sym[1], sizeof(struct nlist), n, fp);
		tot += n;
	}
	sl = &symlist[insert_pt - 1];
	n = nsyms - (sl->sym - syms + 1);
	to = copy_string(newstrings, to, strings, &sl->sym[1], n);
	fwrite(&sl->sym[1], sizeof(struct nlist), n, fp);
	tot += n;
	if (tot != nsyms) {
		error("Symbol botch was %d now is %d", nsyms, tot);
	}
	if ((to - newstrings + sizeof(int)) != stringsize) {
		error("String table botch - was %d now is %d", stringsize,
		    to - newstrings);
	}
	fwrite(&stringsize, sizeof(stringsize), 1, fp);
	fwrite(newstrings, stringsize - sizeof(int), 1, fp);
}

/*
 * Copy a symbol's name into the new string table.
 * Update the string index in the symbol.
 * Do this for 'n' symbols.
 */
char *
copy_string(newstrings, to, strings, sym, n)
char	*newstrings;
register char	*to;
char	*strings;
struct	nlist	*sym;
int	n;
{
	register char	*from;
	int	newix;
	int	ix;
	int	i;

	for (i = 0; i < n; i++) {
		ix = sym->n_un.n_strx;
		if (ix != 0) {
			newix = to - newstrings + sizeof(int);
			from = &strings[ix];
			while ((*to++ = *from++) != '\0')
				;
			sym->n_un.n_strx = newix;
		}
		sym++;
	}
	return(to);
}

error(str, a, b, c, d, e)
char	*str;
{
	fprintf(stderr, "%s: ", cmdname);
	fprintf(stderr, str, a, b, c, d, e);
	fprintf(stderr, "\n");
	exit(1);
}
