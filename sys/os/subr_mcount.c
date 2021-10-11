/*	@(#)subr_mcount.c 1.1 92/07/30 SMI; from UCB 4.5 83/07/01	*/

/* last integrated from: gmon.c	4.10 (Berkeley) 1/14/83 */

#ifdef GPROF
#include <sys/gprof.h>
#include <sys/param.h>
#include <sys/systm.h>

/*
 * Froms is actually a bunch of unsigned shorts indexing tos
 */
int profiling = 3;
u_short *froms;
struct tostruct *tos = 0;
long tolimit = 0;
#ifdef vax
char *s_lowpc = (char *)KERNELBASE;
#endif
#ifdef sun
extern char start[];
char *s_lowpc = start;
#endif
extern char etext[];
char *s_highpc = etext;
u_long	s_textsize = 0;
u_int ssiz;
u_short	*sbuf;
u_short	*kcount;

kmstartup()
{
	u_int fromssize, tossize;

	/*
	 * Round lowpc and highpc to multiples of the density we're using
	 * so the rest of the scaling (here and in gprof) stays in ints.
	 */
	s_lowpc = (char *)
	    ROUNDDOWN((unsigned)s_lowpc, HISTFRACTION*sizeof (HISTCOUNTER));
	s_highpc = (char *)
	    ROUNDUP((unsigned)s_highpc, HISTFRACTION*sizeof (HISTCOUNTER));
	s_textsize = s_highpc - s_lowpc;
	printf("Profiling kernel, s_textsize=%d [%x..%x]\n",
		s_textsize, s_lowpc, s_highpc);
	ssiz = (s_textsize / HISTFRACTION) + sizeof (struct phdr);
	sbuf = (u_short *)new_kmem_alloc(ssiz, KMEM_SLEEP);
	blkclr((caddr_t)sbuf, ssiz);
	fromssize = s_textsize / HASHFRACTION;
	froms = (u_short *)new_kmem_alloc(fromssize, KMEM_SLEEP);
	blkclr((caddr_t)froms, fromssize);
	tolimit = s_textsize * ARCDENSITY / 100;
	if (tolimit < MINARCS) {
		tolimit = MINARCS;
	} else if (tolimit > 65534) {
		tolimit = 65534;
	}
	tossize = tolimit * sizeof (struct tostruct);
	tos = (struct tostruct *)new_kmem_alloc(tossize, KMEM_SLEEP);
	blkclr((caddr_t)tos, tossize);
	tos[0].link = 0;
	((struct phdr *)sbuf)->lpc = s_lowpc;
	((struct phdr *)sbuf)->hpc = s_highpc;
	((struct phdr *)sbuf)->ncnt = ssiz;
	kcount = (u_short *)(((int)sbuf) + sizeof (struct phdr));
#ifdef notdef
	/*
	 * Profiling is what mcount checks to see if
	 * all the data structures are ready!!!
	 */
	profiling = 0;		/* patch by hand when you're ready */
#endif
}

#ifdef vax
/*
 * This routine is massaged so that it may be jsb'ed to
 * XXX - move this stuff to a machine specific file?
 */
asm(".text");
asm("#the beginning of mcount()");
asm(".data");
mcount()
{
	register char			*selfpc;	/* r11 => r5 */
	register unsigned short		*frompcindex;	/* r10 => r4 */
	register struct tostruct	*top;		/* r9  => r3 */
	register struct tostruct	*prevtop;	/* r8  => r2 */
	register long			toindex;	/* r7  => r1 */

#ifdef lint
	selfpc = (char *)0;
	frompcindex = 0;
#else not lint
	/*
	 * Find the return address for mcount,
	 * and the return address for mcount's caller.
	 */
	asm("	.text");		/* make sure we're in text space */
	asm("	movl (sp), r11");	/* selfpc = ... (jsb frame) */
	asm("	movl 16(fp), r10");	/* frompcindex = (calls frame) */
#endif not lint
	/*
	 * Check that we are profiling and that we aren't recursively invoked.
	 */
	if (profiling) {
		goto out;
	}
	profiling++;
	/*
	 * Check that frompcindex is a reasonable pc value.
	 * For example:	signal catchers get called from the stack,
	 *		not from text space.  too bad.
	 */
	frompcindex = (unsigned short *)((long)frompcindex - (long)s_lowpc);
	if ((unsigned long)frompcindex > s_textsize) {
		goto done;
	}
	frompcindex =
	    &froms[((long)frompcindex) / (HASHFRACTION * sizeof (*froms))];
	toindex = *frompcindex;
	if (toindex == 0) {
		/*
		 * First time traversing this arc.
		 */
		toindex = ++tos[0].link;
		if (toindex >= tolimit) {
			goto overflow;
		}
		*frompcindex = toindex;
		top = &tos[toindex];
		top->selfpc = selfpc;
		top->count = 1;
		top->link = 0;
		goto done;
	}
	top = &tos[toindex];
	if (top->selfpc == selfpc) {
		/*
		 * Arc at front of chain; usual case.
		 */
		top->count++;
		goto done;
	}
	/*
	 * Have to go looking down chain for it.
	 * Top points to what we are looking at,
	 * prevtop points to previous top.
	 * We know it is not at the head of the chain.
	 */
	for (;;) {
		if (top->link == 0) {
			/*
			 * Top is end of the chain and none of the chain
			 * had top->selfpc == selfpc.  So we allocate a
			 * new tostruct and link it to the head of the chain.
			 */
			toindex = ++tos[0].link;
			if (toindex >= tolimit) {
				goto overflow;
			}
			top = &tos[toindex];
			top->selfpc = selfpc;
			top->count = 1;
			top->link = *frompcindex;
			*frompcindex = toindex;
			goto done;
		}
		/*
		 * Otherwise, check the next arc on the chain.
		 */
		prevtop = top;
		top = &tos[top->link];
		if (top->selfpc == selfpc) {
			/*
			 *	there it is.
			 *	increment its count
			 *	move it to the head of the chain.
			 */
			top->count++;
			toindex = prevtop->link;
			prevtop->link = top->link;
			top->link = *frompcindex;
			*frompcindex = toindex;
			goto done;
		}

	}
done:
	profiling--;
	/* and fall through */
out:
	asm("	rsb");

overflow:
	profiling = 3;
	printf("mcount: tos overflow\n");
	goto out;
}
asm(".text");
asm("#the end of mcount()");
asm(".data");
#endif vax
#endif GPROF
