/*      @(#)modloadhdr.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <sys/param.h>

#ifdef sun386
#include <sys/filehdr.h>
#include <sys/scnhdr.h>
#include <sys/aouthdr.h>
#endif

#include <sys/exec.h>

void fatal(), vinfo();

#ifdef sun386

#define NUMSECS 4		/* number of section headers in the image */
				/* (text, data, bss, and comment) */
/*
 * Get information about an image file.  This is called to read
 * the headers of either the module being loaded or the kernel image file. 
 */
readhdr(mmapaddr, tvaddr, tsize, toffset, dvaddr, dsize, doffset, bvaddr, bsize)
	caddr_t mmapaddr;
	u_int *tvaddr;
	u_int *tsize;
	u_int *toffset;
	u_int *dvaddr;
	u_int *dsize;
	u_int *doffset;
	u_int *bvaddr;
	u_int *bsize;
{
	struct filehdr *fhdr;
	struct aouthdr *optaout; 	/* optional a.out header */
	struct scnhdr *scn;		/* COFF per-section info */
	unsigned int numsecs;	 	/* number of COFF sections in file */
	unsigned int scns = 0; 		/* must have all three sections! */

	/*
	 * Read the image file header to get module sizes.
	 */
	fhdr = (struct filehdr *)mmapaddr;

	/* check and see if this is a COFF header. */
	if (!ISCOFF(fhdr->f_magic))
		fatal("module image is not a valid COFF file\n");

	numsecs = fhdr->f_nscns; /* get number of section headers */

	if (numsecs != NUMSECS)
	    fatal("there must be only text, data, bss, and comment sections\n");
	if (fhdr->f_opthdr != sizeof(struct aouthdr))
	    fatal("invalid a.out header\n");

	optaout = (struct aouthdr *)(mmapaddr +sizeof(struct filehdr));

	/*
	 * let's get magic number and entry location from optional
	 * a.out header.
	 */
	if (optaout->magic != ZMAGIC && optaout->magic != OMAGIC &&
	    optaout->magic != NMAGIC)
	      fatal("bad a.out header magic number\n", optaout->magic);

	/*
	 * go through each COFF section header and get
	 * text, data, and bss sizes.  
	 */
	scn = (struct scnhdr *)(mmapaddr + sizeof(struct filehdr) +
				 sizeof(struct aouthdr));

	while (numsecs-- > 0) {
		switch ((int) scn->s_flags) {
		case STYP_TEXT:
			if (scn->s_size == 0)
				fatal("text size is 0\n");

			*tvaddr = ptob(btop(scn->s_vaddr));
			*tsize  = scn->s_size;
			if (optaout->magic == ZMAGIC)
			    *tsize += gethdrsize();

			scns |= STYP_TEXT;
			break;

		case STYP_DATA:
			if (scn->s_size == 0)
				fatal("data size is 0\n");

			*dvaddr  = scn->s_vaddr;
			*dsize   = scn->s_size;
			*doffset = scn->s_scnptr;

			scns |= STYP_DATA;
			break;

		case STYP_BSS:
			*bvaddr = scn->s_vaddr;
			*bsize  = ctob(btoc(scn->s_size));
			scns |= STYP_BSS;
			break;
		}

		scn++;		/* point to next section header */
	}

	/*
	 * For now, we assume there must be data and bss sections.
	 * Someday, we can ease this restriction.
	 */
	if (scns != (STYP_TEXT|STYP_DATA|STYP_BSS)) 
		fatal("missing text, data, or bss sections\n");

	return;
}

/*
 * Return the size of the file headers in the memory image for the file.
 */
gethdrsize()
{
	return (sizeof(struct filehdr) + sizeof(struct aouthdr) +
		NUMSECS * sizeof(struct scnhdr));
}

/*
 * Return the virtual address and offset of the kernel data segment
 */
getkerneldata(mmapaddr, daddr, doffset)
	caddr_t mmapaddr;
	u_int *daddr;
	u_int *doffset;
{
	u_int xxx;

	readhdr(mmapaddr, &xxx, &xxx, &xxx, daddr, &xxx, doffset, &xxx, &xxx);
}

#else

/*
 * Get information about an image file.  This is called to read
 * the headers of either the module being loaded or the kernel image file. 
 */
readhdr(mmapaddr, tvaddr, tsize, toffset, dvaddr, dsize, doffset, bvaddr, bsize)
	addr_t mmapaddr;
	u_int *tvaddr;
	u_int *tsize;
	u_int *toffset;
	u_int *dvaddr;
	u_int *dsize;
	u_int *doffset;
	u_int *bvaddr;
	u_int *bsize;
{
	register struct exec *exec = (struct exec *)mmapaddr;

	extern u_long mod_addr;

	if (exec->a_magic != OMAGIC)
		fatal("module image is not a valid OMAGIC a.out file\n");

	*tvaddr  = mod_addr;
	*tsize	 = exec->a_text;
	*toffset = sizeof(struct exec);

	*dvaddr  = *tvaddr + exec->a_text; 
	*dsize   = exec->a_data;
	*doffset = *toffset + exec->a_text;

	*bvaddr  = *dvaddr + exec->a_data;
	*bsize   = exec->a_bss;
}

/*
 * Return the size of the file headers in the memory image for the file.
 */
gethdrsize()
{
	return (0);	/* OMAGIC has no header in memory */
}

/*
 * Return the virtual address and offset of the kernel data segment
 */
getkerneldata(mmapaddr, daddr, doffset)
	caddr_t mmapaddr;
	u_int *daddr;
	u_int *doffset;
{
	register struct exec *exec = (struct exec *)mmapaddr;
	
	if (exec->a_magic != OMAGIC)
		fatal("module image is not a valid OMAGIC a.out file\n");

	*daddr  = exec->a_entry + exec->a_text; 
	*doffset = sizeof(struct exec) + exec->a_text;
}

#endif

/*
 * Verify that virtual addresses and sizes are ok.
 */
/*ARGSUSED*/
checkhdr(tvaddr, tsize, toffset, dvaddr, dsize, doffset, bvaddr, bsize)
	u_int tvaddr;
	u_int tsize;
	u_int toffset;
	u_int dvaddr;
	u_int dsize;
	u_int doffset;
	u_int bvaddr;
	u_int bsize;
{
	vinfo("textv %x, ts %x, datav %x, ds %x, bssv %x, bs %x\n",
		tvaddr, tsize, dvaddr, dsize, bvaddr, bsize);

	if (tsize == 0)
		fatal("text size is 0\n");

	if (tvaddr + tsize != dvaddr)
		fatal("data must immediately follow the text\n");

#ifdef COFF
	if (bvaddr < dvaddr || ctob(btoc(dvaddr + dsize)) != bvaddr)
		fatal("bss must start in the page following the data\n");
#endif
}
