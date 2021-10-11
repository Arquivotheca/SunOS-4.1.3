#ifndef lint
static	char sccsid[] = "@(#)util.c 1.1 92/07/30 SMI";
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash-3b2:util.c	1.11.2.1"
/*
 * This file contains code for utilities used by more than one crash function.
 */

#include "sys/param.h"
#include "a.out.h"
#include "signal.h"
#include "stdio.h"
#include "setjmp.h"
#include "ctype.h"
#include "sys/types.h"
#include "sys/time.h"
#include "sys/proc.h"
#include "machine/vm_hat.h"
#include "vm/as.h"
#include "machine/vmparam.h"
#include "crash.h"

extern jmp_buf jmp;
extern struct nlist *Curproc, *Start;
extern int procv;
extern int opipe;
extern FILE *rp;
void exit();
void free();

extern int nproc;

/* close pipe and reset file pointers */
int
resetfp()
{
	extern long pipesig;

	if (opipe == 1) {
		pclose(fp);
		signal(SIGPIPE, pipesig);
		opipe = 0;
		fp = stdout;
	}
	else {
		if ((fp != stdout) && (fp != rp) && (fp != NULL)) {
			fclose(fp);
			fp = stdout;
		}
	}
}

/* signal handling */
void 
sigint()
{
	extern int *stk_bptr;

	signal(SIGINT, sigint);		
	if (stk_bptr)
		free((char *)stk_bptr);
	fflush(fp);
	resetfp();
	fprintf(fp, "\n");
	longjmp(jmp, 0);
}

/* used in init.c to exit program */
/*VARARGS1*/
int
fatal(string, arg1, arg2, arg3)
char *string;
int arg1, arg2, arg3;
{
	fprintf(stderr, "crash:  ");
	fprintf(stderr, string, arg1, arg2, arg3);
	exit(1);
}

/* string to hexadecimal long conversion */
long
hextol(s)
char *s;
{
	int	i, j;
		
	for (j = 0; s[j] != '\0'; j++)
		if ((s[j] < '0' || s[j] > '9') && (s[j] < 'a' || s[j] > 'f')
			&& (s[j] < 'A' || s[j] > 'F'))
			break ;
	if (s[j] != '\0' || sscanf(s, "%x", &i) != 1) {
		prerrmes("%c is not a digit or letter a - f\n", s[j]);
		return(-1);
	}
	return(i);
}


/* string to decimal long conversion */
long
stol(s)
char *s;
{
	int	i, j;
		
	for (j = 0; s[j] != '\0'; j++)
		if ((s[j] < '0' || s[j] > '9'))
			break ;
	if (s[j] != '\0' || sscanf(s, "%d", &i) != 1) {
		prerrmes("%c is not a digit 0 - 9\n", s[j]);
		return(-1);
	}
	return(i);
}


/* string to octal long conversion */
long
octol(s)
char *s;
{
	int	i, j;
		
	for (j = 0; s[j] != '\0'; j++)
		if ((s[j] < '0' || s[j] > '7')) 
			break ;
	if (s[j] != '\0' || sscanf(s, "%o", &i) != 1) {
		prerrmes("%c is not a digit 0 - 7\n", s[j]);
		return(-1);
	}
	return(i);
}


/* string to binary long conversion */
long
btol(s)
char *s;
{
	int	i, j;
		
	i = 0;
	for (j = 0; s[j] != '\0'; j++)
		switch(s[j]) {
			case '0' :	i = i << 1;
					break;
			case '1' :	i = (i << 1) + 1;
					break;
			default  :	prerrmes("%c is not a 0 or 1\n", s[j]);
					return(-1);
		}
	return(i);
}

/* string to number conversion */
long
strcon(string, format)
char *string;
char format;
{
	char *s;

	s = string;
	if (*s == '0') {
		if (strlen(s) == 1)
			return(0); 
		switch(*++s) {
			case 'X' :
			case 'x' :	format = 'h';
					s++;
					break;
			case 'B' :
			case 'b' :	format = 'b';
					s++;
					break;
			case 'D' :
			case 'd' :	format = 'd';
					s++;
					break;
			default  :	format = 'o';
		}
	}
	if (!format)
		format = 'd';
	switch(format) {
		case 'h' :	return(hextol(s));
		case 'd' :	return(stol(s));
		case 'o' :	return(octol(s));
		case 'b' :	return(btol(s));
		default  :	return(-1);
	}
}

/* lseek and read */
readmem(addr, mode, proc, buffer, size, name)
unsigned addr;
int mode, proc;
char *buffer;
unsigned size;
char *name;
{

	if ((proc != -1) && (addr < USRSTACK)) {
		struct proc procbuf;
		struct as asbuf;

		readbuf(-1, procv + proc * sizeof procbuf, 0, -1, 
			(char *)&procbuf, sizeof procbuf, "proc table");
		if (!procbuf.p_stat) error("process slot %d doesn't exist", proc);
		if (procbuf.p_as == NULL) error("no context on slot %d\n", proc);
		readmem(procbuf.p_as, 1, -1, (char *)&asbuf, sizeof asbuf, "address space");
		if (kvm_as_read(kd, &asbuf, addr, buffer, size) != size)
			error("read error on %s at %x\n", name, addr);
	}
	else if (mode) {
		if (kvm_read(kd, addr, buffer, size) != size)
			error("read error on %s at %x\n", name, addr);
	} else {
		if (lseek(mem, addr, 0) == -1)
			error("seek error on address %x\n", addr);
		if (read(mem, buffer, size) != size)
			error("read error on %s\n", name);
	}
}

/* lseek to symbol name and read */
int
readsym(sym, buffer, size)
char *sym;
char *buffer;
unsigned size;
{
	register unsigned addr;
	register struct nlist *np;
	
	if (!(np = symsrch(sym)))
		error("%s not found in symbol table\n", sym);
	readmem(np->n_value, 1, -1, buffer, size, sym);
}

/* lseek and read into given buffer */
int
readbuf(addr, offset, phys, proc, buffer, size, name)
unsigned addr, offset;
char *buffer;
unsigned size;
int phys, proc;
char *name;
{
	if ((phys || !Virtmode) && (addr != -1))
		readmem(addr, 0, proc, buffer, size, name);
	else if (addr != -1)
		readmem(addr, 1, proc, buffer, size, name);
		else readmem(offset, 1, proc, buffer, size, name);
}


/* error handling */
/*VARARGS1*/
int
error(string, arg1, arg2, arg3)
char *string;
int arg1, arg2, arg3;
{

	if (rp) 
		fprintf(stdout, string, arg1, arg2, arg3);
	fprintf(fp, string, arg1, arg2, arg3);
	fflush(fp);
	resetfp();
	longjmp(jmp, 0);
}


/* print error message */
/*VARARGS1*/
int
prerrmes(string, arg1, arg2, arg3)
char *string;
int arg1, arg2, arg3;
{

	if ((rp && (rp != stdout)) || (fp != stdout)) 
		fprintf(stdout, string, arg1, arg2, arg3);
	fprintf(fp, string, arg1, arg2, arg3);
	fflush(fp);
}


/* simple arithmetic expression evaluation ( +  - & | * /) */
long
eval(string)
char *string;
{
	int j, i;
	char rand1[ARGLEN];
	char rand2[ARGLEN];
	char *op;
	unsigned addr1, addr2;
	struct nlist *sp;
	extern char *strpbrk();

	if (string[strlen(string)-1] != ')') {
		prerrmes("(%s is not a well-formed expression\n", string);
		return(-1);
	}
	if (!(op = strpbrk(string, "+-&|*/"))) {
		prerrmes("(%s is not an expression\n", string);
		return(-1);
	}
	for (j=0, i=0; string[j] != *op; j++, i++) {
		if (string[j] == ' ')
			--i;
		else rand1[i] = string[j];
	}
	rand1[i] = '\0';
	j++;
	for (i = 0; string[j] != ')'; j++, i++) {
		if (string[j] == ' ')
			--i;
		else rand2[i] = string[j];
	}
	rand2[i] = '\0';
	if (!strlen(rand2) || strpbrk(rand2, "+-&|*/")) {
		prerrmes("(%s is not a well-formed expression\n", string);
		return(-1);
	}
	if (sp = symsrch(rand1))
		addr1 = sp->n_value;
	else if ((addr1 = strcon(rand1, NULL)) == -1)
		return(-1);
	if (sp = symsrch(rand2))
		addr2 = sp->n_value;
	else if ((addr2 = strcon(rand2, NULL)) == -1)
		return(-1);
	switch(*op) {
		case '+' : return(addr1 + addr2);
		case '-' : return(addr1 - addr2);
		case '&' : return(addr1 & addr2);
		case '|' : return(addr1 | addr2);
		case '*' : return(addr1 * addr2);
		case '/' : if (addr2 == 0) {
				prerrmes("cannot divide by 0\n");
				return(-1);
			   }
			   return(addr1 / addr2);
	}
	return(-1);
}
		
/* get current process slot number */
int
getcurproc()
{
	int curproc;

	readmem(Curproc->n_value, 1, -1, (char *)&curproc,
		sizeof curproc, "current process");
	return((curproc - procv) / sizeof(struct proc));
}


/* determine valid process table entry */
int
procntry(slot, prbuf)
int slot;
struct  proc *prbuf;
{
	if (slot == -1) 
		slot = getcurproc();

	if ((slot > nproc) || (slot < 0))
		error("%d out of range\n", slot);

	readmem(procv + slot * sizeof(struct proc), 1, -1,
		(char *)prbuf, sizeof(struct proc), "process table");
	if (!prbuf->p_stat)
		error(" %d is not a valid process\n", slot);
}

/* argument processing from **args */
long
getargs(max, arg1, arg2)
int max;
long *arg1, *arg2;
{	
	struct nlist *sp;
	unsigned long slot;

	/* range */
	if (strpbrk(args[optind], "-")) {
		range(max, arg1, arg2);
		return;
	}
	/* expression */
	if (*args[optind] == '(') {
		*arg1 = (eval(++args[optind]));
		return;
	}
	/* symbol */
	if ((sp = symsrch(args[optind])) != NULL) {
		*arg1 = (sp->n_value);
		return;
	}
	if (isasymbol(args[optind])) {
		prerrmes("%s not found in symbol table\n", args[optind]);
		*arg1 = -1;
		return;
	}
	/* slot number */
	if ((slot = strcon(args[optind], 'h')) == -1) {
		*arg1 = -1;
		return;
	}
	if (slot < Start->n_value) {
		if ((slot = strcon(args[optind], 'd')) == -1) {
			*arg1 = -1;
			return;
		}
		if ((slot < max) && (slot >= 0)) {
			*arg1 = slot;
			return;
		}
		else {
			prerrmes("%d is out of range\n", slot);
			*arg1 = -1;
			return;
		}
	}
	/* address */
	*arg1 = slot;
} 

/* get slot number in table from address */
int
getslot(addr, base, size, phys, max)
unsigned addr, base, max;
int size, phys;
{
	long pbase;
	int slot;
	
	if (phys || !Virtmode) {
		pbase = vtop(base, -1);
		if (pbase == -1)
			error("%x is an invalid address\n", base);
		slot = (addr - pbase) / size;
	}
	else slot = (addr - base) / size;
	if ((slot >= 0) && (slot < max))
		return(slot);
	else return(-1);
}


/* file redirection */
int
redirect()
{
	int i;
	FILE *ofp;

	ofp = fp;
	if (opipe == 1) {
		fprintf(stdout, "file redirection (-w) option ignored\n");
		return;
	}
	if (fp = fopen(optarg, "a")) {
		fprintf(fp, "\n> ");
		for (i=0;i<argcnt;i++)
			fprintf(fp, "%s ", args[i]);
		fprintf(fp, "\n");
	}
	else {
		fp = ofp;
		error("unable to open %s\n", optarg);
	}
}


/*
 * putch() recognizes escape sequences as well as characters and prints the
 * character or equivalent action of the sequence.
 */
int
putch(c)
char  c;
{
	c &= 0377;
	if (c < 040 || c > 0176) {
		fprintf(fp, "\\");
		switch(c) {
		case '\0': c = '0'; break;
		case '\t': c = 't'; break;
		case '\n': c = 'n'; break;
		case '\r': c = 'r'; break;
		case '\b': c = 'b'; break;
		default:   c = '?'; break;
		}
	}
	else fprintf(fp, " ");
	fprintf(fp, "%c ", c);
}

/* sets process to input argument */
int
setproc()
{
	int slot;

	if ((slot = strcon(optarg, 'd')) == -1)
		error("\n");
	if ((slot > nproc) || (slot < 0))
		error("%d out of range\n", slot);
	return(slot);
}

/* check to see if string is a symbol or a hexadecimal number */
int
isasymbol(string)
char *string;
{
	int i;

	for (i = strlen(string); i > 0; i--)
		if (!isxdigit(*string++))
			return(1);
	return(0);
}


/* convert a string into a range of slot numbers */
int
range(max, begin, end)
int max;
long *begin, *end;
{
	int i, j, len, pos;
	char string[ARGLEN];
	char temp1[ARGLEN];
	char temp2[ARGLEN];

	strcpy(string, args[optind]);
	len = strlen(string);
	if ((*string == '-') || (string[len-1] == '-')){
		fprintf(fp, "%s is an invalid range\n", string);
		*begin = -1;
		return;
	}
	pos = strcspn(string, "-");
	for (i = 0; i < pos; i++)
		temp1[i] = string[i];
	temp1[i] = '\0';
	for (j = 0, i = pos+1; i < len; j++, i++)
		temp2[j] = string[i];
	temp2[j] = '\0';
	if ((*begin = (int)stol(temp1)) == -1)
		return;
	if ((*end = (int)stol(temp2)) == -1) {
		*begin = -1;
		return;
	}
	if (*begin > *end) {
		fprintf(fp, "%d-%d is an invalid range\n", *begin, *end);
		*begin = -1;
		return;
	}
	if (*end >= max) 
		*end = max - 1;
}

notvalid()
{
	error("not a valid function for this architecture\n");
}
