/*      @(#)modloadsym.c 1.1 92/07/30 SMI	*/
 
/* 
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#ifdef sun386

#include <sys/types.h>
#include <stdio.h>   
#include <filehdr.h>
#include <ldfcn.h>
#include <stab.h>
#include <syms.h>
#include <ar.h>
#include <sun/vddrv.h>

#define SYMESZ  18  /* number of bytes in one symbol table entry */

void fatal();

/*
 * Read the symbol table (if any) and keep only those symbols which
 * are within the module.
 *
 * This code from taken from nm.c.
 *
 * Someday we'll have support from the linker to do all this.
 */

LDFILE  *fi;

getsymtab(filename, mmapaddr, modstart, modsize)
	char *filename;
	addr_t mmapaddr;
	u_long modstart;
	u_int modsize;
{
	long	symindx;		/* symbol table index */
	u_int 	sym_tab_size;		/* symbol table size */

	SYMENT  *symp;			/* pointer to symbol table */
	SYMENT  sym;			/* symbol table entry */
	AUXENT	aux;

	char    *malloc();
	char 	*symnametmp;		/* ptr to symbol name in file*/

	int 	i;
	int	nsyms;			/* number of symbols we'll keep */
	struct	vdsym *vdsym;		/* start of sym tab in mmap'd image */
	int	vdstroff;		/* offset of sym name in mmap'd image */
	char	*vdstraddr;		/* address of name in mmap'd image */
	struct	filehdr *fhdr;		/* pointer to filehdr in mmap'd image */

	extern int ncompare();

	if ((fi = ldopen(filename, fi)) == NULL)
		fatal("modload:  %s:  cannot open\n", filename);

	if (!ISCOFF(HEADER(fi).f_magic))
		fatal("modload:  %s:  bad magic\n", filename);

	if (HEADER(fi).f_nsyms == 0)
		return (0);		/* no symbols */

	/*
	 * seek to the symbol table 
	 */
	if (ldtbseek(fi) != SUCCESS) 
		fatal("modload:  %s:  symbols missing from file", filename);
	
	/*
         * allocate symbol table memory 
         * the amount of memory needed is calculated by the
         * number of entries multiplied by the size of each entry 
	 */
        sym_tab_size = HEADER(fi).f_nsyms * sizeof (struct syment);
	symp = (struct syment *)malloc(sym_tab_size);
	if (symp == NULL) 
		fatal("ran out of memory processing symbol table", filename);

	/* 
	 * read the symbol table 
	 */
	nsyms = 0;
       	for (symindx = 0L; symindx < HEADER(fi).f_nsyms; ++symindx) {

            if (FREAD(&sym, SYMESZ, 1, fi) != 1) 
		fatal("error reading symbol table", filename);
	  	    
	    /* 
	     * read in and skip over any aux entries (if there are any) 
	     */
            for (i = 1; i <= sym.n_numaux; ++i) {
                    if (FREAD(&aux, AUXESZ, 1, fi) != 1) 
                    	fatal("error reading symnbol table", filename);
                    ++symindx;
            }

	    if ((u_int)sym.n_value < modstart || 
		(u_int)sym.n_value > modstart + modsize)
		    continue; 		/* skip symbols out of range */

	    if (sym.n_name[0] == '.')
		continue;		/* skip names starting with '.' */

	    symp[nsyms++] = sym;
	} 

	qsort((char *)symp, nsyms, sizeof(struct syment), ncompare);

	/*
	 * At this point we have the symbols that we're interested in
	 * sorted by value.  Now we will generate our own symbol table
	 * and place it in our (private) mmap'd copy of the image.
	 * 
	 * Our symbol table entry has 2 elements.  The first is the offset
	 * from the start of the symbol table to the name of the symbol.
	 * This offset is converted to a virtual address by the VDLOAD ioctl
	 * to /dev/vd.  The second element is the symbol value.
	 */
	vdsym = (struct vdsym *)(mmapaddr + (int)(HEADER(fi).f_symptr)); 
	vdstroff = nsyms * sizeof(struct vdsym);
	vdstraddr = (char *)&vdsym[nsyms];

       	for (i = 0 ; i < nsyms; i++) {
		vdsym->vdsym_name  = vdstroff;
		vdsym->vdsym_value = symp[i].n_value;
		vdsym++;
		symnametmp = (char *)ldgetname(fi, &symp[i]);
		strcpy(vdstraddr, symnametmp);
		vdstraddr += strlen(symnametmp) + 1;
		vdstroff += strlen(symnametmp) + 1;
	}
	fhdr = (struct filehdr *)mmapaddr;
	fhdr->f_nsyms = nsyms;

	ldclose(fi);
	return (vdstroff);		/* return symbol table size */
}

static int ncompare(p1, p2)
register struct syment *p1, *p2;
{
        if (p1->n_value > p2->n_value)
                return(1);
        if (p1->n_value < p2->n_value)
                return(-1);
	return (0);
}
#else

#include <sys/types.h>
#include <sys/param.h>
#include <a.out.h>
#include <strings.h>
#include <sun/vddrv.h>

void fatal();

/*
 * Read the symbol table (if any) and keep only those symbols which
 * are within the module.
 *
 * Some of this code if from _nlist.c.
 */

getsymtab(filename, mmapaddr, modstart, modsize)
	char *filename;
	addr_t mmapaddr;
	u_long modstart;
	u_int modsize;
{
	struct	nlist *ksymp;		/* pointer to symbols we're keeping */ 
	struct	nlist *symp;		/* pointer to sym table in a.out image*/
	struct  nlist *symaddr;		/* ptr to start of sym tab in image */
	char 	*symnametmp;		/* ptr to symbol name in file*/

	int 	i;
	int	n;
	int	nsyms;			/* number of symbols we'll keep */
	struct	vdsym *vdsym;		/* start of sym tab in mmap'd image */
	int	vdstroff;		/* offset of sym name in mmap'd image */
	char	*vdstraddr;		/* address of name in mmap'd image */
	struct	exec *exec;		/* pointer to hdr in mmap'd image */

	long	straddr;		/* start of strings in a.out image */

	extern char *malloc();
	extern int ncompare();

	exec = (struct exec *)mmapaddr;	/* pointer to a.out header */

	if (exec->a_magic != OMAGIC)
	    fatal("modload:  %s:  bad magic %o\n", filename, exec->a_magic);

	if (exec->a_syms == 0)
		return (0);		/* no symbols */

	nsyms = 0;			/* number of syms we'll keep */

	symaddr = (struct nlist *)(mmapaddr + N_SYMOFF(*exec));

	symp = symaddr;			/* ptr to first sym tab entry */
	ksymp = (struct nlist *)malloc((u_int)exec->a_syms);

	if (ksymp == NULL)
		fatal("ran out of memory processing symbol table", filename);

	for (n = exec->a_syms; n > 0; n -= sizeof(struct nlist), symp++) {
	    if ((u_int)symp->n_value < modstart || 
		(u_int)symp->n_value > modstart + modsize)
		    continue; 		/* skip symbols out of range */

	    ksymp[nsyms++] = *symp;	/* keep this symtab entry */
	} 

	qsort((char *)ksymp, nsyms, sizeof(struct nlist), ncompare);

	/*
	 * At this point we have the symbols that we're interested in
	 * sorted by value.  Now we will generate our own symbol table
	 * and place it in our (private) mmap'd copy of the image.
	 * 
	 * Our symbol table entry has 2 elements.  The first is the offset
	 * from the start of the symbol table to the name of the symbol.
	 * This offset is converted to a virtual address by the VDLOAD ioctl
	 * to /dev/vd.  The second element is the symbol value.
	 */
	vdsym = (struct vdsym *)symaddr;
	vdstroff = nsyms * sizeof(struct vdsym);
	vdstraddr = (char *)&vdsym[nsyms];
	straddr = (long)((long)symaddr + exec->a_syms);

       	for (i = 0; i < nsyms; i++) {
		symnametmp = (char *)(straddr + ksymp[i].n_un.n_strx);
		vdsym->vdsym_value = ksymp[i].n_value;
		vdsym->vdsym_name  = vdstroff;
		vdsym++;
		
		strcpy(vdstraddr, symnametmp);
		vdstraddr += strlen(symnametmp) + 1;
		vdstroff += strlen(symnametmp) + 1;
	}
	exec->a_syms = nsyms;

	return (vdstroff);		/* return symbol table size */
}

static int ncompare(p1, p2)
register struct nlist *p1, *p2;
{
        if (p1->n_value > p2->n_value)
                return(1);
        if (p1->n_value < p2->n_value)
                return(-1);
	return (0);
}
#endif
