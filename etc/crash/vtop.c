#ifndef lint
static	char sccsid[] = "@(#)vtop.c 1.1 92/07/30 SMI";
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash-3b2:vtop.c	1.5.1.1"
/*
 * This file contains code for the crash functions:  vtop and mode, as well as
 * the virtual to physical offset conversion routine vtop.
 */

#include "sys/param.h"
#include "a.out.h"
#include "stdio.h"
#include "sys/types.h"
#include "sys/time.h"
#include "sys/proc.h"
#include "machine/vm_hat.h"
#include "vm/as.h"
#include "crash.h"

extern unsigned procv;

/* virtual to physical offset address translation */
vtop(vaddr, slot)
long vaddr;
int slot;
{
	struct proc procbuf;
	struct as asbuf;

	readbuf(-1, procv + slot * sizeof procbuf, 0, -1, 
			(char *)&procbuf, sizeof procbuf, "proc table");
	if (!procbuf.p_stat)
		return(-1);
	if (procbuf.p_as == NULL) return(-1);
	if (vaddr > KERNELBASE) procbuf.p_as = (struct as *)(symsrch("kas")->n_value);
	readmem(procbuf.p_as, 1, -1, (char *)&asbuf, sizeof asbuf, "address space");
	return(kvm_physaddr(kd, &asbuf, vaddr));
}

/* get arguments for vtop function */
int
getvtop()
{
	int proc = Procslot;
	struct nlist *sp;
	long addr;
	int c;

	optind = 1;
	while((c = getopt(argcnt,args,"w:s:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 's' :	proc = setproc();
					break;
			default  :	longjmp(syn,0);
		}
	}
	if (args[optind]) {
		fprintf(fp," VIRTUAL  PHYSICAL\n");
		do {
			if (*args[optind] == '(') {
				if ((addr = eval(++args[optind])) == -1)
					continue;
				prvtop(addr, proc);
			}
			else if (sp = symsrch(args[optind])) 
				prvtop((long)sp->n_value, proc);
			else if (isasymbol(args[optind]))
				error("%s not found in symbol table\n",
					args[optind]);
			else {
				if ((addr = strcon(args[optind],'h')) == -1)
					continue;
				prvtop(addr, proc);
			}
		} while (args[++optind]);
	}
	else longjmp(syn, 0);
}

/* print vtop information */
int
prvtop(addr, proc)
long addr;
int proc;
{
	int paddr;

	paddr = vtop(addr, proc);
	if (paddr == -1) fprintf(fp, "%8x not mapped\n", addr);
	else fprintf(fp, "%8x %8x\n",
		addr,
		paddr);
}

/* get arguments for mode function */
int
getmode()
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
		prmode(args[optind]);
	else prmode("s");
}

/* print mode information */
int
prmode(mode)
char *mode;
{

	switch(*mode) {
		case 'p' :  Virtmode = 0;
			    break;
		case 'v' :  Virtmode = 1;
			    break;
		case 's' :  break;
		default  :  longjmp(syn,0);
	}
	if (Virtmode)
		fprintf(fp,"Mode = virtual\n");
	else fprintf(fp,"Mode = physical\n");
}
