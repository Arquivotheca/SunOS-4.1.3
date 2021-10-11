#ifndef lint
static	char sccsid[] = "@(#)status.c 1.1 92/07/30 SMI";
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * This file contains code for the crash function:  status.
 */

#include "sys/param.h"
#include "a.out.h"
#include "stdio.h"
#include "sys/sysmacros.h"
#include "sys/types.h"
#include "time.h"
#include "crash.h"
#ifdef sun3
#include "sun3/cpu.h"
#endif
#ifdef sun3x
#include "sun3x/cpu.h"
#endif
#ifdef sun4
#include "sun4/cpu.h"
#endif
#ifdef sun4c
#include "sun4c/cpu.h"
#endif sun4c
#ifdef sun4m
#include "sun4m/cpu.h"
#endif sun4m

extern int active;		/* active system flag */
static struct nlist *Sys, *Time, *Lbolt;	/* namelist symbol */
extern struct nlist *Panic;				/* pointers */
extern char *ctime();
char *asctime();
struct tm *localtime();

/* get arguments for stat function */
int
getstat()
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
		longjmp(syn,0);
	else prstat(); 
}

/* print system status */
int
prstat()
{
	int toc, lbolt, boottime, panicp;
	char panicstr[80];
	char hostname[32];
	extern char version[];
	label_t regs;
	int days;
	char *machtype();

	/* 
	 * Locate, read, and print the system name, node name, release number,
	 * version number, and machine name.
	 */

	if (!Sys)
		if (!(Sys = symsrch("hostname")))
			error("hostname not found in symbol table\n");
	readmem((long)Sys->n_value, 1, -1, hostname,
		32, "hostname");

	fprintf(fp, "version:\t%s", version);
	fprintf(fp, "machine name:\t%s\n", hostname);
	fprintf(fp, "machine type:\tSUN %s\n", machtype());
	/*
	 * Locate, read, and print the time of the crash.
	 */

	if (!Time)
		if (!(Time = symsrch("time")))
			error("time not found in symbol table\n");

	readmem((long)Time->n_value,1,-1,(char *)&toc,
		sizeof toc,"time of crash");
	fprintf(fp,"time of crash:\t%s", ctime((long *)&toc));

	/*
	 * Locate, read, and print the age of the system since the last boot.
	 */

	if (!Lbolt)
		if (!(Lbolt = symsrch("boottime")))
			error("boottime not found in symbol table\n");

	readmem((long)Lbolt->n_value,1,-1,(char *)&boottime,
		sizeof boottime,"boottime");

	fprintf(fp,"age of system:\t");
	lbolt = toc - boottime;
	lbolt /= 60;
	days = lbolt / (long)(60 * 24);
	if (days)
		fprintf(fp,"%d day%s, ", days, (days > 1) ? "s" : "");
	lbolt %= (long)(60 * 24);
	if (lbolt / (long)60)
		fprintf(fp,"%d hr., ", lbolt / (long)60);
	lbolt %= (long) 60;
	if (lbolt)
		fprintf(fp,"%d min.", lbolt);
	fprintf(fp,"\n");
	if (active) return;

	fprintf(fp,"panicstr:\t");
	if (kvm_read(kd, Panic->n_value, &panicp, sizeof (char *)) == sizeof (char *))
		if (kvm_read(kd, panicp, panicstr, sizeof panicstr) == sizeof panicstr)
			fprintf(fp, "%s", panicstr);
	fprintf(fp,"\n");
	readsym("panic_regs", &regs, sizeof(regs));
#define rv regs.val
	fprintf(fp, "pc %8x\n", rv[0]);
#ifdef m68000
	fprintf(fp, "d2 %8x d3 %8x d4 %8x d5 %8x d6 %8x d7 %8x\n", rv[1], rv[2], rv[3], rv[4], rv[5], rv[6]);
	fprintf(fp, "a2 %8x a3 %8x a4 %8x a5 %8x a6 %8x sp %8x\n", rv[7], rv[8], rv[9], rv[10], rv[11], rv[12]);
#endif
#ifdef sparc
	fprintf(fp, "sp %8x\n", rv[1]);
#endif
}

char *
machtype()
{
	int cpu;

	readsym("cpu", &cpu, sizeof(int));
	switch (cpu) {
#ifdef sun2
		case CPU_SUN2_50:
			return("2/50");
		case CPU_SUN2_120:
			return("2/120");
#endif
#ifdef sun3
		case CPU_SUN3_50:
			return("3/50");
		case CPU_SUN3_60:
			return("3/60");
		case CPU_SUN3_110:
			return("3/110");
		case CPU_SUN3_160:
			return("3/160");
		case CPU_SUN3_260:
			return("3/260");
#endif
#ifdef sun3x
		case CPU_SUN3X_80:
			return("3/80");
		case CPU_SUN3X_470:
			return("3/470");
#endif
#ifdef sun4
		case CPU_SUN4_110:
			return("4/110");
		case CPU_SUN4_260:
			return("4/260");
		case CPU_SUN4_330:
			return("4/330");
		case CPU_SUN4_470:
			return("4/470");
#endif
#ifdef sun4c
		case CPU_SUN4C_60:
			return("4C/60");
		case CPU_SUN4C_20:
			return("4C/20");
		case CPU_SUN4C_40:
			return("4C/40");
		case CPU_SUN4C_65:
			return("4C/65");
		case CPU_SUN4C_75:
			return("4C/75");
		case CPU_SUN4C_25:
			return("4C/25");
		case CPU_SUN4C_50:
			return("4C/50");
#endif sun4c
#ifdef sun4m
		case CPU_SUN4M_690:
			return("4M/690");
		case CPU_SUN4M_50:
			return("4M/SparcStation 10");
#endif sun4m
		default:
			return("???");
	}
}
