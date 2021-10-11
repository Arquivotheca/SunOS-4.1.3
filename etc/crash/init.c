#ifndef lint
static	char sccsid[] = "@(#)init.c 1.1 92/07/30 SMI";
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash-3b2:init.c	1.7.1.1"
/*
 * This file contains code for the crash initialization.
 */

#include "sys/param.h"
#include "a.out.h"
#include "signal.h"
#include "stdio.h"
#include "sys/types.h"
#include "sys/user.h"
#include "sys/fcntl.h"
#include "crash.h"
#ifdef sparc
#include <sys/proc.h>
#include <sys/file.h>
#include <link.h>
#include <sys/ptrace.h>
#include "struct.h"
#endif

#define VSIZE 160	/* version string length */

char	version[VSIZE]; 	/* version strings */
extern char *strtbl;		/* pointer to string table in symtab.c */
extern char *dumpfile;		
extern char *namelist;
extern int active;		/* flag for active system */
extern long lseek();
extern char *malloc();
struct nlist *sp;		/* pointer to symbol table */
extern struct nlist *Panic, *Curproc, *Start;	/* namelist symbol pointers */

kvm_t *kd, *fkd;
extern unsigned nproc, procv;
extern unsigned nfiles, filev;
extern unsigned mountv;
extern unsigned bufv;
int localfiles;

/* initialize buffers, symbols, and global variables for crash session */

int
init()
{
	int offset;
	struct nlist	*ts_symb = NULL;
	extern void sigint();
	int magic;
	
	/*
	 * XXX - dumpfile will be opened by libkvm but would need to use
	 * internal structure of _kvmd structure
	 */
	if ((mem = open(dumpfile, 0)) < 0)	/* open dump file, if error print */
		fatal("cannot open dump file %s\n",dumpfile);
	/*
	 * Set a flag if the dumpfile is of an active system.
	 */
	if (strcmp(dumpfile, "/dev/mem") == 0) {
		active = 1;
		dumpfile = NULL;
	}

	if ((kd = kvm_open(namelist, dumpfile, NULL, O_RDONLY, "crash")) == NULL)
		fatal("cannot open kvm - dump file %s\n",dumpfile);

	rdsymtab();			/* open and read the symbol table */

	/* check version */
	ts_symb = symsrch("version");
	if (ts_symb && (kvm_read(kd, ts_symb->n_value, version, VSIZE)) != VSIZE)
		fatal("could not process dumpfile with supplied namelist %s\n",namelist);

	if (!(Start = symsrch("start")))
		fatal("start not found in symbol table\n");
	if (!(symsrch("file"))) 
		fatal("file not found in symbol table\n");
	if (!(symsrch("proc"))) 
		fatal("proc not found in symbol table\n");
	readsym("proc", &procv, sizeof(int));
	readsym("nproc", &nproc, sizeof(int));
	readsym("file", &filev, sizeof(int));
	readsym("nfile", &nfiles, sizeof(int));
	readsym("bufchain", &bufv, sizeof(int));
	if (symsrch("mounttab")) {
		readsym("mounttab", &mountv, sizeof(int));
		localfiles = 1;
	}
	if (!(Panic = symsrch("panicstr")))
		fatal("panicstr not found in symbol table\n");
	if (!(Curproc = symsrch("masterprocp")))
		fatal("masterprocp not found in symbol table\n");

	Procslot = getcurproc();
	/* setup break signal handling */
	if (signal(SIGINT,sigint) == SIG_IGN)
		signal(SIGINT,SIG_IGN);
}
#ifdef sparc
stack_init()
{
	setsym();
	setcor();
}
setcor()
{
	struct map_range *fcor_rng1, *fcor_rng2;

	fcor_rng1 = (struct map_range*) calloc(1, sizeof (struct map_range));
	fcor_rng2 = (struct map_range*) calloc(1, sizeof (struct map_range));
	datmap.map_head = fcor_rng1;
	datmap.map_tail = fcor_rng2;
	fcor_rng1->mpr_next = fcor_rng2;
	fcor_rng1->mpr_fn = fcor_rng2->mpr_fn  = corfile;
	fcor = getfile1(corfile, 2);
	fcor_rng1->mpr_fd  = fcor_rng2->mpr_fd = fcor;
	if (fcor == -1) {
		fcor_rng1->mpr_e = MAXFILE;
		return;
	}
	ksetcor();
}
static
ksetcor()
{
	char *looking;
	label_t pregs;

	datmap.map_head->mpr_e = MAXFILE;
	getproc1();
	(void) lookup(looking = "_panic_regs");
	if (cursym == 0)
		goto nosym;
	if (kvm_read(kd, (long)cursym->s_value, (char *)&pregs,
		sizeof pregs) != sizeof pregs)
			goto nosym;
	getpcb1(&pregs);
	return;
nosym:
	printf("Error: %s missing symbol %s\n",
	    corfile, looking);
	exit(2);
}
getpcb1(regaddr)
	label_t *regaddr;
{
	register int i;
	register int *rp;
	unsigned int pcval, spval;

	rp = &(regaddr->val[0]);
	pcval = *rp++;
	spval = *rp++;
	if (pcval == 0 && spval == 0)
		return;
	setreg(REG_PC, pcval);
	setreg(REG_SP, spval);
	for (i = REG_G1; i <= REG_G7; ++i ) setreg( i, 0 );
	for (i = REG_O1; i <= REG_O5; ++i ) setreg( i, 0 );
	setreg( REG_O7, pcval);
	for (i = REG_L0; i <= REG_L7; ++i ) {
		setreg( i, rwmap( 'r', (spval + FR_LREG(i)), DSP, 0 ) );
	}
	for (i = REG_I0; i <= REG_I7; ++i ) {
		setreg( i, rwmap( 'r', (spval + FR_IREG(i)), DSP, 0 ) );
	}
}
getfile1(filnam, cnt)
	char *filnam;
{
	register int fsym;

	if (!strcmp(filnam, "-"))
		return (-1);
	fsym = open(filnam, wtflag);
	if (fsym < 0 ) {
		if (wtflag) {
			fsym = create(filnam);
			if (fsym < 0) {
				fsym = open(filnam, 0);
				if (fsym >=0)
				    printf("warning: `%s' read-only\n", filnam);
				}
			}
			if (fsym < 0)
				printf("cannot open `%s'\n", filnam);
		}
		return (fsym);
}
getproc1()
{
	struct proc proc;
	int slot=Procslot;

	readmem((long)(procv + slot * sizeof proc), 1,
		slot, (char *)&proc, sizeof proc,
			"proc table");
	if (proc.p_stat == SZOMB) {
		errflg = "zombie process - no u-area";
		return;
	}
	if ((uarea = kvm_getu(kd, &proc)) == NULL) {
		errflg = "cannot find u-area";
		return;
	}
	getpcb1((label_t *)&uarea->u_pcb.pcb_regs);
}
get(addr, space)
	addr_t addr;
	int space;
{
	return (rwmap('r', addr, space, 0));
}
rwmap(mode, addr, space, value)
	char mode;
	addr_t addr;
	int space, value;
{
	int file, w;

	if (space == NSP)
		return (0);
	if (pid) {
		if (mode == 'r') {
			if (readproc(addr, (char *)&value, 4) != 4)
				rwerr(space);
			return (value);
		}
		if (writeproc(addr, (char *)&value, 4) != 4)
			rwerr(space);
		return (value);
	}
	w = 0;
	if (mode == 'w' && wtflag == 0)
		error("not in write mode");
	if (!chkmap(&addr, space, &file))
		return (0);
	if (space == DSP) {
		if ((mode == 'r') ? kread(addr, &w) : kwrite(addr, &value))
			rwerr(space);
		return (w);
	}
	if (rwphys(file, addr, mode == 'r' ? &w : &value, mode) < 0)
		rwerr(space);
	return (w);
}
setreg(reg, val)
	int reg;
	int val;
{
	reg_address( reg );
	switch ( adb_raddr.ra_type ) {
	case r_gzero:  /* Always zero -- setreg does nothing */
			break;
	case r_floating: /* Treat floating regs like normals */
	case r_normal: /* Normal one -- we have a good address */
			*( adb_raddr.ra_raddr ) = val;
			break;
	case r_window: /* Go figure it out */
			(void) reg_windows( reg, val, 1 );
			break;
	}
}
readreg(reg)
	int reg;
{
	int val=0;

	reg_address( reg );
	switch( adb_raddr.ra_type ) {

	case r_gzero:  /* Always zero -- val is already zero */
			break;
	case r_floating: /* Treat floating regs like normals */
			val = *( adb_raddr.ra_raddr );
			break;
	case r_normal: /* Normal one -- we have a good address */
			val = *( adb_raddr.ra_raddr );
			break;
	case r_window: /* Go figure it out */
			val = reg_windows( reg, val, 0 );
			break;
	}
	return val;
}
regs_not_on_stack ( ) 
{
	return ( u.u_pcb.pcb_wbcnt > 0 );
}
static
fileseek(f, a)
	int f;
	addr_t a;
{
	return (lseek(f, (long)a, 0) != -1);
}
kread(addr, p)
	unsigned addr;
	int *p;
{
	if (kvm_read(kd, (long)addr, p, sizeof *p) != sizeof *p)
		return (-1);
	return (0);
}
kwrite(addr, p)
	unsigned addr;
	int *p;
{
	if (kvm_write(kd, (long)addr, p, sizeof *p) != sizeof *p)
		return (-1);
	return (0);
}
writeproc(a, buf, len0)
	int a;
	char *buf;
	int len0;
{
	int len = len0;
	int  count;
	unsigned long val;
	char *writeval;

	errno = 0;
	if (len <= 0)
		return len;
	if (a & 03) {
		int  ard32;           /* a rounded down to 32-bit boundary */
		unsigned short *sh;

		/* We must align to a four-byte boundary */
		ard32 = a & ~3;
		(void) readproc( ard32, &val, 4);
		switch( a&3 ) {
		case 1: /* Must insert a byte and a short */
			*((char *)&val + 1) = *buf++;   /* insert byte */
			--len;  ++a;
			/* FALL THROUGH to insert short */
		case 2: /* insert short-word */
			*((char *)&val + 2) = *buf++;   /* insert byte */
			*((char *)&val + 3) = *buf++;   /* insert byte */
			len -= 2; a += 2;
			break;
		case 3: /* insert byte -- big or little end machines */
			*((char *)&val +3) = *buf++;
			--len;  ++a;
			break;
		}
		(void) ptrace(PTRACE_POKETEXT, pid, ard32, val);
	}
	while (len > 0) {
		if (len < 4) {
			(void) readproc(a, (char *)&val, 4);
			count = len;
		} else {
			count = 4;
		}
		writeval = (char *)&val;
		while (count--) {
			*writeval++ = *buf++;
		}
		(void) ptrace(PTRACE_POKETEXT, pid, a, val);
		len -= 4; a += 4;
	}
	if (errno)
		return (0);
	return (len0);
}
readproc(a, buf, len0)
	int a;
	char *buf;
	int len0;
{
	int len = len0;
	int count;
	char *readchar;
	unsigned val;

	errno = 0;
	if (len0 <= 0)
		return len0;
	if (a & 03) {
		unsigned short *sh;
		val = ptrace(a < txtmap.map_head->mpr_e ? PTRACE_PEEKTEXT :
		    PTRACE_PEEKDATA, pid, a & ~3, 0);
		switch( a&3 ) {
		case 1: /* Must insert a byte and a short */
			*buf++ = *((char *)&val + 1);
			--len;  ++a;
		case 2: /* insert two bytes */
			*buf++ = *((char *)&val + 2);
			*buf++ = *((char *)&val + 3);
			len -= 2; a += 2;
			break;
		case 3: /* insert byte -- big or little end machines */
			*buf++ = *((char *)&val + 3);
			--len; ++a;
			break;
		}
	}
	while (len > 0) {
		val = ptrace(a < txtmap.map_head->mpr_e ? PTRACE_PEEKTEXT :
		    PTRACE_PEEKDATA, pid, a, 0);
		readchar = (char *)&val;
		if (len < 4) {
			count = len;
		} else {
			count = 4;
		}
		while (count--) {
			*buf++ = *readchar++;
		}
		len -= 4; a += 4;
	}
	if (errno)
		return (0);
	return (len0);
}
static
rwerr(space)
	int space;
{
	if (space & DSP)
		errflg = "data address not found\n";
	else
		errflg = "text address not found\n";
}
rwphys(file, addr, aw, rw)
	int file;
	unsigned addr;
	int *aw, rw;
{
	int rc;

	if (fileseek(file, addr)==0)
		return (-1);
	if (rw == 'r')
		rc = read(file, (char *)aw, sizeof (int));
	else
		rc = write(file, (char *)aw, sizeof (int));
	if (rc != sizeof (int))
		return (-1);
	return (0);
}
static
chkmap(addr, space, fdp)
        register addr_t *addr;
        int space;
        int *fdp;
{
        register struct map *amap;
        register struct map_range *mpr;

        amap = (space&DSP) ? &datmap : &txtmap;
        for(mpr = amap->map_head; mpr; mpr= mpr->mpr_next){
                if(mpr == amap->map_head && space&STAR)
                        continue;
                if(within(*addr, mpr->mpr_b, mpr->mpr_e)) {
                        *addr += mpr->mpr_f - mpr->mpr_b;
                        *fdp = mpr->mpr_fd;
                        return (1);
                }
        }
        rwerr(space);
        return (0);
}
static
within(addr, lbd, ubd)
        addr_t addr, lbd, ubd;
{

        return (addr >= lbd && addr < ubd);
}
#endif
